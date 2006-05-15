#!/bin/sh
FIRST=".glade-diff-1"
SECOND=".glade-diff-2"
xmllint --format "$1" > $FIRST
xmllint --format "$2" > $SECOND

diff -u $FIRST $SECOND | sed -e "s/--- $FIRST/--- $1/g" | \
                         sed -e "s/+++ $SECOND/+++ $2/g"
rm -f $FIRST $SECOND
