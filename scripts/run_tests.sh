#!/bin/sh

# TODO: Remove this when RPC is fixed
rm "build/rpc_test"

set -e
for f in build/*_test; do
  echo "Running test: $f"
  ./"$f"
done
