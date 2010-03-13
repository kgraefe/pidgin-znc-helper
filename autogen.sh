#! /bin/sh

aclocal \
&& autoheader \
&& automake --add-missing \
&& autoconf \
&& libtoolize --copy --force --install \
&& ((intltoolize --version) < /dev/null > /dev/null 2>&1 || {
    echo;
    echo "You must have intltool installed to compile birthday reminder!";
    echo;
    exit;
})
