#include <errno.h>
#include <unistd.h>
#include <stdio.h>

struct pinfo {
	long state;
	pid_t pid;
	uid_t uid;
	char comm[16];
	unsigned int depth;
};

struct pinfo infos[1600];

int main(int argc, char **argv)
{
	printf("testing syscall 294...\n");
	errno = 0;
	long ret = syscall(294, &infos, 1600);
	printf("syscall(294, %p, 1600) returned %ld, errno is %d\n", &infos,
	       ret, errno);
	if (ret >= 0) {
		for (int i = 0; i < ret; i++) {
			printf("infos[%d].state = %ld\n", i, infos[i].state);
			printf("infos[%d].pid = %d\n", i, infos[i].pid);
			printf("infos[%d].uid = %d\n", i, infos[i].uid);
			printf("infos[%d].comm = %s\n", i, infos[i].comm);
			printf("infos[%d].depth = %d\n", i, infos[i].depth);
		}
	}

	return 0;
}
