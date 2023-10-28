#!/bin/bash

set -e
DIR=$(mktemp -d -t "weld-XXXXXX")
make && WELD_TMPDIR="$DIR" ./bin/testweld

