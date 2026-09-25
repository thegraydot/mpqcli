# Installation

## Precompiled Binaries

Pre-built binaries are available for Linux and Windows.

Linux/WSL:

```bash
$ curl -fsSL https://github.com/thegraydot/mpqcli/releases/latest/download/install.sh | bash
```

Microsoft Windows:

```powershell
PS> Invoke-WebRequest -Uri https://github.com/thegraydot/mpqcli/releases/latest/download/install.ps1 -OutFile install.ps1
PS> .\install.ps1
```

Both installers put `mpqcli` on the system path and may ask for elevation; pass `--user` on Linux or `-User` on Windows to install for the current user instead. Each release also ships `checksums.txt`, signed with cosign: the installer verifies the SHA-256 of the binary always, and the signature when cosign is installed.

Check the [latest release with binaries](https://github.com/thegraydot/mpqcli/releases).

## Docker Image

The Docker image for `mpqcli` is hosted on [GitHub Container Registry (GHCR)](https://ghcr.io). It provides a lightweight and portable way to use `mpqcli` without needing to build or download a binary.

To download the latest version of the `mpqcli` Docker image, run:

```bash
$ docker pull ghcr.io/thegraydot/mpqcli:latest
```

You can run `mpqcli` commands directly using the Docker container. For example:

```bash
$ docker run --rm ghcr.io/thegraydot/mpqcli:latest version
```

To use local files, mount a directory from your host system at `/data`. That is the container's working directory, so paths are relative to the mounted directory. The following example lists `example.mpq` from the present working directory:

```bash
$ docker run --rm -v "$PWD":/data ghcr.io/thegraydot/mpqcli:latest list example.mpq
```

The container runs as root, so a file it creates or extracts is owned by root on the host. To have the output owned by you, run the container as your own user:

```bash
$ docker run --rm --user "$(id -u):$(id -g)" -v "$PWD":/data ghcr.io/thegraydot/mpqcli:latest create -o example.mpq files/
```

A shell function makes that the default for everyday use:

```bash
mpqcli() { docker run --rm --user "$(id -u):$(id -g)" -v "$PWD":/data ghcr.io/thegraydot/mpqcli:latest "$@"; }
```

Under rootless Docker or Podman the container's root already maps to your own user, so run without `--user` there; on Podman, `--userns=keep-id` is the equivalent of the flag above.
