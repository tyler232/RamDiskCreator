#!/bin/bash

sudo umount /mnt/ramdisk
sudo losetup -d /dev/loop0
sudo losetup -a
