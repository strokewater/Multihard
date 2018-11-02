vgaromimage: file=$BXSHARE/VGABIOS-lgpl-latest
ata0-master: type=disk, path="rootfs.img", mode=flat
# ata0-slave: type=disk, path="c.img.bak", mode=flat
#com1: enabled=1, mode=term, dev=/dev/pts/3
#com2: enabled=1, mode=term, dev=/dev/pts/4
boot:disk
log: bochs.out
#debug: action=report
megs:64
gdbstub: enabled=1, port=1234
port_e9_hack: enabled=1
