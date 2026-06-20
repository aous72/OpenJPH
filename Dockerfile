FROM alpine:latest

# Working source root.
ENV _workroot=/usr/local/src
ENV PATH=${PATH}:/usr/local/bin
ENV LD_LIBRARY_PATH=/usr/local/lib

# install developement tools
RUN apk add --no-cache build-base tiff-dev cmake tzdata

ENV TZ=America/Los_Angeles

# compile OpenJPH
WORKDIR /usr/src/openjph/
COPY . .
WORKDIR /usr/src/openjph/build
RUN rm -R * || true 
RUN cmake -DCMAKE_BUILD_TYPE=Release -DOJPH_BUILD_TESTS=ON ..
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

# to use s390x big-endian build on a little-endian host follow these steps: 
# step 1 - config QEMU to run s390x big-endian binaries on a little-endian
#          host, this only needs to be done once per host:
# docker run --privileged --rm tonistiigi/binfmt --install all
# step 2 - build docker image using --platform linux/s390x command:
# docker build --rm -f Dockerfile -t openjph_s390x:latest --platform linux/s390x .
# step 3 - check that s390x is being used, this should print "s390x" to terminal
# docker run -it --rm openjph_s390x:latest uname -m
# step 4 - run docker image
# docker run -it --rm openjph_s390x:latest
# docker run -it --rm -v C:\\temp:/tmp openjph_s390x:latest
