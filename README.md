# MPI-tutorial-on-ubuntu-20.04
<p><p>
  This tutorial contains my personal notes on using MPI on Ubuntu 20.04. I have an ubuntu 20.04 host computer and I am going to run two virtual machines (VMs), namely vm01 and vm02, on it. Each VM has two NICs. The first NIC connects the VM to internet, while the other one connects it to a local network, implemented using an openvswitch bridge. 
<p><p>
<h4>Network</h4>
<p><p>
There are two networks that the VMs use. First, it use a primary network (ens3) to communicate to the Internet. Iassume you know how to set up brige network for QEMU VMs. 
<p><p>
The second network is a local network that we will use for MPI communication. I use openvswitch to create a virtual switch called "br-int" below. Note that the "host$" represents the shell command line prompt string of the host computer. 
<pre>
host$ sudo apt install openvswitch-switch
host$ sudo ovs-vsctl add-br br-int
host$ sudo ovs-vsctl add-port br-int gw1 -- set interface gw1 type=internal
</pre>

<h4>Virtual Machines</h4>
<p><p>
  The hypervisor I used to create VMs is QEMU-KVM. I have downloaded th source code of the software and compile them on a user-defined directory. You can see the script be low for more details of both VMs. 
 <p><p>
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
     -rtc base=localtime,clock=vm 

</pre>

