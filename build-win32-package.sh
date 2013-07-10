#!/bin/bash
set -x

PROJECT=pidgin-znchelper
WIN32DIR=${PROJECT}-$(cat VERSION)-win32

test -f README.win32 || exit

make -f Makefile.mingw clean || exit
make -f Makefile.mingw || exit
mkdir -p ${WIN32DIR}/pidgin/plugins || exit
sed 's/$/\r/' ChangeLog >${WIN32DIR}/ChangeLog.txt || exit
sed 's/$/\r/' README.win32 >${WIN32DIR}/ReadMe.txt || exit
cp src/${PROJECT}.dll ${WIN32DIR}/pidgin/plugins || exit
#i586-mingw32msvc-strip --strip-unneeded ${WIN32DIR}/pidgin/plugins/${PROJECT}.dll || exit
for f in po/*.po; do
	if [ -f $f ]; then
		lang=$(basename $f .po)
		mkdir -p ${WIN32DIR}/pidgin/locale/${lang}/LC_MESSAGES || exit
		cp po/${lang}.gmo ${WIN32DIR}/pidgin/locale/${lang}/LC_MESSAGES/${PROJECT}.mo || exit
	fi
done
rm -f ${WIN32DIR}.zip || exit
cd ${WIN32DIR} || exit
zip -r ../${WIN32DIR}.zip * || exit
cd .. || exit
rm -rf ${WIN32DIR} || exit
echo "filename: ${WIN32DIR}.zip"

