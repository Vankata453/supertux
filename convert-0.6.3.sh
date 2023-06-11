#!/usr/bin/env bash

# Usage: gentileids.sh path/to/level.stl

set -e

DATA_ARR="$(dirname "$0")/test2.arr"

echo "Converting $1 with $DATA_ARR"

if grep -E "( |^)0[0-9]+( |$)" "$1"; then
  echo
  echo "Can't convert '$1': presence of numbers with leading zero" >&2
  exit 1
fi

VALUE="$(cat "$1" | tr '\n' '\t')"

for CONVERSION in $(cat "$DATA_ARR" | tr -d ' '); do
  NEEDLE="${CONVERSION%%->*}"
  REPLACE="${CONVERSION##*->}"
  #for i in $(seq 1 100); do
    VALUE="$(echo "$VALUE" | sed "s/\\((tiles[^)]*\\) $NEEDLE\\( \\|\\t\\)\\([^)]*)\\)/\\1 0$REPLACE\\2\\3/g")"
  #done
done

RESULT="$(echo "$VALUE" | sed 's/\([^0-9]\)0\([0-9]\+[^0-9]\)/\1\2/g' | tr '\t' '\n')"

echo "$VALUE" > "$1"
