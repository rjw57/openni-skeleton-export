#!/bin/bash
#
# Run skeleton tracker and logger on a recording and ensure that output exists.

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR="${DIR}/build"
RECORDINGS_DIR="${DIR}/recordings"

# Utilities
H5LS=$(which h5ls)
if [ -z "${H5LS}" ]; then
	echo "Could not find h5ls utility."
	exit 1
fi

if [ ! -d "${BUILD_DIR}" ]; then
	echo "Could not find build directory at ${BUILD_DIR}."
	exit 1
fi

if [ ! -d "${RECORDINGS_DIR}" ]; then
	echo "Could not find recordings directory at ${RECORDINGS_DIR}."
	exit 1
fi

LOGSKEL="${BUILD_DIR}/logskel"
if [ ! -x "${LOGSKEL}" ]; then
	echo "${LOGSKEL} does not exist or is not executable"
	exit 1
fi

# Try running logger
LOG_FILE="/tmp/logskel"
"${LOGSKEL}" --playback "${RECORDINGS_DIR}/Captured-2014-10-31.oni" --duration 8 --log ${LOG_FILE}
if [ $? -ne 0 ]; then
	echo "Logging command failed."
	exit 1
fi

# Check output exists
echo "Checking ${LOG_FILE} exists..."
if [ ! -s "${LOG_FILE}" ]; then
	echo "${LOG_FILE} does not exist or has zero size"
	exit 1
fi
echo "Checking ${LOG_FILE} is parseable..."
_h5ls_out=$(${H5LS} -r "${LOG_FILE}")
echo "Checking depth in ${LOG_FILE}"
if ! echo "${_h5ls_out}" | grep -q 'frame_000050/depth'; then
	echo "depth not present in h5ls output"
	exit 1
fi
if ! echo "${_h5ls_out}" | grep -q 'frame_000050/label'; then
	echo "label not present in h5ls output"
	exit 1
fi
