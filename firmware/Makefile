DEPS=		../deps

CFLAGS= 	-Wall -Werror -Os
CFLAGS+=	-mmcu=atmega32u4
CFLAGS+=	-I$(DEPS)/usb_serial
CFLAGS+=	--param=min-pagesize=0
CFLAGS+=	-DF_CPU=16000000UL

SRCS=		main.c
SRCS+=		$(DEPS)/usb_serial/usb_serial.c

build: firmware.hex

prepare:
	$(MAKE) -C $(DEPS)

firmware.hex: prepare $(SRCS)
	avr-gcc $(CFLAGS) -o main.elf $(SRCS)
	avr-objcopy -O ihex main.elf firmware.hex

flash: firmware.hex
	teensy_loader_cli -mmcu=atmega32u4 -w firmware.hex

clean:
	rm -rf *.hex *.elf *.o
