#!/bin/bash

. config.sh

set -e

echo "*** Mount software image ***"
mount ${SOFTWARE_IMG} ${SOFTWARE_MOUNT_DIR}

echo "*** Mount root filesystem image ***"
kpartx -a rootfs.img
LOOP_PARTITION_NAME=$(sudo kpartx -l rootfs.img | grep -Eo "loop[[:digit:]]p[[:digit:]]")
LOOP_PARTITION_NAME="/dev/mapper/${LOOP_PARTITION_NAME}"

mount ${LOOP_PARTITION_NAME} ${ROOTFS_IMG_MOUNT_DIR}

echo "*** Copy software to root filesystem ***"
mkdir -p ${ROOTFS_IMG_MOUNT_DIR}/usr
cp -r ${SOFTWARE_MOUNT_DIR}/bin   	${ROOTFS_IMG_MOUNT_DIR}
cp -r ${SOFTWARE_MOUNT_DIR}/share 	${ROOTFS_IMG_MOUNT_DIR}
cp -r ${SOFTWARE_MOUNT_DIR}/usr/bin	${ROOTFS_IMG_MOUNT_DIR}/usr
cp -r ${SOFTWARE_MOUNT_DIR}/usr/i686-multihard ${ROOTFS_IMG_MOUNT_DIR}/usr
cp -r ${SOFTWARE_MOUNT_DIR}/usr/include ${ROOTFS_IMG_MOUNT_DIR}/usr
cp -r ${SOFTWARE_MOUNT_DIR}/usr/lib	${ROOTFS_IMG_MOUNT_DIR}/usr
cp -r ${SOFTWARE_MOUNT_DIR}/usr/libexec ${ROOTFS_IMG_MOUNT_DIR}/usr
cp -r ${SOFTWARE_MOUNT_DIR}/usr/share	${ROOTFS_IMG_MOUNT_DIR}/usr

echo "*** Umount root filesystem and software image ***"
umount ${ROOTFS_IMG_MOUNT_DIR}
umount ${SOFTWARE_MOUNT_DIR}
kpartx -dv rootfs.img
