#!/bin/bash

KERN_IMG=$1
SOURCE_DIR=`dirname $0`

pushd $SOURCE_DIR

cp thirdparty/grub/grub.cfg isofiles/boot/grub/
cp thirdparty/grub/grub.img isofiles/boot/grub/
cp ${KERN_IMG} isofiles/boot/
touch isofiles/boot/grub/stage2
xorriso -as mkisofs -R -b boot/grub/grub.img -no-emul-boot -boot-load-size 4 -boot-info-table -o boot.iso isofiles

popd
