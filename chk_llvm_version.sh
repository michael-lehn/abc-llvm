#!/bin/bash

versions="17 18 19 20 21 22"

rm -f chk_llvm.log

for version in $versions; do
    make clean

    if make -j 4 \
        llvm-config=/usr/local/opt/llvm@${version}/bin/llvm-config \
        install && \
	(cd raylib-example && make clean && make) && \
	(cd not-abc && make clean && make)
    then
        echo "${version} ok" >> chk_llvm.log
    else
        echo "${version} failed" >> chk_llvm.log
    fi
done

cat chk_llvm.log
