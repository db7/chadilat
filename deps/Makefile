all: usb_serial/.patched

usb_serial.zip:
	curl -L -o usb_serial.zip https://www.pjrc.com/teensy/usb_serial.zip

usb_serial: usb_serial.zip
	unzip usb_serial.zip

usb_serial/.patched: usb_serial
	patch -p1 < patches/const-globals
	@touch $@

clean:
	rm -rf usb_serial

distclean: clean
	rm -f *.zip *.tar.gz

