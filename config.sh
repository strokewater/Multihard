SOFTWARE_IMG=build/software.img
SOFTWARE_MOUNT_DIR=/multihard
ROOTFS_IMG_MOUNT_DIR=/mnt/multihard

CC=i686-elf-gcc
ASM=nasm
LD=i686-elf-ld

CFLAGS="-Wall -ffreestanding -nostdinc -g -I\$(SRC_ROOT)/include/"
ASMFLAGS="-f elf -g -I\$(SRC_ROOT)/include/"
