# Stage 1: Build
FROM alpine:3.23 AS build

RUN apk add --no-cache \
    build-base \
    cmake \
    git

WORKDIR /src
COPY . .

# MPQCLI_BUILD_STATIC is what makes the scratch stage viable: the binary carries
# musl and libstdc++ with it and needs no loader at runtime
RUN cmake -B build/release \
        -DCMAKE_BUILD_TYPE=Release \
        -DMPQCLI_BUILD_APP=ON \
        -DMPQCLI_BUILD_STATIC=ON \
    && cmake --build build/release --parallel "$(nproc)" \
    && strip build/release/bin/mpqcli

# Stage 2: Runtime
FROM scratch

LABEL org.opencontainers.image.source="https://github.com/thegraydot/mpqcli"
LABEL org.opencontainers.image.description="A command-line tool to create, add, remove, list, extract, read, rename, and verify MPQ archives using the StormLib library"
LABEL org.opencontainers.image.licenses="MIT"

COPY --from=build /src/build/release/bin/mpqcli /mpqcli
USER 1001:1001
ENTRYPOINT ["/mpqcli"]
