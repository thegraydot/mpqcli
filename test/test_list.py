import subprocess
from pathlib import Path
import platform


def test_list_mpq_with_output_v1(binary_path):
    """
    Test MPQ file listing with no parameters.

    This test checks:
    - That running the list command with no parameters renders a list of the files inside, with no details.
    """
    script_dir = Path(__file__).parent
    test_file = script_dir / "data" / "mpq_with_output_v1.mpq"

    expected_output = {
        "bytes",
        "cats.txt",
        "dogs.txt",
    }

    result = subprocess.run(
        [str(binary_path), "list", str(test_file)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    output_lines = set(result.stdout.splitlines())
    assert len(result.stdout.splitlines()) == len(expected_output)
    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    assert output_lines == expected_output, f"Unexpected output: {output_lines}"


def test_list_mpq_with_standard_details(binary_path):
    """
    Test MPQ file listing with the standard details.

    This test checks:
    - That the standard long listing parameters work as expected.
    """
    script_dir = Path(__file__).parent
    test_file = script_dir / "data" / "mpq_with_output_v1.mpq"

    expected_output = {
        "       8 enUS 2025-07-29 14:31:00  bytes",
        "      27 enUS 2025-07-29 14:31:00  cats.txt",
        "      27 enUS 2025-07-29 14:31:00  dogs.txt",
        "      27 enUS                      (listfile)",
        "     149 enUS                      (attributes)",
    }

    # Adjust filesize for Windows
    if platform.system() == "Windows":
        expected_output.remove("      27 enUS 2025-07-29 14:31:00  dogs.txt")
        expected_output.add("      28 enUS 2025-07-29 14:31:00  dogs.txt")
        expected_output.remove("      27 enUS 2025-07-29 14:31:00  cats.txt")
        expected_output.add("      28 enUS 2025-07-29 14:31:00  cats.txt")

    result = subprocess.run(
        [str(binary_path), "list", "-a", "-d", str(test_file)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    output_lines = set(result.stdout.splitlines())
    assert len(result.stdout.splitlines()) == len(expected_output)
    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    assert output_lines == expected_output, f"Unexpected output: {output_lines}"


def test_list_mpq_with_specified_details(binary_path):
    """
    Test MPQ file listing with specified details.

    This test checks:
    - That providing parameters to be listed works as expected.
    """
    script_dir = Path(__file__).parent
    test_file = script_dir / "data" / "mpq_with_output_v1.mpq"

    expected_output = {
        "   35 147178ed c99b9ee2 0000000000000000       16    cexmn eaa753f9  bytes",
        "   44 0fd58937 70ab788e 0000000000000000       35    cexmn 935a7772  cats.txt",
        "    0 eb30456b 48345fbb 0000000000000000       35    cexmn a073c614  dogs.txt",
        "   25 fd657910 4e9b98a7 0000000000000000       35  ce2xmnf 2d2f0a94  (listfile)",
        "   14 d38437cb 07dfeaec 0000000000000000      124  ce2xmnf 50e314af  (attributes)",
    }
    # Adjust filesize for Windows
    if platform.system() == "Windows":
        expected_output.remove("   44 0fd58937 70ab788e 0000000000000000       35    cexmn 935a7772  cats.txt")
        expected_output.add("   44 0fd58937 70ab788e 0000000000000000       36    cexmn 935a7772  cats.txt")
        expected_output.remove("    0 eb30456b 48345fbb 0000000000000000       35    cexmn a073c614  dogs.txt")
        expected_output.add("    0 eb30456b 48345fbb 0000000000000000       36    cexmn a073c614  dogs.txt")

    result = subprocess.run(
        [str(binary_path), "list", "-a", "-d", str(test_file),
         "-p", "hash-index", "-p", "name-hash1", "-p", "name-hash2", "-p", "name-hash3",
         "-p", "compressed-size", "-p", "flags", "-p", "encryption-key-raw"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    output_lines = set(result.stdout.splitlines())
    assert len(result.stdout.splitlines()) == len(expected_output)
    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    assert output_lines == expected_output, f"Unexpected output: {output_lines}"


def test_list_mpq_with_weak_signature(binary_path):
    """
    Test MPQ file listing of MPQ with weak signature.

    This test checks:
    - That handling MPQs with weak signatures generates the expected output.
    """
    script_dir = Path(__file__).parent
    test_file = script_dir / "data" / "mpq_with_weak_signature.mpq"

    expected_output = {
        "bytes",
        "cats.txt",
        "dogs.txt",
        "(listfile)",  # Only included as "-a" specified
        "(signature)",  # Only included as "-a" specified
        "(attributes)",  # Only included as "-a" specified
    }

    result = subprocess.run(
        [str(binary_path), "list", "-a", str(test_file)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    output_lines = set(result.stdout.splitlines())
    assert len(result.stdout.splitlines()) == len(expected_output)
    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    assert output_lines == expected_output, f"Unexpected output: {output_lines}"


def test_list_mpq_without_providing_listfile(binary_path, generate_mpq_without_internal_listfile):
    """
    Test MPQ file listing of MPQ that contains no internal listfile.

    This test checks:
    - That handling MPQs with no internal listfile generates the expected output.
    """
    _ = generate_mpq_without_internal_listfile
    script_dir = Path(__file__).parent
    test_file = script_dir / "data" / "mpq_without_internal_listfile.mpq"

    ## No flags
    expected_output = {
        "File00000000.xxx",
        "File00000001.xxx",
        "File00000002.xxx",
    }
    result = subprocess.run(
        [str(binary_path), "list", str(test_file)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    output_lines = set(result.stdout.splitlines())
    assert len(result.stdout.splitlines()) == len(expected_output)
    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    assert output_lines == expected_output, f"Unexpected output: {output_lines}"

    ## --all flag
    expected_output = {
        "File00000000.xxx",
        "File00000001.xxx",
        "File00000002.xxx",
    }
    result = subprocess.run(
        [str(binary_path), "list", "-a", str(test_file)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    output_lines = set(result.stdout.splitlines())
    assert len(result.stdout.splitlines()) == len(expected_output)
    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    assert output_lines == expected_output, f"Unexpected output: {output_lines}"

    ## --all, --detailed flag
    if platform.system() != "Windows":
        expected_output = {
            "      31 enUS                      File00000000.xxx",
            "      33 deDE                      File00000001.xxx",
            "      27 041D                      File00000002.xxx",
        }
    else:
        expected_output = {
            "      31 enUS                      File00000000.xxx",
            "      32 deDE                      File00000001.xxx",
            "      26 041D                      File00000002.xxx",
        }

    result = subprocess.run(
        [str(binary_path), "list", "-ad", str(test_file)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    output_lines = set(result.stdout.splitlines())
    assert len(result.stdout.splitlines()) == len(expected_output)
    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    assert output_lines == expected_output, f"Unexpected output: {output_lines}"


def test_list_mpq_providing_partial_external_listfile(binary_path, generate_mpq_without_internal_listfile):
    """
    Test MPQ file listing of MPQ that contains no internal listfile, when providing a partially compete external listfile.

    This test checks:
    - That handling MPQs with no internal listfile generates the expected output.
    - That providing a partially complete external listfile shows the files it lists.
    - That the files not listed in the external listfile still show up in the output.
    """
    _ = generate_mpq_without_internal_listfile
    script_dir = Path(__file__).parent
    test_file = script_dir / "data" / "mpq_without_internal_listfile.mpq"
    listfile = script_dir / "data" / "listfile.txt"
    listfile.write_text("cats.txt")

    ## No flags
    expected_output = {
        "File00000000.xxx",
        "File00000002.xxx",
        "cats.txt",
    }
    result = subprocess.run(
        [str(binary_path), "list", str(test_file), "--listfile", str(listfile)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    output_lines = set(result.stdout.splitlines())
    assert len(result.stdout.splitlines()) == len(expected_output)
    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    assert output_lines == expected_output, f"Unexpected output: {output_lines}"

    ## --all flag
    expected_output = {
        "File00000000.xxx",
        "File00000002.xxx",
        "cats.txt",
    }
    result = subprocess.run(
        [str(binary_path), "list", "-a", str(test_file), "--listfile", str(listfile)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    output_lines = set(result.stdout.splitlines())
    assert len(result.stdout.splitlines()) == len(expected_output)
    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    assert output_lines == expected_output, f"Unexpected output: {output_lines}"

    ## --all, --detailed flag
    if platform.system() != "Windows":
        expected_output = {
            "      31 enUS                      File00000000.xxx",
            "      27 041D                      File00000002.xxx",
            "      33 deDE                      cats.txt",
        }
    else:
        expected_output = {
            "      31 enUS                      File00000000.xxx",
            "      26 041D                      File00000002.xxx",
            "      32 deDE                      cats.txt",
        }
    result = subprocess.run(
        [str(binary_path), "list", "-ad", str(test_file), "--listfile", str(listfile)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    output_lines = set(result.stdout.splitlines())
    assert len(result.stdout.splitlines()) == len(expected_output)
    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    assert output_lines == expected_output, f"Unexpected output: {output_lines}"


def test_list_mpq_providing_complete_external_listfile(binary_path, generate_mpq_without_internal_listfile):
    """
    Test MPQ file listing of MPQ that contains no internal listfile, when providing a compete external listfile.

    This test checks:
    - That handling MPQs with no internal listfile generates the expected output.
    - That providing a complete external listfile shows the files it lists.
    """
    _ = generate_mpq_without_internal_listfile
    script_dir = Path(__file__).parent
    test_file = script_dir / "data" / "mpq_without_internal_listfile.mpq"
    listfile = script_dir / "data" / "listfile.txt"
    listfile.write_text("cats.txt\ndogs.txt\ncapybaras.txt")

    ## No flags
    expected_output = {
        "capybaras.txt",
        "cats.txt",
        "dogs.txt",
    }
    result = subprocess.run(
        [str(binary_path), "list", str(test_file), "--listfile", str(listfile)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    output_lines = set(result.stdout.splitlines())
    assert len(result.stdout.splitlines()) == len(expected_output)
    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    assert output_lines == expected_output, f"Unexpected output: {output_lines}"

    ## --all flag
    expected_output = {
        "capybaras.txt",
        "cats.txt",
        "dogs.txt",
    }
    result = subprocess.run(
        [str(binary_path), "list", "-a", str(test_file), "--listfile", str(listfile)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    output_lines = set(result.stdout.splitlines())
    assert len(result.stdout.splitlines()) == len(expected_output)
    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    assert output_lines == expected_output, f"Unexpected output: {output_lines}"

    ## --all, --detailed flag
    if platform.system() != "Windows":
        expected_output = {
            "      31 enUS                      capybaras.txt",
            "      33 deDE                      cats.txt",
            "      27 041D                      dogs.txt",
        }
    else:
        expected_output = {
            "      31 enUS                      capybaras.txt",
            "      32 deDE                      cats.txt",
            "      26 041D                      dogs.txt",
        }

    result = subprocess.run(
        [str(binary_path), "list", "-ad", str(test_file), "--listfile", str(listfile)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    output_lines = set(result.stdout.splitlines())
    assert len(result.stdout.splitlines()) == len(expected_output)
    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    assert output_lines == expected_output, f"Unexpected output: {output_lines}"


def test_list_mpq_target_does_not_exist(binary_path):
    script_dir = Path(__file__).parent
    target_file = script_dir / "does" / "not" / "exist.mpq"

    result = subprocess.run(
        [str(binary_path), "list", str(target_file)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    assert result.returncode == 105, f"mpqcli failed with error: {result.stderr}"


def test_list_mpq_with_nonexistent_listfile(binary_path):
    script_dir = Path(__file__).parent
    test_file = script_dir / "data" / "mpq_with_output_v1.mpq"
    listfile = script_dir / "does" / "not" / "exist.txt"

    result = subprocess.run(
        [str(binary_path), "list", str(test_file), "--listfile", str(listfile)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    assert result.returncode == 105, f"mpqcli failed with error: {result.stderr}"


def test_list_mpq_property_file_index(binary_path):
    script_dir = Path(__file__).parent
    test_file = script_dir / "data" / "mpq_with_output_v1.mpq"

    expected_output = {
        "    0  bytes",
        "    1  cats.txt",
        "    2  dogs.txt",
        "    3  (listfile)",
        "    4  (attributes)",
    }

    result = subprocess.run(
        [str(binary_path), "list", "-a", "-d", str(test_file), "-p", "file-index"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    output_lines = set(result.stdout.splitlines())
    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    assert output_lines == expected_output, f"Unexpected output: {output_lines}"


def test_list_mpq_property_byte_offset(binary_path):
    script_dir = Path(__file__).parent
    test_file = script_dir / "data" / "mpq_with_output_v1.mpq"

    expected_output = {
        "      20  bytes",
        "      30  cats.txt",
        "      53  dogs.txt",
        "      76  (listfile)",
        "      99  (attributes)",
    }
    if platform.system() == "Windows":
        expected_output.discard("      53  dogs.txt")
        expected_output.add("      54  dogs.txt")
        expected_output.discard("      76  (listfile)")
        expected_output.add("      78  (listfile)")
        expected_output.discard("      99  (attributes)")
        expected_output.add("      9b  (attributes)")

    result = subprocess.run(
        [str(binary_path), "list", "-a", "-d", str(test_file), "-p", "byte-offset"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    output_lines = set(result.stdout.splitlines())
    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    assert output_lines == expected_output, f"Unexpected output: {output_lines}"


def test_list_mpq_property_file_time(binary_path):
    script_dir = Path(__file__).parent
    test_file = script_dir / "data" / "mpq_with_output_v1.mpq"

    expected_output = {
        "2025-07-29 14:31:00  bytes",
        "2025-07-29 14:31:00  cats.txt",
        "2025-07-29 14:31:00  dogs.txt",
        "                     (listfile)",
        "                     (attributes)",
    }

    result = subprocess.run(
        [str(binary_path), "list", "-a", "-d", str(test_file), "-p", "file-time"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    output_lines = set(result.stdout.splitlines())
    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    assert output_lines == expected_output, f"Unexpected output: {output_lines}"


def test_list_mpq_property_encryption_key(binary_path):
    script_dir = Path(__file__).parent
    test_file = script_dir / "data" / "mpq_with_output_v1.mpq"

    expected_output = {
        "eaa753f9  bytes",
        "935a7772  cats.txt",
        "a073c614  dogs.txt",
        "2d2f0b11  (listfile)",
        "50e315dd  (attributes)",
    }
    if platform.system() == "Windows":
        expected_output.discard("2d2f0b11  (listfile)")
        expected_output.add("2d2f0b17  (listfile)")
        expected_output.discard("50e315dd  (attributes)")
        expected_output.add("50e315df  (attributes)")

    result = subprocess.run(
        [str(binary_path), "list", "-a", "-d", str(test_file), "-p", "encryption-key"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    output_lines = set(result.stdout.splitlines())
    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    assert output_lines == expected_output, f"Unexpected output: {output_lines}"


def test_list_mpq_property_locale(binary_path):
    script_dir = Path(__file__).parent
    test_file = script_dir / "data" / "mpq_with_output_v1.mpq"

    expected_output = {
        "enUS  bytes",
        "enUS  cats.txt",
        "enUS  dogs.txt",
        "enUS  (listfile)",
        "enUS  (attributes)",
    }

    result = subprocess.run(
        [str(binary_path), "list", "-a", "-d", str(test_file), "-p", "locale"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    output_lines = set(result.stdout.splitlines())
    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    assert output_lines == expected_output, f"Unexpected output: {output_lines}"
