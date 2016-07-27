#!/bin/bash

set +x

cd "$(dirname "$0")/.."

vim ChangeLog || exit 1
vim VERSION || exit 1

./autogen.sh || exit 1
./configure || exit 1

