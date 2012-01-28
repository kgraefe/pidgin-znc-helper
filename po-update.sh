#!/bin/bash
set -x

ls -1 src/*.c > po/POTFILES.in || exit 1

cd po || exit 1
intltool-update -po || exit 1

for f in *.po
do test -f $f && intltool-update ${f%.po}
done

exit 0

