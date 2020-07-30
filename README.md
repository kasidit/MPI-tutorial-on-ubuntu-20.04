# MPI-tutorial-on-ubuntu-20.04
<p><p>
  This tutorial contains my personal notes on using MPI on Ubuntu 20.04. I have an ubuntu 20.04 host computer and I am going to run two virtual machines (VMs), namely vm01 and vm02, on it. Each VM has two NICs. The first NIC connects the VM to internet, while the other one connects it to a local network, implemented using an openvswitch bridge. This is how it gonna look like. 
<pre>
internet 
   |
   |  host
+-------------------------------------------+
| eno1                                      |
|  |                                        |
|  br0 --------- vm01 ---------- br-int     |
|  |                               |        |
|  +-------------vm02 -------------+        |
+-------------------------------------------+
</pre>
<p><p>
<h4>Network</h4>
<p><p>
There are two networks that the VMs use. The VM use its first NIC to access internet. It connect TAP NIC to br0, a linux bridge I created that in turn connect to the physical NIC of the host (eno1). 
<p><p>
  Referring to <a href="https://levelup.gitconnected.com/how-to-setup-bridge-networking-with-kvm-on-ubuntu-20-04-9c560b3e3991">this tutorial</a>, I do the followings. First, I download QEMU-KVM and linux bridge using the following command. 
<pre>
$ sudo apt install qemu-kvm bridge-utils
</pre>
I will also disable netfilter on bridge network for performance and then reboot. (optional). 
<pre>
$ sudo vi /etc/sysctl.d/bridge.conf
$ cat /etc/sysctl.d/bridge.conf
net.bridge.bridge-nf-call-ip6tables=0
net.bridge.bridge-nf-call-iptables=0
net.bridge.bridge-nf-call-arptables=0
$ sudo vi /etc/udev/rules.d/99-bridge.rules
$ cat /etc/udev/rules.d/99-bridge.rules
ACTION=="add", SUBSYSTEM=="module", KERNEL=="br_netfilter", RUN+="/sbin/sysctl -p /etc/sysctl.d/bridge.conf"
$ sudo reboot
</pre>

<p><p>
The second network is a local network that we will use for MPI communication. I use openvswitch to create a virtual switch called "br-int" below. Note that the "$" represents the shell command line prompt of the host computer. 
<pre>
$ sudo apt install openvswitch-switch
$ sudo ovs-vsctl add-br br-int
$ sudo ovs-vsctl add-port br-int gw1 -- set interface gw1 type=internal
</pre>
The gw1 interface is used by the host to connect to the VMs that will be in this internal LAN. 
I do the following to set its IP address to 10.1.0.1/24.
First, make sure network is not managed by cloud-init. 
<pre>
$ cat /etc/cloud/cloud.cfg.d/subiquity-disable-cloudinit-networking.cfg
network: {config: disabled}
</pre>
Then, I check if the gw1 interface exist. 
<pre>
$ ip add show
1: lo: <LOOPBACK,UP,LOWER_UP> mtu 65536 qdisc noqueue state UNKNOWN group default qlen 1000
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
<snip>
5: gw1: <BROADCAST,MULTICAST> mtu 1500 qdisc noop state DOWN group default qlen 1000
    link/ether ce:8a:10:00:e7:9d brd ff:ff:ff:ff:ff:ff
<snip>
</pre>
Now, I edit netplan's network interface configuration file, and apply the new configuration. Note that gw1 and its address are addes as highlighted. 
<pre>
$ sudo vi /etc/netplan/00-installer-config.yaml
$ cat /etc/netplan/00-installer-config.yaml
# This is the network config written by 'subiquity'
network:
  ethernets:
    eno1:
      dhcp4: false
 <b>gw1: 
      addresses: [10.1.0.1/24]</b>
  bridges:
    br0:
      interfaces: [ eno1 ] 
      parameters: 
        stp: true
        forward-delay: 0
      dhcp4: true
  version: 2
$ sudo netplan apply
</pre>
Finally, I check if gw1 has the IP I wanted using the following commands. 
<pre>
$ ip add show
<snip>
5: gw1: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc noqueue state UNKNOWN group default qlen 1000
    link/ether ce:8a:10:00:e7:9d brd ff:ff:ff:ff:ff:ff
    inet 10.1.0.1/24 brd 10.1.0.255 scope global gw1
       valid_lft forever preferred_lft forever
    inet6 fe80::cc8a:10ff:fe00:e79d/64 scope link 
       valid_lft forever preferred_lft forever
<snip>
$ ip route show
default via 192.168.1.1 dev br0 proto dhcp src 192.168.1.35 metric 100 
10.1.0.0/24 dev gw1 proto kernel scope link src 10.1.0.1 
<snip>
$ 
</pre>
Now, I am going to use ovs-vsctl command to check openvswitch configuration of the host.
<pre>
$ sudo ovs-vsctl show
45428e03-a95f-4652-81a5-017a59e669e2
    Bridge br-int
        Port br-int
            Interface br-int
                type: internal
        Port gw1
            Interface gw1
                type: internal
    ovs_version: "2.13.0"
$ 
</pre>
<h4>Virtual Machines</h4>
<p><p>
Next, I am going to create script files to run VMs on the host. The hypervisor I used to create VMs is QEMU-KVM. I have downloaded th source code of the software and compile them on the /home/kasidit/qemu-kvm/bin directory. You can see the script be low for more details of both VMs. I use the word "trail" cos it's kinda like the path I take exploring a jungle. 
 <p>
The first interface in the script below use qemu-ifup and qemu-ifdown scripts to attach and detach a VM to the br0 Linux bridge. The second interface option tn the script use ovs-ifup and ovs-ifdown to do the same thing for the br-int ovs bridge. The details of these *-ifup and *-ifdown are shown after the launching scripts of the VMs. 
 <p>
vm01: <br>
<pre>
#!/bin/bash
numsmp="4"
memsize="10G"
etcloc=/srv/kasidit/trail-00/etc
imgloc=/srv/kasidit/trail-00/images
imgfile="vm01.img"
ovlloc=/home/kasidit/trail-00/images
ovlfile="vm01.ovl"
exeloc="/srv/kasidit/qemu-kvm/bin"
#
#rm ${ovlloc}/${ovlfile}
#qemu-img create -f qcow2 -b ${imgloc}/${imgfile} ${ovlloc}/${ovlfile}
#
sudo ${exeloc}/qemu-system-x86_64 \
     -enable-kvm \
     -cpu host,kvm=off \
     -smp ${numsmp} \
     -m ${memsize} \
     -drive file=${imgloc}/${imgfile},format=qcow2 \
     -boot c \
     -vnc :95 \
     -qmp tcp::9556,server,nowait \
     -monitor tcp::6559,server,nowait \
     -netdev type=tap,script=${etcloc}/qemu-ifup,downscript=${etcloc}/qemu-ifdown,id=hostnet1 \
     -device virtio-net-pci,romfile=,netdev=hostnet1,mac=00:81:50:00:01:95 \
     -netdev type=tap,script=${etcloc}/ovs-ifup,downscript=${etcloc}/ovs-ifdown,id=hostnet2 \
     -device virtio-net-pci,romfile=,netdev=hostnet2,mac=00:81:50:00:02:95 \
     -rtc base=localtime,clock=vm 
</pre>
<p><p>
vm02: <br>
<pre>
#!/bin/bash
numsmp="4"
memsize="6G"
etcloc=/srv/kasidit/trail-00/etc
imgloc=/srv/kasidit/trail-00/images
imgfile="vm02.img"
ovlloc=/home/kasidit/trail-00/images
ovlfile="vm01.ovl"
exeloc="/srv/kasidit/qemu-kvm/bin"
#
#rm ${ovlloc}/${ovlfile}
#qemu-img create -f qcow2 -b ${imgloc}/${imgfile} ${ovlloc}/${ovlfile}
#
sudo ${exeloc}/qemu-system-x86_64 \
     -enable-kvm \
     -cpu host,kvm=off \
     -smp ${numsmp} \
     -m ${memsize} \
     -drive file=${imgloc}/${imgfile},format=qcow2 \
     -boot c \
     -vnc :85 \
     -qmp tcp::8558,server,nowait \
     -monitor tcp::7557,server,nowait \
     -netdev type=tap,script=${etcloc}/qemu-ifup,downscript=${etcloc}/qemu-ifdown,id=hostnet1 \
     -device virtio-net-pci,romfile=,netdev=hostnet1,mac=00:71:50:00:01:85 \
     -netdev type=tap,script=${etcloc}/ovs-ifup,downscript=${etcloc}/ovs-ifdown,id=hostnet2 \
     -device virtio-net-pci,romfile=,netdev=hostnet2,mac=00:71:50:00:02:85 \
     -rtc base=localtime,clock=vm 

</pre>
<p>
  Here are the *-ifup and *-ifdown scripts. 
<pre>
$ cat qemu-ifup
#!/bin/sh
switch=br0
/sbin/ifconfig $1 0.0.0.0 promisc up
/sbin/brctl addif ${switch} $1
$ cat qemu-ifdown
#!/bin/sh
switch=br0
/sbin/ifconfig $1 down
/sbin/brctl delif ${switch} $1
$ cat ovs-ifdown
#!/bin/sh
switch='br-int'
/sbin/ifconfig $1 0.0.0.0 down
ovs-vsctl del-port ${switch} $1
$ cat ovs-ifup
#!/bin/sh
switch='br-int'
/sbin/ifconfig $1 0.0.0.0 up
ovs-vsctl add-port ${switch} $1
$ 
</pre>
Next, I launched both VMs. The original network setup of the VMs use dhcp which I don't like cos IP address may change when I lofin next time. Also, since I am going to use the fist NIC for NFS between the VMs, those IP addresses on the first NIC should be static. Supposed the IP address of the ens3 nic of vm01 and vm02 are 192.168.1.221 and 192.168.1.222, and the IP addresses of the ens4 nic of the vms are 10.0.1.221 and 10.0.1.222, respectively. I will just show what I did on VM02 below. 
<pre>
vm02$ cat /etc/cloud/cloud.cfg.d/subiquity-disable-cloudinit-networking.cfg
network: {config: disabled}
vm02$ ip link
1: lo: <LOOPBACK,UP,LOWER_UP> mtu 65536 qdisc noqueue state UNKNOWN mode DEFAULT group default qlen 1000
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
2: ens3: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc fq_codel state UP mode DEFAULT group default qlen 1000
    link/ether 00:71:50:00:01:85 brd ff:ff:ff:ff:ff:ff
3: ens4: <BROADCAST,MULTICAST> mtu 1500 qdisc noop state DOWN mode DEFAULT group default qlen 1000
    link/ether 00:71:50:00:02:85 brd ff:ff:ff:ff:ff:ff
vm02$ 
vm02$ 
vm02$ sudo vi /etc/netplan/00-installer-config.yaml
vm02$ 
vm02$ 
vm02$ cat /etc/netplan/00-installer-config.yaml
# This is the network config written by 'subiquity'
network:
  ethernets:
    ens3:
      addresses: [192.168.1.222/24]
      gateway4: 192.168.1.1
      nameservers: 
        addresses: [8.8.8.8]
    ens4:
      addresses: [10.0.1.222/24]
  version: 2
vm02$ sudo netplan apply
vm02$ ifconfig
</pre>
Now that we have the IP addresses set. I will install NFS where vm02 is the NFS server and vm01 is the client. 
<p>
on VM02 (nfs server): <br>
<pre>
vm02$ sudo apt update
vm02$ sudo apt install nfs-kernel-server
vm02$ sudo mkdir /srv/nfs
vm02$ sudo vi /etc/exports
vm02$ cat /etc/exports
# /etc/exports: the access control list for filesystems which may be exported
#		to NFS clients.  See exports(5).
#
# Example for NFSv2 and NFSv3:
# /srv/homes       hostname1(rw,sync,no_subtree_check) hostname2(ro,sync,no_subtree_check)
#
# Example for NFSv4:
# /srv/nfs4        gss/krb5i(rw,sync,fsid=0,crossmnt,no_subtree_check)
# /srv/nfs4/homes  gss/krb5i(rw,sync,no_subtree_check)
#
/srv/nfs 192.168.1.221(rw,sync,no_root_squash,no_subtree_check)
vm02$ 
vm02$ sudo systemctl restart nfs-kernel-server
vm02$ sudo ufw allow from 192.168.1.221 to any port nfs
vm02$ sudo ufw status
</pre>
on VM01 (nfs client): <br>
<pre>
sudo apt update
sudo apt install nfs-common
kasidit@vm01:~$ sudo mkdir -p /srv/nfs
kasidit@vm01:~$ sudo mount 192.168.1.222:/srv/nfs /srv/nfs
kasidit@vm01:~$ 
kasidit@vm01:~$ 
kasidit@vm01:~$ df -h
Filesystem              Size  Used Avail Use% Mounted on
udev                    4.9G     0  4.9G   0% /dev
tmpfs                   997M  1.2M  996M   1% /run
/dev/sda2               148G   33G  109G  24% /
tmpfs                   4.9G     0  4.9G   0% /dev/shm
tmpfs                   5.0M     0  5.0M   0% /run/lock
tmpfs                   4.9G     0  4.9G   0% /sys/fs/cgroup
/dev/loop0               55M   55M     0 100% /snap/core18/1754
/dev/loop3               72M   72M     0 100% /snap/lxd/16100
/dev/loop2               72M   72M     0 100% /snap/lxd/16044
/dev/loop1               55M   55M     0 100% /snap/core18/1880
/dev/loop4               30M   30M     0 100% /snap/snapd/8542
/dev/loop5               30M   30M     0 100% /snap/snapd/8140
tmpfs                   997M   16K  997M   1% /run/user/122
tmpfs                   997M  4.0K  997M   1% /run/user/1000
192.168.1.222:/srv/nfs   49G  6.3G   40G  14% /srv/nfs
$
kasidit@vm01:~$ du -s /srv/nfs
4	/srv/nfs
kasidit@vm01:~$ 
kasidit@vm01:~$ sudo touch /srv/nfs/kasidit
kasidit@vm01:~$ ls -l /srv/nfs/kasidit
-rw-r--r-- 1 root root 0 Jul 27 11:24 /srv/nfs/kasidit

</pre>

