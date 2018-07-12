FROM ubuntu:16.04
MAINTAINER Billy Wong "billywong@m800.com"
RUN apt-get update && apt-get install -y --fix-missing apt-utils
RUN apt-get update && apt-get install -y --fix-missing make
RUN apt-get update && apt-get install -y --fix-missing binutils
RUN apt-get update && apt-get install -y --fix-missing gcc-4.9
RUN apt-get update && apt-get install -y --fix-missing g++-4.9
RUN apt-get update && apt-get install -y --fix-missing gdb libz-dev
RUN apt-get update && apt-get install -y --fix-missing autoconf
RUN apt-get update && apt-get install -y --fix-missing libtool
RUN apt-get update && apt-get install -y --fix-missing ccache
RUN apt-get update && apt-get install -y --fix-missing wget
RUN apt-get update && apt-get install -y --fix-missing doxygen
RUN apt-get update && apt-get install -y --fix-missing openssh-server
RUN apt-get update && apt-get install -y --fix-missing lsb-release
WORKDIR /usr/bin
RUN ln -s ./gcc-4.9 gcc
RUN ln -s ./g++-4.9 g++
# CMake
WORKDIR /usr/local
RUN wget https://cmake.org/files/v3.10/cmake-3.10.2.tar.gz
RUN tar xzf cmake-3.10.2.tar.gz
WORKDIR /usr/local/cmake-3.10.2
RUN ./bootstrap
RUN make
RUN make install
WORKDIR /usr/local
RUN rm -rf cmake-3.10.2.tar.gz
#bc gawk alsa-utils libpulse-dev libx11-dev libxext-dev for Media5 build
RUN apt-get install -y --fix-missing bc gawk
RUN apt-get install -y --fix-missing libx11-dev libxext-dev
RUN apt-get update && apt-get install -y libpulse-dev --no-install-recommends
RUN apt-get install -y alsa-utils
RUN apt-get install -y tcl
#pulseaudio socat ffmpeg for Audio debug
#RUN apt-get install -y --fix-missing pulseaudio socat ffmpeg
RUN apt-get update && apt-get install -y --fix-missing valgrind gcovr
RUN apt-get update && apt-get install -y --fix-missing ninja-build
WORKDIR /usr/bin
# Install pip and conan
RUN wget -q https://bootstrap.pypa.io/get-pip.py -O - | python
RUN pip install -q conan==0.30.1
RUN pip install -q conan_package_tools
WORKDIR ~/.conan
RUN rm -rf profiles
RUN mkdir /var/run/sshd
RUN echo 'root:screencast' | chpasswd
RUN sed -i 's/PermitRootLogin prohibit-password/PermitRootLogin yes/' /etc/ssh/sshd_config
# SSH login fix. Otherwise user is kicked off after login
RUN sed 's@session\s*required\s*pam_loginuid.so@session optional pam_loginuid.so@g' -i /etc/pam.d/sshd
ENV NOTVISIBLE "in users profile"
RUN echo "export VISIBLE=now" >> /etc/profile
EXPOSE 22
CMD ["/usr/sbin/sshd", "-D"]