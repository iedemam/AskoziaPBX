
#CC=gcc

CFLAGS=-D_GNU_SOURCE -DAST_MODULE=\"res_configman\"

# SAM CFLAGS += -include $(shell if grep AST_MODULE_INFO /usr/include/asterisk/module.h>/dev/null ; then echo "compat-14.h"; else echo "compat-12.h"; fi)
#CFLAGS += -include "compat-14.h"
CFLAGS += -include "compat-16.h"

CFLAGS += -fPIC
CFLAGS+=-I$(ASTDIR)/include

# We need this for Asterisk 1.6
CFLAGS += -DASTERISK_TRUNK

TRUNK-CM=/home/nadi/asterisk/trunk-cm-csel-hash

# SAM FIXME
INSTALL_MODPATH=$(ASTDIR)/modules

all: res_configman.so res_csel.so res_statemachine.so

res_configman.so: res_configman.o hash.o
	$(CC) -shared -Xlinker -x -o $@ res_configman.o hash.o

res_configman.o: res_configman.c
	sed -ie 's/^#include "asterisk.h"$$//' res_configman.c
	sed -ie 's/^ASTERISK_FILE_VERSION.*$$//' res_configman.c
	sed -ie 's/static int \(load_module\|unload_module\|reload\)/__static__ int \1/' res_configman.c
	sed -ie 's/^\([ \t]*ast_cli_[un]*register[^(]*(\)\([^(].*\)$$/\1(struct ast_cli_entry *)\2/' res_configman.c
	sed -ie 's/struct ast_cli_entry clis/cm_cli_entry_t clis/' res_configman.c
	sed -ie 's/generator = cli/generator = (generator_t)cli/' res_configman.c
	$(CC) $(CFLAGS) -c res_configman.c

hash.o: hash.c
	sed -ie 's/^#include "asterisk.h"$$//' hash.c
	$(CC) $(CFLAGS) -c hash.c

res_csel.so: res_csel.o
	$(CC) -shared -Xlinker -x -o $@ res_csel.o

res_csel.o: res_csel.c
	sed -ie 's/^#include "asterisk.h"$$//' res_csel.c
	sed -ie 's/^ASTERISK_FILE_VERSION.*$$//' res_csel.c
	sed -ie 's/static int \(load_module\|unload_module\|reload\)/__static__ int \1/' res_csel.c
	$(CC) $(CFLAGS) -c res_csel.c

res_statemachine.so: res_statemachine.o
	$(CC) -shared -Xlinker -x -o $@ res_statemachine.o

res_statemachine.o: res_statemachine.c
	sed -ie 's/^#include "asterisk.h"$$//' res_statemachine.c
	sed -ie 's/^ASTERISK_FILE_VERSION.*$$//' res_statemachine.c
	sed -ie 's/static int \(load_module\|unload_module\|reload\)/__static__ int \1/' res_statemachine.c
	$(CC) $(CFLAGS) -c res_statemachine.c

clean:
	rm -f *.so *.o *.ce

update: clean
	cp $(TRUNK-CM)/main/hash.c $(TRUNK-CM)/res/res_{configman,csel,statemachine}.c .
	cp $(TRUNK-CM)/include/asterisk/{hash,configman,csel,statemachine}.h asterisk/

install: res_configman.so res_csel.so res_statemachine.so
	mkdir -p $(INSTALL_MODPATH)
	install -m 755 res_configman.so $(INSTALL_MODPATH)/
	install -m 755 res_csel.so $(INSTALL_MODPATH)/
	install -m 755 res_statemachine.so $(INSTALL_MODPATH)/
	install -m 644 asterisk/*.h $(ASTDIR)/include/asterisk/

