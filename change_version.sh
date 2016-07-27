#!/bin/bash
set +x

vim ChangeLog || exit
vim VERSION || exit

./autogen.sh || exit
./configure || exit

./po-update.sh

