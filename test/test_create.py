import subprocess
import shutil
from pathlib import Path


def test_create_mpq_target_does_not_exist(binary_path, generate_test_files):
    """
    Test MPQ file creation with a non-existent target.

    This test checks:
    - If the application exits correctly when the target does not exist.
    """
    _ = generate_test_files
    script_dir = Path(__file__).parent
    target_dir = script_dir / "does" / "not" / "exist"

    result = subprocess.run(
        [str(binary_path), "create", "--version", "1", str(target_dir)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    assert result.returncode == 105, f"mpqcli failed with error: {result.stderr}"


def test_create_mpq_versions(binary_path, generate_test_files):
    """
    Test MPQ archive creation with different MPQ versions.

    This test checks:
    - MPQ archive creation with version 1 and version 2.
    - If the MPQ archive is created in the correct directory.
    - If the MPQ archive is not empty.
    """
    _ = generate_test_files
    script_dir = Path(__file__).parent
    target_dir = script_dir / "data" / "files"

    for version in [1, 2, 3, 4]:
        target_file = target_dir.with_suffix(".mpq")
        # Remove the target file if it exists
        # Testing creation when file exists is handled:
        # test_create_mpq_already_exists
        target_file.unlink(missing_ok=True)
        result = subprocess.run(
            [str(binary_path), "create", "--version", str(version), str(target_dir)],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )

        assert result.returncode == 0, f"mpqcli failed with error (version {version}): {result.stderr}"
        assert target_file.exists(), f"MPQ file was not created (version {version})"
        assert target_file.stat().st_size > 0, f"MPQ file is empty (version {version})"

        # Verify the correct version was written to the archive
        version_result = subprocess.run(
            [str(binary_path), "info", "-p", "format-version", str(target_file)],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        assert version_result.returncode == 0
        assert version_result.stdout.strip() == str(version), \
            f"Expected version {version}, got {version_result.stdout.strip()}"


def test_create_mpq_with_output(binary_path, generate_test_files):
    """
    Test MPQ archive creation with output file argument.

    This test checks:
    - If the MPQ archive is created in the correct directory.
    - If the MPQ archive is not empty.
    """
    _ = generate_test_files
    script_dir = Path(__file__).parent
    target_dir = script_dir / "data" / "files"

    for version in [1, 2]:
        output_file = script_dir / "data" / f"mpq_with_output_v{version}.mpq"
        # Remove the target file if it exists
        # Testing creation when file exists is handled:
        # test_create_mpq_already_exists
        if output_file.exists():
            output_file.unlink()

        result = subprocess.run(
            [
                str(binary_path), "create",
                "--version", str(version),
                "--file-flags1", "4294967295",
                "--file-flags2", "4294967295",
                "--file-flags3", "0",
                "--attr-flags", "15",
                "--flags", "66048",
                "--compression", "2",
                "--compression-next", "4294967295",
                "-o", str(output_file),
                str(target_dir),
            ],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )

        assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
        assert output_file.exists(), "MPQ file was not created"
        assert output_file.stat().st_size > 0, "MPQ file is empty"


def test_create_mpq_with_weak_signature(binary_path, generate_test_files):
    """
    Test MPQ archive creation with signature and output file argument.

    This test checks:
    - If the MPQ archive is created in the correct directory.
    - If the MPQ archive is not empty.
    - If the MPQ archive is signed correctly.
    """
    _ = generate_test_files
    script_dir = Path(__file__).parent
    target_dir = script_dir / "data" / "files"

    output_file = script_dir / "data" / "mpq_with_weak_signature.mpq"
    # Remove the target file if it exists
    # Testing creation when file exists is handled in following function:
    # test_create_mpq_already_exists
    if output_file.exists():
        output_file.unlink()

    result = subprocess.run(
        [
            str(binary_path), "create", "-s",
            "--file-flags1", "4294967295",
            "--file-flags2", "4294967295",
            "--file-flags3", "0",
            "--attr-flags", "15",
            "--flags", "512",
            "--compression", "2",
            "--compression-next", "4294967295",
            "-o", str(output_file),
            str(target_dir),
        ],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    assert output_file.exists(), "MPQ file was not created"
    assert output_file.stat().st_size > 0, "MPQ file is empty"

    # Verify the signature is actually valid
    verify_result = subprocess.run(
        [str(binary_path), "verify", str(output_file)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    assert verify_result.returncode == 0, f"Signature verification failed: {verify_result.stderr}"
    assert "[*] Verify success" in verify_result.stdout, \
        f"Unexpected verify output: {verify_result.stdout}"


def test_create_mpq_already_exists(binary_path, generate_test_files):
    """
    Test MPQ file creation with an existing output file.

    This test checks:
    - If the MPQ archive is not created (as archive exists already).
    """
    _ = generate_test_files
    script_dir = Path(__file__).parent
    target_dir = script_dir / "data" / "files"

    output_file = script_dir / "data" / "mpq_with_output_v1.mpq"

    result = subprocess.run(
        [str(binary_path), "create", "--version", "1", "-o", str(output_file), str(target_dir)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    assert result.returncode == 1, f"mpqcli failed with error: {result.stderr}"


def test_create_mpq_from_file(binary_path, generate_test_files):
    """
    Test MPQ archive creation from a file rather than a directory.

    This test checks:
    - MPQ archive creation from a file.
    """
    _ = generate_test_files
    script_dir = Path(__file__).parent

    # Create a new test file on the fly
    test_file = script_dir / "data" / "test.txt"
    test_file.write_text("This is a test file for MPQ addition.")

    target_file = test_file.with_suffix(".mpq")
    # Remove the target file if it exists
    # Testing creation when file exists is handled:
    # test_create_mpq_already_exists
    target_file.unlink(missing_ok=True)
    result = subprocess.run(
        [str(binary_path), "create", str(test_file), "--output", str(target_file)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    assert target_file.exists(), "MPQ file was not created"
    assert target_file.stat().st_size > 0, "MPQ file is empty"

    verify_archive_file_content(binary_path, target_file, {"enUS  test.txt"})


def test_create_mpq_from_file_with_path_parameter(binary_path, generate_test_files):
    """
    Test MPQ archive creation from a file, with the --path parameter.
    This test checks:
    - MPQ archive creation from a file.
    - That the --path parameter is correctly handled.
    """
    _ = generate_test_files
    script_dir = Path(__file__).parent

    # Create a new test file on the fly
    test_file = script_dir / "data" / "test.txt"
    test_file.write_text("This is a test file for MPQ addition.")

    target_file = test_file.with_suffix(".mpq")
    # Remove the target file if it exists
    # Testing creation when file exists is handled:
    # test_create_mpq_already_exists
    target_file.unlink(missing_ok=True)
    result = subprocess.run(
        [str(binary_path), "create", str(test_file), "--output", str(target_file), "--path", "messages\\important.txt"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    assert target_file.exists(), "MPQ file was not created"
    assert target_file.stat().st_size > 0, "MPQ file is empty"

    verify_archive_file_content(binary_path, target_file, {"enUS  messages\\important.txt"})


def test_create_mpq_from_directory_with_path_parameter(binary_path, generate_test_files):
    """
    Test MPQ archive creation from a directory, with the --path parameter.
    This test checks:
    - No MPQ archive is created.
    """
    _ = generate_test_files
    script_dir = Path(__file__).parent
    target_dir = script_dir / "data" / "files2"
    # Create test directory with files
    target_dir.mkdir(exist_ok=True)

    # Create a new test file on the fly
    test_file = script_dir / "data" / "files2" / "important.txt"
    test_file.write_text("This is a test file for MPQ addition.")

    try:
        target_file = target_dir.with_name("files_test").with_suffix(".mpq")
        # Remove the target file if it exists
        # Testing creation when file exists is handled:
        # test_create_mpq_already_exists
        target_file.unlink(missing_ok=True)
        result = subprocess.run(
            [str(binary_path), "create", str(target_dir), "--output", str(target_file), "--path", "messages"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )

        assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
        assert target_file.exists(), "MPQ file was not created"
        assert target_file.stat().st_size > 0, "MPQ file is empty"

        verify_archive_file_content(binary_path, target_file, {"enUS  messages\\important.txt"})
    finally:
        # Clean up
        if target_dir.exists():
            shutil.rmtree(target_dir)


def test_create_mpq_with_illegal_locale(binary_path, generate_test_files):
    """
    Test MPQ file creation with illegal locale.

    This test checks:
    - That the MPQ archive is not created
    """
    _ = generate_test_files
    script_dir = Path(__file__).parent
    target_dir = script_dir / "data" / "files"

    output_file = script_dir / "data" / "new_mpq.mpq"

    result = subprocess.run(
        [str(binary_path), "create", "--version", "1", "-o", str(output_file), str(target_dir), "--locale", "illegal_locale"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    assert result.returncode == 105, f"mpqcli failed with error: {result.stderr}"
    assert not output_file.exists(), "MPQ file was created"


def test_create_mpq_with_locale(binary_path, generate_locales_mpq_test_files):
    """
    Test MPQ file creation with a given locale.

    This test checks:
    - That the MPQ archive contains the expected file with the expected locale.
    """

    # Creating an MPQ with a given locale happens in conftest.py. This test only checks the output.
    _ = generate_locales_mpq_test_files
    script_dir = Path(__file__).parent
    output_file = script_dir / "data" / "mpq_with_one_locale.mpq"

    verify_archive_file_content(binary_path, output_file, {"esES  cats.txt"})


def test_create_mpq_with_game_profile(binary_path, generate_test_files):
    """
    Test MPQ archive creation with different game profiles.

    This test checks:
    - If the MPQ archive is created with various game profiles.
    - If the game profile is accepted and applied.
    - If the correct MPQ version is used for each profile.
    - If the correct compression flags are applied.
    """
    _ = generate_test_files
    script_dir = Path(__file__).parent
    target_dir = script_dir / "data" / "files"

    # Test profiles with their expected MPQ versions and expected flags for .txt files
    test_cases = [
        ("generic", "1", "ce"),      # Generic: version 1, compressed + encrypted
        ("diablo1", "1", "ie"),      # Diablo1: version 1, imploded + encrypted
        ("starcraft1", "1", "ce2"),  # StarCraft: version 1, compressed + encrypted + key v2
        ("warcraft3", "1", "ce2"),   # Warcraft3: version 1, compressed + encrypted + key v2
        ("diablo2", "1", "c"),       # Diablo2: version 1, compressed (txt files not encrypted)
        ("wow1", "1", "c"),          # WoW 1.x: version 1, compressed
        ("wow2", "2", "cr"),         # WoW 2.x: version 2, compressed + sector CRC
        ("starcraft2", "2", "c"),    # StarCraft2: version 2, compressed (small files use single unit)
        ("diablo3", "4", "c"),       # Diablo3: version 4, compressed
    ]

    for profile, expected_version, expected_flags in test_cases:
        output_file = script_dir / "data" / f"mpq_with_profile_{profile}.mpq"
        output_file.unlink(missing_ok=True)

        result = subprocess.run(
            [str(binary_path), "create", "-g", profile, "-o", str(output_file), str(target_dir)],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )

        assert result.returncode == 0, f"mpqcli failed with error for profile {profile}: {result.stderr}"
        assert output_file.exists(), f"MPQ file was not created for profile {profile}"
        assert output_file.stat().st_size > 0, f"MPQ file is empty for profile {profile}"
        assert f"[*] Game profile: {profile}" in result.stdout, f"Game profile message not found for {profile}"

        # Verify the MPQ version
        version_result = subprocess.run(
            [str(binary_path), "info", "-p", "format-version", str(output_file)],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        assert version_result.returncode == 0, f"Failed to get version for {profile}"
        actual_version = version_result.stdout.strip()
        assert actual_version == expected_version, f"Profile {profile}: expected version {expected_version}, got {actual_version}"

        # Verify compression flags on a .txt file
        list_result = subprocess.run(
            [str(binary_path), "list", str(output_file), "-d", "-p", "flags"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        assert list_result.returncode == 0, f"Failed to list files for {profile}"

        # Check that at least one .txt file has the expected flags
        found_txt_file = False
        for line in list_result.stdout.splitlines():
            if "cats.txt" in line or "dogs.txt" in line:
                found_txt_file = True
                flags = line.split()[0]  # Extract flags
                # Check that expected flags are present in the actual flags
                for flag in expected_flags:
                    assert flag in flags, f"Profile {profile}: expected flag '{flag}' in '{flags}' for .txt file"

        assert found_txt_file, f"Profile {profile}: no .txt file found in archive"


def test_create_mpq_with_invalid_game_profile(binary_path, generate_test_files):
    """
    Test MPQ archive creation with an invalid game profile.

    This test checks:
    - If the application exits correctly when an invalid game profile is provided.
    """
    _ = generate_test_files
    script_dir = Path(__file__).parent
    target_dir = script_dir / "data" / "files"
    output_file = script_dir / "data" / "mpq_with_invalid_profile.mpq"

    result = subprocess.run(
        [str(binary_path), "create", "-g", "invalid_profile", "-o", str(output_file), str(target_dir)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    assert result.returncode != 0, "mpqcli should have failed with invalid game profile"
    assert not output_file.exists(), "MPQ file should not be created with invalid profile"


def test_create_mpq_with_all_game_profiles(binary_path, generate_test_files):
    """
    Test MPQ archive creation with all available game profiles.

    This test checks:
    - If all game profiles are valid.
    - If the MPQ archive is created successfully for each profile.
    - If the correct MPQ version is used for each profile.
    """
    _ = generate_test_files
    script_dir = Path(__file__).parent
    target_dir = script_dir / "data" / "files"

    # All profiles with their expected MPQ versions
    profile_versions = {
        "generic": "1",
        "diablo1": "1",
        "lordsofmagic": "1",
        "starcraft1": "1",
        "warcraft2": "1",
        "diablo2": "1",
        "warcraft3": "1",
        "warcraft3-map": "1",
        "wow1": "1",
        "wow2": "2",
        "wow3": "2",
        "wow4": "4",
        "wow5": "4",
        "starcraft2": "2",
        "diablo3": "4"
    }

    for profile, expected_version in profile_versions.items():
        output_file = script_dir / "data" / f"mpq_all_profiles_{profile}.mpq"
        output_file.unlink(missing_ok=True)

        result = subprocess.run(
            [str(binary_path), "create", "-g", profile, "-o", str(output_file), str(target_dir)],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )

        assert result.returncode == 0, f"mpqcli failed for profile {profile}: {result.stderr}"
        assert output_file.exists(), f"MPQ file was not created for profile {profile}"
        assert f"[*] Game profile: {profile}" in result.stdout, f"Game profile message not found for {profile}"

        # Verify the MPQ version
        version_result = subprocess.run(
            [str(binary_path), "info", "-p", "format-version", str(output_file)],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        assert version_result.returncode == 0, f"Failed to get version for {profile}"
        actual_version = version_result.stdout.strip()
        assert actual_version == expected_version, f"Profile {profile}: expected version {expected_version}, got {actual_version}"


def test_deletion_marker_only_for_zero_size_files(binary_path):
    """
    Test that deletion marker is only applied to files with size 0, not all small files.
    """
    script_dir = Path(__file__).parent
    data_dir = script_dir / "data"
    test_dir = data_dir / "deletion_marker_test"
    output_file = data_dir / "test_deletion_marker.mpq"

    # Clean up from previous runs
    output_file.unlink(missing_ok=True)
    if test_dir.exists():
        shutil.rmtree(test_dir)

    # Create test directory with files
    test_dir.mkdir(exist_ok=True)

    # Create test files: one with size 0, one with non-zero size
    zero_size_file = test_dir / "zero_size.txt"
    nonzero_size_file = test_dir / "nonzero_size.txt"

    # Create a truly empty file (0 bytes)
    zero_size_file.touch()
    nonzero_size_file.write_text("Hello")  # 5 bytes

    # Verify file sizes
    assert zero_size_file.stat().st_size == 0, f"zero_size.txt should be 0 bytes, got {zero_size_file.stat().st_size}"
    assert nonzero_size_file.stat().st_size == 5, f"nonzero_size.txt should be 5 bytes, got {nonzero_size_file.stat().st_size}"

    try:
        # Create MPQ with StarCraft2 profile (which has deletion marker rule for size 0)
        result = subprocess.run(
            [str(binary_path), "create", "-g", "starcraft2", "-o", str(output_file), str(test_dir)],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )

        assert result.returncode == 0, f"Failed to create MPQ: {result.stderr}\nStdout: {result.stdout}"

        # Check flags for both files
        list_result = subprocess.run(
            [str(binary_path), "list", str(output_file), "-d", "-p", "flags"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )

        assert list_result.returncode == 0, f"Failed to list files: {list_result.stderr}"

        # Parse the output
        zero_size_flags = None
        nonzero_size_flags = None

        for line in list_result.stdout.splitlines():
            parts = line.split()
            if len(parts) >= 2:
                flags = parts[0]
                filename = parts[1]

                if filename == "zero_size.txt":
                    zero_size_flags = flags
                elif filename == "nonzero_size.txt":
                    nonzero_size_flags = flags

        # Verify that zero-size file has deletion marker
        assert zero_size_flags is not None, "zero_size.txt not found in archive"
        assert 'd' in zero_size_flags, f"Expected deletion marker 'd' in flags for zero_size.txt, got: {zero_size_flags}"

        # Verify that non-zero size file does NOT have deletion marker but has compression
        assert nonzero_size_flags is not None, "nonzero_size.txt not found in archive"
        assert 'd' not in nonzero_size_flags, f"Unexpected deletion marker 'd' in flags for nonzero_size.txt, got: {nonzero_size_flags}"
        assert 'c' in nonzero_size_flags, f"Expected compression 'c' in flags for nonzero_size.txt, got: {nonzero_size_flags}"

    finally:
        # Clean up
        if test_dir.exists():
            shutil.rmtree(test_dir)
        output_file.unlink(missing_ok=True)


def test_create_mpq_no_sign_flag_has_no_signature(binary_path, generate_test_files):
    """
    Test that a newly created archive without --sign does not have a weak signature slot.

    This test checks:
    - If the MPQ archive is created successfully.
    - If the signature type is None (no signature slot pre-allocated).
    """
    _ = generate_test_files
    script_dir = Path(__file__).parent
    target_dir = script_dir / "data" / "files"

    output_file = script_dir / "data" / "mpq_no_sign.mpq"
    output_file.unlink(missing_ok=True)

    result = subprocess.run(
        [str(binary_path), "create", str(target_dir), "-o", str(output_file)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    assert output_file.exists(), "MPQ file was not created"

    info = subprocess.run(
        [str(binary_path), "info", "-p", "signature-type", str(output_file)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    assert info.returncode == 0, f"mpqcli info failed with error: {info.stderr}"
    assert info.stdout.strip() == "None", f"Expected signature type 'None', got: {info.stdout.strip()!r}"

    output_file.unlink(missing_ok=True)


def test_create_mpq_directory_with_trailing_slash(binary_path, generate_test_files):
    """
    Test MPQ archive creation when the target directory has a trailing slash.

    Regression test for: mpqcli create dir/ producing "dir/.mpq" instead of "dir.mpq".

    This test checks:
    - The archive is named after the directory, not "<dir>/.mpq".
    - The archive is created successfully and is non-empty.
    """
    _ = generate_test_files
    script_dir = Path(__file__).parent
    # Construct the path string with an explicit trailing slash
    target_dir_str = str(script_dir / "data" / "files") + "/"

    expected_output = script_dir / "data" / "files.mpq"
    malformed_output = script_dir / "data" / "files" / ".mpq"

    # Clean up from any previous run
    expected_output.unlink(missing_ok=True)
    malformed_output.unlink(missing_ok=True)

    result = subprocess.run(
        [str(binary_path), "create", target_dir_str],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    assert expected_output.exists(), (
        f"Expected archive '{expected_output}' was not created. "
        f"Malformed path exists: {malformed_output.exists()}"
    )
    assert expected_output.stat().st_size > 0, "MPQ file is empty"
    assert not malformed_output.exists(), (
        f"Malformed archive path '{malformed_output}' was created, trailing slash bug is present"
    )

    expected_output.unlink(missing_ok=True)


def test_create_mpq_skips_special_files(binary_path, tmp_path):
    """
    Test that special MPQ files in the source directory are skipped during archive creation.

    This test checks:
    - That (listfile), (signature), and (attributes) are not added as regular files.
    - That the signature type is None after creation (no false signature from (signature) file).
    - That the special files do not appear in the archive file listing.
    """
    source_dir = tmp_path / "src"
    source_dir.mkdir()
    (source_dir / "readme.txt").write_text("hello")

    # Plant all three special files in the source directory
    for name in ["(listfile)", "(signature)", "(attributes)"]:
        (source_dir / name).write_bytes(b"fake content")

    output_file = tmp_path / "output.mpq"

    result = subprocess.run(
        [str(binary_path), "create", str(source_dir), "-o", str(output_file)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    assert output_file.exists(), "MPQ file was not created"

    # Signature type must be None (signature) must not have been ingested
    info = subprocess.run(
        [str(binary_path), "info", "-p", "signature-type", str(output_file)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    assert info.returncode == 0, f"mpqcli info failed with error: {info.stderr}"
    assert info.stdout.strip() == "None", f"Expected signature type 'None', got: {info.stdout.strip()!r}"

    # Special files must not appear in the regular file listing
    listing = subprocess.run(
        [str(binary_path), "list", str(output_file)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    assert listing.returncode == 0, f"mpqcli list failed with error: {listing.stderr}"
    for name in ["(listfile)", "(signature)", "(attributes)"]:
        assert name not in listing.stdout, f"Special file {name!r} unexpectedly found in archive listing"


def verify_archive_file_content(binary_path, test_file, expected_output):
    result = subprocess.run(
        [str(binary_path), "list", str(test_file), "-d", "-p", "locale"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    output_lines = set(result.stdout.splitlines())

    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    assert output_lines == expected_output, f"Unexpected output: {output_lines}"


def test_create_mpq_from_empty_directory(binary_path, tmp_path):
    empty_dir = tmp_path / "empty"
    empty_dir.mkdir()
    output_file = tmp_path / "empty.mpq"

    result = subprocess.run(
        [str(binary_path), "create", str(empty_dir), "-o", str(output_file)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    assert output_file.exists(), "MPQ file was not created"
    assert output_file.stat().st_size > 0, "MPQ file is empty"

    # No user files should be listed
    list_result = subprocess.run(
        [str(binary_path), "list", str(output_file)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    assert list_result.returncode == 0
    assert list_result.stdout.strip() == "", \
        f"Expected no files listed, got: {list_result.stdout}"

    # max-files should be 64 (minimum value returned by CalculateMpqMaxFileValue for 0 files)
    info_result = subprocess.run(
        [str(binary_path), "info", "-p", "max-files", str(output_file)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    assert info_result.returncode == 0
    assert info_result.stdout.strip() == "64", \
        f"Expected max-files of 64, got: {info_result.stdout.strip()}"
