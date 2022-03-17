#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/pinfo.h>

static void copy_pinfo(struct pinfo *info, const struct task_struct *task,
		       unsigned int depth)
{
	info->state = task->state;
	info->depth = depth;
	info->pid = task->pid;
	info->uid = task_uid(task).val;
	strscpy(info->comm, task->comm, TASK_COMM_LEN);
}

static long do_ptree(struct pinfo __user *buf, size_t maxlen)
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

	// always maxlen > 0 here
	copy_pinfo(&kbuf[buflen++], &init_task, 0);

	while (stack_ptr >= 0) {
		// `task' is current task, `next_task' is the next child task of to push to stack
		task = task_stack[stack_ptr];
		next_task = next_task_stack[stack_ptr];

		// check if we have visited all child tasks of `task'
		if (next_task && &next_task->sibling != &task->children) {
			// update next child task
			next_task_stack[stack_ptr] =
				list_next_entry(next_task, sibling);

			// push the next child task to the stack
			task_stack[++stack_ptr] = next_task;
			next_task_stack[stack_ptr] = list_first_entry_or_null(
				&next_task->children, typeof(**next_task_stack),
				sibling);

			printk(KERN_DEBUG
			       "iterate_ptree pid=%d,uid=%ld,comm=%s,depth=%d\n",
			       next_task->pid, task_uid(next_task),
			       next_task->comm, stack_ptr);

			// check and break if there is no more space left in kernel buffer
			if (buflen < maxlen) {
				copy_pinfo(&kbuf[buflen++], next_task,
					   stack_ptr);
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

SYSCALL_DEFINE2(ptree, struct pinfo __user *, buf, size_t, maxlen)
{
	return do_ptree(buf, maxlen);
}
