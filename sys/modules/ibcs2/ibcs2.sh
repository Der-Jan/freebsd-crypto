#!/bin/sh
# $FreeBSD$
if [ $# -le 1 ]; then
	LOADERS="coff" # elf
fi

set -e

kernelfile=`sysctl -n kern.bootfile`
kernelfile=`basename $kernelfile`
newkernelfile="/tmp/${kernelfile}+ibcs2"

modload -e ibcs2_mod -o $newkernelfile -q /lkm/ibcs2_mod.o

for loader in $LOADERS; do
	modload -e ibcs2_${loader}_mod -o/tmp/ibcs2_${loader} -q -u \
		-A${newkernelfile} /lkm/ibcs2_${loader}_mod.o
done
rm ${newkernelfile}
set +e
