#!/bin/sh
qemu-system-x86_64 -s -S -kernel ~/Document/OS/linux-4.9.263/arch/x86_64/boot/bzImage -initrd ~/Document/OS/initramfs-busybox-x64.cpio.gz --append "nokaslr root=/dev/ram init=/init"

