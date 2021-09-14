FROM ubuntu:focal

RUN apt-get update

# disable interactive install 
ENV DEBIAN_FRONTEND noninteractive

RUN apt-get -y install cmake
RUN apt-get -y install g++
RUN apt-get -y install libtiff-dev

# OpenJPH
WORKDIR /usr/src/openjph/
COPY . .
WORKDIR /usr/src/openjph/build
RUN cmake -DCMAKE_BUILD_TYPE=Release ../
RUN make
ENV LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/src/openjph/bin
ENV PATH=$PATH:/usr/src/openjph/bin

# finalize docker environment
WORKDIR /usr/src/openjph

# step 1 - build docker image
# docker build --rm -f Dockerfile -t openjph:latest .
# step 2 - run docker image
# docker run -it --rm openjph:latest
# docker run -it --rm -v C:\\temp:/usr/src/openjph/build openjph:latest
