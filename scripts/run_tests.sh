#!/bin/sh

set -e
for f in build/*_test; do
  echo "Running test: $f"
  ./"$f"
done
