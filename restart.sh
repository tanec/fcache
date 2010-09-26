#!/bin/bash

set -x
set -e

F=`readlink -f $0`
D=`dirname $F`

function myip() {
    for nic in 0 1 2; do
        /sbin/ifconfig eth$nic 2>/dev/null | grep -q " UP .* MTU:" || continue
        /sbin/ifconfig eth$nic | grep -Eo "inet addr:[^ ]*" | cut -d: -f2
        return
    done
}

function getcfg() {
  case `myip` in
  202.108.1.121) echo '.165' ;;
  202.108.1.122) echo '.175.test'  ;;
  114.112.*)     echo '.114' ;;
  *) echo '' ;;
  esac
}

function start() {
  case $1 in
  ishc)
    $D/keeprun $D/ishc -i 0.0.0.0 ;;
  fcache)
    $D/keeprun $D/fcache -C $D/fcache.cfg`getcfg` ;;
  esac
}

ulimit -c unlimited
mkdir -p $D/../core
cd  $D/../core

export LC_ALL=zh_CN.utf8
pkill -9 -x $1 || true
ps -ef | grep -v grep | grep -E "keeprun $D/$1" || start $1
