#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>

#define SKULL "skull"

MODULE_AUTHOR("Lautaro Jayat");
MODULE_DESCRIPTION("A cool module to interact with the undeads");
MODULE_VERSION("0.0.0");
MODULE_LICENSE("GPL");

static int devNum;
static int maj;
static int min = 0;
static int count = 4;
const char* PREF = "[ skull ]";


static int init_skull(void) {
    int result = alloc_chrdev_region(&devNum, min, count, SKULL);
    if (result < 0) {
        printk(KERN_ALERT  "%s - THIS IS NO GOOD, ERROR\n", PREF);
        return result;
    }
    maj = MAJOR(devNum);
    printk(KERN_ALERT "%s - Device mounted successfully\n", PREF);
    return 0;
}

static void exit_skull(void) {
    // unregistering
    unregister_chrdev_region(devNum, count);
    printk(KERN_ALERT "%s - Goodbye, cruel world\n", PREF);
}

module_init(init_skull);
module_exit(exit_skull);