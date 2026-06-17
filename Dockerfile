FROM ubuntu:26.04

RUN apt-get update

# disable interactive install 
ENV DEBIAN_FRONTEND=noninteractive

# install developement tools
RUN apt-get -y install cmake
RUN apt-get -y install g++
RUN apt-get -y install libtiff-dev

# install developement debugging tools
RUN apt-get -y install valgrind

# compile OpenJPH
WORKDIR /usr/src/openjph/
COPY . .
WORKDIR /usr/src/openjph/build
RUN rm -R * || true
RUN cmake -DCMAKE_BUILD_TYPE=Release -DOJPH_BUILD_TESTS=ON ../ 
RUN make
# install OpenJPH
RUN make install
RUN ldconfig

# finalize docker environment
WORKDIR /usr/src/openjph

# step 1 - build docker image
# docker build --rm -f Dockerfile -t openjph:latest .
# step 2 - run docker image
# docker run -it --rm openjph:latest
# docker run -it --rm -v C:\\temp:/tmp openjph:latest
