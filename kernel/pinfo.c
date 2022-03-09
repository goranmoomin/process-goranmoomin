#include <linux/slab.h>
#include <linux/syscalls.h>

struct pinfo {
	long state;
	pid_t pid;
	uid_t uid;
	char comm[TASK_COMM_LEN];
	unsigned int depth;
};

void recurse_ptree(struct task_struct *task, int depth)
{
	struct task_struct *next_task;

	printk(KERN_INFO "recurse_ptree pid=%d,uid=%d,comm=%s,depth=%d\n",
	       task->pid, task_uid(task), task->comm, depth);

	list_for_each_entry (next_task, &task->children, sibling) {
		recurse_ptree(next_task, depth + 1);
	}
}

SYSCALL_DEFINE2(ptree, struct pinfo __user *, buf, size_t, maxlen)
{
	int err = -EINVAL;
	struct pinfo *kbuf;
	int buflen = 0;

	struct task_struct *task, *next_task;
	struct task_struct **task_stack, **next_task_stack;
	int stack_ptr = -1;

	if (!buf || maxlen == 0)
		return -EINVAL;

	kbuf = kmalloc_array(maxlen, sizeof(*kbuf), GFP_KERNEL);
	task_stack = kmalloc_array(maxlen, sizeof(*task_stack), GFP_KERNEL);
	next_task_stack =
		kmalloc_array(maxlen, sizeof(*next_task_stack), GFP_KERNEL);
	if (!kbuf || !task_stack || !next_task_stack) {
		err = -ENOMEM;
		goto out;
	}

	read_lock(&tasklist_lock);

	task_stack[++stack_ptr] = &init_task;
	next_task_stack[stack_ptr] = list_first_entry(
		&init_task.children, typeof(**next_task_stack), sibling);

	while (stack_ptr >= 0) {
		task = task_stack[stack_ptr];
		// find next child not yet visited
		next_task = next_task_stack[stack_ptr];

		// check if there is a child to visit
		if (next_task && &next_task->sibling != &task->children) {
			// update next child marker
			next_task_stack[stack_ptr] =
				list_next_entry(next_task, sibling);

			// push the next child to the stack
			task_stack[++stack_ptr] = next_task;
			next_task_stack[stack_ptr] = list_first_entry_or_null(
				&next_task->children, typeof(**next_task_stack),
				sibling);

			printk(KERN_INFO
			       "iterate_ptree pid=%d,uid=%d,comm=%s,depth=%d\n",
			       next_task->pid, task_uid(next_task),
			       next_task->comm, stack_ptr);

			if (buflen < maxlen) {
				kbuf[buflen].state = next_task->state;
				kbuf[buflen].depth = stack_ptr;
				kbuf[buflen].pid = next_task->pid;
				kbuf[buflen].uid = task_uid(next_task).val;
				strscpy(kbuf[buflen].comm, next_task->comm,
					TASK_COMM_LEN);
				buflen++;
			} else {
				break;
			}
		} else {
			stack_ptr--;
		}
	}

	read_unlock(&tasklist_lock);

	if (buflen <= maxlen) {
		err = buflen;
		if (copy_to_user(buf, kbuf, sizeof(*kbuf) * buflen)) {
			err = -EFAULT;
		}
	}

out:
	kfree(kbuf);
	kfree(task_stack);
	kfree(next_task_stack);
	return err;
}
