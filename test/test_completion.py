import subprocess
from pathlib import Path


def test_completion_bash(binary_path):
    result = subprocess.run(
        [str(binary_path), "completion", "bash"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    assert "_mpqcli()" in result.stdout, "Bash completion function not found in output"
    assert "complete -F _mpqcli mpqcli" in result.stdout, "Bash complete registration not found in output"


def test_completion_zsh(binary_path):
    result = subprocess.run(
        [str(binary_path), "completion", "zsh"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    assert "#compdef mpqcli" in result.stdout, "Zsh compdef directive not found in output"
    assert "_mpqcli()" in result.stdout, "Zsh completion function not found in output"


def test_completion_bash_output_size(binary_path):
    result = subprocess.run(
        [str(binary_path), "completion", "bash"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    script_path = Path(__file__).parent.parent / "src" / "completion" / "mpqcli.bash"
    assert len(result.stdout) == len(script_path.read_text())


def test_completion_zsh_output_size(binary_path):
    result = subprocess.run(
        [str(binary_path), "completion", "zsh"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    script_path = Path(__file__).parent.parent / "src" / "completion" / "mpqcli.zsh"
    assert len(result.stdout) == len(script_path.read_text())


def test_completion_powershell(binary_path):
    result = subprocess.run(
        [str(binary_path), "completion", "powershell"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    assert "Register-ArgumentCompleter" in result.stdout, "PS completer registration not found in output"
    assert "mpqcli" in result.stdout, "mpqcli reference not found in output"


def test_completion_powershell_output_size(binary_path):
    result = subprocess.run(
        [str(binary_path), "completion", "powershell"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    script_path = Path(__file__).parent.parent / "src" / "completion" / "mpqcli.ps1"
    assert len(result.stdout) == len(script_path.read_text())


def test_completion_fish(binary_path):
    result = subprocess.run(
        [str(binary_path), "completion", "fish"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    assert "complete -c mpqcli" in result.stdout, "Fish complete directive not found in output"


def test_completion_fish_output_size(binary_path):
    result = subprocess.run(
        [str(binary_path), "completion", "fish"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    assert result.returncode == 0, f"mpqcli failed with error: {result.stderr}"
    script_path = Path(__file__).parent.parent / "src" / "completion" / "mpqcli.fish"
    assert len(result.stdout) == len(script_path.read_text())


def test_completion_no_shell(binary_path):
    result = subprocess.run(
        [str(binary_path), "completion"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    assert result.returncode != 0, "Expected non-zero exit code when no shell is specified"


def test_completion_invalid_shell(binary_path):
    result = subprocess.run(
        [str(binary_path), "completion", "nushell"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    assert result.returncode != 0, "Expected non-zero exit code for unsupported shell"
    assert result.stderr, "Expected error output for unsupported shell"
