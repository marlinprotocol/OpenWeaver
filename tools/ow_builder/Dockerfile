#we are using ubuntu base image
FROM ubuntu:18.04

# installing requirements to get and extract prebuilt binaries
RUN apt-get update && apt-get install -y \
 xz-utils  curl  build-essential  libtool    sudo \
 automake  vim   doxygen          autoconf   wget \
 lsb-release     software-properties-common \
 && rm -rf /var/lib/apt/lists/*

# setting up clang 12
RUN wget https://apt.llvm.org/llvm.sh
RUN chmod +x llvm.sh
RUN sudo ./llvm.sh 12

# setting up cmake (the bundled cmake with apt in 18.04 is too old)
RUN wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | sudo tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null
RUN sudo apt-add-repository 'deb https://apt.kitware.com/ubuntu/ bionic main'
RUN sudo apt update
RUN sudo apt-get install -y cmake

# set c++ and cc to clang 12
RUN sudo update-alternatives --install /usr/bin/c++ c++ /usr/bin/clang++-12 100
RUN sudo update-alternatives --install /usr/bin/cc cc /usr/bin/clang-12 100

# set a distinct shell prompt
RUN echo 'export PS1="\[$(tput sgr0)\]\[$(tput bold)\][OW-BUILDER] $(tput sgr0)\]user:\u pwd:\w \\$\[$(tput sgr0)\] "' >> ~/.bashrc

# version testing script
COPY list_versions.sh .
RUN /bin/bash list_versions.sh

# start the container from bash
ENTRYPOINT [ “/bin/bash” ]