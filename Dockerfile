FROM ubuntu:14.04

RUN apt-get update 
RUN apt-get install -y
RUN apt-get install -y gcc
RUN apt-get install -y make
RUN apt-get install -y build-essential gdb
RUN apt-get install -y gcc-multilib
RUN apt-get install -y zlib1g-dev
RUN apt-get install -y libglib2.0-dev
RUN apt-get install -y libpixman-1-dev
RUN apt-get install -y git
RUN git clone https://github.com/geofft/qemu.git -b 6.828-1.7.0
RUN cd qemu && ./configure --disable-kvm --target-list="i386-softmmu x86_64-softmmu" && make && make install



RUN mkdir to_build

WORKDIR "/to_build"

CMD ["bash"]
