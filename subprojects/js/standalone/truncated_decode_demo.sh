#!/bin/bash
set -euo pipefail

# Demo: verify truncated codestreams fail gracefully (no process abort).
# Usage:
#   ./truncated_decode_demo.sh <input.j2c> <output.pgm>
#
# The script keeps only the first 10 KiB from <input.j2c>, writes a truncated
# codestream to a temp file, and runs ojph_expand on it. A non-zero return code
# is acceptable; the important behavior is graceful termination.

if [ "$#" -ne 2 ]; then
  echo "Usage: $0 <input.j2c> <output.pgm>"
  exit 2
fi

INPUT_J2C="$1"
OUTPUT_PGM="$2"
TRUNCATED_J2C="$(mktemp /tmp/openjph-truncated-XXXXXX.j2c)"

trap 'rm -f "$TRUNCATED_J2C"' EXIT

dd if="$INPUT_J2C" of="$TRUNCATED_J2C" bs=1024 count=10 status=none

set +e
./ojph_expand -i "$TRUNCATED_J2C" -o "$OUTPUT_PGM"
RESULT=$?
set -e

echo "ojph_expand return code: $RESULT"
echo "If this process exits normally (even with non-zero code), the decoder handled truncation without aborting."
