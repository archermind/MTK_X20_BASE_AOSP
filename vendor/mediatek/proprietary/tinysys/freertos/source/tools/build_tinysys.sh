#!/bin/bash
###############################################################################
# This script provides the customer a quick way to build Tiny System from
# source.
###############################################################################

PROG=$(basename ${0})

usage() {
    cat >&2 <<- EOF
USAGE
  ${PROG} [-h] [...]

  This script provides a quicker way to build tinysys.

PREREQUISITE
  The Android environment must be initialize.
  That is, you need to run those steps at least once:
    $ cd <ANDROID_ROOT_DIR>
    $ . buid/envsetup.sh
    $ lunch

  This script must be executed in Android top directory.

OPTIONS
  -h            Print this help message

Other options for GNU make or build targets can be provided.
For example:
  clean         Clean up all built directories and objects
  configheader  Generate C header that contains all config options
  -jN           Run N parallel build tasks
EOF

exit 1
}

info() {
    echo "${PROG}: [INFO] ${*}"
}

error() {
    echo "${PROG}: [ERROR] ${*}"
    exit 1
}

check_Android_env() {
    if [ -z "${ANDROID_PRODUCT_OUT}" ] || [ -z "${TARGET_PRODUCT}" ] ; then
        cat >&2 <<- EOF
[ERROR] Android environment is not ready yet.

Please make sure build/envsetup.sh is sourced and lunch is executed.
EOF
        return 1
    fi

    return 0
}

run_build_cmd() {
    echo "Build command: ${*}"
    eval "${*}" || exit 1
}

#######################################
# Main
#######################################
TINYSYS_ROOT='vendor/mediatek/proprietary/tinysys/freertos'
SCP_TARGET='tinysys-scp'
CLEAN_SCP_TARGET="clean-${SCP_TARGET}"
SCP_ANDROID_MK="${TINYSYS_ROOT}/source/Android.mk"
CLEAN_TARGET=0

# Categorize options
for i in "${@}"; do
    case "${i}" in
    'clean')
        KEYWORDS="${KEYWORDS} ${i}"
        CLEAN_TARGET=1
        ;;
    '-h')
        usage
        ;;
    *) CMD_ARGS="${CMD_ARGS} ${i}"
    esac
done

check_Android_env || exit 1

# This script must be run in Android root directory
[ -f 'build/envsetup.sh' ] || \
    error "Please execute this command in Android top directory"

#######################################
# Here we build
#######################################
SCP_BUILD_CMD="ONE_SHOT_MAKEFILE=${SCP_ANDROID_MK} make ${SCP_TARGET} ${CMD_ARGS}"
SCP_CLEAN_CMD="ONE_SHOT_MAKEFILE=${SCP_ANDROID_MK} make ${CLEAN_SCP_TARGET} ${CMD_ARGS}"

if [ ${CLEAN_TARGET} -eq 1 ]; then
    run_build_cmd "${SCP_CLEAN_CMD}"
else
    run_build_cmd "${SCP_BUILD_CMD}"
fi

exit ${?}
