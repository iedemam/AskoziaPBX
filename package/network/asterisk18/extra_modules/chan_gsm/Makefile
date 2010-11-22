
ASTDIR=/root/work/gsm/pads/trunk/build_warp/asterisk

COMPAT=$(ASTDIR)/include/asterisk/configman.h

all: $(COMPAT) chan_gsm.so

install: all
	install -m644 chan_gsm.so /usr/lib/asterisk/modules
	if [ ! -f /etc/asterisk/gsm.conf ] ; then \
		install -m644 gsm.conf /etc/asterisk/ ; \
	fi
	rm -rf /var/spool/asterisk/smsin/*


$(COMPAT):
	make -C compat ASTDIR=$(ASTDIR) install



PREFIX=
CC=$(CROSS_COMPILE)gcc

CFLAGS+=-ggdb -fPIC -Wall -D_GNU_SOURCE -D_REENTRANT
CFLAGS+=-I$(ASTDIR)/include

# SAM CFLAGS+=$(shell if test -f $(PREFIX)/usr/include/asterisk/abstract_jb.h; then echo '-DASTERISK_1_4' ; fi)
CFLAGS+=-DASTERISK_1_4 -DASTERISK_TRUNK


#LDFLAGS+=-lpthread
LDFLAGS+=-lrt


chan_gsm.so: chan_gsm.o gsmctl.o
	$(CC) -shared -Xlinker -x $(LDFLAGS) -o chan_gsm.so chan_gsm.o gsmctl.o

chan_gsm.o: chan_gsm.c
	$(CC) $(CFLAGS) -c -o chan_gsm.o chan_gsm.c

gsmctl.o: gsmctl.c
	$(CC) $(CFLAGS) -c -o gsmctl.o gsmctl.c

gsmtest: gsmctl.c
	$(CC) -DTEST_GSM -lpthread -ggdb -o gsmtest gsmctl.c

clean:
	rm -rf gsmtest *.o *.so
	make -C compat clean

release: clean
	DIR=chan_gsm-$(VERSION);\
	rm -rf /tmp/$$DIR ; \
	mkdir -p /tmp/$$DIR ; \
	cp * /tmp/$$DIR ; \
	cp -a compat /tmp/$$DIR ; \
	cd /tmp/; \
	tar czf chan_gsm-$(VERSION).tar.gz $$DIR

