#!/bin/bash

mkdir -p "./tmp"

make && WELD_TMPDIR="./tmp" ./bin/testweld

