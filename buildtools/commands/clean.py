import shutil
import sys
from pathlib import Path

from buildtools.commands.base import Command
from buildtools.config import ProjectConfig
from buildtools.errors import BuildError


class CleanCommand(Command):
    """Removes build artifacts, and on request the vcpkg dependency tree.

    `deps_dir` lives outside the project (see ``project.json``), so it is NOT
    removed by default -- wiping it forces a full source rebuild of Qt.
    """

    name = "clean"
    summary = "Remove build dirs and venv (--deps also removes dependencies)"

    def __init__(self, config: ProjectConfig, remove_deps: bool = False,
                 assume_yes: bool = False):
        self.config = config
        self.remove_deps = remove_deps
        self.assume_yes = assume_yes

    def execute(self) -> None:
        targets: dict[str, Path] = {
            "Build":   self.config.project_dir / "build",
            "Staging": self.config.project_dir / "staging",
            "Dist":    self.config.project_dir / "dist",
            "Venv":    self.config.venv_dir,
        }
        # Ask BEFORE deleting anything: aborting at the prompt must not leave
        # a half-cleaned tree behind.
        if self.remove_deps and self.config.deps_dir.exists():
            if self._confirm_deps(self.config.deps_dir):
                targets["Dependencies"] = self.config.deps_dir
            else:
                print("  Keeping Dependencies.\n")

        removed = []
        failed: dict[str, OSError] = {}
        for label, path in targets.items():
            if not path.exists():
                continue
            if self.config.dry_run:
                print(f"  [dry-run] would remove {label}: {path}")
                continue
            print(f"  Removing {label}: {path}")
            try:
                shutil.rmtree(path)
            except OSError as e:
                # Typically an IDE (CLion's CMake, clangd) holding a build dir
                # open. Clean everything else rather than stopping half-way.
                failed[label] = e
                continue
            removed.append(label)

        if not self.remove_deps:
            print(f"\nKept dependencies: {self.config.deps_dir}")
            print("  (outside the project; pass --deps to remove them)")

        if removed:
            print(f"\nCleaned: {', '.join(removed)}")
        elif not self.config.dry_run and not failed:
            print("\nNothing to clean.")

        if failed:
            details = "\n".join(f"  {label}: {error}"
                                for label, error in failed.items())
            raise BuildError(
                "Could not remove everything -- is an IDE or a running "
                f"Chronexa holding these open?\n{details}")

    def _confirm_deps(self, path: Path) -> bool:
        if self.assume_yes or self.config.dry_run:
            return True
        print(f"\n  WARNING: {path} is outside the project and is shared by")
        print("  every project whose deps_dir points at it. Removing it forces")
        print("  a full source rebuild of Qt and everything else installed there.")
        try:
            if not sys.stdin.isatty():
                raise EOFError
            answer = input("  Remove it? [y/N] ")
        except (EOFError, KeyboardInterrupt):
            # No usable stdin (CI, piped input, IDE terminal). Never guess yes.
            print("\n  No interactive input -- refusing. Re-run with --yes.")
            return False
        return answer.strip().lower() in ("y", "yes")
