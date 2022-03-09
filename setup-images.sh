#!/bin/bash

############################################
# OSSPR2022                                #
#                                          #
# Sets up images to boot up QEMU.          #
# You should have built the kernel.        #
############################################

set -Eeu
trap cleanup EXIT

SCRIPTDIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd -P)

msg() {
	printf >&2 "%s\n" "${1:-}"
}

die() {
	local msg=$1
	local code=${2:-1}
	msg "$msg"
	exit "$code"
}

depriv() {
	if [[ -n "${SUDO_USER:-}" ]]; then
		sudo -u "$SUDO_USER" -- "$@"
	else
		"$@"
	fi
}

fetch() {
	if [[ ! -f $1 ]]; then
		msg "$(basename "$1") not found. downloading from $2..."
		depriv curl -sSL "$2" -o "$1"
		msg "download done."
	else
		msg "$(basename "$1") found."
	fi
}

RELEASE_URL='https://download.tizen.org/releases/milestone/tizen/unified/tizen-unified_20201020.1'

ROOTFS_URL="$RELEASE_URL/images/standard/iot-headless-2parts-armv7l-btrfs-rootfs-rpi/tizen-unified_20201020.1_iot-headless-2parts-armv7l-btrfs-rootfs-rpi.tar.gz"
ROOTFS="$SCRIPTDIR/$(basename "$ROOTFS_URL")"

IMAGEDIR="$(depriv mktemp -d)"
MOUNTDIR="$(depriv mktemp -d)"

DESTDIR="$SCRIPTDIR/tizen-image"

cleanup() {
	trap - EXIT
	umount "$MOUNTDIR" &>/dev/null || true
	rm -rf "$IMAGEDIR" "$MOUNTDIR"
}

mkramdisk() {
	msg "creating ramdisk.img..."
	msg "mounting..."
	mount -o sync "$1" "$MOUNTDIR"
	sed -i 's/\/bin\/mount -o remount,ro .//' "$MOUNTDIR/usr/sbin/init"
	msg "unmounting..."
	umount "$MOUNTDIR"
	msg "creating ramdisk.img done."
}

mkrootfs() {
	msg "creating rootfs.img..."
	msg "mounting..."
	mount -o sync "$1" "$MOUNTDIR"
	msg "adding test binaries..."
	find "$SCRIPTDIR" -maxdepth 1 -iname 'test_*' -executable -exec cp '{}' "$MOUNTDIR/root" \;
	msg "unmounting..."
	umount "$MOUNTDIR"
	msg "creating rootfs.img done."
}

main() {
	if [[ "$EUID" != '0' ]]; then die "This script must be run as root."; fi

	msg "creating images..."

	fetch "$ROOTFS" "$ROOTFS_URL"

	msg "unarchiving downloaded images..."
	depriv tar -xzf "$ROOTFS" -C "$IMAGEDIR"
	msg "unarchiving done."

	mkramdisk "$IMAGEDIR/ramdisk.img"
	mkrootfs "$IMAGEDIR/rootfs.img"

	msg "emptying $(basename "$DESTDIR")..."
	depriv rm -rf "$DESTDIR"
	depriv mkdir -p "$DESTDIR"
	msg "copying images to $(basename "$DESTDIR")..."
	depriv cp "$IMAGEDIR"/* "$DESTDIR"
	msg "creating images done."
}

main "$@"
