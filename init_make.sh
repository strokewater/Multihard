#!/bin/bash
set -e

. config.sh

echo "*** Build auto make system ***"
# build auto make system.

rm -rf Makefile.config
echo "CC=${CC}" >Makefile.config
echo "ASM=${ASM}" >>Makefile.config
echo "LD=${LD}" >>Makefile.config
echo "CFLAGS=${CFLAGS}" >>Makefile.config
echo "ASMFLAGS=${ASMFLAGS}" >>Makefile.config
echo "ROOTFS_IMG_MOUNT_DIR=${ROOTFS_IMG_MOUNT_DIR}" >>Makefile.config
rm -rf Makefile.sub
rm -rf SrcRoot.txt
echo "SRC_ROOT=$(pwd)" >>SrcRoot.txt
cat SrcRoot.txt build/Makefile.sub >Makefile.sub
rm -rf SrcRoot.txt

cp build/Makefile.top ./Makefile
cp build/Makefile.main ./
cp build/Makefile.project ./

tools/reinstmf.py 1>/dev/null
