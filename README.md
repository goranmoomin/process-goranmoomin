# Project 1: Hello, Process!

## Building the project

TLDR: the below shell command will build the kernel and test programs,
create the images, and boot QEMU.

```shellsession
$ sudo -v && ./build-rpi3.sh && sudo ./setup-images.sh && ./qemu.sh
```

`build-rpi3.sh` checks `clang` and `ccache` and sets `CC`
appropriately before compiling the config and kernel. It also installs
the UAPI headers to `usr/include` and compiles the test programs in
`test/` with the installed headers.

Compilation with `gcc` and `clang` are both tested. To force `gcc`
even if `clang` is installed, pass the `--noclang` flag when calling
`build-rpi3.sh`. The `--noccache` flag is also supported in a similar
manner.

To separately compile the test programs, invoke `make` in the `test/`
directory with the `HDRDIR` variable pointing to the installed kernel
headers. Compiling test programs that use new headers will fail if
`HDRDIR` is not provided.

`setup-images.sh` also copies all `test/test_*` binaries to the root
file system, hence copying the executables separately is not required.

## Implementation notes

The `ptree` syscall is implemented as a aarch64-only syscall with
syscall number 294. The below notes assume the syscall signature and
parameter names are declared as below.

```c
long sys_ptree(struct pinfo __user *buf, size_t maxlen);
```

To prevent a kernel stack overflow, DFS-iterating the process tree
must happen without unbounded recursion. To prevent deadlocking on
`&tasklist_lock`, memory allocation while iterating is not
possible. Hence the syscall must know the required amount of stack
beforehand.

The function `do_ptree` allocates the stacks on the heap with max
depth `maxlen` that contains two elements – the current task and the
next child task to push onto the stack. (Implementation-wise, the
stack is represented as two `struct task_struct *` arrays,
`task_stack` and `next_task_stack` and the index of head `stack_ptr`.)

Each iteration checks if there are child tasks of the current task
that are not pushed yet, push the next child task, and update the
next task marker on `next_task_stack`. If `maxlen` processes are seen,
the iteration is terminated and the `struct pinfo`s are copied to the
provided `buf`.

No errors can happen during the lock of `&tasklist_lock`, so the only
clean up required is to free the allocated buffers. Error handling is
done by setting the return value `err` and `goto`ing to the cleanup
`out:` label.

## Testing

To create a deep process tree and check if the syscall doesn’t choke
on it, the following shell code can be used.

```shellsession
localhost login: root
Password:
Welcome to Tizen
root:~> :(){(:)};:&
[1] 739
root:~> ./test_pinfo 1000
swapper/0, 0, 0, 0
	systemd, 1, 1, 0
		dbus-daemon, 164, 1, 81
		systemd-journal, 168, 1, 0
		systemd-udevd, 193, 1, 0
		dlog_logger, 232, 0, 1901
		amd, 247, 1, 301
		actd, 249, 1, 0
		buxton2d, 250, 1, 375
		key-manager, 251, 1, 444
		alarm-server, 252, 1, 301
		bt-service, 253, 1, 551
		cynara, 254, 1, 401
		deviced, 255, 1, 0
		esd, 258, 1, 301
		license-manager, 262, 1, 402
		resourced-headl, 273, 1, 0
		login, 279, 1, 0
			bash, 482, 1, 0
				bash, 739, 1, 0
					bash, 740, 1, 0
						bash, 742, 1, 0
							bash, 743, 1, 0
								bash, 744, 1, 0
									bash, 745, 1, 0
										bash, 746, 1, 0
											bash, 747, 1, 0
												bash, 748, 1, 0
[lots of output snipped]
```
