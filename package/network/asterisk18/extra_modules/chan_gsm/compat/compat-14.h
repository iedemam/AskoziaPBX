
#include "asterisk.h"

#include "asterisk/cli.h"
#include "asterisk/module.h"

typedef struct cm_cli_entry {
	char * cmda[AST_MAX_CMD_LEN];
	/*! Handler for the command (fd for output, # of args, argument list).
	  Returns RESULT_SHOWUSAGE for improper arguments.
	  argv[] has argc 'useful' entries, and an additional NULL entry
	  at the end so that clients requiring NULL terminated arrays
	  can use it without need for copies.
	  You can overwrite argv or the strings it points to, but remember
	  that this memory is deallocated after the handler returns.
	 */
	int (*handler)(int fd, int argc, char *argv[]);
	/*! Summary of the command (< 60 characters) */
	char *summary;
	/*! Detailed usage information */
	char *usage;
	/*! Generate the n-th (starting from 0) possible completion
	  for a given 'word' following 'line' in position 'pos'.
	  'line' and 'word' must not be modified.
	  Must return a malloc'ed string with the n-th value when available,
	  or NULL if the n-th completion does not exist.
	  Typically, the function is called with increasing values for n
	  until a NULL is returned.
	 */
	char *(*generator)(const char *line, const char *word, int pos, int n);
	struct ast_cli_entry *deprecate_cmd;
	/*! For keeping track of usage */
	int inuse;
	struct module *module;	/*! module this belongs to */
	char *_full_cmd;	/* built at load time from cmda[] */
	/* This gets set in ast_cli_register()
	  It then gets set to something different when the deprecated command
	  is run for the first time (ie; after we warn the user that it's deprecated)
	 */
	int deprecated;
	char *_deprecated_by;	/* copied from the "parent" _full_cmd, on deprecated commands */
	/*! For linking */
	AST_LIST_ENTRY(ast_cli_entry) list;
} cm_cli_entry_t;

typedef char * (*generator_t) (const char *line, const char *word, int pos, int state);

#define __static__ static
