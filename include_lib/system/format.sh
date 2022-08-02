#!/bin/sh

FILES=`git diff --diff-filter=ACMR --name-only | grep -E "\.(c|cpp|h)$"`

for FILE in $FILES; do
  format-source $FILE
done

    remove-orig
