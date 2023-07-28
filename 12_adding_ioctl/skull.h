#include <linux/ioctl.h>
#include <linux/cdev.h>
#include <linux/mutex.h>

#define SKULL "skull"
#define Q_SET_SIZE   16
#define QUANTUM_SIZE 16

/* IOCTL */
#define SKULL_IOC_MAGIC             0xCD
#define SKULL_IOC_RESET             _IO(SKULL_IOC_MAGIC,    0)
#define SKULL_IOC_SET_QUANTUM       _IOW(SKULL_IOC_MAGIC,   1, int)
#define SKULL_IOC_SET_QSET          _IOW(SKULL_IOC_MAGIC,   2, int)
#define SKULL_IOC_TELL_QUANTUM      _IO(SKULL_IOC_MAGIC,    3)
#define SKULL_IOC_TELL_QSET         _IO(SKULL_IOC_MAGIC,    4)
#define SKULL_IOC_GET_QUANTUM       _IOR(SKULL_IOC_MAGIC,   5 , int)
#define SKULL_IOC_GET_QSET          _IOR(SKULL_IOC_MAGIC,   6 , int)
#define SKULL_IOC_QUERY_QUANTUM     _IO(SKULL_IOC_MAGIC,    7)
#define SKULL_IOC_QUERY_QSET        _IO(SKULL_IOC_MAGIC,    8)
#define SKULL_IOC_EXCHANGE_QUANTUM  _IOWR(SKULL_IOC_MAGIC,  9, int)
#define SKULL_IOC_EXCHANGE_QSET     _IOWR(SKULL_IOC_MAGIC,  10, int)
#define SKULL_IOC_SHIFT_QUANTUM     _IO(SKULL_IOC_MAGIC,    11)
#define SKULL_IOC_SHIFT_QSET        _IO(SKULL_IOC_MAGIC,    12)
#define SKULL_IOC_MAXNR 12

struct node {
    struct node* next;
    void** data;
};

struct skull_d {
    struct node* data;  /* Pointer to first node of the linked lisit */
    int quantum;              /* the current quantum size */
    int qset;                 /* the current array size */
    unsigned long size;       /* amount of data stored here */
    struct mutex lock;
    struct cdev skull_cdev;
};
