import subprocess

def test_version_without_arguments(binary_path):
    """
    Test the version subcommand without arguments.

    This test checks:
    - If the version subcommand succeeds when called without arguments.
    - If the output contains version information.
    """
    result = subprocess.run(
        [str(binary_path), "version"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    assert len(result.stdout.strip()) > 0, "Version output should not be empty"
    # Version output should contain a dash separator (e.g., "1.0.0-abc123")
    assert "-" in result.stdout, f"Version output should contain version-hash format: {result.stdout}"


def test_version_with_arguments(binary_path):
    """
    Test the version subcommand with arguments.

    This test checks:
    - If the version subcommand fails when called with arguments.
    """
    result = subprocess.run(
        [str(binary_path), "version", "extra_argument"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    assert result.returncode != 0, "Version subcommand should fail when given arguments"


def test_about_without_arguments(binary_path):
    """
    Test the about subcommand without arguments.

    This test checks:
    - If the about subcommand succeeds when called without arguments.
    - If the output contains expected information fields.
    """
    result = subprocess.run(
        [str(binary_path), "about"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"

    # Check that the output contains expected fields
    output = result.stdout
    assert "Name:" in output, "About output should contain 'Name:'"
    assert "Version:" in output, "About output should contain 'Version:'"
    assert "Author:" in output, "About output should contain 'Author:'"
    assert "License:" in output, "About output should contain 'License:'"
    assert "GitHub:" in output, "About output should contain 'GitHub:'"
    assert "Dependencies:" in output, "About output should contain 'Dependencies:'"
    assert "StormLib" in output, "About output should mention StormLib"
    assert "CLI11" in output, "About output should mention CLI11"


def test_about_with_arguments(binary_path):
    """
    Test the about subcommand with arguments.

    This test checks:
    - If the about subcommand fails when called with arguments.
    """
    result = subprocess.run(
        [str(binary_path), "about", "extra_argument"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    assert result.returncode != 0, "About subcommand should fail when given arguments"
