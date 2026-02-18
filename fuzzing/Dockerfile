FROM gcr.io/oss-fuzz-base/base-builder

RUN apt-get update
RUN apt-get -y install cmake
RUN apt-get -y install libtiff-dev

WORKDIR /app
RUN git clone --depth 1 https://github.com/aous72/jp2k_test_codestreams.git
