#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>

#define SKULL "skull"

static int devNum;
static int min = 0;
static int count = 1;
const char* PREF = "[ skull ]";

static int open(struct inode* inode, struct file* filp) {
    printk(KERN_ALERT  "%s - OPEN TRIGGERED!\n", PREF);
    return 0;
};

static ssize_t read(struct file* filp, char __user* buf, size_t len, loff_t* off) {
    printk(KERN_ALERT  "%s - READ TRIGGERED!\n", PREF);
    return 0;
}

static ssize_t write(struct file* filp, const char __user* buf, size_t len, loff_t* off) {
    printk(KERN_ALERT  "%s - WRITE TRIGGERED!\n", PREF);
    return len;
}

static int release(struct inode* inode, struct file* filp) {
    printk(KERN_ALERT  "%s - RELEASE TRIGGERED!\n", PREF);
    return 0;
}

static loff_t llseek(struct file* filp, loff_t off, int whence) {
    printk(KERN_ALERT  "%s - LLSEEK TRIGGERED!\n", PREF);
    return 1;
}

static const struct file_operations fops = {
  .owner = THIS_MODULE,
  .open = open,
  .read = read,
  .write = write,
  .release = release,
  .llseek = llseek,
};

struct skull_d {
    struct cdev skull_cdev;
};

static struct skull_d skull;

static int init_skull(void) {
    int err;
    err = alloc_chrdev_region(&devNum, min, count, SKULL);
    if (err != 0) {
        goto error;
    }
    printk(KERN_ALERT "%s - Char region allocated!\n", PREF);

    cdev_init(&skull.skull_cdev, &fops);
    printk(KERN_ALERT "%s - Character device struct initiated!\n", PREF);
    
    skull.skull_cdev.owner = THIS_MODULE;
    skull.skull_cdev.ops = &fops;
    
    err = cdev_add(&skull.skull_cdev, devNum, 1);
    if (err != 0) {
        goto remove_cdev;
    }
    printk(KERN_ALERT "%s - Character device ready to use\n", PREF);


    return 0;

remove_cdev:
    cdev_del(&skull.skull_cdev);
    unregister_chrdev_region(devNum, count);
error:
    printk(KERN_ALERT  "%s - THIS IS NO GOOD, ERROR\n", PREF);
    return err;
}

static void exit_skull(void) {
    // unregistering
    cdev_del(&skull.skull_cdev);
    printk(KERN_ALERT "%s - Character device struct deallocated!\n", PREF);
    unregister_chrdev_region(devNum, count);
    printk(KERN_ALERT "%s - Char region deallocated\n", PREF);
}

MODULE_AUTHOR("Lautaro Jayat");
MODULE_DESCRIPTION("A cool module to interact with the undeads");
MODULE_VERSION("0.0.0");
MODULE_LICENSE("GPL");

module_init(init_skull);
module_exit(exit_skull);