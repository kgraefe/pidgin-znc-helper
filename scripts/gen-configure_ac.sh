#!/bin/bash

set +x

test -f configure.ac.in || exit
VERSION=$(./scripts/gen-version.sh) || exit 1

languages=""
for f in po/*.po; do
	test -f $f && languages="$languages $(basename $f .po)"
done


sed \
	-e "s/@@VERSION@@/$VERSION/" \
	-e "s/@@LANGUAGES@@/$(echo $languages)/" \
configure.ac.in >configure.ac || exit 1
