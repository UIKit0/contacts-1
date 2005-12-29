#! /bin/sh
autoreconf -v --install || exit 1
./configure --enable-maintainer-mode --enable-gnome-vfs --enable-gconf "$@"
