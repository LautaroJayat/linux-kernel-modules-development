# Messing with the Linux kernel

At the moment I'm just following the Linux Device Drivers book to get a hint on how the kernel works. I will follow the path it proposes, so the experiments will correlate with the exposition order of the topics in the book.

But, as this book is from 2009, and it works with the version 2.6 of the kernel, I will be using some other resources to update the examples.

This repo will document my journey on learning some cool kernel stuff.
Keep in mind I'm just hacking my way through this... things will be messy and suboptimal, I'm just making some experiments.

This series of experiments does not start from building a custom kernel image, booting with a custom `init`, mounting everything and so on. If you are interested in that please refer to the following links:

- [Tutorial: Building the Simplest Possible Linux System - Rob Landley](https://www.youtube.com/watch?v=Sk9TatW9ino)
- [Linux initramfs for fun, and, uh... - David Hand](https://www.youtube.com/watch?v=KQjRnuwb7is)
- [Linux From Scratch](https://www.linuxfromscratch.org/)
- [Minimal Linux - Ivan Davidov](https://github.com/ivandavidov/minimal/)
- [Mkroot - Landley](https://github.com/landley/mkroot)

## Experiments

| index | title                                                                                  | contents                                                                                                        |
| ----- | -------------------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------- |
| 0     | [Hello World from the kernel](./00_hello_world/)                                       | setting up a vm, sharing files between gest and host, compiling and inserting a new module                      |
| 1     | [Mounting and checking some info](./01_mounting_and_checking/)                         | allocating minor and major numbers, checking our char device is listed                                          |
| 2     | [Using parameters with insmod](./02_insmod_parameters/)                                | updating module to accept parameters when mounting                                                              |
| 3     | [The load script](./03_load_script/)                                                   | creating a simple init script to set the files for our devices                                                  |
| 4     | [Registering File Operations](./04_registering_file_operations/)                       | setting the callbacks that will enable to interact with the module from userland                                |
| 5     | [Simplest Open Callback](./05_simplest_open_fop/)                                      | setting private_data to file pointer and checking access mode                                                   |
| 6     | [Simplest Read Callback](./06_simplest_read_fop/)                                      | making our device to stream a long string :)                                                                    |
| 7     | [Reading from private_data](./07_reading_from_private_data/)                           | using the private_data field as the source of the string we are going to print                                  |
| 8     | [Writing to private_data](./08_write_fop/)                                             | writing to a fixed size buffer stored in private_data                                                           |
| 9     | [Dynamically allocating memory from the device](./09_dynamically_allocating_a_buffer/) | allocating, reading and resizing from our file operations                                                       |
| 10    | [Implementing an interesting memory management strategy](./10_simple_pagination/)      | Implementing a memory management strategy similar to the one used in chapter 3 of the Linux Device Drivers Book |
| 11    | [Introducing the mutex](./11_introducing_the_mutex/)                                   | Implementing a simple mutex to avoid race conditions when multiple processes try to read/write into the device  |
| 12    | [Intorucing IOCTL](./12_adding_ioctl/)                                                 | Introducing a simple implementation of ioctl to demostrate how it works                                         |

## Various Resources

- [Linux Device Drivers, Third Edition - by Jonathan Corbet, Alessandro Rubini, and Greg Kroah-Hartman](https://lwn.net/Kernel/LDD3)
- [Updated examples from the book](https://github.com/martinezjavier/ldd3)
- [Linux Kernel in a Nutshell - Greg Kroah-Hartman](http://www.kroah.com/lkn/)
- [Linux Kernel Module cheat - Ciro Santilli](https://cirosantilli.com/linux-kernel-module-cheat/)
- [Linux Kernel Docs > driver-api > infrastructure](https://www.kernel.org/doc/html/latest/driver-api/infrastructure.html)
- [Understanding the Structure of a Linux Kernel Device Driver - Sergio Prado, Toradex](https://www.youtube.com/watch?v=pIUTaMKq0Xc)
- [A super quick guide on registering devices - dhanushka / stackoverflow ](https://stackoverflow.com/a/48837481/11280510)
- [cdev_init vs cdev_alloc - Tsyvarev / stackoverflow](https://stackoverflow.com/questions/70004545/cdev-alloc-vs-cdev-init)
- [linux kernel labs > device drivers](https://linux-kernel-labs.github.io/refs/heads/master/labs/device_drivers.html)

## The Monkey

![The Monkey](the_monke.jpg)
