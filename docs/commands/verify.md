# verify

Verify an MPQ archive signature.

## Verify an MPQ archive

Check the digital signature of an MPQ archive by verifying the signature in the archive with a collection of known Blizzard public keys (bundled in StormLib library). The tool will print if verification succeeds or fails, as well as return `0` for success and any other value for failure.

```bash
$ mpqcli verify wow-patch.mpq
[*] Verify success
```

If verification passes, a zero (`0`) exit status will be returned. This can be helpful to verify a large number of MPQ archives without the need to review the status message that is printed out.

```bash
$ echo $?
0
```

## Verify an MPQ archive and print the digital signature

Check the digital signature of an MPQ archive, and also write the signature's raw bytes to stdout, using the `-x` or `--print` argument. The verification result still goes to stderr, so the bytes can be redirected to a file on their own:

```bash
$ mpqcli verify --print wow-patch.mpq > signature
```

To view the signature as hexadecimal, pipe it through a tool such as `xxd`:

```bash
$ mpqcli verify --print wow-patch.mpq | xxd
```
