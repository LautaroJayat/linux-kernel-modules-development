#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>

MODULE_LICENSE("GPL");

// Making those numbers
int devNum;
static int maj;
static int min = 0;
static int count = 4;


static int hello_init(void) {
    // registering the device
    int result = alloc_chrdev_region(&devNum, min, count, "hello_kernel");
    if (result < 0) {
        printk(KERN_ALERT "[ lauti ] - THIS IS NO GOOD, ERROR\n");
        return result;
    }
    maj = MAJOR(devNum);
    printk(KERN_ALERT "[ lauti ] - Hello, world\n");
    printk(KERN_ALERT "[ lauti ] - The device number is %d\n", devNum);
    printk(KERN_ALERT "[ lauti ] - Major num is: %d\n", maj);
    printk(KERN_ALERT "[ lauti ] - Minor num is: %d\n", min);
    return 0;
}

static void hello_exit(void) {
    // unregistering
    unregister_chrdev_region(devNum, count);
    printk(KERN_ALERT "[ lauti ] - Goodbye, cruel world\n");
}

module_init(hello_init);
module_exit(hello_exit);