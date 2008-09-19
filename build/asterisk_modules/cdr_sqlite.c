/*
 * Asterisk -- An open source telephony toolkit.
 *
 * Copyright (C) 2004 - 2005, Holger Schurig
 *
 *
 * Ideas taken from other cdr_*.c files
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

/*! \file
 *
 * \brief Store CDR records in a SQLite database.
 * 
 * \author Holger Schurig <hs4233@mail.mn-solutions.de>
 *
 * See also
 * \arg \ref Config_cdr
 * \arg http://www.sqlite.org/
 * 
 * Creates the database and table on-the-fly
 * \ingroup cdr_drivers
 */

/*** MODULEINFO
	<depend>sqlite</depend>
 ***/

#include "asterisk.h"

ASTERISK_FILE_VERSION(__FILE__, "$Revision: 69392 $")

#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sqlite.h>

#include "asterisk/channel.h"
#include "asterisk/module.h"
#include "asterisk/logger.h"
#include "asterisk/utils.h"

/* When you change the DATE_FORMAT, be sure to change the CHAR(19) below to something else */
#define DATE_FORMAT "%Y-%m-%d %T"

static char *name = "sqlite";
static sqlite* db = NULL;

AST_MUTEX_DEFINE_STATIC(sqlite_lock);

/*! \brief SQL table format */
static char sql_create_table[] = "CREATE TABLE cdr ("
"	id			INTEGER PRIMARY KEY,"
"	clid		VARCHAR(80),"
"	src			VARCHAR(80),"
"	dst			VARCHAR(80),"
"	dcontext	VARCHAR(80),"
"	channel		VARCHAR(80),"
"	dstchannel	VARCHAR(80),"
"	lastapp		VARCHAR(80),"
"	lastdata	VARCHAR(80),"
"	start		CHAR(19),"
"	answer		CHAR(19),"
"	end			CHAR(19),"
"	duration	INTEGER,"
"	billsec		INTEGER,"
"	disposition	VARCHAR(16),"
"	amaflags	INTEGER,"
"	accountcode	VARCHAR(20),"
"	uniqueid	VARCHAR(32),"
"	userfield	VARCHAR(255)"
");";

/*static char sql_create_trigger[] = "CREATE TRIGGER maxentries"
" AFTER INSERT ON cdr"
" BEGIN"
"	DELETE FROM cdr"
"	WHERE id <= (SELECT max(id) FROM cdr) - 200;"
" END;";*/

static int sqlite_log(struct ast_cdr *cdr)
{
	int res = 0;
	char *zErr = 0;
	struct tm tm;
	time_t t;
	char startstr[80];
	char answerstr[80];
	char endstr[80];
	char *dispositionstr = NULL;
	int count;

	ast_mutex_lock(&sqlite_lock);

	t = cdr->start.tv_sec;
	ast_localtime(&t, &tm, NULL);
	strftime(startstr, sizeof(startstr), DATE_FORMAT, &tm);

	t = cdr->answer.tv_sec;
	ast_localtime(&t, &tm, NULL);
	strftime(answerstr, sizeof(answerstr), DATE_FORMAT, &tm);

	t = cdr->end.tv_sec;
	ast_localtime(&t, &tm, NULL);
	strftime(endstr, sizeof(endstr), DATE_FORMAT, &tm);
	
	dispositionstr = ast_cdr_disp2str(cdr->disposition);

	for(count=0; count<5; count++) {
		res = sqlite_exec_printf(db,
			"INSERT INTO cdr ("
				"clid,"
				"src,"
				"dst,"
				"dcontext,"
				"channel,"
				"dstchannel,"
				"lastapp,"
				"lastdata,"
				"start,"
				"answer,"
				"end,"
				"duration,"
				"billsec,"
				"disposition,"
				"amaflags,"
				"accountcode,"
				"uniqueid,"
				"userfield"
			") VALUES ("
				"'%q', "	// clid
				"'%q', "	// src
				"'%q', "	// dst
				"'%q', "	// dcontext 
				"'%q', "	// channel
				"'%q', "	// dstchannel
				"'%q', "	// lastapp
				"'%q', "	// lastdata
				"'%q', "	// start
				"'%q', "	// answer
				"'%q', "	// end
				"%d, "		// duration
				"%d, "		// billsec
				"'%q', "		// disposition
				"%d, "		// amaflags
				"'%q', "	// accountcode
				"'%q', "	// uniqueiq
				"'%q' "		// userfield
			")", NULL, NULL, &zErr,
				cdr->clid, 
				cdr->src, 
				cdr->dst, 
				cdr->dcontext,
				cdr->channel, 
				cdr->dstchannel, 
				cdr->lastapp, 
				cdr->lastdata,
				startstr, 
				answerstr, 
				endstr,
				cdr->duration, 
				cdr->billsec, 
				dispositionstr,
				cdr->amaflags,
				cdr->accountcode,
				cdr->uniqueid,
				cdr->userfield
			);
		if (res != SQLITE_BUSY && res != SQLITE_LOCKED)
			break;
		usleep(200);
	}
	
	if (zErr) {
		ast_log(LOG_ERROR, "cdr_sqlite: %s\n", zErr);
		free(zErr);
	}

	ast_mutex_unlock(&sqlite_lock);
	return res;
}

static int unload_module(void)
{
	if (db)
		sqlite_close(db);
	ast_cdr_unregister(name);
	return 0;
}

static int load_module(void)
{
	char *zErr;
	char fn[PATH_MAX];
	int res;

	/* is the database there? */
	snprintf(fn, sizeof(fn), "%s/cdr.db", ast_config_AST_LOG_DIR);
	db = sqlite_open(fn, 0660, &zErr);
	if (!db) {
		ast_log(LOG_ERROR, "cdr_sqlite: %s\n", zErr);
		free(zErr);
		return -1;
	}

	/* is the table there? */
	res = sqlite_exec(db, "SELECT COUNT(id) FROM cdr;", NULL, NULL, NULL);
	if (res) {
		res = sqlite_exec(db, sql_create_table, NULL, NULL, &zErr);
		if (res) {
			ast_log(LOG_ERROR, "cdr_sqlite: Unable to create table 'cdr': %s\n", zErr);
			free(zErr);
			goto err;
		}
		/*res = sqlite_exec(db, sql_create_trigger, NULL, NULL, &zErr);
		if (res) {
			ast_log(LOG_ERROR, "cdr_sqlite: Unable to create maxentries trigger: %s\n", zErr);
			free(zErr);
			goto err;
		}*/

		/* TODO: here we should probably create an index */
	}
	
	res = ast_cdr_register(name, ast_module_info->description, sqlite_log);
	if (res) {
		ast_log(LOG_ERROR, "Unable to register SQLite CDR handling\n");
		return -1;
	}
	return 0;

err:
	if (db)
		sqlite_close(db);
	return -1;
}

AST_MODULE_INFO_STANDARD(ASTERISK_GPL_KEY, "SQLite CDR Backend");
