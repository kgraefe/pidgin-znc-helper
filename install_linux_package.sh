#!/bin/bash

if [ $UID -eq 0 ]; then
	echo There is no need to run this script with superuser privileges.
	echo You have to build the plugin from source in order to get a system wide installation.
	exit
fi

if [ -z $PIDGIN_CONFIG_DIR ]; then
	PIDGIN_CONFIG_DIR=${HOME}/.purple
fi

echo -n "Enter your pidgin's config directory: [$PIDGIN_CONFIG_DIR] "
read in

if [ $in ]; then
	PIDGIN_CONFIG_DIR=$in
fi

if [ ! -d ${PIDGIN_CONFIG_DIR} ]; then
	echo ERROR: ${PIDGIN_CONFIG_DIR} is not a directory.
	exit
fi

cp -rv plugins/ ${PIDGIN_CONFIG_DIR}
cp -rv locale/ ${PIDGIN_CONFIG_DIR}
