#!/bin/bash
# Makes a fat32 disk image containing HaydenOS

img=bin/HaydenOS.img
mnt=/mnt/fatgrub

dd if=/dev/zero of=$img bs=512 count=32768
sudo parted $img mklabel msdos
sudo parted $img mkpart primary fat32 2048s 30720s
sudo parted $img set 1 boot on
loop0=$(sudo losetup -f)
sudo losetup $loop0 $img
loop1=$(sudo losetup -f)
sudo losetup $loop1 $img -o 1MiB
sudo mkdosfs -F32 -f 2 $loop1
sudo mkdir $mnt
sudo mount $loop1 $mnt
sudo grub-install --target=i386-pc --root-directory=$mnt --no-floppy \
    --modules="normal part_msdos ext2 multiboot" $loop0
sudo cp -r bin/img/* $mnt
sudo umount $mnt
sudo rmdir $mnt
sudo losetup -d $loop0
sudo losetup -d $loop1