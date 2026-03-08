CC = clang
CFLAGS = -framework Cocoa -framework CoreGraphics -framework CoreText -framework ImageIO

all: march8

march8: window.m
	$(CC) -o march8 window.m $(CFLAGS)

clean:
	rm -f march8