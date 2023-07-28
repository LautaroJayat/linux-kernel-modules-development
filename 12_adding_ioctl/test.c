#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "test.h"


int main(void) {
    int newQuantumSize = 32;
    int fd = open("/dev/skull0", O_RDWR);
    ioctl(fd, SKULL_IOC_RESET);
    int actualQuantumSize = ioctl(fd, SKULL_IOC_QUERY_QUANTUM);
    if (actualQuantumSize != QUANTUM_SIZE) {
        printf("Oh no!, the actual size is %d and the expected size was %d\n", actualQuantumSize, QUANTUM_SIZE);
    }
    else {
        printf("worked! the actual size now is %d\n", actualQuantumSize);
    }
    int res = ioctl(fd, SKULL_IOC_SET_QUANTUM, &newQuantumSize);
    actualQuantumSize = ioctl(fd, SKULL_IOC_QUERY_QUANTUM);
    if (actualQuantumSize == QUANTUM_SIZE) {
        printf("Oh no!, the actual size is %d and the expected size was %d\n", actualQuantumSize, newQuantumSize);
    }
    else {
        printf("worked! the actual size now is %d\n", actualQuantumSize);
    }
    return 0;
}