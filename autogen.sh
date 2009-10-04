#! /bin/sh

aclocal \
&& autoheader \
&& automake --add-missing \
&& autoconf \
&& (intltoolize --version) < /dev/null > /dev/null 2>&1 || {
    echo;
    echo "You must have intltool installed to compile purple-znchelper";
    echo;
    exit;
}
