MCU=atmega32
CC=avr-gcc
OBJCOPY=avr-objcopy
CFLAGS=-std=c99 -Wall -g -Os -mmcu=atmega32a -I.
TARGET=bootloader
SRCS=bootloader.c

all:
	${CC} ${CFLAGS} -o ${TARGET}.bin ${SRCS}
	${OBJCOPY} -j .text -j .data -O ihex ${TARGET}.bin ${TARGET}.hex

flash:
	avrdude -p ${MCU} -c atmelice_isp -U flash:w:${TARGET}.hex:i -F -P usb

clean:
	rm -f *.bin *.hex

