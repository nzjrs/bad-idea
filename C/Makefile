bad-idea: bad-idea.c missile-control-lib.c
	gcc -ggdb -Wall `pkg-config libusb opencv --cflags --libs` bad-idea.c missile-control-lib.c missile-control-lib.h -o bad-idea

clean:
	rm bad-idea

