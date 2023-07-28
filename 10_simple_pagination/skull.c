#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/mutex.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include<linux/string.h>
#include <linux/uaccess.h> /* copy_from_user, copy_to_user */
#include <linux/errno.h> /* EFAULT */

#define SKULL "skull"
#define Q_SET_SIZE 16
#define QUANTUM_SIZE 16

static int devNum;
static int min = 0;
static int count = 1;
const char* PREF = "[ skull ]";


struct node {
    struct node* next;
    void** data;
};



struct skull_d {
    struct node* data;  /* Pointer to first node of the linked lisit */
    int quantum;              /* the current quantum size */
    int qset;                 /* the current array size */
    unsigned long size;       /* amount of data stored here */
    struct cdev skull_cdev;
};

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
    dev->qset = Q_SET_SIZE;
    dev->quantum = QUANTUM_SIZE;
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
        pr_info("%s - About to trim on open\n", PREF);
        skull_trim(dev);
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
    pr_info("%s - reading %ld bytes from quantum number %d\n", PREF, len, s_pos);
    if (copy_to_user(buf, targetNode->data[s_pos] + q_pos, len)) {
        result = -EFAULT;
        goto out;
    }
    *off = *off + len;
    result = len;
out:
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

static const struct file_operations fops = {
  .owner = THIS_MODULE,
  .open = open,
  .read = read,
  .write = write,
  .release = release,
  .llseek = llseek,
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