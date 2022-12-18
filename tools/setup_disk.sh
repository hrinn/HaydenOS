#!/bin/bash
# Sets up MBR, FAT32 partition, and installs grub on drive

disk=$1
part=$11
mnt=/mnt/fatdisk

read -p "Are you sure you want to format $disk? y/(n) " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]
then
    exit 1
fi

echo "Formatting $disk for HaydenOS"
sudo parted $disk mklabel msdos
sudo parted $disk mkpart primary fat32 1MB 1GB
sudo parted $disk set 1 boot on

echo "Creating FAT32 $part on $disk"
sudo mkdosfs -F32 -f 2 $part

echo "Mounting $part at $mnt"
sudo mkdir $mnt
sudo mount $part $mnt

echo "Installing grub"
sudo grub-install --target=i386-pc --root-directory=$mnt --no-floppy \
    --modules="normal part_msdos ext2 multiboot" $disk

echo "Cleanup"
sudo umount $mnt
sudo rmdir $mnt