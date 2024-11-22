FROM ubuntu:noble

RUN apt-get update

# disable interactive install 
ENV DEBIAN_FRONTEND noninteractive

# install developement tools
RUN apt-get -y install cmake
RUN apt-get -y install g++
RUN apt-get -y install git
RUN apt-get -y install valgrind

# install optional openjph dependencies
RUN apt-get -y install libtiff-dev
#RUN apt-get -y install libilmbase-dev
RUN apt-get -y install libopenexr-dev


######### build/install openexr from source
# WORKDIR /usr/src/
# RUN git clone https://github.com/madler/zlib.git
# WORKDIR /usr/src/zlib/build
# RUN cmake ..
# RUN make
# RUN make install

# WORKDIR /usr/src/
# RUN git clone https://github.com/AcademySoftwareFoundation/Imath.git
# WORKDIR /usr/src/Imath/build
# RUN cmake ..
# RUN make
# RUN make install

# WORKDIR /usr/src/
# RUN git clone https://github.com/AcademySoftwareFoundation/openexr.git
# WORKDIR /usr/src/openexr
# #RUN git checkout RB-3.1 
# WORKDIR /usr/src/openexr/build
# RUN cmake .. -DBUILD_TESTING=OFF -DOPENEXR_BUILD_TOOLS=OFF -DOPENEXR_INSTALL_EXAMPLES=OFF
# RUN make
# RUN make install

# OpenJPH
WORKDIR /usr/src/openjph/
COPY . .
WORKDIR /usr/src/openjph/build
RUN rm -R * || true
RUN cmake -DCMAKE_BUILD_TYPE=Release ../ 
RUN make
RUN make install
ENV LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/src/openjph/bin;/usr/local/lib/
ENV PATH=$PATH:/usr/src/openjph/bin

# finalize docker environment
WORKDIR /usr/src/openjph

# step 1 - build docker image
# docker build --rm -f Dockerfile -t openjph:latest .
# step 2 - run docker image
# docker run -it --rm openjph:latest
# docker run -it --rm -v C:\\temp:/tmp openjph:latest
