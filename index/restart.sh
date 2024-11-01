#!/bin/bash
set -Eeuo pipefail

git pull
make clean
make
./indexServer