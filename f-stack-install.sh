#!/bin/bash

export HOME=$PWD

echo 1 > /proc/sys/vm/drop_caches
echo 2 > /proc/sys/vm/drop_caches
echo 3 > /proc/sys/vm/drop_caches

echo 4096 > /sys/devices/system/node/node0/hugepages/hugepages-2048kB/nr_hugepages
echo 4096 > /sys/devices/system/node/node1/hugepages/hugepages-2048kB/nr_hugepages

echo 0 > /sys/devices/system/node/node0/hugepages/hugepages-1048576kB/nr_hugepages
echo 0 > /sys/devices/system/node/node1/hugepages/hugepages-1048576kB/nr_hugepages

cat /sys/devices/system/node/node0/hugepages/hugepages-2048kB/nr_hugepages
cat /sys/devices/system/node/node1/hugepages/hugepages-2048kB/nr_hugepages

cat /sys/devices/system/node/node0/hugepages/hugepages-1048576kB/nr_hugepages
cat /sys/devices/system/node/node1/hugepages/hugepages-1048576kB/nr_hugepages

mkdir /mnt/huge
mount -t hugetlbfs nodev /mnt/huge

echo 0 > /proc/sys/kernel/randomize_va_space
cat /proc/sys/kernel/randomize_va_space

modprobe uio
insmod $HOME/dpdk/x86_64-native-linuxapp-gcc/kmod/igb_uio.ko
insmod $HOME/dpdk/x86_64-native-linuxapp-gcc/kmod/rte_kni.ko

cd $HOME/dpdk/usertools
python dpdk-devbind.py --status
ifconfig ens192 down
python dpdk-devbind.py --bind=igb_uio ens192
python dpdk-devbind.py --status
