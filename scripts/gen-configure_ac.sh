#!/bin/bash

set -e
set -o pipefail

test -f configure.ac.in
VERSION=$(./scripts/gen-version.sh)

languages=""
for f in po/*.po; do
	[ -f "$f" ] || continue
	languages="$languages $(basename $f .po)"
done


sed \
	-e "s/@@VERSION@@/$VERSION/" \
	-e "s/@@LANGUAGES@@/$(echo $languages)/" \
configure.ac.in >configure.ac
