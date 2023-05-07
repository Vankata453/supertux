#!/usr/bin/env bash

# Usage: gentileids.sh path/to/image.png
# This will add a new image to data/images/tiles.strf with new IDs based on
# the `;; next-id` line in the file. It also updates that line after running.
# Note that it will not set any attributes.

set -e

for file in "$@"; do
  if [ ! -f "$file" ]; then
    echo "No file '$file' found" >&2
    exit 1
  fi

  IMG_FOLDER="$(dirname "$0")/../data/images"
  TILES_FILE="$IMG_FOLDER/tiles.strf"
  NEXT_ID_LINE="$(grep 'next-id' "$TILES_FILE" | head -1)"
  REF_PATH="$(realpath --relative-to="$IMG_FOLDER" "$file")"

  if [ "$(grep "\"$REF_PATH\"" "$TILES_FILE")" != "" ]; then
    echo "File '$REF_PATH' already in tiles" >&2
    exit 1
  fi

  if [ "$NEXT_ID_LINE" == "" ]; then
    echo "Can't find 'next-id' line" >&2
    exit 1
  fi

  NEXT_ID=$(echo $NEXT_ID_LINE | sed 's/[^0-9]//g')

  if [ "$NEXT_ID" == "" ]; then
    echo "Can't find next id from next-id line" >&2
    exit 1
  fi

  if [ "$(($NEXT_ID > 10000 || $NEXT_ID < 4000))" == "1" ]; then
    echo "Next id '$NEXT_ID' is probably wrong" >&2
    exit 1
  fi

  WIDTH="$(identify -format "%w" "$file")"
  HEIGHT="$(identify -format "%h" "$file")"

  if [ "$(($WIDTH % 32 != 0 || $HEIGHT % 32 != 0))" == "1" ]; then
    echo "Wrong dimensions $WIDTH x $HEIGHT for image" >&2
    exit 1
  fi

  TW=$(($WIDTH / 32))
  TH=$(($HEIGHT / 32))

  sed -i '$ d' "$TILES_FILE"

  cat <<EOF >> "$TILES_FILE"
  (tiles
    (width $TW)(height $TH)
    (ids
EOF

  for y in $(seq 0 $(($TH - 1))); do
    echo -n "     " >> "$TILES_FILE"
    for x in $(seq 0 $(($TW - 1))); do
      echo -n " $(($NEXT_ID + $y * $TW + $x))" >>"$TILES_FILE"
    done
    echo >> "$TILES_FILE"
  done

  cat <<EOF >> "$TILES_FILE"
    )
    (image "$(realpath --relative-to="$IMG_FOLDER" "$file")")
  )
)
EOF

  sed -i "s/\(next-id.*\)[0-9]\{4\}/\1$(($NEXT_ID + $TH * $TW))/g" "$TILES_FILE"
done
