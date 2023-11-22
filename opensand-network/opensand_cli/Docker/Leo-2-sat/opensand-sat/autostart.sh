#!/bin/bash
# Copyright (c) 2023 RomARS
# The code was developed by Mattia Quadrini <quadrini@romars.tech>


# Start SSH service
service ssh start 

#Run SSHD daemon
/usr/sbin/sshd -D

make
modprobe br_netfilter

iptables -t nat -A PREROUTING -i eth0 -j DNAT --to-destination 192.168.0.4
iptables -A FORWARD -i eth0 -d 192.168.0.4 -j ACCEPT

# Keep alive
sleep inf
