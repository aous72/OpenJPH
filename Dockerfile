FROM ubuntu:noble

RUN apt-get update

# disable interactive install 
ENV DEBIAN_FRONTEND=noninteractive

# install developement tools
RUN apt-get -y install cmake
#RUN apt-get -y install g++
RUN apt-get -y install clang
RUN apt-get -y install libtiff-dev

# install developement debugging tools
RUN apt-get -y install valgrind

# OpenJPH
WORKDIR /usr/src/openjph/
COPY . .
WORKDIR /usr/src/openjph/build
RUN rm -R * || true
RUN cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=asan ../
RUN make
ENV LD_LIBRARY_PATH=/usr/src/openjph/bin
ENV PATH=$PATH:/usr/src/openjph/bin

# finalize docker environment
WORKDIR /usr/src/openjph

# step 1 - build docker image
# docker build --rm -f Dockerfile -t openjph:latest .
# step 2 - run docker image
# docker run -it --rm openjph:latest
# docker run -it --rm -v C:\\temp:/tmp openjph:latest
