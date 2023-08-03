#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>	
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/errno.h> /* EFAULT */


#define ASYNC "polling_d"
static int devNum;
static int min = 0;
static int count = 1;
const char* PREF = "[ polling_d ] - ";



struct polling_dev_t {
    char* buff;
    unsigned int buffSize, buffNotEmpty;
    unsigned int numOfReaders, numOfWriters;
    wait_queue_head_t inq, outq;
    struct mutex lock;
    struct cdev cdev;
};

static struct polling_dev_t polling_d = { .buffSize = 256, .numOfReaders = 0, .numOfWriters = 0, .buffNotEmpty = 0 };


static int open(struct inode* inode, struct file* filp) {
    struct polling_dev_t* dev;
    dev = container_of(inode->i_cdev, struct polling_dev_t, cdev);
    filp->private_data = dev;
    pr_info("%s PID-> %d | OPEN triggered\n", PREF, current->pid);
    if (mutex_lock_interruptible(&dev->lock)) {
        pr_info("%s PID-> %d | could not get mutex\n", PREF, current->pid);
        // make the vfs take care or re try the syscall
        // this should be transparent for the userspace process
        return -ERESTARTSYS;
    }
    if (!dev->buff) {
        pr_info("%s PID-> %d | allocating memory\n", PREF, current->pid);
        dev->buff = kmalloc(dev->buffSize, GFP_KERNEL);
        if (!dev->buff) {
            mutex_unlock(&dev->lock);
            return -ENOMEM;
        }
    }
    if (filp->f_mode & FMODE_READ) {
        dev->numOfReaders++;
    }
    if (filp->f_mode & FMODE_WRITE) {
        dev->numOfWriters++;
    }
    mutex_unlock(&dev->lock);


    return nonseekable_open(inode, filp);
};



static ssize_t read(struct file* filp, char __user* buf, size_t len, loff_t* off) {
    struct polling_dev_t* dev;
    dev = filp->private_data;
    pr_info("%s PID-> %d | READ triggered\n", PREF, current->pid);
    if (mutex_lock_interruptible(&dev->lock)) {
        pr_info("%s PID-> %d | couldn't held the lock\n", PREF, current->pid);
        return -ERESTARTSYS;
    }
    // copy only up to the max buff size
    if (len > dev->buffSize) {
        len = dev->buffSize;
    }
    pr_info("%s PID-> %d | to read len -> %ld \n", PREF, current->pid, len);
    if (copy_to_user(buf, dev->buff, len)) {
        mutex_unlock(&dev->lock);
        return -EFAULT;
    }
    memset(dev->buff, 0, dev->buffSize);
    dev->buffNotEmpty = 0;
    mutex_unlock(&dev->lock);
    return len;
}

static ssize_t write(struct file* filp, const char __user* buf, size_t len, loff_t* off) {
    struct polling_dev_t* dev;
    dev = filp->private_data;
    pr_info("%s PID-> %d | WRITE triggered\n", PREF, current->pid);
    if (mutex_lock_interruptible(&dev->lock)) {
        return -ERESTARTSYS;
    }
    len = min(len, (size_t)dev->buffSize);
    pr_info("%s PID-> %d | to write len -> %ld\n", PREF, current->pid, len);
    memset(dev->buff, 0, dev->buffSize);
    if (copy_from_user(dev->buff, buf, len)) {
        mutex_unlock(&dev->lock);
        return -EFAULT;
    }
    dev->buffNotEmpty = 1;
    mutex_unlock(&dev->lock);
    return len;
}

static unsigned int poll(struct file* filp, poll_table* wait) {

    struct polling_dev_t* dev;
    unsigned int mask;

    pr_info("%s PID-> %d | POLL triggered, checking if we can read or write \n", PREF, current->pid);
    dev = filp->private_data;
    mask = 0;
    mutex_lock(&dev->lock);
    poll_wait(filp, &dev->inq, wait);
    poll_wait(filp, &dev->outq, wait);
    if (dev->buffNotEmpty == 1) {
        mask |= POLLIN | POLLRDNORM;
    }
    else if (dev->buffNotEmpty == 0) {
        mask |= POLLOUT | POLLWRNORM;
    }
    mutex_unlock(&dev->lock);
    return mask;
}

static int release(struct inode* inode, struct file* filp) {
    struct polling_dev_t* dev;
    dev = filp->private_data;
    mutex_lock(&dev->lock);
    if (filp->f_mode & FMODE_READ) {
        dev->numOfReaders--;
    }
    if (filp->f_mode & FMODE_WRITE) {
        dev->numOfWriters--;
    }
    if (dev->numOfReaders + dev->numOfWriters == 0) {
        pr_info("%s PID-> %d | No more writers or readers, flushing buffer \n", PREF, current->pid);
        kfree(dev->buff);
        dev->buff = NULL;
    }
    mutex_unlock(&dev->lock);
    return 0;
}


static const struct file_operations fops = {
  .owner = THIS_MODULE,
  .open = open,
  .read = read,
  .write = write,
  .release = release,
  .poll = poll,
};



static int init(void) {
    int err;
    err = alloc_chrdev_region(&devNum, min, count, ASYNC);
    if (err != 0) {
        goto error;
    }
    pr_alert("%s - Char region allocated!\n", PREF);

    cdev_init(&polling_d.cdev, &fops);
    pr_alert("%s - Character device struct initiated!\n", PREF);

    polling_d.cdev.owner = THIS_MODULE;
    polling_d.cdev.ops = &fops;
    mutex_init(&polling_d.lock);
    init_waitqueue_head(&polling_d.inq);
    init_waitqueue_head(&polling_d.outq);

    err = cdev_add(&polling_d.cdev, devNum, 1);
    if (err != 0) {
        goto remove_cdev;
    }
    pr_alert("%s - Character device ready to use\n", PREF);
    return 0;

remove_cdev:
    cdev_del(&polling_d.cdev);
    unregister_chrdev_region(devNum, count);
error:
    pr_alert("%s - THIS IS NO GOOD, ERROR\n", PREF);
    return err;
}

static void exit_polling_d(void) {
    cdev_del(&polling_d.cdev);
    pr_alert("%s - Character device struct deallocated!\n", PREF);
    unregister_chrdev_region(devNum, count);
    pr_alert("%s - Char region deallocated\n", PREF);
}

MODULE_AUTHOR("Lautaro Jayat");
MODULE_DESCRIPTION("A cool module to interact with the undeads");
MODULE_VERSION("0.0.0");
MODULE_LICENSE("GPL");

module_init(init);
module_exit(exit_polling_d);