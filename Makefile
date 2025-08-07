all:
	${MAKE} -C firmware build
	${MAKE} -C reader
	${MAKE} -C viewer

flash:
	${MAKE} -C firmware flash

clean:
	${MAKE} -C firmware clean
	${MAKE} -C reader clean
	${MAKE} -C viewer clean
