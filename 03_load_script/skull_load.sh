#!/bin/bash
module="skull"
device="skull"
mode="664"
maxDevIndex=3

# run insmod from the admins binaries
# use module variable to create the string
# representing the kernel object file
# append the arguments to the chain if needed/provided
# exit if something bad happens
/sbin/insmod $module.ko $* || exit 1

# select the group that will own the nodesgr
major=$(awk -v mod=$module '$2==mod {print $1}' /proc/devices)

# get staff or wheel group
group="staff"
grep -q '^staff:' /etc/group || group="wheel"

# remove all nodes and create all we need, with correct defaults
for i in $(seq 0 $maxDevIndex);
do 
    rm -f /dev/${device}${i}  
    mknod /dev/${device}${i} c $major $i
    chgrp $group /dev/${device}${i}
    chmod $mode /dev/${device}${i}
done 