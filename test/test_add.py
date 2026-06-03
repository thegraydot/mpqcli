import subprocess
import shutil
from pathlib import Path


def test_add_target_mpq_does_not_exist(binary_path, generate_test_files):
    _ = generate_test_files
    script_dir = Path(__file__).parent
    target_mpq = script_dir / "does" / "not" / "exist.mpq"
    target_file = script_dir / "data" / "files" / "cats.txt"

    result = subprocess.run(
        [str(binary_path), "add", str(target_mpq), str(target_file)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    assert result.returncode == 105, f"mpqcli failed with error: {result.stderr}"


def test_add_target_file_does_not_exist(binary_path, generate_test_files):
    _ = generate_test_files
    script_dir = Path(__file__).parent
    target_mpq = script_dir / "data" / "files.mpq"

    create_mpq_archive_for_test(binary_path, script_dir)

    result = subprocess.run(
        [str(binary_path), "add", str(target_mpq), str(script_dir / "does" / "not" / "exist.txt")],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    assert "[!] Path does not exist:" in result.stderr


def test_add_file_to_mpq_archive(binary_path, generate_test_files):
    _ = generate_test_files
    script_dir = Path(__file__).parent
    target_file = script_dir / "data" / "files.mpq"

    create_mpq_archive_for_test(binary_path, script_dir)

    test_file = script_dir / "data" / "test.txt"
    test_file.write_text("This is a test file for MPQ addition.")

    result = subprocess.run(
        [str(binary_path), "add", str(target_file), str(test_file)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    output_lines = set(result.stdout.splitlines())
    expected_stdout_output = {
        "[+] Adding file: test.txt",
    }
    assert output_lines == expected_stdout_output, f"Unexpected output: {output_lines}"

    output_lines = set(result.stderr.splitlines())
    expected_stderr_output = set()
    assert output_lines == expected_stderr_output, f"Unexpected output: {output_lines}"

    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"

    expected_content = {
        "enUS  bytes",
        "enUS  dogs.txt",
        "enUS  cats.txt",
        "enUS  test.txt",
    }
    verify_archive_file_content(binary_path, target_file, expected_content)


def test_add_multiple_files_to_mpq_archive(binary_path, generate_test_files):
    _ = generate_test_files
    script_dir = Path(__file__).parent
    target_file = script_dir / "data" / "files.mpq"

    create_mpq_archive_for_test(binary_path, script_dir)

    test_file_a = script_dir / "data" / "alpha.txt"
    test_file_b = script_dir / "data" / "beta.txt"
    test_file_a.write_text("Alpha file content.")
    test_file_b.write_text("Beta file content.")

    result = subprocess.run(
        [str(binary_path), "add", str(target_file), str(test_file_a), str(test_file_b)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    assert "[+] Adding file: alpha.txt" in result.stdout
    assert "[+] Adding file: beta.txt" in result.stdout

    expected_content = {
        "enUS  bytes",
        "enUS  dogs.txt",
        "enUS  cats.txt",
        "enUS  alpha.txt",
        "enUS  beta.txt",
    }
    verify_archive_file_content(binary_path, target_file, expected_content)


def test_add_file_with_filenameinarchive_and_directoryinarchive_and_path_to_mpq_archive(binary_path, generate_test_files):
    _ = generate_test_files
    script_dir = Path(__file__).parent
    target_file = script_dir / "data" / "files.mpq"

    create_mpq_archive_for_test(binary_path, script_dir)

    test_file0 = script_dir / "data" / "test0.txt"
    test_file0.write_text("This is a test file for MPQ addition.")
    test_file1 = script_dir / "data" / "test1.txt"
    test_file1.write_text("This is another test file for MPQ addition.")
    test_file2 = script_dir / "data" / "test2.txt"
    test_file2.write_text("This is yet another test file for MPQ addition.")
    test_file3 = script_dir / "data" / "test3.txt"
    test_file3.write_text("This is yet yet another test file for MPQ addition.")

    bin_dir = binary_path.parent
    test_file1_copy = bin_dir / "test1.txt"
    shutil.copy(test_file1, test_file1_copy)

    try:
        result = subprocess.run(
            [str(binary_path), "add", str(target_file), str(test_file0), "--directory-in-archive", "directory", "--path", "important.txt"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        assert result.returncode == 1, f"mpqcli failed with error: {result.stderr}"

        result = subprocess.run(
            [str(binary_path), "add", str(target_file), str(test_file0), "--filename-in-archive", "important.txt", "--path", "texts/important.txt"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        assert result.returncode == 1, f"mpqcli failed with error: {result.stderr}"

        result = subprocess.run(
            [str(binary_path), "add", str(target_file), str(test_file0), "--directory-in-archive", "directory"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"

        result = subprocess.run(
            [str(binary_path), "add", str(target_file), test_file1_copy.name, "--filename-in-archive", "msg.txt"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            cwd=str(bin_dir)
        )
        assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"

        result = subprocess.run(
            [str(binary_path), "add", str(target_file), str(test_file2), "--directory-in-archive", "texts", "--filename-in-archive", "info.txt"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"

        result = subprocess.run(
            [str(binary_path), "add", str(target_file), str(test_file3), "--path", "important\\message.txt"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"

        result = subprocess.run(
            [str(binary_path), "list", str(target_file)],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )

        output_lines = set(result.stdout.splitlines())

        expected_output = {
            "cats.txt",
            "dogs.txt",
            "bytes",
            "directory\\test0.txt",
            "msg.txt",
            "texts\\info.txt",
            "important\\message.txt",
        }
        assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
        assert output_lines == expected_output, f"Unexpected output: {output_lines}"
    finally:
        test_file1_copy.unlink(missing_ok=True)


def test_add_existing_file_without_overwrite_should_fail(binary_path, generate_test_files):
    _ = generate_test_files
    script_dir = Path(__file__).parent
    target_file = script_dir / "data" / "files.mpq"

    create_mpq_archive_for_test(binary_path, script_dir)

    expected_content = {"This is a file about cats."}
    verify_file_in_mpq_has_content(binary_path, target_file, "cats.txt", expected_content)

    test_file = script_dir / "data" / "cats.txt"
    test_file.write_text("Attempting to make this file about dogs.")

    result = subprocess.run(
        [str(binary_path), "add", str(target_file), str(test_file)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    output_lines = set(result.stdout.splitlines())
    assert output_lines == set(), f"Unexpected output: {output_lines}"

    output_lines = set(result.stderr.splitlines())
    expected_stderr_output = {
        "[!] File already exists in MPQ archive: cats.txt - Skipping...",
    }
    assert output_lines == expected_stderr_output, f"Unexpected output: {output_lines}"

    assert result.returncode == 1, f"mpqcli failed with error: {result.stderr}"

    verify_file_in_mpq_has_content(binary_path, target_file, "cats.txt", expected_content)


def test_add_existing_file_with_overwrite_should_succeed(binary_path, generate_test_files):
    _ = generate_test_files
    script_dir = Path(__file__).parent
    target_file = script_dir / "data" / "files.mpq"

    create_mpq_archive_for_test(binary_path, script_dir)

    verify_file_in_mpq_has_content(binary_path, target_file, "cats.txt", { "This is a file about cats." })

    test_file = script_dir / "data" / "cats.txt"
    test_file.write_text("This file is suddenly about dogs.")

    result = subprocess.run(
        [str(binary_path), "add", str(target_file), str(test_file), "--overwrite"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    output_lines = set(result.stdout.splitlines())
    expected_stdout_output = {
        "[+] File already exists in MPQ archive: cats.txt - Overwriting...",
        "[+] Adding file: cats.txt",
    }
    assert output_lines == expected_stdout_output, f"Unexpected output: {output_lines}"

    output_lines = set(result.stderr.splitlines())
    expected_stderr_output = set()
    assert output_lines == expected_stderr_output, f"Unexpected output: {output_lines}"

    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"

    verify_file_in_mpq_has_content(binary_path, target_file, "cats.txt", { "This file is suddenly about dogs." })


def test_add_nonexisting_file_with_overwrite_should_succeed(binary_path, generate_test_files):
    _ = generate_test_files
    script_dir = Path(__file__).parent
    target_file = script_dir / "data" / "files.mpq"

    create_mpq_archive_for_test(binary_path, script_dir)

    test_file = script_dir / "data" / "test.txt"
    test_file.write_text("This file is newly added.")

    result = subprocess.run(
        [str(binary_path), "add", str(target_file), str(test_file), "--overwrite"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    output_lines = set(result.stdout.splitlines())
    expected_stdout_output = {
        "[+] Adding file: test.txt",
    }
    assert output_lines == expected_stdout_output, f"Unexpected output: {output_lines}"

    output_lines = set(result.stderr.splitlines())
    expected_stderr_output = set()
    assert output_lines == expected_stderr_output, f"Unexpected output: {output_lines}"

    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"

    verify_file_in_mpq_has_content(binary_path, target_file, "test.txt", { "This file is newly added." })


def test_create_mpq_with_illegal_locale(binary_path, generate_test_files):
    _ = generate_test_files
    script_dir = Path(__file__).parent
    target_file = script_dir / "data" / "files.mpq"

    create_mpq_archive_for_test(binary_path, script_dir)

    expected_content = {
        "enUS  bytes",
        "enUS  dogs.txt",
        "enUS  cats.txt",
    }
    verify_archive_file_content(binary_path, target_file, expected_content)

    test_file = script_dir / "data" / "horses.txt"
    test_file.write_text("This is a file about horses.")

    result = subprocess.run(
        [str(binary_path), "add", str(target_file), str(test_file), "--locale", "illegal_locale"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    assert result.returncode == 105, f"mpqcli failed with error: {result.stderr}"

    expected_content = {
        "enUS  bytes",
        "enUS  dogs.txt",
        "enUS  cats.txt",
    }
    verify_archive_file_content(binary_path, target_file, expected_content)


def test_create_mpq_with_locale(binary_path, generate_test_files):
    _ = generate_test_files
    script_dir = Path(__file__).parent
    target_file = script_dir / "data" / "files.mpq"

    create_mpq_archive_for_test(binary_path, script_dir)

    test_file = script_dir / "data" / "cats.txt"
    test_file.write_text("Este es un archivo sobre gatos.")

    result = subprocess.run(
        [str(binary_path), "add", str(target_file), str(test_file), "--locale", "esES"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    output_lines = set(result.stdout.splitlines())
    expected_stdout_output = {
        "[+] Adding file for locale esES: cats.txt",
    }
    assert output_lines == expected_stdout_output, f"Unexpected output: {output_lines}"

    output_lines = set(result.stderr.splitlines())
    expected_stderr_output = set()
    assert output_lines == expected_stderr_output, f"Unexpected output: {output_lines}"

    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"

    expected_content = {
        "enUS  bytes",
        "enUS  dogs.txt",
        "enUS  cats.txt",
        "esES  cats.txt",
    }
    verify_archive_file_content(binary_path, target_file, expected_content)


def test_add_file_with_game_profile(binary_path, generate_test_files):
    _ = generate_test_files
    script_dir = Path(__file__).parent
    target_file = script_dir / "data" / "files.mpq"

    test_cases = [
        ("generic", "ce"),
        ("diablo1", "ie"),
        ("starcraft1", "ce2"),
        ("wow1", "c"),
        ("wow2", "cr"),
        ("starcraft2", "c"),
        ("diablo3", "c"),
    ]

    for profile, expected_flags in test_cases:
        create_mpq_archive_for_test(binary_path, script_dir)

        test_file = script_dir / "data" / f"test_{profile}.txt"
        test_file.write_text(f"Test file for {profile} profile.")

        result = subprocess.run(
            [str(binary_path), "add", str(target_file), str(test_file), "--game", profile],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )

        assert result.returncode == 0, f"mpqcli failed for profile {profile}: {result.stderr}"
        assert f"Using game profile: {profile}" in result.stdout, f"Game profile message not found for {profile}"
        assert f"Adding file: test_{profile}.txt" in result.stdout

        list_result = subprocess.run(
            [str(binary_path), "list", str(target_file), "-d", "-p", "flags"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        assert list_result.returncode == 0, f"Failed to list files for {profile}"

        found_test_file = False
        for line in list_result.stdout.splitlines():
            if f"test_{profile}.txt" in line:
                found_test_file = True
                flags = line.split()[0]
                for flag in expected_flags:
                    assert flag in flags, f"Profile {profile}: expected flag '{flag}' in '{flags}' for added file"

        assert found_test_file, f"Profile {profile}: added file not found in archive"


def test_add_file_with_invalid_game_profile(binary_path, generate_test_files):
    _ = generate_test_files
    script_dir = Path(__file__).parent
    target_file = script_dir / "data" / "files.mpq"

    create_mpq_archive_for_test(binary_path, script_dir)

    test_file = script_dir / "data" / "test_invalid.txt"
    test_file.write_text("Test file for invalid profile.")

    result = subprocess.run(
        [str(binary_path), "add", str(target_file), str(test_file), "-g", "invalid_profile"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    assert result.returncode != 0, "mpqcli should have failed with invalid game profile"


def test_add_file_with_all_game_profiles(binary_path, generate_test_files):
    _ = generate_test_files
    script_dir = Path(__file__).parent
    target_file = script_dir / "data" / "files.mpq"

    all_profiles = [
        "generic", "diablo1", "lordsofmagic", "starcraft1", "warcraft2", "diablo2",
        "warcraft3", "warcraft3-map", "wow1", "wow2", "wow3", "wow4", "wow5",
        "starcraft2", "diablo3"
    ]

    for profile in all_profiles:
        create_mpq_archive_for_test(binary_path, script_dir)

        test_file = script_dir / "data" / f"test_all_{profile}.txt"
        test_file.write_text(f"Test file for {profile} profile.")

        result = subprocess.run(
            [str(binary_path), "add", str(target_file), str(test_file), "-g", profile],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )

        assert result.returncode == 0, f"mpqcli failed for profile {profile}: {result.stderr}"
        assert f"Using game profile: {profile}" in result.stdout, f"Game profile message not found for {profile}"

        list_result = subprocess.run(
            [str(binary_path), "list", str(target_file)],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        assert list_result.returncode == 0, f"Failed to list files for {profile}"
        assert f"test_all_{profile}.txt" in list_result.stdout, f"Profile {profile}: added file not found in archive"

        flags_result = subprocess.run(
            [str(binary_path), "list", str(target_file), "-d", "-p", "flags"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        assert flags_result.returncode == 0, f"Failed to get flags for {profile}"

        found_with_compression = False
        for line in flags_result.stdout.splitlines():
            if f"test_all_{profile}.txt" in line:
                flags = line.split()[0]
                if 'c' in flags or 'i' in flags:
                    found_with_compression = True
                break

        assert found_with_compression, f"Profile {profile}: no compression flag found on added file"


def test_add_existing_locale_file_with_overwrite_should_succeed(binary_path, generate_test_files):
    _ = generate_test_files
    script_dir = Path(__file__).parent
    target_file = script_dir / "data" / "files.mpq"

    create_mpq_archive_for_test(binary_path, script_dir)

    test_file = script_dir / "data" / "cats.txt"
    test_file.write_text("Este es un archivo sobre gatos.")

    result = subprocess.run(
        [str(binary_path), "add", str(target_file), str(test_file), "--locale", "esES"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    assert result.returncode == 0, f"Initial locale add failed: {result.stderr}"

    test_file.write_text("Este archivo ahora es sobre perros.")

    result = subprocess.run(
        [str(binary_path), "add", str(target_file), str(test_file),
         "--locale", "esES", "--overwrite"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    expected_stdout = {
        "[+] File for locale esES already exists in MPQ archive: cats.txt - Overwriting...",
        "[+] Adding file for locale esES: cats.txt",
    }
    output_lines = set(result.stdout.splitlines())

    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    assert output_lines == expected_stdout, f"Unexpected output: {output_lines}"

    verify_file_in_mpq_has_content(
        binary_path, target_file, "cats.txt",
        {"Este archivo ahora es sobre perros."},
        locale="esES"
    )

    verify_file_in_mpq_has_content(
        binary_path, target_file, "cats.txt",
        {"This is a file about cats."}
    )


def test_add_file_after_all_locale_variants_removed(binary_path, generate_locales_mpq_test_files):
    _ = generate_locales_mpq_test_files
    script_dir = Path(__file__).parent
    target_file = script_dir / "data" / "mpq_with_many_locales.mpq"

    for locale in ["", "deDE", "esES", "041D"]:
        cmd = [str(binary_path), "remove", str(target_file), "cats.txt"]
        if locale:
            cmd += ["--locale", locale]
        result = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        assert result.returncode == 0, f"Failed to remove locale '{locale}': {result.stderr}"

    list_result = subprocess.run(
        [str(binary_path), "list", str(target_file)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    assert "cats.txt" not in list_result.stdout, \
        "cats.txt still present after removing all locales"

    test_file = script_dir / "data" / "locale_files" / "cats.txt"
    test_file.write_text("This is a file about cats.")

    result = subprocess.run(
        [str(binary_path), "add", str(target_file), str(test_file)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    expected_stdout = {"[+] Adding file: cats.txt"}
    output_lines = set(result.stdout.splitlines())

    assert result.returncode == 0, f"Re-add after full removal failed: {result.stderr}"
    assert output_lines == expected_stdout, f"Unexpected output: {output_lines}"

    verify_file_in_mpq_has_content(
        binary_path, target_file, "cats.txt",
        {"This is a file about cats."}
    )


# ---- Directory add tests ----

def test_add_directory_to_mpq_archive(binary_path, generate_test_files):
    _ = generate_test_files
    script_dir = Path(__file__).parent
    target_mpq = script_dir / "data" / "files.mpq"
    add_dir = script_dir / "data" / "add_dir"

    create_mpq_archive_for_test(binary_path, script_dir)

    sub_dir = add_dir / "sub"
    sub_dir.mkdir(parents=True, exist_ok=True)
    (sub_dir / "a.txt").write_text("Sub file A.")
    (add_dir / "b.txt").write_text("Root file B.")

    try:
        result = subprocess.run(
            [str(binary_path), "add", str(target_mpq), str(add_dir)],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )

        assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
        assert "[+] Adding file: sub\\a.txt" in result.stdout
        assert "[+] Adding file: b.txt" in result.stdout

        list_result = subprocess.run(
            [str(binary_path), "list", str(target_mpq)],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        assert "sub\\a.txt" in list_result.stdout
        assert "b.txt" in list_result.stdout
    finally:
        shutil.rmtree(add_dir, ignore_errors=True)


def test_add_directory_with_path_prefix(binary_path, generate_test_files):
    _ = generate_test_files
    script_dir = Path(__file__).parent
    target_mpq = script_dir / "data" / "files.mpq"
    add_dir = script_dir / "data" / "add_dir_prefix"

    create_mpq_archive_for_test(binary_path, script_dir)

    sub_dir = add_dir / "sub"
    sub_dir.mkdir(parents=True, exist_ok=True)
    (sub_dir / "a.txt").write_text("Sub file A.")
    (add_dir / "b.txt").write_text("Root file B.")

    try:
        result = subprocess.run(
            [str(binary_path), "add", str(target_mpq), str(add_dir), "--path", "textures"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )

        assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"

        list_result = subprocess.run(
            [str(binary_path), "list", str(target_mpq)],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        output_lines = set(list_result.stdout.splitlines())
        assert "textures\\sub\\a.txt" in output_lines
        assert "textures\\b.txt" in output_lines
        assert "sub\\a.txt" not in output_lines
    finally:
        shutil.rmtree(add_dir, ignore_errors=True)


def test_add_directory_with_overwrite(binary_path, generate_test_files):
    _ = generate_test_files
    script_dir = Path(__file__).parent
    target_mpq = script_dir / "data" / "files.mpq"
    add_dir = script_dir / "data" / "add_dir_overwrite"

    create_mpq_archive_for_test(binary_path, script_dir)

    verify_file_in_mpq_has_content(binary_path, target_mpq, "cats.txt", {"This is a file about cats."})

    add_dir.mkdir(parents=True, exist_ok=True)
    (add_dir / "cats.txt").write_text("This is the replaced cats file.")

    try:
        result = subprocess.run(
            [str(binary_path), "add", str(target_mpq), str(add_dir), "--overwrite"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )

        assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
        assert "[+] File already exists in MPQ archive: cats.txt - Overwriting..." in result.stdout

        verify_file_in_mpq_has_content(binary_path, target_mpq, "cats.txt", {"This is the replaced cats file."})
    finally:
        shutil.rmtree(add_dir, ignore_errors=True)


def test_add_directory_without_overwrite_skips_existing(binary_path, generate_test_files):
    _ = generate_test_files
    script_dir = Path(__file__).parent
    target_mpq = script_dir / "data" / "files.mpq"
    add_dir = script_dir / "data" / "add_dir_conflict"

    create_mpq_archive_for_test(binary_path, script_dir)

    original_content = {"This is a file about cats."}
    verify_file_in_mpq_has_content(binary_path, target_mpq, "cats.txt", original_content)

    add_dir.mkdir(parents=True, exist_ok=True)
    (add_dir / "cats.txt").write_text("Conflicting cats content.")

    try:
        result = subprocess.run(
            [str(binary_path), "add", str(target_mpq), str(add_dir)],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )

        assert result.returncode == 1, f"mpqcli failed with error: {result.stderr}"
        assert "[!] File already exists in MPQ archive: cats.txt - Skipping..." in result.stderr

        verify_file_in_mpq_has_content(binary_path, target_mpq, "cats.txt", original_content)
    finally:
        shutil.rmtree(add_dir, ignore_errors=True)


def test_add_dir_with_directory_in_archive_flag_errors(binary_path, generate_test_files):
    _ = generate_test_files
    script_dir = Path(__file__).parent
    target_mpq = script_dir / "data" / "files.mpq"
    add_dir = script_dir / "data" / "add_dir_flag_error"

    create_mpq_archive_for_test(binary_path, script_dir)
    add_dir.mkdir(parents=True, exist_ok=True)
    (add_dir / "x.txt").write_text("x")

    try:
        result = subprocess.run(
            [str(binary_path), "add", str(target_mpq), str(add_dir), "--directory-in-archive", "subdir"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        assert result.returncode == 1, f"Expected failure but got: {result.returncode}"
        assert "--directory-in-archive" in result.stderr

        result = subprocess.run(
            [str(binary_path), "add", str(target_mpq), str(add_dir), "--filename-in-archive", "name.txt"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        assert result.returncode == 1, f"Expected failure but got: {result.returncode}"
        assert "--filename-in-archive" in result.stderr
    finally:
        shutil.rmtree(add_dir, ignore_errors=True)


# ---- --update flag tests ----

def test_add_update_skips_unchanged_files(binary_path, generate_test_files):
    _ = generate_test_files
    script_dir = Path(__file__).parent
    target_mpq = script_dir / "data" / "files.mpq"
    update_dir = script_dir / "data" / "update_dir_unchanged"

    create_mpq_archive_for_test(binary_path, script_dir)

    update_dir.mkdir(parents=True, exist_ok=True)
    (update_dir / "cats.txt").write_text("This is a file about cats.\n")
    (update_dir / "dogs.txt").write_text("This is a file about dogs.\n")

    try:
        result = subprocess.run(
            [str(binary_path), "add", str(target_mpq), str(update_dir), "--update", "--overwrite"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )

        assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
        assert "[~] Skipping unchanged file: cats.txt" in result.stdout
        assert "[~] Skipping unchanged file: dogs.txt" in result.stdout
        assert "files added" in result.stdout
        assert "files skipped" in result.stdout
    finally:
        shutil.rmtree(update_dir, ignore_errors=True)


def test_add_update_adds_changed_files(binary_path, generate_test_files):
    _ = generate_test_files
    script_dir = Path(__file__).parent
    target_mpq = script_dir / "data" / "files.mpq"
    update_dir = script_dir / "data" / "update_dir_changed"

    create_mpq_archive_for_test(binary_path, script_dir)

    update_dir.mkdir(parents=True, exist_ok=True)
    (update_dir / "cats.txt").write_text("This cat content is completely different and longer now.")
    (update_dir / "dogs.txt").write_text("This is a file about dogs.\n")

    try:
        result = subprocess.run(
            [str(binary_path), "add", str(target_mpq), str(update_dir), "--update", "--overwrite"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )

        assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
        assert "[+] Adding file: cats.txt" in result.stdout
        assert "[~] Skipping unchanged file: dogs.txt" in result.stdout
        assert "1 files added" in result.stdout
        assert "1 files skipped" in result.stdout

        verify_file_in_mpq_has_content(
            binary_path, target_mpq, "cats.txt",
            {"This cat content is completely different and longer now."}
        )
    finally:
        shutil.rmtree(update_dir, ignore_errors=True)


def test_add_update_second_run_skips_all(binary_path, generate_test_files):
    _ = generate_test_files
    script_dir = Path(__file__).parent
    target_mpq = script_dir / "data" / "files.mpq"
    update_dir = script_dir / "data" / "update_dir_idempotent"

    create_mpq_archive_for_test(binary_path, script_dir)

    update_dir.mkdir(parents=True, exist_ok=True)
    (update_dir / "cats.txt").write_text("This is a file about cats.\n")

    try:
        subprocess.run(
            [str(binary_path), "add", str(target_mpq), str(update_dir), "--update", "--overwrite"],
            stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True
        )

        result = subprocess.run(
            [str(binary_path), "add", str(target_mpq), str(update_dir), "--update", "--overwrite"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )

        assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
        assert "[+] Adding file:" not in result.stdout
        assert "0 files added" in result.stdout
    finally:
        shutil.rmtree(update_dir, ignore_errors=True)


def test_add_update_single_file_emits_warning(binary_path, generate_test_files):
    _ = generate_test_files
    script_dir = Path(__file__).parent
    target_mpq = script_dir / "data" / "files.mpq"

    create_mpq_archive_for_test(binary_path, script_dir)

    test_file = script_dir / "data" / "files" / "cats.txt"

    result = subprocess.run(
        [str(binary_path), "add", str(target_mpq), str(test_file), "--update"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    assert result.returncode == 1, f"mpqcli failed with error: {result.stderr}"
    assert "--update is only meaningful when adding a directory" in result.stderr


# ---- stdin tests ----

def test_add_files_via_stdin(binary_path, generate_test_files):
    _ = generate_test_files
    script_dir = Path(__file__).parent
    target_mpq = script_dir / "data" / "files.mpq"

    create_mpq_archive_for_test(binary_path, script_dir)

    test_file_a = script_dir / "data" / "stdin_a.txt"
    test_file_b = script_dir / "data" / "stdin_b.txt"
    test_file_a.write_text("Stdin file A.")
    test_file_b.write_text("Stdin file B.")

    stdin_input = f"{test_file_a}\n{test_file_b}\n"

    result = subprocess.run(
        [str(binary_path), "add", str(target_mpq), "-"],
        input=stdin_input,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    assert "[+] Adding file: stdin_a.txt" in result.stdout
    assert "[+] Adding file: stdin_b.txt" in result.stdout

    list_result = subprocess.run(
        [str(binary_path), "list", str(target_mpq)],
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True
    )
    assert "stdin_a.txt" in list_result.stdout
    assert "stdin_b.txt" in list_result.stdout


# ---- Helpers ----

def create_mpq_archive_for_test(binary_path, script_dir):
    target_dir = script_dir / "data" / "files"
    target_file = target_dir.with_suffix(".mpq")
    target_file.unlink(missing_ok=True)
    result = subprocess.run(
        [str(binary_path), "create", "--version", "1", str(target_dir)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    assert target_file.exists(), "MPQ file was not created"
    assert target_file.stat().st_size > 0, "MPQ file is empty"


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

def verify_file_in_mpq_has_content(binary_path, mpq_archive, file_name, expected_content, locale=None):
    cmd = [str(binary_path), "read", file_name, str(mpq_archive)]
    if locale is not None:
        cmd += ["--locale", locale]
    result = subprocess.run(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    output_lines = set(line for line in result.stdout.splitlines() if line.strip() != "")
    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    assert output_lines == expected_content, f"Unexpected output: {output_lines}"
