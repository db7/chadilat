all: usb_serial/.patched include/vsync

usb_serial.zip:
	curl -L -o usb_serial.zip https://www.pjrc.com/teensy/usb_serial.zip

usb_serial: usb_serial.zip
	unzip usb_serial.zip

usb_serial/.patched: usb_serial
	patch -p1 < patches/const-globals
	@touch $@

libvsync.tar.gz:
	curl -L -o $@ \
	https://github.com/open-s4c/libvsync/archive/refs/tags/v4.2.1.tar.gz

libvsync-4.2.1: libvsync.tar.gz
	tar xfz libvsync.tar.gz

include/vsync: libvsync-4.2.1
	mkdir -p include
	cp -r libvsync-4.2.1/include .
	cp -r libvsync-4.2.1/vatomic/include .

clean:
	rm -rf usb_serial libvsync-* include

distclean: clean
	rm -f *.zip *.tar.gz

