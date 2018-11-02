#!/bin/bash
set -e

. config.sh

echo "*** Get hard disk image ***"
# Get hard disk image.

cp build/rootfs.tar.bz2 ./
tar xf rootfs.tar.bz2
md5sum -c rootfs.md5
rm -rf rootfs.md5
rm -rf rootfs.tar.bz2
kpartx -a rootfs.img

# Get loop name
LOOP_PARTITION_NAME=$(sudo kpartx -l rootfs.img | grep -Eo "loop[[:digit:]]p[[:digit:]]")
LOOP_PARTITION_NAME="/dev/mapper/${LOOP_PARTITION_NAME}"
LOOP_DEV_PATH=$(sudo kpartx -l rootfs.img | grep -Eo "/dev/loop[[:digit:]]")

echo "*** Install grub2 on hard disk ***"
# Install grub2 on hard disk
mkdir -p ${ROOTFS_IMG_MOUNT_DIR}
mount ${LOOP_PARTITION_NAME} ${ROOTFS_IMG_MOUNT_DIR}
grub2-install --root-directory=${ROOTFS_IMG_MOUNT_DIR} ${LOOP_DEV_PATH}
cp build/grub.cfg ${ROOTFS_IMG_MOUNT_DIR}/boot/grub2

echo "*** Build dev on root filesystem ***"
# Build device file in /dev
mkdir -p ${ROOTFS_IMG_MOUNT_DIR}/dev
cd ${ROOTFS_IMG_MOUNT_DIR}/dev

rm -rf hda hda1 hda2 hda3 hda4 hda5
rm -rf hdb hdb1 hdb2 hdb3 hdb4 hdb5
rm -rf ram0 ram1 ram2 ram3 ram4
rm -rf tty tty0 tty1 tty2 tty3 tty4 tty5 tty6 tty7
rm -rf ttyS0 ttyS1

mknod hda  b 2 0
mknod hda1 b 2 1
mknod hda2 b 2 2
mknod hda3 b 2 3
mknod hda4 b 2 4
mknod hda5 b 2 5

mknod hdb  b 2 20
mknod hdb1 b 2 21
mknod hdb2 b 2 22
mknod hdb3 b 2 23
mknod hdb4 b 2 24
mknod hdb5 b 2 25

mknod ram0 b 3 0
mknod ram1 b 3 1
mknod ram2 b 3 2
mknod ram3 b 3 3
mknod ram4 b 3 4

mknod tty  c 5 0

mknod tty0 c 4 0
mknod tty1 c 4 1
mknod tty2 c 4 2
mknod tty3 c 4 3
mknod tty4 c 4 4
mknod tty5 c 4 5
mknod tty6 c 4 6
mknod tty7 c 4 7

mknod ttyS0 c 4 10
mknod ttyS1 c 4 11

cd -

echo "*** Create daemons directory ***"
mkdir -p ${ROOTFS_IMG_MOUNT_DIR}/usr/sbin/daemons

umount ${ROOTFS_IMG_MOUNT_DIR}
kpartx -d rootfs.img
