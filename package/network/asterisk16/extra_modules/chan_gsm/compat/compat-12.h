
#include <limits.h>
#include <stdint.h>

#include "asterisk/cli.h"
#include "asterisk/module.h"

typedef struct ast_cli_entry cm_cli_entry_t;

#define AST_MODULE_INFO(KEY, FLAGS, DESC, LOAD, UNLOAD, RELOAD, REST)	\
int usecount (void) { return 1; }	\
char *description (void) { static char *desc = DESC; return desc; }	\
char *key (void) { return KEY; }

#define ast_module_ref(...)
#define ast_module_unref(...)

#define __static__

typedef char * (*generator_t) (char *line, char *word, int pos, int state);
