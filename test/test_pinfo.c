#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <linux/pinfo.h>

int main(int argc, char **argv)
{
	if (argc != 2) {
		printf("Usage: %s buflen\n", argv[0]);
		return EXIT_FAILURE;
	}

	int buflen = atoi(argv[1]);
	struct pinfo *pinfos = malloc(sizeof(*pinfos) * buflen);

	errno = 0;
	long ret = syscall(294, pinfos, buflen);

	if (ret < 0) {
		const char *errstr = "unknown";
		switch (errno) {
		case EINVAL:
			errstr = "EINVAL";
			break;
		case ENOMEM:
			errstr = "ENOMEM";
			break;
		case EFAULT:
			errstr = "EFAULT";
			break;
		}
		printf("syscall(294, %p, %d) failed with errno %d (%s)\n",
		       pinfos, buflen, errno, errstr);
		return EXIT_FAILURE;
	}

	for (int i = 0; i < ret; i++) {
		struct pinfo pinfo = pinfos[i];
		for (int j = 0; j < pinfo.depth; j++) {
			printf("\t");
		}

		printf("%s, %d, %ld, %ld\n", pinfo.comm, pinfo.pid, pinfo.state,
		       pinfo.uid);
	}

	return EXIT_SUCCESS;
}
