#!/bin/bash
# Hexdumps a specified block from build/HaydenOS.img

dd bs=512 if=build/HaydenOS.img skip=$1 count=1 | hexdump -C