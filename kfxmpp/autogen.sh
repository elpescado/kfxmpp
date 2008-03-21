#!/bin/bash

echo "Generating kfxmpp configuration..."
echo ""

echo "running libtoolize..."
libtoolize

echo "running aclocal..."
aclocal-1.9

echo "running autoheader..."
autoheader

echo "running automake..."
automake-1.9 -a --gnu

echo "running autoconf..."
autoconf

echo ""
echo "Setup done. Type './configure' to proceed"
echo ""
