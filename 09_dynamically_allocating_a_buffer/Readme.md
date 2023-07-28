# Dynamically allocating a buffer

In the previous example our device used a fixed size buffer for storing data. Although it is an easy way to have some space for storing things, it isn't something too flexible.

In this example we will allocate a buffer that grows on demand.
Each time we want to write to the device we will check how much memory do we have, if we don't have enough we will reallocate more.

First we will modify the struct of our device to keep track of how much memory we have allocated, and to store the pointer to our buffer:

```c
struct skull_d {
    unsigned int bufferSize;
    char* data;
    struct cdev skull_cdev;
};
```

Then, we will modify our `write` callback:

```c
static ssize_t write(struct file* filp, const char __user* buf, size_t len, loff_t* off) {
    int result;
    struct skull_d* dev = filp->private_data;
    pr_alert("%s - WRITE TRIGGERED!", PREF);

    // if user input is bigger than our buffer...
    if (!dev->data || dev->bufferSize < len) {
        pr_info("%s - The ammount of data you want is greater than what have, resizing!", PREF);
        // allocate the double (just in case)
        dev->data = krealloc(dev->data, len * 2, GFP_KERNEL);
        if (!dev->data) return -EFAULT;
        // and keep track of the size
        dev->bufferSize = len * 2;
    }
    // clean the buffer
    memset(dev->data, 0, dev->bufferSize);
    pr_info("%s - the bufferSize is: %d", PREF, dev->bufferSize);
    result = copy_from_user(dev->data, buf, len);
    *off = *off + len - result;
    return result == 0 ? len - result : -EFAULT;
}
```

So now we can perform some tests:

[Resizing was ok](./writing_to_a_buffer.jpg)

## Additional reading

- [Kernel docs > core api > memory allocation](https://www.kernel.org/doc/html/latest/core-api/memory-allocation.html)
