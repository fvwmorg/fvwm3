FROM fvwmorg/fvwm3-build:latest
#FROM docker.pkg.github.com/fvwmorg/fvwm3/fvwm3-build:latest

ENV GOROOT="/usr/lib/go-1.14/"
ENV PATH="$GOROOT/bin:$PATH"
ENV GO111MODULE="on"

COPY . /build
WORKDIR /build

RUN meson setup --reconfigure build -Dmandoc=true && meson compile -C build
