import random
import shutil
from pathlib import Path
import subprocess


def test_compact_nonexistent_file(binary_path):
    """
    Test compacting non-existent MPQ file.

    This test checks:
    - If the application exits correctly.
    """
    script_dir = Path(__file__).parent
    test_file = script_dir / "data" / "non_existent_file.mpq"

    result = subprocess.run(
        [str(binary_path), "compact", str(test_file)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    assert result.returncode != 1, f"mpqcli failed with error: {result.stderr}"


def test_compact_nonmpq_file(binary_path):
    """
    Test compacting illegal MPQ file.

    This test checks:
    - If the application exits correctly.
    """
    script_dir = Path(__file__).parent
    test_file = script_dir / "data" / "cats.txt"

    expected_prefix = "[!] Failed to open MPQ archive"

    result = subprocess.run(
        [str(binary_path), "compact", str(test_file)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    output_lines = result.stderr.splitlines()

    assert result.returncode == 1, f"mpqcli failed with error: {result.stderr}"
    assert len(output_lines) == 1, f"Unexpected output: {output_lines}"
    assert output_lines[0].startswith(expected_prefix), f"Unexpected output: {output_lines}"


def test_compact_file(binary_path):
    """
    Test compacting an MPQ archive.

    This test checks:
    - Creating an archive with a small file and a ~512 KB incompressible file
    - Removing the larger file from the archive
    - Compacting the archive
    - Verifying that the archive size shrinks significantly afterwards
    """
    script_dir = Path(__file__).parent
    data_dir = script_dir / "data"

    compaction_files_dir = data_dir / "compaction_files"
    shutil.rmtree(compaction_files_dir, ignore_errors=True)
    compaction_files_dir.mkdir(parents=True, exist_ok=True)

    mpq_file = data_dir / "mpq_for_compaction.mpq"
    mpq_file.unlink(missing_ok=True)

    small_file = compaction_files_dir / "small.txt"
    small_file.write_text("This is a small text file.\n", newline="\n")

    large_file_name = "large.bin"
    large_file = compaction_files_dir / large_file_name
    large_file_size = 512 * 1024
    large_file.write_bytes(random.Random(0).randbytes(large_file_size))

    result = subprocess.run(
        [str(binary_path), "create", "-o", str(mpq_file), str(compaction_files_dir)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    assert result.returncode == 0, f"create failed: {result.stderr}"

    result = subprocess.run(
        [str(binary_path), "remove", str(mpq_file), large_file_name],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    assert result.returncode == 0, f"remove failed: {result.stderr}"

    size_before = mpq_file.stat().st_size

    result = subprocess.run(
        [str(binary_path), "compact", str(mpq_file)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    assert result.returncode == 0, f"compact failed: {result.stderr}"

    size_after = mpq_file.stat().st_size

    assert size_before - size_after > size_before / 2, (
        f"Expected compaction to shrink the archive significantly, "
        f"but size went from {size_before} bytes to {size_after} bytes"
    )
