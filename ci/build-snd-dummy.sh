#!/bin/bash
set -eu

LINUX_VERSION=$(uname -r | cut -d. -f1-2)
LINUX_ARCHIVE_FILE=v${LINUX_VERSION}.tar.gz
LINUX_SOURCE_DIR=linux-${LINUX_VERSION}

mkdir -p "${TMPDIR}"
cd "${TMPDIR}"

wget -q "https://github.com/torvalds/linux/archive/${LINUX_ARCHIVE_FILE}"

tar -xf "${LINUX_ARCHIVE_FILE}" "${LINUX_SOURCE_DIR}/sound"
cd "${LINUX_SOURCE_DIR}/sound"

CC=gcc CFLAGS="-Wno-error" make -C "/lib/modules/$(uname -r)/build" M="${PWD}" CONFIG_SOUND=m CONFIG_SND=m CONFIG_SND_PCM=m CONFIG_SND_DUMMY=m modules

mkdir -p "${SND_DUMMY_DIR}"
cp soundcore.ko core/snd.ko core/snd-pcm.ko drivers/snd-dummy.ko "${SND_DUMMY_DIR}"
