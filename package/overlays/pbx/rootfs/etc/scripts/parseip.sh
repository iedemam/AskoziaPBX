#!/bin/sh
/sbin/ifconfig eth0 | awk '/dr:/{gsub(/.*:/,"",$2);print$2}'
