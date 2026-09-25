# Stage 1: Build
FROM alpine:3.23 AS build

RUN apk add --no-cache \
    # The C and C++ toolchain, with the musl-dev the static link needs
    build-base \
    # Configures and drives the build
    cmake \
    # Stamps the commit hash into the version header; see .dockerignore
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

# No USER, deliberately. The image exists to process files on a bind mount, and
# no fixed uid can match whoever owns the host directory, so a non-root default
# would fail every write for most Linux users. Callers who want output owned by
# themselves pass --user "$(id -u):$(id -g)", which the installation docs show
WORKDIR /data
ENTRYPOINT ["/mpqcli"]
