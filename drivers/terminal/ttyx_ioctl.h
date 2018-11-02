#ifndef PWD_TTYX_IOCTL_H
#define PWD_TTYX_IOCTL_H


int DTTYXIoctl(dev_no dev, int cmd, unsigned int arg);
int DTTYXSelect(dev_no dev, enum SelectCheckType check_type, select_table *tbl);

#endif
