#!/bin/bash

# qemu-system-x86_64 -enable-kvm -chardev socket,id=vhost0,path=/tmp/vhost.sock -netdev vhost-user,id=user0,chardev=vhost0 -device virtio-net-pci,id=net0,netdev=user0 -drive file=/home/king/share/ovs/img/tinycore.raw,format=raw -cdrom /home/king/share/ovs/img/Core-current.iso

qemu-system-x86_64 -enable-kvm -m 512 -object memory-backend-file,id=mem0,size=512M,mem-path=/mnt/huge/,share=on  -numa node,memdev=mem0 -chardev socket,id=vhost0,path=/tmp/vhost.sock -netdev vhost-user,id=user0,chardev=vhost0 -device virtio-net-pci,id=net0,netdev=user0 -drive file=/home/king/share/ovs/img/tinycore.raw,format=raw -cdrom /home/king/share/ovs/img/Core-current.iso

