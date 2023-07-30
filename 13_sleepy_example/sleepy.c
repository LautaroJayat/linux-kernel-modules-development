#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/errno.h> /* EFAULT */

#define SLEEPY "sleepy"
static int devNum;
static int min = 0;
static int count = 1;
static DECLARE_WAIT_QUEUE_HEAD(wq);
const char* PREF = "[ sleepy ] - ";
static int flag = 0;




static int open(struct inode* inode, struct file* filp) {
    pr_info("%s PID-> %d | command-> %s | OPEN triggered\n", PREF, current->pid, current->comm);
    return 0;
};

static ssize_t read(struct file* filp, char __user* buf, size_t len, loff_t* off) {
    pr_info("%s PID-> %d | command-> %s | READ triggered\n", PREF, current->pid, current->comm);
    pr_info("%s PID-> %d | command-> %s | About to sleep until flag is 1\n", PREF, current->pid, current->comm);
    wait_event_interruptible(wq, flag == 1);
    flag = 0;
    pr_info("%s PID-> %d | command-> %s | AWOKEN!\n", PREF, current->pid, current->comm);
    return 0;
}

static ssize_t write(struct file* filp, const char __user* buf, size_t len, loff_t* off) {
    pr_info("%s PID-> %d | command-> %s | WRITE triggered\n", PREF, current->pid, current->comm);
    pr_info("%s PID-> %d | command-> %s | Changing flag to 1\n", PREF, current->pid, current->comm);
    flag = 1;
    pr_info("%s PID-> %d | command-> %s | Triggering awake signal\n", PREF, current->pid, current->comm);
    wake_up_interruptible(&wq);
    return count;
}

static int release(struct inode* inode, struct file* filp) {
    pr_info("%s PID-> %d | command-> %s | RELEASE triggered\n", PREF, current->pid, current->comm);
    return 0;
}


static const struct file_operations fops = {
  .owner = THIS_MODULE,
  .open = open,
  .read = read,
  .write = write,
  .release = release,
};

static struct cdev sleepy;

static int init(void) {
    int err;
    err = alloc_chrdev_region(&devNum, min, count, SLEEPY);
    if (err != 0) {
        goto error;
    }
    pr_alert("%s - Char region allocated!\n", PREF);

    cdev_init(&sleepy, &fops);
    pr_alert("%s - Character device struct initiated!\n", PREF);

    sleepy.owner = THIS_MODULE;
    sleepy.ops = &fops;

    err = cdev_add(&sleepy, devNum, 1);
    if (err != 0) {
        goto remove_cdev;
    }
    pr_alert("%s - Character device ready to use\n", PREF);


    return 0;

remove_cdev:
    cdev_del(&sleepy);
    unregister_chrdev_region(devNum, count);
error:
    pr_alert("%s - THIS IS NO GOOD, ERROR\n", PREF);
    return err;
}

static void exit_sleepy(void) {
    cdev_del(&sleepy);
    pr_alert("%s - Character device struct deallocated!\n", PREF);
    unregister_chrdev_region(devNum, count);
    pr_alert("%s - Char region deallocated\n", PREF);
}

MODULE_AUTHOR("Lautaro Jayat");
MODULE_DESCRIPTION("A cool module to interact with the undeads");
MODULE_VERSION("0.0.0");
MODULE_LICENSE("GPL");

module_init(init);
module_exit(exit_sleepy);