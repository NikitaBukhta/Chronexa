import shutil
import subprocess
from pathlib import Path


class Shell:
    """Runs shell commands with logging."""

    def __init__(self, dry_run: bool = False):
        # When True, run() only prints commands instead of executing them.
        # Read-only queries (which/get_output) still run so control flow
        # can be resolved.
        self.dry_run = dry_run

    def run(self, cmd: list[str | Path],
            env: dict[str, str] | None = None) -> subprocess.CompletedProcess[bytes]:
        str_cmd = [str(c) for c in cmd]
        if self.dry_run:
            print(f"[dry-run] >>> {' '.join(str_cmd)}")
            return subprocess.CompletedProcess(str_cmd, 0)
        print(f">>> {' '.join(str_cmd)}")
        return subprocess.run(str_cmd, check=True, env=env)

    @staticmethod
    def which(name: str) -> str | None:
        return shutil.which(name)

    def get_output(self, cmd: list[str | Path],
                   timeout: int = 5) -> str | None:
        """Run a command and return its first stdout line, or None on failure."""
        try:
            result = subprocess.run(
                [str(c) for c in cmd],
                capture_output=True, text=True, timeout=timeout,
            )
            lines = result.stdout.strip().splitlines()
            return lines[0] if lines else None
        except (subprocess.SubprocessError, FileNotFoundError, OSError):
            return None
