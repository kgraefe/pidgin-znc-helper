#!/bin/bash
vim ChangeLog &&
vim VERSION &&
vim configure.in &&
./autogen.sh &&
./configure &&
cp config.h config.h.mingw
