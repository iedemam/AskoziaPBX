/*
 * $Id: cdr_syslog.c 252 2007-10-30 12:38:16Z michael.iedema $
 * 
 * Simple Syslog CDR logger - part of AskoziaPBX <http://askozia.com/pbx>
 * 
 * Copyright (C) 2008 tecema (a.k.a IKT) <http://www.tecema.de> 
 *
 * See http://www.asterisk.org for more information about
 * the Asterisk project. Please do not directly contact
 * any of the maintainers of this project for assistance;
 * the project provides a web site, mailing lists and IRC
 * channels for your use.
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License Version 2. See the LICENSE file
 * at the top of the source tree.
 */

#include "asterisk.h"

ASTERISK_FILE_VERSION(__FILE__, "$Revision: 110779 $")

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "asterisk/channel.h"
#include "asterisk/cdr.h"
#include "asterisk/module.h"
#include "asterisk/config.h"
#include "asterisk/pbx.h"
#include "asterisk/logger.h"
#include "asterisk/utils.h"
#include "asterisk/lock.h"

#define DATE_FORMAT "%Y-%m-%d %T"

AST_MUTEX_DEFINE_STATIC(lock);

static char *name = "cdr-syslog";

static char master[PATH_MAX];
static char format[1024]="";

static int load_config(int reload) 
{
	struct ast_config *cfg;
	struct ast_variable *var;
	int res = -1;

	strcpy(format, "");
	strcpy(master, "");
	ast_mutex_lock(&lock);
	if((cfg = ast_config_load("cdr_custom.conf"))) {
		var = ast_variable_browse(cfg, "mappings");
		while(var) {
			if (!ast_strlen_zero(var->name) && !ast_strlen_zero(var->value)) {
				if (strlen(var->value) > (sizeof(format) - 1))
					ast_log(LOG_WARNING, "Format string too long, will be truncated, at line %d\n", var->lineno);
				ast_copy_string(format, var->value, sizeof(format) - 1);
				strcat(format,"\n");
				snprintf(master, sizeof(master),"%s/%s/%s", ast_config_AST_LOG_DIR, name, var->name);
				if (var->next) {
					ast_log(LOG_NOTICE, "Sorry, only one mapping is supported at this time, mapping '%s' will be ignored at line %d.\n", var->next->name, var->next->lineno); 
					break;
				}
			} else
				ast_log(LOG_NOTICE, "Mapping must have both filename and format at line %d\n", var->lineno);
			var = var->next;
		}
		ast_config_destroy(cfg);
		res = 0;
	} else {
		if (reload)
			ast_log(LOG_WARNING, "Failed to reload configuration file.\n");
		else
			ast_log(LOG_WARNING, "Failed to load configuration file. Module not activated.\n");
	}
	ast_mutex_unlock(&lock);
	
	return res;
}



static int syslog_log(struct ast_cdr *cdr)
{
	char buf[2048];
	struct ast_channel dummy;

	if (ast_strlen_zero(master))
		return 0;

	memset(buf, 0 , sizeof(buf));
	memset(&dummy, 0, sizeof(dummy));
	dummy.cdr = cdr;
	pbx_substitute_variables_helper(&dummy, format, buf, sizeof(buf) - 1);
	ast_log(LOG_CDR, "%s", buf);
	return 0;
}

static int unload_module(void)
{
	ast_cdr_unregister(name);
	return 0;
}

static int load_module(void)
{
	int res = 0;

	if (!load_config(0)) {
		res = ast_cdr_register(name, ast_module_info->description, syslog_log);
		if (res)
			ast_log(LOG_ERROR, "Unable to register syslog CDR handling\n");
		return res;
	} else 
		return AST_MODULE_LOAD_DECLINE;
}

static int reload(void)
{
	return load_config(1);
}

AST_MODULE_INFO(ASTERISK_GPL_KEY, AST_MODFLAG_DEFAULT, "Syslog CDR Backend",
	.load = load_module,
	.unload = unload_module,
	.reload = reload,
);

