import shutil

from buildtools.commands.base import Command
from buildtools.config import ProjectConfig


class CleanCommand(Command):
    """Removes all installed dependencies and build artifacts."""

    name = "clean"
    summary = "Remove dependencies, build dirs, and venv"

    def __init__(self, config: ProjectConfig):
        self.config = config

    def execute(self) -> None:
        dirs = {
            "Build":        self.config.project_dir / "build",
            "Staging":      self.config.project_dir / "staging",
            "Dist":         self.config.project_dir / "dist",
            "Venv":         self.config.venv_dir,
            "Dependencies": self.config.deps_dir,
        }

        removed = []
        for label, path in dirs.items():
            if path.exists():
                print(f"  Removing {label}: {path}")
                shutil.rmtree(path)
                removed.append(label)

        if removed:
            print(f"\nCleaned: {', '.join(removed)}")
        else:
            print("Nothing to clean.")
