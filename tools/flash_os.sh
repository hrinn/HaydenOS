#!/bin/bash
# Sets up MBR, FAT32 partition, and installs grub on drive

part=$1
mnt=/mnt/fatdisk

read -p "Are you sure you want to install HaydenOS on $part? y/(n) " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]
then
    exit 1
fi

echo "Mounting $part at $mnt"
sudo mkdir $mnt
sudo mount $part $mnt

echo "Installing HaydenOS"
sudo cp -r bin/img/* $mnt

echo "Cleanup"
sudo umount $mnt
sudo rmdir $mnt