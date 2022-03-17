#ifndef _LINUX_PINFO_H
#define _LINUX_PINFO_H

#include <linux/types.h>

#define PINFO_COMM_LEN 64

struct pinfo {
	int64_t state;
	pid_t pid;
	int64_t uid;
	char comm[PINFO_COMM_LEN];
	unsigned int depth;
};

#endif /* _LINUX_SYSINFO_H */
