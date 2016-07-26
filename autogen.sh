#!/bin/bash

test -f VERSION || exit
test -f configure.ac.in || exit
test -f Makefile.am.in || exit
test -f COPYING || exit

languages=""
for f in po/*.po; do
	test -f $f && languages="$languages $(basename $f .po)"
done

headers=""
for f in src/*.h; do
	test -f $f && headers="$headers $f"
done

sed \
	-e "s/@@VERSION@@/$(cat VERSION)/" \
	-e "s/@@LANGUAGES@@/$(echo $languages)/" \
configure.ac.in >configure.ac || exit 1

sed \
	-e "s#@@HEADERFILES@@#$(echo $headers)#" \
Makefile.am.in >Makefile.am || exit 1

mkdir -p m4
intltoolize --force --copy --automake || exit 1
autoreconf --force --install --verbose || exit 1
