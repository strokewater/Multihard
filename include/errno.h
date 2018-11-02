#ifndef LIB_ERRNO_H
#define LIB_ERRNO_H

extern int errno;

#define ERROR		99	// 一般错误
#define	EPERM		1	// 操作没有许可
#define	ENOENT		2	// 文件或目录不存在
#define	ESRCH		3	// 指定的进程不存在
#define	EINTR		4	// 中断的函数调用
#define	EIO		5	// IO错误
#define	ENXIO		6	// 指定设备或地址不存在
#define	E2BIG		7	// 参数列表太长
#define	ENOEXEC		8	// 执行程序格式错误
#define	EBADF		9	// 文件句柄（描述符）错误
#define	ECHILD		10	// 子进程不存在
#define	EAGAIN		11	// 资源暂时不可用
#define	ENOMEM		12	// 内存不足
#define	EACCES		13	// 没有许可权限
#define	EFAULT		14	// 地址错误
#define	ENOTBLK		15	// 不是块设备文件
#define	EBUSY		16	// 资源正忙
#define	EEXIST		17	// 文件已存在
#define	EXDEV		18	// 跨设备连接
#define	ENODEV		19	// 设备不存在
#define	ENOTDIR		20	// 不是目录文件
#define	EISDIR		21	// 是目录文件
#define	EINVAL		22	// 参数无效
#define	ENFILE		23	// 总文件打开表溢出
#define	EMFILE		24	// 打开文件数太多
#define	ENOTTY		25	// 不是打字机（不恰当的IO控制操作）（没有TTY终端）
#define	ETXTBSY		26	// 文本文件忙(不再使用)
#define	EFBIG		27	// 文件太大
#define	ENOSPC		28	// 设备已满（设备已经没有空间）
#define	ESPIPE		29	// 无效的文件指针重定位。
#define	EROFS		30	// 文件系统只读。
#define	EMLINK		31	// 连接太多
#define	EPIPE		32	// 管道错误
#define	EDOM		33	// 域（domain）错误
#define	ERANGE		34	// 结果太大。
#define ENOSYS		38	// 功能未实现

#endif
