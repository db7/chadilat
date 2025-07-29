DEPS=		../deps
CFLAGS= 	-Wall -Werror -O2 -g
CFLAGS+=	-I${DEPS}/include
LDFLAGS=	-L/usr/pkg/lib
LDFLAGS+=	-lraylib -lpthread

build: viewer

main.o: main.c
	${CC} ${CFLAGS} -c -o $@ main.c
viewer: main.o
	${CC} -o $@ main.o ${LDFLAGS}
clean:
	rm -rf viewer *.o
