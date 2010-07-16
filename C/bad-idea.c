/**
 * Foo
 *
 * Copyright (C) Ian Jeffray 2005, all rights reserved.
 * Copyright (C) John Stowers 2007
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <limits.h>
#include <time.h>
#include <ctype.h>

#include <cv.h>
#include <highgui.h>

#include "missile-control-lib.h"

#define USB_TIMEOUT 1000 /* milliseconds */
#define CASCADE_XML_FILE "haarcascade_frontalface_alt2.xml"
#define CONTROL_MS 250 /* number of ms before control updates */
#define DEADBAND_X 15
#define DEADBAND_Y 20
#define PAUSE_BEFORE_SHOOT_MS 8000
#define DEBUG 0

#define HACK

static CvMemStorage* storage = 0;
static CvHaarClassifierCascade* cascade = 0;

static void missile_do( missile_usb *control, int cmd )
{
    int a, b, c, d, e;

    /* Two fixed-format initiator packets appear to be required */

    if (missile_usb_sendcommand(control, 'U', 'S', 'B', 'C', 0, 0, 4, 0 ))
    {
        perror("missile_usb_sendcommand failed");
        return;
    }

    if (missile_usb_sendcommand(control, 'U', 'S', 'B', 'C', 0, 64, 2, 0 ))
    {
        perror("missile_usb_sendcommand failed");
        return;
    }

    /* Now the actual movement command! */

    a = cmd & missile_left ? 1 : 0;
    b = cmd & missile_right ? 1 : 0;
    c = cmd & missile_up ? 1 : 0;
    d = cmd & missile_down ? 1 : 0;
    e = cmd & missile_fire ? 1 : 0;
    if (missile_usb_sendcommand64(control, 0, a, b, c, d, e, 8, 8 ))
    {
        perror("missile_usb_sendcommand failed");
        return;
    }
}

static void detect_and_draw( IplImage* img, int *x, int *y )
{
    static CvScalar colors[] = 
    {
        {{0,0,255}},
        {{0,128,255}},
        {{0,255,255}},
        {{0,255,0}},
        {{255,128,0}},
        {{255,255,0}},
        {{255,0,0}},
        {{255,0,255}}
    };

    double scale = 1.3;
    IplImage* gray = cvCreateImage( cvSize(img->width,img->height), 8, 1 );
    IplImage* small_img = cvCreateImage( cvSize( cvRound (img->width/scale),
                         cvRound (img->height/scale)),
                     8, 1 );
    int i;

    cvCvtColor( img, gray, CV_BGR2GRAY );
    cvResize( gray, small_img, CV_INTER_LINEAR );
    cvEqualizeHist( small_img, small_img );
    cvClearMemStorage( storage );

    if( cascade )
    {
        CvSeq* faces = cvHaarDetectObjects( small_img, cascade, storage,
                                            1.1, 2, 0/*CV_HAAR_DO_CANNY_PRUNING*/,
                                            cvSize(30, 30) );

        for( i = 0; i < (faces ? faces->total : 0); i++ )
        {
            CvRect* r = (CvRect*)cvGetSeqElem( faces, i );
            CvPoint center;
            int radius;
            center.x = cvRound((r->x + r->width*0.5)*scale);
            center.y = cvRound((r->y + r->height*0.5)*scale);
            radius = cvRound((r->width + r->height)*0.25*scale);
            cvCircle( img, center, radius, colors[i%8], 3, 8, 0 );

            *x = center.x;
            *y = center.y;
        }
    }

    cvShowImage( "result", img );
    cvReleaseImage( &gray );
    cvReleaseImage( &small_img );
}

static double get_ms(void)
{
    return (double)cvGetTickCount()/((double)cvGetTickFrequency()*1000.0);
}

int main(int argc, char *argv[])
{
    missile_usb *control = NULL;
    IplImage *frame, *frame_copy = NULL;
    CvCapture* capture = NULL;

    int cmd;

    int x;
    int y;
    int dx;
    int dy;
    double t1;
    double t2;
    int shoot;

    /* Initialize the face detector camera */
    cascade = (CvHaarClassifierCascade*)cvLoad(CASCADE_XML_FILE, 0, 0, 0 );
    if( !cascade )
    {
        fprintf( stderr, "Could not load classifier cascade\n" );
        exit(EXIT_FAILURE);
    }

    storage = cvCreateMemStorage(0);
    capture = cvCaptureFromCAM(0);
    cvNamedWindow( "result", 1 );

    /* Initialize the USB Sub System */
    if (missile_usb_initialise() != 0)
    {
        fprintf(stderr, "missile_usb_initalise failed\n");
        exit(EXIT_FAILURE);
    }

    control = missile_usb_create(DEBUG, USB_TIMEOUT);
    if (control == NULL)
    {
        fprintf(stderr, "unable to create missile launcher usb handle\n");
        exit(EXIT_FAILURE);
    }

    if (missile_usb_finddevice(control, 0) != 0)
    {
        fprintf(stderr, "unable to find usb missile launcher device\n");
        exit(EXIT_FAILURE);
    }

    if( capture )
    {
        t1 = get_ms();
        shoot = 0;
        for(;;)
        {
            if( !cvGrabFrame( capture ))
                break;
            frame = cvRetrieveFrame( capture, 0 );
            if( !frame )
                break;
            if( !frame_copy )
                frame_copy = cvCreateImage( cvSize(frame->width,frame->height),
                                            IPL_DEPTH_8U, frame->nChannels );
            if( frame->origin == IPL_ORIGIN_TL )
                cvCopy( frame, frame_copy, 0 );
            else
                cvFlip( frame, frame_copy, 0 );

            /* Detect the face and return it in x and y */            
            x = -1;
            y = -1;
            detect_and_draw( frame_copy, &x, &y );

            /* Limit the control frequency */
            t2 = get_ms();
            if ( (t2 - t1) > CONTROL_MS )
            {
                dx = (frame->width/2) - x;
                dy = (frame->height/2) - y;
                cmd = missile_stop;
                
                /* did we find the face? */
                if ( x != -1 && y != -1 )
                {

                    if ( abs(dx) > DEADBAND_X )
                        cmd |= ( dx > 0 ? missile_left : missile_right );
                    if ( abs(dy) > DEADBAND_Y )
                        cmd |= ( dy > 0 ? missile_up : missile_down );

                    printf("Offset: %i %i (%u)\n", dx, dy, cmd);

                    /* Allow the launcher to steady on the target */
                    if ( cmd == missile_stop)
                    {
                        if ( ++shoot > (PAUSE_BEFORE_SHOOT_MS / CONTROL_MS) )
                        {
                            shoot = 0;
                            cmd = missile_fire;
                        }
                    }
                    else
                        shoot = 0;

                }

#ifdef HACK                
                missile_usb_sendcommand_foo(control, cmd);
#else
                missile_do(control, cmd);
#endif
            }

            if( cvWaitKey( 10 ) >= 0 )
                break;
        }

        cvReleaseImage( &frame_copy );
        cvReleaseCapture( &capture );
 
    }

    /* Clean up USB*/
    cmd = missile_down;
#ifdef HACK                
    missile_usb_sendcommand_foo(control, cmd);
#else
    missile_do(control, cmd);
#endif
    missile_usb_destroy(control);
    missile_usb_finalise();

    /* Clean up Camera */
    cvDestroyWindow("result");
    return EXIT_SUCCESS;
}
