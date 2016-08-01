#!/bin/bash

test -f Makefile.am.in || exit 1
test -f COPYING || exit 1

./scripts/gen-configure_ac.sh || exit 1

headers=""
for f in src/*.h; do
	test -f $f && headers="$headers $f"
done

sed \
	-e "s#@@HEADERFILES@@#$(echo $headers)#" \
Makefile.am.in >Makefile.am || exit 1

mkdir -p m4
intltoolize --force --copy --automake || exit 1
autoreconf --force --install --verbose || exit 1
