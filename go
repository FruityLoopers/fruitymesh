#!/bin/bash

set -e

function helptext {
    echo "Usage: ./go <command>"
    echo ""
    echo "Available commands are:"
    echo "    deploy            Deploy latest built FruityMesh to all attached devices"
    echo "    term <tty.file>   Open specified terminal to attached device"
    echo "    compile           Clean and compile FruityMesh source"
}

function deploy-to-all-local-devices {
    $HOME/nrf/projects/fruitymesh/deploy/deploy-to-all.sh
}

function term {
    if [ -z "$1" ]
    then
        echo "No tty file supplied."
        exit 1
    fi
    minicom --device $1 --b 38400
}

function compile {
    make clean && make
}

case "$1" in
    deploy) deploy-to-all-local-devices
    ;;
    term) term "$2"
    ;;
    compile) compile
    ;;
    *) helptext
    ;;
esac
