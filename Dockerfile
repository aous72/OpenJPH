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
# docker build --rm -f Dockerfile_alpine -t openjph_alpine:latest .
# step 2 - run docker image
# docker run -it --rm openjph_alpine:latest
# docker run -it --rm -v C:\\temp:/tmp openjph_alpine:latest

# to use s390x big endian build, conifigure QEMU to run 
# 1 - config QEMU: docker run --privileged --rm tonistiigi/binfmt --install all
# to build and run s390x big endian build, add --platform linux/s390x to the docker build and run commands as shown below
# 2 - add --platform linux/s390x to the docker build and run commands
# step 1 - build docker image using s390x big endian architecture
# docker build --rm -f Dockerfile_alpine -t openjph_alpine_s390x:latest --platform linux/s390x .
# step 2 - check that s390x is being used - this should print "s390x" to terminal
# docker run -it --rm openjph_alpine_s390x:latest uname -m
# step 3 - run docker image
# docker run -it --rm openjph_alpine_s390x:latest
# docker run -it --rm -v C:\\temp:/tmp openjph_alpine_s390x:latest



