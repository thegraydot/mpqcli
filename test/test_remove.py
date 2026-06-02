import subprocess
from pathlib import Path


def test_remove_target_mpq_does_not_exist(binary_path, generate_locales_mpq_test_files):
    """
    Test MPQ file removal with a non-existent target.

    This test checks:
    - If the application exits correctly when the target does not exist.
    """
    _ = generate_locales_mpq_test_files
    script_dir = Path(__file__).parent
    test_file = "cats.txt"
    target_file = script_dir / "does" / "not" / "exist.mpq"

    result = subprocess.run(
        [str(binary_path), "remove", str(target_file), str(test_file)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    assert result.returncode == 105, f"mpqcli failed with error: {result.stderr}"


def test_remove_target_file_does_not_exist(binary_path, generate_locales_mpq_test_files):
    """
    Test MPQ file removal with a non-existent file to remove.

    This test checks:
    - If the application exits correctly when the target file to remove does not exist.
    """
    _ = generate_locales_mpq_test_files
    script_dir = Path(__file__).parent
    test_file = "does-not-exist.txt"
    target_file = script_dir / "data" / "mpq_with_many_locales.mpq"

    result = subprocess.run(
        [str(binary_path), "remove", str(target_file), str(test_file)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    output_lines = set(result.stdout.splitlines())
    expected_stdout_output = {
        "[-] Removing file: does-not-exist.txt",
    }
    assert output_lines == expected_stdout_output, f"Unexpected output: {output_lines}"

    output_lines = set(result.stderr.splitlines())
    expected_stderr_output = {
        "[!] Failed: File doesn't exist for locale enUS: does-not-exist.txt",
    }
    assert output_lines == expected_stderr_output, f"Unexpected output: {output_lines}"

    assert result.returncode == 1, f"mpqcli failed with error: {result.stderr}"


def test_remove_file_from_mpq_archive_with_wrong_locale_given(
        binary_path,
        generate_locales_mpq_test_files,
        generate_mpq_without_internal_listfile,
):
    """
    Test MPQ file removal, with the wrong locale given.

    This test checks:
    - When the user gives a locale that does not exist for the file,
      the file is not deleted.
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
            [str(binary_path), "remove", str(target_file), str(test_file), "--locale", "ptPT"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )

        output_lines = set(result.stdout.splitlines())
        expected_output = {
            "[-] Removing file for locale ptPT: " + test_file,
        }
        assert output_lines == expected_output, f"Unexpected output: {output_lines}"

        output_lines = set(result.stderr.splitlines())
        expected_stderr_output = {
            "[!] Failed: File doesn't exist for locale ptPT: " + test_file,
        }
        assert output_lines == expected_stderr_output, f"Unexpected output: {output_lines}"

        assert result.returncode == 1, f"mpqcli failed with error: {result.stderr}"


def test_remove_default_locale_file_from_mpq_archive_unique_name(binary_path, generate_mpq_without_internal_listfile):
    """
    Test MPQ file removal, with no locale given.

    This test checks:
    - When there is only one file with the same name and default locale,
      it should be deleted, when no locale is given by the user.
    """
    _ = generate_mpq_without_internal_listfile
    script_dir = Path(__file__).parent
    test_file = "capybaras.txt"
    target_file = script_dir / "data" / "mpq_without_internal_listfile.mpq"

    result = subprocess.run(
        [str(binary_path), "remove", str(target_file), str(test_file)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    output_lines = set(result.stdout.splitlines())
    expected_output = {
        "[-] Removing file: capybaras.txt",
    }
    assert output_lines == expected_output, f"Unexpected output: {output_lines}"

    output_lines = set(result.stderr.splitlines())
    expected_stderr_output = set()
    assert output_lines == expected_stderr_output, f"Unexpected output: {output_lines}"

    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"


def test_remove_files_from_mpq_archive_shared_name(binary_path, generate_locales_mpq_test_files):
    """
    Test MPQ file removal with locale.

    This test checks:
    - If the application correctly handles removing a file with
      the given locale from an MPQ archive.
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

    # Removing without specifying locale means removing using locale 0 = enUS
    result = subprocess.run(
        [str(binary_path), "remove", str(target_file), str(test_file)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"

    expected_output = {
        "deDE  cats.txt",
        "esES  cats.txt",
        "041D  cats.txt",
    }
    verify_archive_content(binary_path, target_file, expected_output)


    result = subprocess.run(
        [str(binary_path), "remove", str(target_file), str(test_file), "--locale", "esES"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"

    expected_output = {
        "deDE  cats.txt",
        "041D  cats.txt",
    }
    verify_archive_content(binary_path, target_file, expected_output)


    result = subprocess.run(
        [str(binary_path), "remove", str(target_file), str(test_file), "--locale", "041D"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"

    expected_output = {
        "deDE  cats.txt",
    }
    verify_archive_content(binary_path, target_file, expected_output)


def test_remove_files_from_mpq_archive_unique_name(binary_path, generate_mpq_without_internal_listfile):
    """
    Test MPQ file removal with locale.

    This test checks:
    - If the application correctly handles removing a file with
      the given locale from an MPQ archive.
    """
    _ = generate_mpq_without_internal_listfile
    script_dir = Path(__file__).parent
    test_file = "dogs.txt"
    target_file = script_dir / "data" / "mpq_without_internal_listfile.mpq"
    listfile = script_dir / "data" / "listfile.txt"
    listfile.write_text("cats.txt\ndogs.txt\ncapybaras.txt")

    expected_output = {
        "enUS  capybaras.txt",
        "deDE  cats.txt",
        "041D  dogs.txt",
    }
    verify_archive_content(binary_path, target_file, expected_output, listfile)

    # Removing without specifying locale means removing using locale 0 = enUS
    result = subprocess.run(
        [str(binary_path), "remove", str(target_file), str(test_file)],
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
        [str(binary_path), "remove", str(target_file), str(test_file), "--locale", "041D"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"

    expected_output = {
        "enUS  capybaras.txt",
        "deDE  cats.txt",
    }
    verify_archive_content(binary_path, target_file, expected_output, listfile)


def test_remove_multiple_files_at_once(binary_path, generate_locales_mpq_test_files):
    _ = generate_locales_mpq_test_files
    script_dir = Path(__file__).parent
    target_file = script_dir / "data" / "mpq_with_many_locales.mpq"

    result = subprocess.run(
        [str(binary_path), "remove", str(target_file), "cats.txt", "--locale", "deDE"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    assert result.returncode == 0, f"First remove failed: {result.stderr}"

    expected_output = {
        "enUS  cats.txt",
        "esES  cats.txt",
        "041D  cats.txt",
    }
    verify_archive_content(binary_path, target_file, expected_output)

    result = subprocess.run(
        [str(binary_path), "remove", str(target_file), "cats.txt", "cats.txt", "--locale", "esES"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    assert result.returncode == 0, f"Second remove failed: {result.stderr}"

    expected_output = {
        "enUS  cats.txt",
        "041D  cats.txt",
    }
    verify_archive_content(binary_path, target_file, expected_output)


def test_remove_files_via_stdin(binary_path, generate_locales_mpq_test_files):
    _ = generate_locales_mpq_test_files
    script_dir = Path(__file__).parent
    target_file = script_dir / "data" / "mpq_with_many_locales.mpq"

    stdin_input = "cats.txt\n"

    result = subprocess.run(
        [str(binary_path), "remove", str(target_file), "-"],
        input=stdin_input,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    assert "[-] Removing file: cats.txt" in result.stdout

    expected_output = {
        "deDE  cats.txt",
        "esES  cats.txt",
        "041D  cats.txt",
    }
    verify_archive_content(binary_path, target_file, expected_output)


def verify_archive_content(binary_path, target_file, expected_output, listfile = Path()):
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
