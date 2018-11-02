mkdir -p $1/daemons
mkdir -p $1/usr/sbin
mkdir -p $1/dev

mknod $1/dev/hda b 2 0
mknod $1/dev/hdb b 2 20

mknod $1/dev/hda1 b 2 1
mknod $1/dev/hda2 b 2 2
mknod $1/dev/hda3 b 2 3
mknod $1/dev/hda4 b 2 4
mknod $1/dev/hda5 b 2 5

mknod $1/dev/hdb1 b 2 21
mknod $1/dev/hdb2 b 2 22
mknod $1/dev/hdb3 b 2 23
mknod $1/dev/hdb4 b 2 24
mknod $1/dev/hdb5 b 2 25

mknod $1/dev/ram0 b 3 0
mknod $1/dev/ram1 b 3 1
mknod $1/dev/ram2 b 3 2
mknod $1/dev/ram3 b 3 3

mknod $1/dev/tty c 5 0

mknod $1/dev/tty0 c 4 0
mknod $1/dev/tty1 c 4 1
mknod $1/dev/tty2 c 4 2
mknod $1/dev/tty3 c 4 3
mknod $1/dev/tty4 c 4 4
mknod $1/dev/tty5 c 4 5
mknod $1/dev/tty6 c 4 6
mknod $1/dev/tty7 c 4 7

cp ./grub.cfg $1/boot/grub2/grub.cfg
