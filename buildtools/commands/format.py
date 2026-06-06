from pathlib import Path

from buildtools.commands.base import Command
from buildtools.config import ProjectConfig
from buildtools.providers.clang_format import ClangFormatProvider
from buildtools.shell import Shell


class FormatCommand(Command):
    """Formats C++ sources with clang-format."""

    name = "format"
    summary = "Auto-format C++ source files"

    def __init__(self, config: ProjectConfig, shell: Shell,
                 clang_format: ClangFormatProvider):
        self.config = config
        self.shell = shell
        self.clang_format = clang_format

    def execute(self) -> None:
        cf_path = self.clang_format.ensure()
        sources = self._collect_sources()

        if not sources:
            print("No C++ source files found.")
            return

        print(f"\n=== Formatting {len(sources)} file(s) ===")
        self.shell.run([cf_path, "-i", *sources])
        print("Formatting complete.")

    def _collect_sources(self) -> list[Path]:
        src_dir = self.config.project_dir / "src"
        extensions = (".cpp", ".hpp", ".h", ".cxx", ".cc")
        files: list[Path] = []
        for ext in extensions:
            files.extend(src_dir.rglob(f"*{ext}"))
        return sorted(files)
