# $Id: Makefile,v 1.3 2004/05/21 15:02:56 gonzo Exp $

SRCS=oslec_wrap.c echo.c
KMOD=oslec
CFLAGS+=-I$(PWD)/../spandsp-0.0.3/src/spandsp
WERROR=	-Wall
SRCS+=oslec.h

# TODO: this should only be defined for FreeBSD < 7.x
.BEGIN:
	patch -d /sys/sys < $(PWD)/libkern_memset.patch

# TODO: this should only be defined for FreeBSD < 7.x
.END:
	patch -R -d /sys/sys < $(PWD)/libkern_memset.patch

echo.c: $(PWD)/../spandsp-0.0.3/src/echo.c
	$(CC) -c $(PWD)/../spandsp-0.0.3/src/echo.c

load: oslec.ko
	kldload ./oslec.ko

unload:
	kldunload oslec

reload: unload load

.include <bsd.kmod.mk>
