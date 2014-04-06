#!/usr/bin/env sh

# build a.out if it does not exist
export old_d=$PWD
export new_d=`dirname $0`
cd $new_d ;
[ -x a.out ] || make > /dev/null 2>&1 ;
./a.out $@ ;
cd $old_d ;
