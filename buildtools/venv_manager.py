import venv
from pathlib import Path

from buildtools.config import ProjectConfig
from buildtools.shell import Shell


class VenvManager:
    """Creates and manages a project-local virtual environment."""

    def __init__(self, config: ProjectConfig, shell: Shell):
        self.config = config
        self.shell = shell

    def ensure(self) -> None:
        if self.config.venv_python.exists():
            return
        if self.shell.dry_run:
            print(f"[dry-run] would create virtual environment in "
                  f"{self.config.venv_dir}")
            return
        print(f"Creating virtual environment in {self.config.venv_dir}...")
        venv.create(str(self.config.venv_dir), with_pip=True)

    def install(self, *packages: str) -> None:
        self.ensure()
        self.shell.run([
            self.config.venv_python, "-m", "pip",
            "install", "--quiet", *packages,
        ])

    def find_executable(self, name: str) -> Path | None:
        candidate = self.config.venv_executable(name)
        return candidate if candidate.exists() else None
