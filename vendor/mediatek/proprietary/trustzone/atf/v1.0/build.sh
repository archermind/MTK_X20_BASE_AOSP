#!/bin/bash

echo "[ATF] build mode : $1"
#cp $TEST_BINARY $2

echo "[ATF] output file : $2"
echo "[ATF] output folder should be : $3"
echo "[ATF] platform : $4"
echo "[ATF] secure OS support : $5"
echo "[ATF] mach_type : $6"

if [ "$1" = "Debug" ] ; then
DEBUG_ENABLE=1
echo "[ATF] debug enable "
else
DEBUG_ENABLE=1
echo "[ATF] debug disable "
fi

#    make realclean
#echo "[ATF] make realclean "
#    make ATF_BIN=$2 BUILD_BASE=$3/ATF_OBJ DEBUG=${DEBUG_ENABLE} PLAT=$4 SECURE_OS=$5 all
    make ATF_BIN=$2 BUILD_BASE=$3/ATF_OBJ DEBUG=${DEBUG_ENABLE} PLAT=$4 SECURE_OS=$5 MACH_TYPE=$6 all

if [ $? -ne 0 ]; then
    echo "[ERROR] ARM trusted firmware compile failed!"
    exit 1;
fi

echo "[ATF] build code "

echo "[ATF] output file : $2"
#echo "[ATF] BUILD_BASE : ${BUILD_BASE}"

