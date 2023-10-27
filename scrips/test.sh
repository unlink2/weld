#!/bin/bash

DIR="./tmp"

mkdir -p "$DIR"

make && WELD_TMPDIR="$DIR" ./bin/testweld

