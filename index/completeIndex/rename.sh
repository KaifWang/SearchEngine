#!/bin/bash
set -Eeuo pipefail

for f in hashfile*.bin; 
    do mv -- "$f" "${f#hashfile}" 
done