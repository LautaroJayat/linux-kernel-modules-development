#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/ioctl.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/mutex.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/uaccess.h> /* copy_from_user, copy_to_user */
#include <asm/current.h>
#include <linux/errno.h> /* EFAULT */
#include "skull.h"

static int devNum;
static int min = 0;
static int count = 1;
const char* PREF = "[ skull ]";
int qset_size = Q_SET_SIZE;
int  quantum_size = QUANTUM_SIZE;

static struct skull_d skull = { .data = NULL, .qset = Q_SET_SIZE, .quantum = QUANTUM_SIZE, .size = 0 };

static struct node* getNodeByIndex(struct skull_d* dev, int index) {
    struct node* targetNode = dev->data;
    if (!targetNode) {
        targetNode = dev->data = kmalloc(sizeof(struct node), GFP_KERNEL);
        if (targetNode == NULL) return NULL;
        memset(targetNode, 0, sizeof(struct node));
    }

    while (index--) {
        if (!targetNode->next) {
            targetNode->next = kmalloc(sizeof(struct node), GFP_KERNEL);
            if (targetNode->next == NULL) return NULL;
            memset(targetNode->next, 0, sizeof(struct node));
        }
        targetNode = targetNode->next;
        continue;
    }
    return targetNode;
}

int skull_trim(struct skull_d* dev) {
    struct node* currentNode;
    struct node* nextNode;
    int qset = dev->qset;
    int i;

    for (currentNode = dev->data; currentNode; currentNode = nextNode) {
        if (currentNode->data) {
            for (i = 0; i < qset; i++) {
                kfree(currentNode->data[i]);
            }
            kfree(currentNode->data);
            currentNode->data = NULL;
        }
        nextNode = currentNode->next;
        kfree(currentNode);
    }
    dev->size = 0;
    dev->qset = qset_size;
    dev->quantum = quantum_size;
    dev->data = NULL;
    return 0;
}

static int open(struct inode* inode, struct file* filp) {
    // Getting char device struct and adding it to private_data field
    struct skull_d* dev;
    dev = container_of(inode->i_cdev, struct skull_d, skull_cdev);
    filp->private_data = dev;
    // Checking access mode with f_flags
    if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
        pr_info("%s - [PID %d ] - about to GET the lock to OPEN device!", PREF, current->pid);
        if (mutex_lock_interruptible(&dev->lock)) {
            pr_alert("%s - we were killed while waiting");
            return -ERESTARTSYS;
        }
        pr_info("%s - About to trim on open\n", PREF);
        skull_trim(dev);
        pr_info("%s - [PID %d ] - about to RELEASE lock after trimming!", PREF, current->pid);
        mutex_unlock(&dev->lock);
    }
    return 0;
};

static ssize_t read(struct file* filp, char __user* buf, size_t len, loff_t* off) {
    struct skull_d* dev;
    struct node* targetNode;
    int quantum, qset, pageSize, nodeIndex, s_pos, q_pos, rest;
    ssize_t result;

    dev = filp->private_data;
    targetNode = dev->data;
    quantum = dev->quantum;
    qset = dev->qset;
    pageSize = quantum * qset;
    result = 0;
    pr_info("%s - [PID %d ] - about to GET the lock to READ!", PREF, current->pid);
    if (mutex_lock_interruptible(&dev->lock)) {
        pr_alert("%s - we were killed while waiting");
        return -ERESTARTSYS;
    }
    pr_info("%s - [PID %d ] - GOT the lock for READING!", PREF, current->pid);
    if (dev->size < *off) {
        goto out;
    }
    if (*off + len > dev->size) {
        len = dev->size - *off;
    }

    nodeIndex = (long)*off / pageSize;
    rest = (long)*off % pageSize;
    s_pos = rest / quantum;
    q_pos = rest % quantum;
    targetNode = getNodeByIndex(dev, nodeIndex);

    if (targetNode == NULL || !targetNode->data || !targetNode->data[s_pos]) {
        goto out;
    }
    if (len > quantum - q_pos) {
        len = quantum - q_pos;
    }
    if (copy_to_user(buf, targetNode->data[s_pos] + q_pos, len)) {
        result = -EFAULT;
        goto out;
    }
    *off = *off + len;
    result = len;
out:
    pr_info("%s - [PID %d ] - about to RELEASE the lock after READING!", PREF, current->pid);
    mutex_unlock(&dev->lock);
    return result;

}

static ssize_t write(struct file* filp, const char __user* buf, size_t len, loff_t* off) {
    struct skull_d* dev;
    struct node* targetNode;
    int quantum, qset, pageSize, nodeIndex, s_pos, q_pos, rest;
    ssize_t result;

    dev = filp->private_data;
    targetNode = dev->data;
    quantum = dev->quantum;
    qset = dev->qset;
    pageSize = quantum * qset;
    result = -ENOMEM;

    pr_info("%s - [PID %d ] - about to GET the lock to WRITE!", PREF, current->pid);
    if (mutex_lock_interruptible(&dev->lock)) {
        pr_alert("%s - we were killed while waiting", PREF);
        return -ERESTARTSYS;
    }
    pr_info("%s - [PID %d ] - GOT the lock for WRITING!", PREF, current->pid);

    nodeIndex = (long)*off / pageSize;
    rest = (long)*off % pageSize;
    s_pos = rest / quantum;
    q_pos = rest % quantum;

    targetNode = getNodeByIndex(dev, nodeIndex);
    if (targetNode == NULL) {
        goto out;
    }
    if (!targetNode->data) {
        targetNode->data = kmalloc(qset * sizeof(char*), GFP_KERNEL);
        if (targetNode->data == NULL) {
            goto out;
        }
        memset(targetNode->data, 0, qset * sizeof(char*));
    }
    if (!targetNode->data[s_pos]) {
        targetNode->data[s_pos] = kmalloc(quantum, GFP_KERNEL);
        if (!targetNode->data[s_pos]) {
            goto out;
        }
    }
    if (len > quantum - q_pos) {
        len = quantum - q_pos;
    }
    if (copy_from_user(targetNode->data[s_pos] + q_pos, buf, len)) {
        result = -EFAULT;
        goto out;
    }
    *off = *off + len;
    result = len;
    if (dev->size < *off) {
        dev->size = *off;
    }
out:
    pr_info("%s - [PID %d ] - about to RELEASE the lock after WRITING!", PREF, current->pid);
    mutex_unlock(&dev->lock);
    return result;

}

static int release(struct inode* inode, struct file* filp) {
    return 0;
}

static loff_t llseek(struct file* filp, loff_t off, int whence) {
    struct skull_d* dev;
    loff_t newpos;
    dev = filp->private_data;
    switch (whence) {
    case 0: /* SEEK_SET */
        newpos = off;
        break;

    case 1: /* SEEK_CUR */
        newpos = filp->f_pos + off;
        break;

    case 2: /* SEEK_END */
        newpos = dev->size + off;
        break;

    default: /* can't happen */
        return -EINVAL;
    }
    if (newpos < 0) return -EINVAL;
    filp->f_pos = newpos;
    return newpos;
}


static long ioctl(struct file* filp, unsigned int cmd, unsigned long arg) {
    unsigned int dir;
    int err = 0, tmp;
    int result = 0;
    if (_IOC_TYPE(cmd) != SKULL_IOC_MAGIC) return -ENOTTY;
    if (_IOC_NR(cmd) > SKULL_IOC_MAXNR) return -ENOTTY;
    dir = _IOC_DIR(cmd);
    if (dir & _IOC_READ || dir & IOCB_WRITE) {
        err = !access_ok((void __user*)arg, _IOC_SIZE(cmd));
    }
    if (err) return -EFAULT;

    switch (cmd) {
    case SKULL_IOC_RESET: /* set the default valuees */
        quantum_size = QUANTUM_SIZE;
        qset_size = Q_SET_SIZE;
        break;
    case SKULL_IOC_SET_QUANTUM: /* set variable from pointer */
        if (!capable(CAP_SYS_ADMIN)) return -EPERM;
        result = __get_user(quantum_size, (int __user*) arg);
        break;
    case SKULL_IOC_SET_QSET: /* set variable from pointer */
        if (!capable(CAP_SYS_ADMIN)) return -EPERM;
        result = __get_user(qset_size, (int __user*)arg);
        break;
    case SKULL_IOC_TELL_QUANTUM: /* the arg is the value */
        if (!capable(CAP_SYS_ADMIN)) return -EPERM;
        quantum_size = arg;
        break;
    case SKULL_IOC_TELL_QSET: /* the arg is the value */
        if (!capable(CAP_SYS_ADMIN)) return -EPERM;
        qset_size = arg;
        break;
    case SKULL_IOC_GET_QUANTUM: /* the return value is sent in the pointer */
        result = __put_user(quantum_size, (int __user*)arg);
        break;
    case SKULL_IOC_GET_QSET: /* the return value is sent in the pointer */
        result = __put_user(qset_size, (int __user*)arg);
        break;
    case SKULL_IOC_QUERY_QUANTUM: /* size is positive, so we can return it to userspace */
        return quantum_size;
    case SKULL_IOC_QUERY_QSET:
        return qset_size; /* size is positive, so we can return it to userspace */
    case SKULL_IOC_EXCHANGE_QUANTUM: /* read from the pointer and sent the old value using the same pointer*/
        if (!capable(CAP_SYS_ADMIN)) return -EPERM;
        tmp = quantum_size;
        result = __get_user(quantum_size, (int __user*)arg);
        if (result == 0) {
            result = __put_user(tmp, (int __user*)arg);
        }
        break;
    case SKULL_IOC_EXCHANGE_QSET: /* read from the pointer and sent the old value using the same pointer*/
        if (!capable(CAP_SYS_ADMIN)) return -EPERM;
        tmp = qset_size;
        result = __get_user(qset_size, (int __user*)arg);
        if (result == 0) {
            result = __put_user(tmp, (int __user*)arg);
        }
        break;
    case SKULL_IOC_SHIFT_QUANTUM: /* arg is the value, and we return the old one*/
        if (!capable(CAP_SYS_ADMIN)) return -EPERM;
        tmp = quantum_size;
        quantum_size = arg;
        return tmp;
    case SKULL_IOC_SHIFT_QSET: /* arg is the value, and we return the old one*/
        if (!capable(CAP_SYS_ADMIN)) return -EPERM;
        tmp = qset_size;
        qset_size = arg;
        return tmp;
    default:
        return -ENOTTY;
    }
    return result;
}

static const struct file_operations fops = {
  .owner = THIS_MODULE,
  .open = open,
  .read = read,
  .write = write,
  .release = release,
  .llseek = llseek,
  .unlocked_ioctl = ioctl,
};



static int init_skull(void) {
    int err;
    err = alloc_chrdev_region(&devNum, min, count, SKULL);
    if (err != 0) {
        goto error;
    }
    pr_alert("%s - Char region allocated!\n", PREF);

    cdev_init(&skull.skull_cdev, &fops);
    pr_alert("%s - Character device struct initiated!\n", PREF);

    skull.skull_cdev.owner = THIS_MODULE;
    skull.skull_cdev.ops = &fops;
    mutex_init(&skull.lock);

    err = cdev_add(&skull.skull_cdev, devNum, 1);
    if (err != 0) {
        goto remove_cdev;
    }
    pr_alert("%s - Character device ready to use\n", PREF);


    return 0;

remove_cdev:
    cdev_del(&skull.skull_cdev);
    unregister_chrdev_region(devNum, count);
error:
    pr_alert("%s - THIS IS NO GOOD, ERROR\n", PREF);
    return err;
}

static void exit_skull(void) {
    skull_trim(&skull);
    cdev_del(&skull.skull_cdev);
    pr_alert("%s - Character device struct deallocated!\n", PREF);
    unregister_chrdev_region(devNum, count);
    pr_alert("%s - Char region deallocated\n", PREF);
}

MODULE_AUTHOR("Lautaro Jayat");
MODULE_DESCRIPTION("A cool module to interact with the undeads");
MODULE_VERSION("0.0.0");
MODULE_LICENSE("GPL");

module_init(init_skull);
module_exit(exit_skull);