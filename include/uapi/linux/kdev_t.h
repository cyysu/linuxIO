#ifndef _UAPI_LINUX_KDEV_T_H
#define _UAPI_LINUX_KDEV_T_H
#ifndef __KERNEL__

/*
Some programs want their definitions of MAJOR and MINOR and MKDEV
from the kernel sources. These must be the externally visible ones.
*/
#define MAJOR(dev)	((dev)>>8)   //主设备号
#define MINOR(dev)	((dev) & 0xff) //次设备号
#define MKDEV(ma,mi)	((ma)<<8 | (mi)) //设备号
#endif /* __KERNEL__ */
#endif /* _UAPI_LINUX_KDEV_T_H */
