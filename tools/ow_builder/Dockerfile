#we are using ubuntu base image
FROM ubuntu:20.04
ARG uid
ARG uname

RUN ln -snf /usr/share/zoneinfo/UTC /etc/localtime && echo UTC > /etc/timezone

# installing requirements to get and extract prebuilt binaries
RUN apt-get -y update && apt-get install -y \
 libtool automake doxygen autoconf clang cmake

# set c++ and cc to clang
RUN update-alternatives --install /usr/bin/c++ c++ /usr/bin/clang++ 100
RUN update-alternatives --install /usr/bin/cc cc /usr/bin/clang 100

# user setup
RUN adduser -u $uid $uname

# set a distinct shell prompt
RUN echo 'export PS1="\[$(tput sgr0)\]\[$(tput bold)\][OW-BUILDER] $(tput sgr0)\]user:\u pwd:\w \\$\[$(tput sgr0)\] "' >> /home/${uname}/.bashrc

# version testing script
COPY list_versions.sh .
RUN /bin/bash list_versions.sh

# start the container from bash
ENTRYPOINT [ “su $uname” ]
