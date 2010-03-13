#!/bin/bash
vim ChangeLog &&
vim VERSION &&
sed "s/@@VERSION@@/$(cat VERSION)/" configure.in.in >configure.in &&
./autogen.sh &&
./configure &&
cp config.h config.h.mingw
