#!/bin/bash

set -x
set -e

F=`readlink -f $0`
D=`dirname $F`

function start() {
  case $1 in
  ishc)
    $D/keeprun $D/ishc -i 0.0.0.0 ;;
  fcache)
    $D/keeprun $D/fcache -C $D/fcache.cfg ;;
esac
}

pkill -9 $1 || true
ps -ef | grep -v grep | grep -E "keeprun $D/$1" || start $1
