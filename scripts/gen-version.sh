#!/bin/bash

test -f VERSION || exit 1

VERSION=$(cat VERSION)
if [[ $VERSION == *"-dev" ]]; then
	if [[ -d .git ]]; then
		VERSION="$VERSION-git$(git describe --always --dirty)"
	fi
fi

echo $VERSION
