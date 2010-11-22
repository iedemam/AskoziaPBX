
#include "asterisk.h"

#include "asterisk/cli.h"
#include "asterisk/module.h"

/*! \brief descriptor for a cli entry. 
 * \arg \ref CLI_command_API
 */
typedef struct cm_cli_entry {
	char * cmda[AST_MAX_CMD_LEN];		/*!< words making up the command.
						* set the first entry to NULL for a new-style entry. */

	char *summary; 				/*!< Summary of the command (< 60 characters) */
	char *usage; 				/*!< Detailed usage information */

	int inuse; 				/*!< For keeping track of usage */
	struct module *module;			/*!< module this belongs to */
	char *_full_cmd;			/*!< built at load time from cmda[] */
	int cmdlen;				/*!< len up to the first invalid char [<{% */
	/*! \brief This gets set in ast_cli_register()
	  It then gets set to something different when the deprecated command
	  is run for the first time (ie; after we warn the user that it's deprecated)
	 */
	int args;				/*!< number of non-null entries in cmda */
	char *command;				/*!< command, non-null for new-style entries */
	cli_fn handler;
	/*! For linking */
	AST_LIST_ENTRY(ast_cli_entry) list;
} cm_cli_entry_t;

#define __static__ static
