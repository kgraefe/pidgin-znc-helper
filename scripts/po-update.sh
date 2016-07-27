#!/bin/bash
set +x

cd "$(dirname "$0")/../po"

echo "Updating POT template..."
intltool-update -pot -gettext-package=pidgin-znchelper || exit 1


for f in *.po; do
	echo "Checking ${f%.po} language file..."
	intltool-update ${f%.po} || exit 1
done

