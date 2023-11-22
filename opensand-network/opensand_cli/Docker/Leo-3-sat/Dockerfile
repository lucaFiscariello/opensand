#!/bin/bash
# Copyright (c) 2023 RomARS
# The code was developed by Mattia Quadrini <quadrini@romars.tech>

FROM ubuntu:jammy
ENV TZ=Europe/Rome
LABEL vendor="RomARS s.r.l."
LABEL product="Docker OpenSand"
LABEL release="1.0.0"
WORKDIR /root
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone \ 
	&& apt update \
	&& apt install -y iputils-ping \
	iproute2 \
	bmon \
	tcpdump \
	iptables \
	vim \
	curl \
	gpg \
	wget \
	openssh-server \
	make \
	python3-yaml \
	kmod \
	linux-modules-$(uname -r) 
RUN curl -sS https://raw.githubusercontent.com/CNES/net4sat-packages/master/gpg/net4sat.gpg.key | gpg --dearmor | tee /etc/apt/trusted.gpg.d/net4sat.gpg >/dev/null \
	&& echo "deb https://raw.githubusercontent.com/CNES/net4sat-packages/master/jammy/ jammy stable" | tee /etc/apt/sources.list.d/net4sat.list \
	&& apt update \
	&& apt install -y opensand 
RUN apt autoremove -y
RUN useradd -rm -d /root -s /bin/bash -g root -G sudo -u 1000 opensand 
RUN  echo 'opensand:opensand' | chpasswd
EXPOSE 22
CMD ["bash"]




