#!/bin/bash
# Installs HaydenOS on my /dev/sdb1 partition

mnt=/mnt/fatpart
part=/dev/disk/by-uuid/3DFE-C175

sudo mkdir $mnt
sudo mount $part $mnt
sudo cp -r build/img/* $mnt
sudo mkdir $mnt/bin
sudo cp build/init.bin $mnt/bin/init.bin
sudo umount $mnt
sudo rmdir $mnt