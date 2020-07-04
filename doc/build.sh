#!/bin/sh

mdroff -Tpdf -e -t -p api.tr >api.pdf

mdroff -Tps -p rpcscheme.tr |  ps2eps > rpcscheme.eps

mdroff -Tps -p requeststruct1.tr | ps2eps >requeststruct1.eps
mdroff -Tps -p requeststruct2.tr | ps2eps >requeststruct2.eps
mdroff -Tps -p requeststruct3.tr | ps2eps >requeststruct3.eps
mdroff -Tps -p requeststruct4.tr | ps2eps >requeststruct4.eps
mdroff -Tps -p requeststruct5.tr | ps2eps >requeststruct5.eps

mdroff -Tps -p responsestruct1.tr | ps2eps >responsestruct1.eps
mdroff -Tps -p responsestruct2.tr | ps2eps >responsestruct2.eps
mdroff -Tps -p responsestruct3.tr | ps2eps >responsestruct3.eps
mdroff -Tps -p responsestruct4.tr | ps2eps >responsestruct4.eps
mdroff -Tps -p responsestruct5.tr | ps2eps >responsestruct5.eps

mdroff -Tpdf -e -t -p implementation.tr >implementation.pdf

rm *.eps
