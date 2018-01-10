#!/bin/bash

set -e
set -o pipefail

GETTEXT_PKG=$(awk -F= '/GETTEXT_PACKAGE=/ {print $2}' configure.ac)

cd "$(dirname "$0")/../po"

echo "Updating POT template..."
intltool-update -pot -gettext-package=$GETTEXT_PKG

sed \
	-i 's#^\"Content-Type: text/plain; charset=CHARSET\\n\"$#"Content-Type: text/plain; charset=UTF-8\\n"#g' \
	${GETTEXT_PKG}.pot
