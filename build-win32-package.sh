#!/bin/bash
make -f Makefile.mingw clean && \
make -f Makefile.mingw && \
PROJECT=znchelper && \
WIN32DIR=${PROJECT}-`cat VERSION`-win32 && \
mkdir -p ${WIN32DIR}/pidgin/plugins && \
mkdir -p ${WIN32DIR}/pidgin/locale/de/LC_MESSAGES && \
sed 's/$/\r/' ChangeLog >${WIN32DIR}/ChangeLog.txt && \
sed 's/$/\r/' README.win32 >${WIN32DIR}/ReadMe.txt && \
cp src/${PROJECT}.dll ${WIN32DIR}/pidgin/plugins && \
i586-mingw32msvc-strip --strip-unneeded ${WIN32DIR}/pidgin/plugins/${PROJECT}.dll && \
cp po/de.gmo ${WIN32DIR}/pidgin/locale/de/LC_MESSAGES/${PROJECT}.mo && \
rm -f ${WIN32DIR}.zip && \
cd ${WIN32DIR} && \
zip -r ../${WIN32DIR}.zip * && \
cd .. && \
rm -rf ${WIN32DIR}
