#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>

MODULE_LICENSE("GPL");

// Making those numbers
const int devNum = MKDEV(15, 20);
const static int maj = MAJOR(devNum);
const static int min = MINOR(devNum);



static int hello_init(void) {
    // registering the device
    int err = register_chrdev_region(devNum, min, "hello_kernel");
    if (err != 0) {
        printk(KERN_ALERT "[ lauti ] - THIS IS NO GOOD, ERROR\n");
        return 1;
    }
    printk(KERN_ALERT "[ lauti ] - Hello, world\n");
    printk(KERN_ALERT "[ lauti ] - The device number is %d\n", devNum);
    printk(KERN_ALERT "[ lauti ] - Major num is: %d\n", maj);
    printk(KERN_ALERT "[ lauti ] - Minor num is: %d\n", min);
    return 0;
}

static void hello_exit(void) {
    // unregistering
    unregister_chrdev_region(devNum, min);
    printk(KERN_ALERT "[ lauti ] - Goodbye, cruel world\n");
}

module_init(hello_init);
module_exit(hello_exit);