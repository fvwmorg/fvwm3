FROM docker.pkg.github.com/fvwmorg/fvwm3/fvwm3-build:latest

COPY . /build
WORKDIR /build

RUN ./autogen.sh && ./configure && make
