#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>

MODULE_LICENSE("GPL");

// Making those numbers
int devNum;
static int maj;
static int min = 0;
static int count = 4;
static int times;
char* family[] = { "Mom", "Dad", "Sister", "Shiva (my dog)" };

module_param(times, int, S_IRUGO);

static int hello_init(void) {
    // registering the device
    int i;
    int result = alloc_chrdev_region(&devNum, min, count, "hello_kernel");
    if (result < 0) {
        printk(KERN_ALERT "[ lauti ] - THIS IS NO GOOD, ERROR\n");
        return result;
    }
    maj = MAJOR(devNum);
    if (!times || times < 1 || times > 4) {
        times = 4;
    }
    for (i = 0; i < times; i++) {
        printk(KERN_ALERT "[ lauti ] - Hello %s \n", family[i]);
    }
    return 0;
}

static void hello_exit(void) {
    // unregistering
    unregister_chrdev_region(devNum, count);
    printk(KERN_ALERT "[ lauti ] - Goodbye, cruel world\n");
}

module_init(hello_init);
module_exit(hello_exit);