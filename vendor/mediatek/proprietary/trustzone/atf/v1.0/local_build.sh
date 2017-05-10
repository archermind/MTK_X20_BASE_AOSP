#!/bin/bash

CFLAGS='-O0 -gdwarf-2' \
#CROSS_COMPILE=../../../prebuilts/gcc/linux-x86/aarch64/linaro-aarch64-linux-gnu-4.8/bin/aarch64-linux-gnu- \

#make DEBUG=1 PLAT=mt6752 all
#make DEBUG=1 PLAT=mt6735 all
make DEBUG=1 PLAT=mt6797 all MACH_TYPE=mt6797
#make DEBUG=1 PLAT=fvp all

#make DEBUG=1 PLAT=mt6752 SPD=tspd all


