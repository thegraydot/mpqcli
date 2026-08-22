import subprocess
from pathlib import Path


def test_rename_file_in_mpq_archive(binary_path, generate_mpq_without_internal_listfile):
    script_dir = Path(__file__).parent
    target_file = script_dir / "data" / "mpq_without_internal_listfile.mpq"
    listfile = script_dir / "data" / "listfile.txt"
    listfile.write_text("cats.txt\ndogs.txt\ncapybaras.txt\nrenamed.txt\n")

    result = subprocess.run(
        [str(binary_path), "rename", str(target_file), "capybaras.txt", "renamed.txt"],
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True
    )

    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    assert "[~] Renaming file: capybaras.txt -> renamed.txt" in result.stdout

    result = subprocess.run(
        [str(binary_path), "list", str(target_file), "--listfile", str(listfile)],
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True
    )
    assert result.returncode == 0
    assert "renamed.txt" in result.stdout
    assert "capybaras.txt" not in result.stdout


def test_rename_missing_file_fails(binary_path, generate_mpq_without_internal_listfile):
    target_file = Path(__file__).parent / "data" / "mpq_without_internal_listfile.mpq"
    result = subprocess.run(
        [str(binary_path), "rename", str(target_file), "missing.txt", "renamed.txt"],
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True
    )

    assert result.returncode == 1
    assert "[!] Failed: File doesn't exist" in result.stderr


def test_rename_target_mpq_does_not_exist(binary_path, generate_locales_mpq_test_files):
    """
    Test MPQ file rename with a non-existent target.

    This test checks:
    - If the application exits correctly when the target does not exist.
    """
    _ = generate_locales_mpq_test_files
    script_dir = Path(__file__).parent
    test_file = "cats.txt"
    target_file = script_dir / "does" / "not" / "exist.mpq"

    result = subprocess.run(
        [str(binary_path), "rename", str(target_file), test_file, "renamed.txt"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    assert result.returncode == 105, f"mpqcli failed with error: {result.stderr}"


def test_rename_target_file_does_not_exist(binary_path, generate_locales_mpq_test_files):
    """
    Test MPQ file rename with a non-existent file to rename.

    This test checks:
    - If the application exits correctly when the target file to rename does not exist.
    """
    _ = generate_locales_mpq_test_files
    script_dir = Path(__file__).parent
    test_file = "does-not-exist.txt"
    target_file = script_dir / "data" / "mpq_with_many_locales.mpq"

    result = subprocess.run(
        [str(binary_path), "rename", str(target_file), test_file, "renamed.txt"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    output_lines = set(result.stdout.splitlines())
    expected_stdout_output = {
        "[~] Renaming file: does-not-exist.txt -> renamed.txt",
    }
    assert output_lines == expected_stdout_output, f"Unexpected output: {output_lines}"

    output_lines = set(result.stderr.splitlines())
    expected_stderr_output = {
        "[!] Failed: File doesn't exist for locale enUS: does-not-exist.txt",
    }
    assert output_lines == expected_stderr_output, f"Unexpected output: {output_lines}"

    assert result.returncode == 1, f"mpqcli failed with error: {result.stderr}"


def test_rename_file_from_mpq_archive_with_wrong_locale_given(
        binary_path,
        generate_locales_mpq_test_files,
        generate_mpq_without_internal_listfile,
):
    """
    Test MPQ file rename, with the wrong locale given.

    This test checks:
    - When the user gives a locale that does not exist for the file,
      the file is not renamed.
    """
    _ = generate_locales_mpq_test_files
    _ = generate_mpq_without_internal_listfile
    script_dir = Path(__file__).parent

    permutations = [
        ("capybaras.txt", "mpq_without_internal_listfile.mpq"), # File exists only for the Default locale
        ("cats.txt", "mpq_without_internal_listfile.mpq"),      # File exists only for the German locale
        ("dogs.txt", "mpq_without_internal_listfile.mpq"),      # File exists only for the Swedish locale (which is not in locales.cpp)
        ("cats.txt", "mpq_with_many_locales.mpq"),              # File exists for many locales
    ]

    for test_file, target_file_name in permutations:
        target_file = script_dir / "data" / target_file_name

        result = subprocess.run(
            [str(binary_path), "rename", str(target_file), test_file, "renamed.txt", "--locale", "ptPT"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )

        output_lines = set(result.stdout.splitlines())
        expected_output = {
            "[~] Renaming file for locale ptPT: " + test_file + " -> renamed.txt",
        }
        assert output_lines == expected_output, f"Unexpected output: {output_lines}"

        output_lines = set(result.stderr.splitlines())
        expected_stderr_output = {
            "[!] Failed: File doesn't exist for locale ptPT: " + test_file,
        }
        assert output_lines == expected_stderr_output, f"Unexpected output: {output_lines}"

        assert result.returncode == 1, f"mpqcli failed with error: {result.stderr}"


def test_rename_default_locale_file_from_mpq_archive_unique_name(binary_path, generate_mpq_without_internal_listfile):
    """
    Test MPQ file rename, with no locale given.

    This test checks:
    - When there is only one file with the same name and default locale,
      it should be renamed, when no locale is given by the user.
    """
    _ = generate_mpq_without_internal_listfile
    script_dir = Path(__file__).parent
    test_file = "capybaras.txt"
    target_file = script_dir / "data" / "mpq_without_internal_listfile.mpq"

    result = subprocess.run(
        [str(binary_path), "rename", str(target_file), test_file, "renamed.txt"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    output_lines = set(result.stdout.splitlines())
    expected_output = {
        "[~] Renaming file: capybaras.txt -> renamed.txt",
    }
    assert output_lines == expected_output, f"Unexpected output: {output_lines}"

    output_lines = set(result.stderr.splitlines())
    expected_stderr_output = set()
    assert output_lines == expected_stderr_output, f"Unexpected output: {output_lines}"

    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"


def test_rename_files_from_mpq_archive_shared_name(binary_path, generate_locales_mpq_test_files):
    """
    Test MPQ file rename with locale, for files sharing the same name across locales.

    This test checks:
    - If the application correctly handles renaming a file with
      the given locale from an MPQ archive, without affecting the other
      locale copies that share the same name.
    """
    _ = generate_locales_mpq_test_files
    script_dir = Path(__file__).parent
    test_file = "cats.txt"
    target_file = script_dir / "data" / "mpq_with_many_locales.mpq"

    expected_output = {
        "enUS  cats.txt",
        "deDE  cats.txt",
        "esES  cats.txt",
        "041D  cats.txt",
    }
    verify_archive_content(binary_path, target_file, expected_output)

    # Renaming without specifying locale means renaming using locale 0 = enUS
    result = subprocess.run(
        [str(binary_path), "rename", str(target_file), test_file, "renamed_enus.txt"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"

    expected_output = {
        "enUS  renamed_enus.txt",
        "deDE  cats.txt",
        "esES  cats.txt",
        "041D  cats.txt",
    }
    verify_archive_content(binary_path, target_file, expected_output)


    result = subprocess.run(
        [str(binary_path), "rename", str(target_file), test_file, "renamed_eses.txt", "--locale", "esES"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"

    expected_output = {
        "enUS  renamed_enus.txt",
        "deDE  cats.txt",
        "esES  renamed_eses.txt",
        "041D  cats.txt",
    }
    verify_archive_content(binary_path, target_file, expected_output)


    result = subprocess.run(
        [str(binary_path), "rename", str(target_file), test_file, "renamed_041d.txt", "--locale", "041D"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"

    expected_output = {
        "enUS  renamed_enus.txt",
        "deDE  cats.txt",
        "esES  renamed_eses.txt",
        "041D  renamed_041d.txt",
    }
    verify_archive_content(binary_path, target_file, expected_output)


def test_rename_files_from_mpq_archive_unique_name(binary_path, generate_mpq_without_internal_listfile):
    """
    Test MPQ file rename with locale, for a file with a unique name stored
    under a non-default locale.

    This test checks:
    - If the application correctly handles renaming a file with
      the given locale from an MPQ archive.
    """
    _ = generate_mpq_without_internal_listfile
    script_dir = Path(__file__).parent
    test_file = "dogs.txt"
    target_file = script_dir / "data" / "mpq_without_internal_listfile.mpq"
    listfile = script_dir / "data" / "listfile.txt"
    listfile.write_text("cats.txt\ndogs.txt\ncapybaras.txt\nrenamed.txt")

    expected_output = {
        "enUS  capybaras.txt",
        "deDE  cats.txt",
        "041D  dogs.txt",
    }
    verify_archive_content(binary_path, target_file, expected_output, listfile)

    # Renaming without specifying locale means renaming using locale 0 = enUS;
    # dogs.txt only exists for locale 041D, so this should fail.
    result = subprocess.run(
        [str(binary_path), "rename", str(target_file), test_file, "renamed.txt"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    assert result.returncode == 1, f"mpqcli failed with error: {result.stderr}"

    expected_output = {
        "enUS  capybaras.txt",
        "deDE  cats.txt",
        "041D  dogs.txt",
    }
    verify_archive_content(binary_path, target_file, expected_output, listfile)


    result = subprocess.run(
        [str(binary_path), "rename", str(target_file), test_file, "renamed.txt", "--locale", "041D"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"

    expected_output = {
        "enUS  capybaras.txt",
        "deDE  cats.txt",
        "041D  renamed.txt",
    }
    verify_archive_content(binary_path, target_file, expected_output, listfile)


def test_rename_to_existing_name_fails(binary_path, generate_mpq_without_internal_listfile, tmp_path):
    """
    Test MPQ file rename to a name that already exists for the same locale.

    This test checks:
    - If the application fails, and leaves the archive unchanged, when
      renaming a file to a name that is already used by another file
      under the same locale.
    """
    _ = generate_mpq_without_internal_listfile
    script_dir = Path(__file__).parent
    target_file = script_dir / "data" / "mpq_without_internal_listfile.mpq"

    # Add a second file under the default locale, so that "capybaras.txt"
    # (also default locale) has a name collision to rename into.
    extra_file = tmp_path / "elephants.txt"
    extra_file.write_text("This is a file about elephants.\n", newline="\n")

    result = subprocess.run(
        [str(binary_path), "add", str(target_file), str(extra_file)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"

    result = subprocess.run(
        [str(binary_path), "rename", str(target_file), "capybaras.txt", "elephants.txt"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    assert result.returncode == 1, f"mpqcli unexpectedly succeeded: {result.stdout}"
    assert "[~] Renaming file: capybaras.txt -> elephants.txt" in result.stdout
    assert "[!] Failed: File cannot be renamed for locale enUS: capybaras.txt -> elephants.txt" in result.stderr

    # Verify the archive is unchanged: both files should still exist under their original names.
    listfile = script_dir / "data" / "listfile.txt"
    listfile.write_text("cats.txt\ndogs.txt\ncapybaras.txt\nelephants.txt")

    result = subprocess.run(
        [str(binary_path), "list", str(target_file), "--listfile", str(listfile)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    assert result.returncode == 0
    assert "capybaras.txt" in result.stdout
    assert "elephants.txt" in result.stdout


def verify_archive_content(binary_path, target_file, expected_output, listfile=Path()):
    # Verify that the archive has the expected content
    cmd = [str(binary_path), "list", "-d", str(target_file), "-p", "locale"]
    if listfile != Path():
        cmd.extend(["--listfile", str(listfile)])

    result = subprocess.run(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    output_lines = set(result.stdout.splitlines())
    assert output_lines == expected_output, f"Unexpected output: {output_lines}"


def test_rename_converts_forward_slashes_to_backslashes(binary_path, generate_mpq_without_internal_listfile):
    """A new name containing forward slashes is stored using the backslash
    separator that MPQ archives use, matching how add and create store paths."""
    script_dir = Path(__file__).parent
    target_file = script_dir / "data" / "mpq_without_internal_listfile.mpq"
    listfile = script_dir / "data" / "listfile.txt"
    listfile.write_text("cats.txt\ndogs.txt\ncapybaras.txt\ndeep\\renamed.txt\n")

    result = subprocess.run(
        [str(binary_path), "rename", str(target_file), "capybaras.txt", "deep/renamed.txt"],
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True
    )

    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    assert "[~] Renaming file: capybaras.txt -> deep\\renamed.txt" in result.stdout

    result = subprocess.run(
        [str(binary_path), "list", str(target_file), "--listfile", str(listfile)],
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True
    )
    assert result.returncode == 0
    assert "deep\\renamed.txt" in result.stdout
    assert "deep/renamed.txt" not in result.stdout
