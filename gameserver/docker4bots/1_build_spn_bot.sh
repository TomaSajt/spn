#!/bin/bash -e

source $(dirname $0)/config.sh

usage() {
	echo "usage: $0 <programming-language> <version-id> <bot-name> <code-file> <file-extension>"
}

PROGLANG="$1"
VERSION_ID=$2
BOT_NAME=$3
CODE_FILE=$4
EXTENSION=$5

if [ -z "$PROGLANG" ]; then
	echo "Argument required: programming language"
	usage
	exit 1
fi

if [ -z "$VERSION_ID" ]; then
	echo "Argument required: snake version id"
	usage
	exit 1
fi

if [ -z "$BOT_NAME" ]; then
	echo "Argument required: bot name"
	usage
	exit 1
fi

if [ -z "$CODE_FILE" ]; then
	echo "Argument required: path to user code file"
	usage
	exit 1
fi

if [ -z "$EXTENSION" ]; then
	echo "Argument required: code file name extension"
	usage
	exit 1
fi

BOT_DATADIR="$SPN_DATA_HOSTDIR/${BOT_NAME}_$VERSION_ID"

mkdir -p "$BOT_DATADIR"
chmod 777 "$BOT_DATADIR"

install -m 444 "$CODE_FILE" "$BOT_DATADIR/usercode.$EXTENSION"

exec docker run --rm \
	$DOCKER_COMPILE_ARGS \
	-v "$BOT_DATADIR:/spndata:rw" \
	--name "build_${BOT_NAME}_${VERSION_ID}" \
	spn_${PROGLANG}_base:latest compile
