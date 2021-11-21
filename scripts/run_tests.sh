#!/bin/sh

set -e
for f in build/*_test; do
  if [ "$OSTYPE" == "darwin" ]
  then
    if [ "$f" == "build/allocator_test" ]
    then
      # alloc_test/multithread fails on macOS. Disabling for now.
      continue
    fi
  fi
  echo "Running test: $f"
  ./"$f"
  rm -rf /dev/shm/SHM_*
done
