import subprocess
from pathlib import Path


def test_read_mpq_v1_plain(binary_path):
    script_dir = Path(__file__).parent
    test_file = script_dir / "data" / "mpq_with_output_v1.mpq"

    expected_output = {
        "This is a file about cats.",
    }

    result = subprocess.run(
        [str(binary_path), "read", "cats.txt", str(test_file)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    # Remove empty lines from output, Windows adds an extra empty line
    output_lines = set(line for line in result.stdout.splitlines() if line.strip() != "")

    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    assert output_lines == expected_output, f"Unexpected output: {output_lines}"


def test_read_mpq_v1_hex(binary_path):
    script_dir = Path(__file__).parent
    test_file = script_dir / "data" / "mpq_with_output_v1.mpq"

    expected_output = {
        "\x00\x01\x02\x03\x04\x05\x06\x07",
    }

    result = subprocess.run(
        [str(binary_path), "read", "bytes", str(test_file)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    output_lines = set(result.stdout.splitlines())

    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    assert output_lines == expected_output, f"Unexpected output: {output_lines}"


def test_read_file_from_mpq_with_locale(binary_path, generate_locales_mpq_test_files):
    """
    Test MPQ archive file reading with a specified locale.

    This test checks:
    - If the file with the given locale is read correctly.
    - That the file content is correct.
    """
    _ = generate_locales_mpq_test_files
    script_dir = Path(__file__).parent
    test_file = script_dir / "data" / "mpq_with_many_locales.mpq"
    file_to_read = "cats.txt"
    locale = "deDE"

    result = subprocess.run(
        [str(binary_path), "read", file_to_read, str(test_file), "--locale", locale],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    expected_content = {
        "Dies ist eine Datei über Katzen.",
    }

    output_lines = set(result.stdout.splitlines())
    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    assert output_lines == expected_content, f"Unexpected output: {output_lines}"


def test_read_file_from_mpq_with_default_locale(binary_path, generate_locales_mpq_test_files):
    """
    Test MPQ archive file reading with no specified locale but many matching file names.

    This test checks:
    - If the file with the default locale is read correctly.
    - That the file content is correct.
    """
    _ = generate_locales_mpq_test_files
    script_dir = Path(__file__).parent
    test_file = script_dir / "data" / "mpq_with_many_locales.mpq"
    file_to_read = "cats.txt"


    result = subprocess.run(
        [str(binary_path), "read", file_to_read, str(test_file)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    expected_content = {
        "This is a file about cats.",
    }

    output_lines = set(result.stdout.splitlines())
    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    assert output_lines == expected_content, f"Unexpected output: {output_lines}"


def test_read_file_from_mpq_with_illegal_locale(binary_path, generate_locales_mpq_test_files):
    """
    Test MPQ archive file reading with an illegal locale.

    This test checks:
    - When a locale is given that does not exist in the file, the file with the default locale is read.
    - That the file content is correct.
    """
    _ = generate_locales_mpq_test_files
    script_dir = Path(__file__).parent
    test_file = script_dir / "data" / "mpq_with_many_locales.mpq"
    file_to_read = "cats.txt"
    locale = "nosuchlocale"


    result = subprocess.run(
        [str(binary_path), "read", file_to_read, str(test_file), "--locale", locale],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    expected_content = {
        "[!] Warning: The locale 'nosuchlocale' is unknown. Will use default locale instead.",
        "This is a file about cats.",
    }

    output_lines = set(result.stdout.splitlines())
    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    assert output_lines == expected_content, f"Unexpected output: {output_lines}"


def test_read_file_from_mpq_with_locale_not_in_file(binary_path, generate_locales_mpq_test_files):
    """
    Test MPQ archive file reading with a specified locale that is not in the file.

    This test checks:
    - When a locale is given that does not exist in the file, the file with the default locale is read.
    - That the file content is correct.
    """
    _ = generate_locales_mpq_test_files
    script_dir = Path(__file__).parent
    test_file = script_dir / "data" / "mpq_with_many_locales.mpq"
    file_to_read = "cats.txt"
    locale = "ptPT"  # There is no file for this locale

    result = subprocess.run(
        [str(binary_path), "read", file_to_read, str(test_file), "--locale", locale],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    expected_content = {
        "This is a file about cats.",
    }

    output_lines = set(result.stdout.splitlines())
    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    assert output_lines == expected_content, f"Unexpected output: {output_lines}"


def test_read_file_from_mpq_with_no_locale_argument_and_no_default_locale(binary_path, generate_locales_mpq_test_files):
    """
    Test MPQ archive file reading without a specified locale, and no file with the default locale.

    This test checks:
    - When no locale is given, and no file by that name exists for the default locale,
    but one does for a different one, no file is read.
    """
    _ = generate_locales_mpq_test_files
    script_dir = Path(__file__).parent
    test_file = script_dir / "data" / "mpq_with_one_locale.mpq"
    file_to_read = "cats.txt"  # There is a file by this name, but for locale esES


    result = subprocess.run(
        [str(binary_path), "read", file_to_read, str(test_file)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    expected_stdout_output = set()
    expected_stderr_output = {
        "[!] Failed: File doesn't exist for locale enUS: " + file_to_read,
    }

    stdout_output_lines = set(result.stdout.splitlines())
    stderr_output_lines = set(result.stderr.splitlines())

    assert result.returncode == 1, f"mpqcli failed with error: {result.stderr}"
    assert stdout_output_lines == expected_stdout_output, f"Unexpected output: {stdout_output_lines}"
    assert stderr_output_lines == expected_stderr_output, f"Unexpected output: {stderr_output_lines}"


def test_read_file_from_mpq_with_wrong_locale_argument_and_no_default_locale(binary_path, generate_locales_mpq_test_files):
    """
    Test MPQ archive file reading with a specified locale, and no file with the default locale.

    This test checks:
    - When no locale is given, and no file by that name exists for the default locale,
    but one does for a different one, no file is read.
    """
    _ = generate_locales_mpq_test_files
    script_dir = Path(__file__).parent
    test_file = script_dir / "data" / "mpq_with_one_locale.mpq"
    file_to_read = "cats.txt"  # There is a file by this name, but for locale esES
    locale = "deDE"


    result = subprocess.run(
        [str(binary_path), "read", file_to_read, str(test_file), "--locale", locale],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    expected_stdout_output = set()
    expected_stderr_output = {
        "[!] Failed: File doesn't exist for locale deDE: " + file_to_read,
        }

    stdout_output_lines = set(result.stdout.splitlines())
    stderr_output_lines = set(result.stderr.splitlines())

    assert result.returncode == 1, f"mpqcli failed with error: {result.stderr}"
    assert stdout_output_lines == expected_stdout_output, f"Unexpected output: {stdout_output_lines}"
    assert stderr_output_lines == expected_stderr_output, f"Unexpected output: {stderr_output_lines}"


def test_read_mpq_target_does_not_exist(binary_path):
    script_dir = Path(__file__).parent
    target_file = script_dir / "does" / "not" / "exist.mpq"

    result = subprocess.run(
        [str(binary_path), "read", "cats.txt", str(target_file)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    assert result.returncode == 105, f"mpqcli failed with error: {result.stderr}"


def test_read_file_does_not_exist_in_mpq(binary_path):
    script_dir = Path(__file__).parent
    test_file = script_dir / "data" / "mpq_with_output_v1.mpq"

    result = subprocess.run(
        [str(binary_path), "read", "does-not-exist.txt", str(test_file)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    assert result.returncode == 1, f"Unexpected exit code: {result.returncode}"
    assert result.stdout == "", f"Unexpected stdout: {result.stdout}"
    assert "does-not-exist.txt" in result.stderr, f"Unexpected stderr: {result.stderr}"
