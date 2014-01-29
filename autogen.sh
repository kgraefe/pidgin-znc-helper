#!/bin/bash

test -f VERSION || exit
test -f ChangeLog|| exit
test -f configure.ac.in || exit
test -f COPYING || exit

./po-update.sh || exit

languages=""
for f in po/*.po
do test -f $f && languages="$languages $(basename $f .po)"
done

headers=""
for f in src/*.h
do test -f $f && headers="$headers $f"
done

sed -e "s/@@VERSION@@/$(cat VERSION)/" -e "s/@@LANGUAGES@@/$(echo $languages)/" configure.ac.in >configure.ac || exit
sed -e "s#@@HEADERFILES@@#$(echo $headers)#" Makefile.am.in >Makefile.am || exit
aclocal || exit
autoheader || exit
libtoolize --copy || exit
automake --add-missing --copy || exit
autoconf || exit
libtoolize --copy --install || exit
intltoolize --copy --force  || exit

