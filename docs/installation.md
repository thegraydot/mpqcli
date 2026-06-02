# Installation

## Precompiled Binaries

Pre-built binaries are available for Linux and Windows.

Linux/WSL:

```bash
$ curl -fsSL https://raw.githubusercontent.com/thegraydot/mpqcli/main/scripts/install.sh | bash
```

Microsoft Windows:

```powershell
PS> irm https://raw.githubusercontent.com/thegraydot/mpqcli/main/scripts/install.ps1 | iex
```

Check the [latest release with binaries](https://github.com/thegraydot/mpqcli/releases).

## Docker Image

The Docker image for `mpqcli` is hosted on [GitHub Container Registry (GHCR)](https://ghcr.io). It provides a lightweight and portable way to use `mpqcli` without needing to build or download a binary.

To download the latest version of the `mpqcli` Docker image, run:

```bash
$ docker pull ghcr.io/thegraydot/mpqcli:latest
```

You can run `mpqcli` commands directly using the Docker container. For example:

```bash
$ docker run ghcr.io/thegraydot/mpqcli:latest version
```

To use local files in the container, mount a directory from your host system. In the following example, the `-v` argument is used to mount the present working directory to `/data` directory in the container. Then the `mpqcli` container runs the `list` subcommand with `/data/example.mpq` as the target MPQ archive.

```bash
$ docker run -v $(pwd):/data ghcr.io/thegraydot/mpqcli:latest list /data/example.mpq
```
