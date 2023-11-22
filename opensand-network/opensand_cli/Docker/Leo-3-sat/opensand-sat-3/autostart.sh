#!/bin/bash
# Copyright (c) 2023 RomARS
# The code was developed by Mattia Quadrini <quadrini@romars.tech>


# Start SSH service
service ssh start 

#Run SSHD daemon
/usr/sbin/sshd -D

make
modprobe br_netfilter


# Keep alive
sleep inf
