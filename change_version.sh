#!/bin/bash
set +x

vim ChangeLog || exit
vim VERSION || exit

./autogen.sh || exit
./configure || exit
cp config.h config.h.mingw || exit

./po-update.sh

