#!/bin/bash
set -Eeuo pipefail

moveDir()
{
    mv outputs/parsed_output ../parsed_output_save$1 && mkdir -p outputs/parsed_output
}

count=30
while true; do
    numFile=$(ls outputs/parsed_output | wc -l)
    if [[ $numFile -gt 20000 ]]; then
        moveDir $count
        ((count=count+1))
    fi;
    sleep 300
done
