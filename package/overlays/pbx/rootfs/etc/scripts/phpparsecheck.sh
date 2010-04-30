#!/bin/sh

# $Id$
# part of AskoziaPBX (http://askozia.com/pbx)
#
# Copyright (C) 2010 tecema (a.k.a IKT) <http://www.tecema.de>.
# All rights reserved.

# unset CGI environment variables so as not to confuse PHP
unset CONTENT_TYPE GATEWAY_INTERFACE REMOTE_USER REMOTE_ADDR AUTH_TYPE
unset HTTP_USER_AGENT CONTENT_LENGTH SCRIPT_FILENAME HTTP_HOST
unset SERVER_SOFTWARE HTTP_REFERER SERVER_PROTOCOL REQUEST_METHOD
unset SERVER_PORT SCRIPT_NAME SERVER_NAME

/usr/bin/php -l $@
