#!/bin/bash

PROJECT=pidgin-znchelper
VERSION=$(cat VERSION)
REPOSITORY=ppa:konradgraefe/pidgin-plugins
DEB_REVISION=1

src_dir=$(pwd)

echo -n "make dist? (y/N) "
read -n 1 in
echo ""

if [ "$in" == "y" ]; then
	make dist
fi

rm -rf deb-pkg
mkdir deb-pkg
cd deb-pkg

tar xzvf ../${PROJECT}-${VERSION}.tar.gz

cd ${PROJECT}-${VERSION}

cp -r ${src_dir}/debian .
sed \
	-e "s/@@VERSION@@/${VERSION}/" \
	-e "s/@@DATE@@/$(date -R)/" \
	-e "s/@@DEB_REVISION@@/${DEB_REVISION}/" \
	debian/changelog.in >debian/changelog

sed \
	-e "s/@@DATE@@/$(date -R)/" \
	debian/copyright.in >debian/copyright

dpkg-buildpackage -S -rfakeroot

cd ..
lintian -i *.dsc

echo ""
echo -n "Upload ${PROJECT}_${VERSION}-${DEB_REVISION} to repository (y/N) "
read -n 1 in
echo ""

if [ "$in" == "y" ]; then
	dput ${REPOSITORY} ${PROJECT}_${VERSION}-${DEB_REVISION}_source.changes
fi

