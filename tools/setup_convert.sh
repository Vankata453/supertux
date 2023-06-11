#!/usr/bin/env bash

# Usage: tools/setup_convert.sh
# Expects a file `compat.arr` at the root of the repo.
# Will write to `data/images/convert.txt` directly.
# Will use a temporary `test.arr` which will be automatically removed.

cd "$(dirname "$0")/.."

if [ -f test.arr ]; then
  echo "There is already a test.arr; please move it or delete it before running this script." >&2
  exit 1
fi

grep -vE "^;;" compat.arr | tr '\n' ' ' > test.arr
sed -i 's/\([0-9]\+\) / \1 /g' test.arr

FILE_HASH="$(md5sum test.arr | cut -d ' ' -f 1)"
while true; do
  sed -i 's/\([0-9]\+\) \+\([0-9 ]*\)=> \+\([0-9]\+\)/\1 -> \3 # \2 => /g' test.arr
  TMP_FILE_HASH="$(md5sum test.arr | cut -d ' ' -f 1)"
  if [ "$TMP_FILE_HASH" = "$FILE_HASH" ]; then
    break
  fi
  FILE_HASH="$TMP_FILE_HASH"
done

sed 's/=>//g;s/# \*#/#/g;s/ \+/ /g;s/\([0-9]\) \+\([0-9]\)/\1#\2/g' test.arr | tr '#' '\n' | sed '/^ *$/d' | sed 's/^ *\([^ ].*[^ ]\) *$/\1/g;/^\([0-9]\+\) -> \1$/d' | grep -E "^[0-9]+ -> [0-9]+$" > data/images/convert.txt

rm test.arr
