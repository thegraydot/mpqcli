import hashlib
from pathlib import Path
import subprocess


def test_verify_no_signature(binary_path):
    """
    Test MPQ file verification with no signature.

    This test checks:
    - If the application exits correctly when the MPQ file has no signature.
    - If the failure message distinguishes "no signature" from other failures.
    """
    script_dir = Path(__file__).parent
    test_file = script_dir / "data" / "mpq_with_output_v1.mpq"

    expected_output = {
        "[!] Verify failed: archive has no signature",
    }

    result = subprocess.run(
        [str(binary_path), "verify", str(test_file)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    stdout_lines = set(result.stdout.splitlines())
    stderr_lines = set(result.stderr.splitlines())

    assert result.returncode == 1, f"mpqcli failed with error: {result.stderr}"
    assert stdout_lines == set(), f"Unexpected stdout: {stdout_lines}"
    assert stderr_lines == expected_output, f"Unexpected stderr: {stderr_lines}"


def test_verify_tampered_signature(binary_path, generate_tampered_signature_mpq):
    """
    Test MPQ file verification with a signature that is present but invalid.

    This test checks:
    - If the application detects an archive that was modified after signing
      (signature present, but no longer valid for the current content).
    - If the application exits with a non-zero code.
    - If the failure message distinguishes "invalid signature" from the
      "no signature" case, instead of printing the same generic failure.
    """
    test_file = generate_tampered_signature_mpq

    expected_output = {
        "[!] Verify failed: signature is present but invalid",
    }

    result = subprocess.run(
        [str(binary_path), "verify", str(test_file)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    stdout_lines = set(result.stdout.splitlines())
    stderr_lines = set(result.stderr.splitlines())

    assert result.returncode == 1, f"mpqcli unexpectedly succeeded: {result.stdout}"
    assert stdout_lines == set(), f"Unexpected stdout: {stdout_lines}"
    assert stderr_lines == expected_output, f"Unexpected stderr: {stderr_lines}"


def test_verify_weak_signature(binary_path):
    """
    Test MPQ file verification with weak signature.

    This test checks:
    - If the application exits with 0 when the MPQ file has a valid weak signature.
    - If the application prints the success message.
    """
    script_dir = Path(__file__).parent
    test_file = script_dir / "data" / "mpq_with_weak_signature.mpq"

    expected_output = {
        "[*] Verify success",
    }

    result = subprocess.run(
        [str(binary_path), "verify", str(test_file)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    output_lines = set(result.stdout.splitlines())

    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    assert output_lines == expected_output, f"Unexpected output: {output_lines}"


def test_verify_weak_signature_patch(binary_path, download_test_files):
    """
    Test MPQ file verification with and printing of weak signature.

    This test checks:
    - If the application exits correctly when the MPQ file has a weak signature.
    - If the application correctly prints a weak signature.
    """
    _ = download_test_files
    script_dir = Path(__file__).parent
    test_file = script_dir / "data" / "v1_weak_signature_patch.mpq"

    # For this test, we calculate the MD5 sum of the signature output
    # This is because we cannot compare binary output
    expected_md5 = "e434b57cdc6b730c17e18f2bb2fa8fd9"

    result = subprocess.run(
        [str(binary_path), "verify", "-p", str(test_file)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=False  # Get raw bytes to avoid UTF decode errors
    )

    output_md5 = hashlib.md5(result.stdout).hexdigest()

    # Decode stderr as text for error messages
    stderr_text = result.stderr.decode('utf-8', errors='replace')

    assert result.returncode == 0, f"mpqcli failed with error: {stderr_text}"
    assert output_md5 == expected_md5, f"Expected MD5: {expected_md5}, got: {output_md5}"


def test_verify_strong_signature_patch(binary_path, download_test_files):
    """
    Test MPQ file verification with and printing of strong signature.

    This test checks:
    - If the application exits correctly when the MPQ file has a strong signature.
    - If the application correctly prints a strong signature.
    """
    _ = download_test_files
    script_dir = Path(__file__).parent
    test_file = script_dir / "data" / "v2_strong_signature_patch.mpq"

    # For this test, we calculate the MD5 sum of the signature output
    # This is because we cannot compare binary output
    expected_md5 = "931fc40bb6bff48b89d39b5acb169bbd"

    result = subprocess.run(
        [str(binary_path), "verify", "-p", str(test_file)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=False  # Get raw bytes to avoid UTF decode errors
    )

    output_md5 = hashlib.md5(result.stdout).hexdigest()

    # Decode stderr as text for error messages
    stderr_text = result.stderr.decode('utf-8', errors='replace')

    assert result.returncode == 0, f"mpqcli failed with error: {stderr_text}"
    assert output_md5 == expected_md5, f"Expected MD5: {expected_md5}, got: {output_md5}"


def test_verify_mpq_target_does_not_exist(binary_path):
    script_dir = Path(__file__).parent
    target_file = script_dir / "does" / "not" / "exist.mpq"

    result = subprocess.run(
        [str(binary_path), "verify", str(target_file)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    assert result.returncode == 105, f"mpqcli failed with error: {result.stderr}"
