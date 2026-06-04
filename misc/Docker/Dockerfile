FROM alpine:3.23

ENV PICO_SDK_PATH=/pico-sdk

# build environment
RUN apk add --no-cache \
    git \
    python3 \
    py3-pip \
    build-base \
    cmake \
    newlib-arm-none-eabi \
    gcc-arm-none-eabi \
    g++-arm-none-eabi \
    linux-headers


RUN git clone --depth=1 https://github.com/raspberrypi/pico-sdk /pico-sdk
WORKDIR /pico-sdk
RUN git submodule update --init --depth=1

RUN git clone https://github.com/raspberrypi/picotool.git /picotool
RUN mkdir /picotool/build
WORKDIR /picotool/build
RUN cmake .. \
 && make -j4 \
 && make install \
 && rm -r /picotool

RUN git config --global safe.directory '*' 

ADD build.sh /build.sh

WORKDIR /src

ENTRYPOINT ["/bin/sh", "/build.sh"]
CMD ["pico"]
