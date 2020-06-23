#!/bin/sh

for f in build/*_test; do
  ./"$f"
done
