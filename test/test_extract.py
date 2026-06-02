import os
import shutil
import subprocess
from pathlib import Path


def test_extract_mpq_target_does_not_exist(binary_path, generate_test_files):
    """
    Test MPQ file extraction with a non-existent target.

    This test checks:
    - If the application exits correctly when the target does not exist.
    """
    _ = generate_test_files
    script_dir = Path(__file__).parent
    target_dir = script_dir / "does" / "not" / "exist"

    result = subprocess.run(
        [str(binary_path), "extract", str(target_dir)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    assert result.returncode == 105, f"mpqcli failed with error: {result.stderr}"


def test_extract_mpq_default_options(binary_path, generate_test_files):
    """
    Test MPQ archive extraction with default options.

    This test checks:
    - If the MPQ archive is extracted correctly.
    - If the output files match the expected files.
    - If the output directory is created.
    """
    _ = generate_test_files
    script_dir = Path(__file__).parent
    test_file = script_dir / "data" / "mpq_with_output_v1.mpq"

    # Create output_file path without suffix (default extract behavior is MPQ without extension)
    output_dir = test_file.with_suffix("")
    if output_dir.exists():
        shutil.rmtree(output_dir)

    expected_output = {
        "cats.txt",
        "dogs.txt",
        "bytes",
        "(listfile)",
        "(attributes)",
    }

    result = subprocess.run(
        [str(binary_path), "extract", str(test_file)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    output_lines = set(result.stdout.splitlines())

    # Create expected_lines set based on expected output with prefix
    expected_lines = {f"[*] Extracted: {line}" for line in expected_output}

    # Create output_files set based on directory contents (not full path)
    output_files = set(fi.name for fi in output_dir.glob("*"))

    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    assert output_lines == expected_lines, f"Unexpected output: {output_lines}"
    assert output_dir.exists(), "Output directory was not created"
    assert output_files == expected_output, f"Unexpected files: {output_files}"

    # Content and file size verification
    import platform
    cats_file = output_dir / "cats.txt"
    dogs_file = output_dir / "dogs.txt"
    expected_cats_content = "This is a file about cats.\n"
    expected_dogs_content = "This is a file about dogs.\n"
    expected_size = 28 if platform.system() == "Windows" else 27

    assert cats_file.read_text(encoding="utf-8") == expected_cats_content, \
        f"Unexpected content in cats.txt"
    assert dogs_file.read_text(encoding="utf-8") == expected_dogs_content, \
        f"Unexpected content in dogs.txt"
    assert cats_file.stat().st_size == expected_size, \
        f"Unexpected size for cats.txt: {cats_file.stat().st_size}"
    assert dogs_file.stat().st_size == expected_size, \
        f"Unexpected size for dogs.txt: {dogs_file.stat().st_size}"


def test_extract_mpq_output_directory_specified(binary_path, generate_test_files):
    """
    Test MPQ archive extraction with specified output directory.

    This test checks:
    - If the MPQ archive is extracted correctly.
    - If the output files match the expected files.
    - If the output directory is created.
    """
    _ = generate_test_files
    script_dir = Path(__file__).parent
    test_file = script_dir / "data" / "mpq_with_output_v1.mpq"
    output_dir = script_dir / "data" / "extracted"
    if output_dir.exists():
        shutil.rmtree(output_dir)

    expected_output = {
        "cats.txt",
        "dogs.txt",
        "bytes",
        "(listfile)",
        "(attributes)",
    }

    result = subprocess.run(
        [str(binary_path), "extract", "-o", str(output_dir), str(test_file)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    output_lines = set(result.stdout.splitlines())

    # Create expected_lines set based on expected output with prefix
    expected_lines = {f"[*] Extracted: {line}" for line in expected_output}

    # Create output_files set based on directory contents (not full path)
    output_files = set(fi.name for fi in output_dir.glob("*"))

    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    assert output_lines == expected_lines, f"Unexpected output: {output_lines}"
    assert output_dir.exists(), "Output directory was not created"
    assert output_files == expected_output, f"Unexpected files: {output_files}"

    # Content and file size verification
    import platform
    cats_file = output_dir / "cats.txt"
    dogs_file = output_dir / "dogs.txt"
    expected_cats_content = "This is a file about cats.\n"
    expected_dogs_content = "This is a file about dogs.\n"
    expected_size = 28 if platform.system() == "Windows" else 27

    assert cats_file.read_text(encoding="utf-8") == expected_cats_content, \
        f"Unexpected content in cats.txt"
    assert dogs_file.read_text(encoding="utf-8") == expected_dogs_content, \
        f"Unexpected content in dogs.txt"
    assert cats_file.stat().st_size == expected_size, \
        f"Unexpected size for cats.txt: {cats_file.stat().st_size}"
    assert dogs_file.stat().st_size == expected_size, \
        f"Unexpected size for dogs.txt: {dogs_file.stat().st_size}"


def test_extract_file_from_mpq_output_directory_specified(binary_path, generate_test_files):
    """
    Test MPQ archive file extraction with specified output directory.

    This test checks:
    - If the file is extracted correctly.
    - If the output files match the expected files.
    - If the output directory is created.
    """
    _ = generate_test_files
    script_dir = Path(__file__).parent
    test_file = script_dir / "data" / "mpq_with_output_v1.mpq"
    output_dir = script_dir / "data" / "extracted_file"
    if output_dir.exists():
        shutil.rmtree(output_dir)


    expected_output = {
        "cats.txt",
    }

    result = subprocess.run(
        [str(binary_path), "extract", "-o", str(output_dir), "-f", "".join(expected_output), str(test_file)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    output_lines = set(result.stdout.splitlines())

    # Create expected_lines set based on expected output with prefix
    expected_lines = {f"[*] Extracted: {line}" for line in expected_output}

    # Create output_files set based on directory contents (not full path)
    output_files = set(fi.name for fi in output_dir.glob("*"))

    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    assert output_lines == expected_lines, f"Unexpected output: {output_lines}"
    assert output_dir.exists(), "Output directory was not created"
    assert output_files == expected_output, f"Unexpected files: {output_files}"

    # Content and file size verification (only cats.txt was extracted)
    import platform
    cats_file = output_dir / "cats.txt"
    expected_cats_content = "This is a file about cats.\n"
    expected_size = 28 if platform.system() == "Windows" else 27

    assert cats_file.read_text(encoding="utf-8") == expected_cats_content, \
        f"Unexpected content in cats.txt"
    assert cats_file.stat().st_size == expected_size, \
        f"Unexpected size for cats.txt: {cats_file.stat().st_size}"


def test_extract_file_from_mpq_with_locale(binary_path, generate_locales_mpq_test_files):
    """
    Test MPQ archive file extraction with a specified locale.

    This test checks:
    - If the file with the given locale is extracted correctly.
    - If the output files match the expected files.
    """
    _ = generate_locales_mpq_test_files
    script_dir = Path(__file__).parent
    test_file = script_dir / "data" / "mpq_with_many_locales.mpq"
    output_dir = script_dir / "data" / "extracted_file"
    file_to_extract = "cats.txt"
    locale = "esES"
    if output_dir.exists():
        shutil.rmtree(output_dir)

    result = subprocess.run(
        [str(binary_path), "extract", "-o", str(output_dir), "-f", file_to_extract, str(test_file), "--locale", locale],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    expected_stdout = {
        "[*] Extracted: " + file_to_extract
    }
    output_file = output_dir / file_to_extract
    expected_content = "Este es un archivo sobre gatos."

    output_lines = set(result.stdout.splitlines())
    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    assert output_lines == expected_stdout, f"Unexpected output: {output_lines}"
    assert output_file.exists(), "Output directory was not created"
    assert output_file.read_text(encoding="utf-8") == expected_content, "Unexpected file content"


def test_extract_file_from_mpq_with_default_locale(binary_path, generate_locales_mpq_test_files):
    """
    Test MPQ archive file extraction with no specified locale but many matching file names.

    This test checks:
    - If the file with the default locale is extracted correctly.
    - If the output files match the expected files.
    """
    _ = generate_locales_mpq_test_files
    script_dir = Path(__file__).parent
    test_file = script_dir / "data" / "mpq_with_many_locales.mpq"
    output_dir = script_dir / "data" / "extracted_file"
    file_to_extract = "cats.txt"
    if output_dir.exists():
        shutil.rmtree(output_dir)


    result = subprocess.run(
        [str(binary_path), "extract", "-o", str(output_dir), "-f", file_to_extract, str(test_file)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    expected_stdout = {
        "[*] Extracted: " + file_to_extract
    }
    output_file = output_dir / file_to_extract
    expected_content = "This is a file about cats."

    output_lines = set(result.stdout.splitlines())
    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    assert output_lines == expected_stdout, f"Unexpected output: {output_lines}"
    assert output_file.exists(), "Output directory was not created"
    assert output_file.read_text(encoding="utf-8") == expected_content, "Unexpected file content"


def test_extract_file_from_mpq_with_illegal_locale(binary_path, generate_locales_mpq_test_files):
    """
    Test MPQ archive file extraction with an illegal locale.

    This test checks:
    - When a locale is given that does not exist in the file, the file with the default locale is extracted.
    - If the output files match the expected files.
    """
    _ = generate_locales_mpq_test_files
    script_dir = Path(__file__).parent
    test_file = script_dir / "data" / "mpq_with_many_locales.mpq"
    output_dir = script_dir / "data" / "extracted_file"
    file_to_extract = "cats.txt"
    locale = "nosuchlocale"
    if output_dir.exists():
        shutil.rmtree(output_dir)


    result = subprocess.run(
        [str(binary_path), "extract", "-o", str(output_dir), "-f", file_to_extract, str(test_file), "--locale", locale],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    expected_stdout = {
        "[!] Warning: The locale 'nosuchlocale' is unknown. Will use default locale instead.",
        "[*] Extracted: " + file_to_extract
    }
    output_file = output_dir / file_to_extract
    expected_content = "This is a file about cats."

    output_lines = set(result.stdout.splitlines())
    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    assert output_lines == expected_stdout, f"Unexpected output: {output_lines}"
    assert output_file.exists(), "Output directory was not created"
    assert output_file.read_text(encoding="utf-8") == expected_content, "Unexpected file content"


def test_extract_file_from_mpq_with_locale_not_in_file(binary_path, generate_locales_mpq_test_files):
    """
    Test MPQ archive file extraction with a specified locale that is not in the file.

    This test checks:
    - When a locale is given that does not exist in the file, the file with the default locale is extracted.
    - If the output files match the expected files.
    """
    _ = generate_locales_mpq_test_files
    script_dir = Path(__file__).parent
    test_file = script_dir / "data" / "mpq_with_many_locales.mpq"
    output_dir = script_dir / "data" / "extracted_file"
    file_to_extract = "cats.txt"
    locale = "ptPT"  # There is no file for this locale
    if output_dir.exists():
        shutil.rmtree(output_dir)

    result = subprocess.run(
        [str(binary_path), "extract", "-o", str(output_dir), "-f", file_to_extract, str(test_file), "--locale", locale],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    expected_stdout = {
        "[*] Extracted: " + file_to_extract
    }
    output_file = output_dir / file_to_extract
    expected_content = "This is a file about cats."

    output_lines = set(result.stdout.splitlines())
    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    assert output_lines == expected_stdout, f"Unexpected output: {output_lines}"
    assert output_file.exists(), "Output directory was not created"
    assert output_file.read_text(encoding="utf-8") == expected_content, "Unexpected file content"


def test_extract_file_from_mpq_with_no_locale_argument_and_no_default_locale(binary_path, generate_locales_mpq_test_files):
    """
    Test MPQ archive file extraction without a specified locale, and no file with the default locale.

    This test checks:
    - When no locale is given, and no file by that name exists for the default locale,
      but one does for a different one, no file is extracted.
    """
    _ = generate_locales_mpq_test_files
    script_dir = Path(__file__).parent
    test_file = script_dir / "data" / "mpq_with_one_locale.mpq"
    output_dir = script_dir / "data" / "extracted_file"
    file_to_extract = "cats.txt"  # There is a file by this name, but for locale esES
    if output_dir.exists():
        shutil.rmtree(output_dir)


    result = subprocess.run(
        [str(binary_path), "extract", "-o", str(output_dir), "-f", file_to_extract, str(test_file)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    expected_stdout_output = set()
    expected_stderr_output = {
        "[!] Failed: File doesn't exist for locale enUS: " + file_to_extract,
        "",
        "[!] Failed to extract all files.",
    }

    stdout_output_lines = set(result.stdout.splitlines())
    stderr_output_lines = set(result.stderr.splitlines())

    output_file = output_dir / file_to_extract

    assert result.returncode == 1, f"mpqcli failed with error: {result.stderr}"
    assert stdout_output_lines == expected_stdout_output, f"Unexpected output: {stdout_output_lines}"
    assert stderr_output_lines == expected_stderr_output, f"Unexpected output: {stderr_output_lines}"
    assert not output_file.exists(), "Output directory was not created"


def test_extract_file_from_mpq_with_wrong_locale_argument_and_no_default_locale(binary_path, generate_locales_mpq_test_files):
    """
    Test MPQ archive file extraction with a specified locale, and no file with the default locale.

    This test checks:
    - When no locale is given, and no file by that name exists for the default locale,
      but one does for a different one, no file is extracted.
    """
    _ = generate_locales_mpq_test_files
    script_dir = Path(__file__).parent
    test_file = script_dir / "data" / "mpq_with_one_locale.mpq"
    output_dir = script_dir / "data" / "extracted_file"
    file_to_extract = "cats.txt"  # There is a file by this name, but for locale esES
    locale = "deDE"
    if output_dir.exists():
        shutil.rmtree(output_dir)


    result = subprocess.run(
        [str(binary_path), "extract", "-o", str(output_dir), "-f", file_to_extract, str(test_file), "--locale", locale],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    expected_stdout_output = set()
    expected_stderr_output = {
        "[!] Failed: File doesn't exist for locale " + locale + ": " + file_to_extract,
        "",
        "[!] Failed to extract all files.",
    }

    stdout_output_lines = set(result.stdout.splitlines())
    stderr_output_lines = set(result.stderr.splitlines())

    output_file = output_dir / file_to_extract

    assert result.returncode == 1, f"mpqcli failed with error: {result.stderr}"
    assert stdout_output_lines == expected_stdout_output, f"Unexpected output: {stdout_output_lines}"
    assert stderr_output_lines == expected_stderr_output, f"Unexpected output: {stderr_output_lines}"
    assert not output_file.exists(), "Output directory was not created"


def test_extract_all_files_from_mpq_without_providing_listfile(binary_path, generate_mpq_without_internal_listfile):
    """
    Test file extraction of all files from MPQ archive with no internal listfile,
    when no external one is provided

    This test checks:
    - That files from MPQs with no internal listfile can still be extracted.
    - That files with different locales than the default are not extracted.
    """
    _ = generate_mpq_without_internal_listfile
    script_dir = Path(__file__).parent
    test_file = script_dir / "data" / "mpq_without_internal_listfile.mpq"
    output_dir = script_dir / "data" / "extracted_file"
    if output_dir.exists():
        shutil.rmtree(output_dir)


    expected_output = {
        "File00000000.xxx",
    }

    result = subprocess.run(
        [str(binary_path), "extract", "-o", str(output_dir), str(test_file)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    output_lines = set(result.stdout.splitlines())

    # Create expected_lines set based on expected output with prefix
    expected_lines = {f"[*] Extracted: {line}" for line in expected_output}

    # Create output_file path without suffix (default extract behavior is MPQ without extension)
    output_file = output_dir.with_suffix("")

    # Create output_files set based on directory contents (not full path)
    output_files = set(fi.name for fi in output_file.glob("*"))

    assert result.returncode == 1, f"mpqcli failed with error: {result.stderr}"
    assert output_lines == expected_lines, f"Unexpected output: {output_lines}"
    assert output_file.exists(), "Output directory was not created"
    assert output_files == expected_output, f"Unexpected files: {output_files}"


def test_extract_all_files_from_mpq_without_providing_listfile_and_with_given_locale(binary_path, generate_mpq_without_internal_listfile):
    """
    Test file extraction of all files from MPQ archive with no internal listfile,
    when no external one is provided

    This test checks:
    - That files from MPQs with no internal listfile can still be extracted.
    - That files with the default and the given locale are extracted.
    """
    _ = generate_mpq_without_internal_listfile
    script_dir = Path(__file__).parent
    test_file = script_dir / "data" / "mpq_without_internal_listfile.mpq"
    output_dir = script_dir / "data" / "extracted_file"
    if output_dir.exists():
        shutil.rmtree(output_dir)


    expected_output = {
        "File00000000.xxx",
        "File00000002.xxx",
    }

    result = subprocess.run(
        [str(binary_path), "extract", "-o", str(output_dir), str(test_file), "--locale", "041D"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    output_lines = set(result.stdout.splitlines())

    # Create expected_lines set based on expected output with prefix
    expected_lines = {f"[*] Extracted: {line}" for line in expected_output}

    # Create output_file path without suffix (default extract behavior is MPQ without extension)
    output_file = output_dir.with_suffix("")

    # Create output_files set based on directory contents (not full path)
    output_files = set(fi.name for fi in output_file.glob("*"))

    assert result.returncode == 1, f"mpqcli failed with error: {result.stderr}"
    assert output_lines == expected_lines, f"Unexpected output: {output_lines}"
    assert output_file.exists(), "Output directory was not created"
    assert output_files == expected_output, f"Unexpected files: {output_files}"


def test_extract_single_file_from_mpq_without_providing_listfile(binary_path, generate_mpq_without_internal_listfile):
    """
    Test file extraction for MPQ archive with no internal listfile and without providing an external one

    This test checks:
    - That files from MPQs with no internal listfile can still be extracted.
    """
    _ = generate_mpq_without_internal_listfile
    script_dir = Path(__file__).parent
    test_file = script_dir / "data" / "mpq_without_internal_listfile.mpq"
    output_dir = script_dir / "data" / "extracted_file"
    file_to_extract = "File00000000.xxx"
    if output_dir.exists():
        shutil.rmtree(output_dir)


    expected_output = {
        "File00000000.xxx",
    }

    result = subprocess.run(
        [str(binary_path), "extract", "-o", str(output_dir), "-f", file_to_extract, str(test_file)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    output_lines = set(result.stdout.splitlines())

    # Create expected_lines set based on expected output with prefix
    expected_lines = {f"[*] Extracted: {line}" for line in expected_output}

    # Create output_file path without suffix (default extract behavior is MPQ without extension)
    output_file = output_dir.with_suffix("")

    # Create output_files set based on directory contents (not full path)
    output_files = set(fi.name for fi in output_file.glob("*"))

    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    assert output_lines == expected_lines, f"Unexpected output: {output_lines}"
    assert output_file.exists(), "Output directory was not created"
    assert output_files == expected_output, f"Unexpected files: {output_files}"


def test_extract_all_files_from_mpq_providing_partial_external_listfile(binary_path, generate_mpq_without_internal_listfile):
    """
    Test file extraction of all files from MPQ archive containing no internal listfile,
    when providing a partially complete external listfile

    This test checks:
    - That files from MPQs with no internal listfile can still be extracted.
    - That files with different locales than the default are not extracted.
    """
    _ = generate_mpq_without_internal_listfile
    script_dir = Path(__file__).parent
    test_file = script_dir / "data" / "mpq_without_internal_listfile.mpq"
    output_dir = script_dir / "data" / "extracted_file"
    if output_dir.exists():
        shutil.rmtree(output_dir)
    listfile = script_dir / "data" / "listfile.txt"
    listfile.write_text("capybaras.txt")


    expected_output = {
        "capybaras.txt",
    }

    result = subprocess.run(
        [str(binary_path), "extract", "-o", str(output_dir), "-l", listfile, str(test_file)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    output_lines = set(result.stdout.splitlines())

    # Create expected_lines set based on expected output with prefix
    expected_lines = {f"[*] Extracted: {line}" for line in expected_output}

    # Create output_file path without suffix (default extract behavior is MPQ without extension)
    output_file = output_dir.with_suffix("")

    # Create output_files set based on directory contents (not full path)
    output_files = set(fi.name for fi in output_file.glob("*"))

    assert result.returncode == 1, f"mpqcli failed with error: {result.stderr}"
    assert output_lines == expected_lines, f"Unexpected output: {output_lines}"
    assert output_file.exists(), "Output directory was not created"
    assert output_files == expected_output, f"Unexpected files: {output_files}"


def test_extract_all_files_from_mpq_providing_partial_external_listfile_and_with_given_locale(binary_path, generate_mpq_without_internal_listfile):
    """
    Test file extraction of all files from MPQ archive containing no internal listfile,
    when providing a partially complete external listfile

    This test checks:
    - That files from MPQs with no internal listfile can still be extracted.
    - That files with the default and the given locale are extracted.
    """
    _ = generate_mpq_without_internal_listfile
    script_dir = Path(__file__).parent
    test_file = script_dir / "data" / "mpq_without_internal_listfile.mpq"
    output_dir = script_dir / "data" / "extracted_file"
    if output_dir.exists():
        shutil.rmtree(output_dir)
    listfile = script_dir / "data" / "listfile.txt"
    listfile.write_text("cats.txt")


    expected_output = {
        "File00000000.xxx",
        "cats.txt",
    }

    result = subprocess.run(
        [str(binary_path), "extract", "-o", str(output_dir), "-l", listfile, str(test_file), "--locale", "deDE"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    output_lines = set(result.stdout.splitlines())

    # Create expected_lines set based on expected output with prefix
    expected_lines = {f"[*] Extracted: {line}" for line in expected_output}

    # Create output_file path without suffix (default extract behavior is MPQ without extension)
    output_file = output_dir.with_suffix("")

    # Create output_files set based on directory contents (not full path)
    output_files = set(fi.name for fi in output_file.glob("*"))

    assert result.returncode == 1, f"mpqcli failed with error: {result.stderr}"
    assert output_lines == expected_lines, f"Unexpected output: {output_lines}"
    assert output_file.exists(), "Output directory was not created"
    assert output_files == expected_output, f"Unexpected files: {output_files}"


def test_extract_all_files_from_mpq_providing_complete_external_listfile(binary_path, generate_mpq_without_internal_listfile):
    """
    Test file extraction of all files from MPQ archive containing no internal listfile,
    when providing a complete external listfile

    This test checks:
    - That files from MPQs with no internal listfile can still be extracted.
    - That files with different locales than the default are not extracted.
    """
    _ = generate_mpq_without_internal_listfile
    script_dir = Path(__file__).parent
    test_file = script_dir / "data" / "mpq_without_internal_listfile.mpq"
    output_dir = script_dir / "data" / "extracted_file"
    if output_dir.exists():
        shutil.rmtree(output_dir)
    listfile = script_dir / "data" / "listfile.txt"
    listfile.write_text("cats.txt\ndogs.txt\ncapybaras.txt")


    expected_output = {
        "capybaras.txt",
    }

    result = subprocess.run(
        [str(binary_path), "extract", "-o", str(output_dir), "-l", listfile, str(test_file)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    output_lines = set(result.stdout.splitlines())

    # Create expected_lines set based on expected output with prefix
    expected_lines = {f"[*] Extracted: {line}" for line in expected_output}

    # Create output_file path without suffix (default extract behavior is MPQ without extension)
    output_file = output_dir.with_suffix("")

    # Create output_files set based on directory contents (not full path)
    output_files = set(fi.name for fi in output_file.glob("*"))

    assert result.returncode == 1, f"mpqcli failed with error: {result.stderr}"
    assert output_lines == expected_lines, f"Unexpected output: {output_lines}"
    assert output_file.exists(), "Output directory was not created"
    assert output_files == expected_output, f"Unexpected files: {output_files}"


def test_extract_all_files_from_mpq_providing_complete_external_listfile_and_with_given_locale(binary_path, generate_mpq_without_internal_listfile):
    """
    Test file extraction of all files from MPQ archive containing no internal listfile,
    when providing a complete external listfile

    This test checks:
    - That files from an MPQs with no internal listfile can still be extracted.
    - That files with the default and the given locale are extracted.
    """
    _ = generate_mpq_without_internal_listfile
    script_dir = Path(__file__).parent
    test_file = script_dir / "data" / "mpq_without_internal_listfile.mpq"
    output_dir = script_dir / "data" / "extracted_file"
    if output_dir.exists():
        shutil.rmtree(output_dir)
    listfile = script_dir / "data" / "listfile.txt"
    listfile.write_text("cats.txt\ndogs.txt\ncapybaras.txt")


    expected_output = {
        "dogs.txt",
        "capybaras.txt",
    }

    result = subprocess.run(
        [str(binary_path), "extract", "-o", str(output_dir), "-l", listfile, str(test_file), "--locale", "041D"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    output_lines = set(result.stdout.splitlines())

    # Create expected_lines set based on expected output with prefix
    expected_lines = {f"[*] Extracted: {line}" for line in expected_output}

    # Create output_file path without suffix (default extract behavior is MPQ without extension)
    output_file = output_dir.with_suffix("")

    # Create output_files set based on directory contents (not full path)
    output_files = set(fi.name for fi in output_file.glob("*"))

    assert result.returncode == 1, f"mpqcli failed with error: {result.stderr}"
    assert output_lines == expected_lines, f"Unexpected output: {output_lines}"
    assert output_file.exists(), "Output directory was not created"
    assert output_files == expected_output, f"Unexpected files: {output_files}"


def test_extract_path_traversal_is_blocked(binary_path, generate_path_traversal_mpq):
    """
    Test that extracting an MPQ containing a path traversal filename is blocked (zip-slip).

    This test checks:
    - That the traversal entry is not written outside the output directory.
    - That the extraction of the traversal entry is reported as blocked on stderr.
    - That safe files in the same archive are still extracted correctly.
    - That the overall exit code is non-zero.
    """
    test_file = generate_path_traversal_mpq
    script_dir = Path(__file__).parent
    output_dir = script_dir / "data" / "extracted_traversal"
    if output_dir.exists():
        shutil.rmtree(output_dir)

    result = subprocess.run(
        [str(binary_path), "extract", "-o", str(output_dir), str(test_file)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    expected_stdout = {
        "[*] Extracted: safe.txt",
        "[*] Extracted: (listfile)",
    }
    expected_stderr = {
        "[!] Blocked: path traversal attempt detected: " + os.path.normpath("../../sneaky.txt"),
        "",
        "[!] Failed to extract all files.",
    }

    stdout_lines = set(result.stdout.splitlines())
    stderr_lines = set(result.stderr.splitlines())

    assert result.returncode == 1, f"Expected non-zero exit for traversal attempt, got: {result.returncode}"
    assert stdout_lines == expected_stdout, f"Unexpected stdout: {stdout_lines}"
    assert stderr_lines == expected_stderr, f"Unexpected stderr: {stderr_lines}"

    # Confirm no file escaped outside the intended output directory
    assert not (output_dir.parent / "sneaky.txt").exists(), "Path traversal was not blocked: sneaky.txt escaped"


def test_extract_mpq_with_nonexistent_listfile(binary_path, generate_test_files):
    _ = generate_test_files
    script_dir = Path(__file__).parent
    test_file = script_dir / "data" / "mpq_with_output_v1.mpq"
    listfile = script_dir / "does" / "not" / "exist.txt"

    result = subprocess.run(
        [str(binary_path), "extract", str(test_file), "--listfile", str(listfile)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    assert result.returncode == 105, f"mpqcli failed with error: {result.stderr}"


def test_extract_nested_file_from_mpq(binary_path, generate_test_files):
    _ = generate_test_files
    script_dir = Path(__file__).parent
    archive_file = script_dir / "data" / "files.mpq"
    output_dir = script_dir / "data" / "extracted_nested"

    if output_dir.exists():
        shutil.rmtree(output_dir)

    # Create a fresh MPQ archive
    target_dir = script_dir / "data" / "files"
    archive_file.unlink(missing_ok=True)
    result = subprocess.run(
        [str(binary_path), "create", "--version", "1", str(target_dir), "-o", str(archive_file)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    assert result.returncode == 0, f"Failed to create archive: {result.stderr}"

    # Add a file at a nested path
    test_file = script_dir / "data" / "files" / "cats.txt"
    nested_path = "texts\\cats.txt"
    result = subprocess.run(
        [str(binary_path), "add", str(archive_file), str(test_file), "--path", nested_path],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    assert result.returncode == 0, f"Failed to add nested file: {result.stderr}"

    # Extract the nested file
    result = subprocess.run(
        [str(binary_path), "extract", "-f", nested_path, "-o", str(output_dir), str(archive_file)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    expected_stdout = {"[*] Extracted: cats.txt"}
    output_lines = set(result.stdout.splitlines())

    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    assert output_lines == expected_stdout, f"Unexpected output: {output_lines}"

    extracted_file = output_dir / "cats.txt"
    assert extracted_file.exists(), "Extracted nested file does not exist"
    assert extracted_file.read_text(encoding="utf-8") == "This is a file about cats.\n", \
        "Unexpected content in extracted nested file"
