#!/bin/bash
set +x

cd "$(dirname "$0")/../po"

echo "Updating POT template..."
intltool-update -pot -gettext-package=pidgin-znchelper || exit 1

sed \
	-i 's#^\"Content-Type: text/plain; charset=CHARSET\\n\"$#"Content-Type: text/plain; charset=UTF-8\\n"#g' \
	pidgin-znchelper.pot
