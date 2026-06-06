import subprocess
from pathlib import Path

from buildtools.errors import ToolNotFoundError
from buildtools.providers.base import ToolProvider
from buildtools.shell import Shell
from buildtools.venv_manager import VenvManager

# Base style for clang-format config generation
CLANG_FORMAT_BASE_STYLE = "LLVM"


class ClangFormatProvider(ToolProvider):

    def __init__(self, shell: Shell, venv_mgr: VenvManager,
                 project_dir: Path):
        super().__init__(shell)
        self.venv_mgr = venv_mgr
        self.project_dir = project_dir

    def ensure(self) -> Path:
        # system clang-format
        found = self.shell.which("clang-format")
        if found:
            return Path(found)

        # venv clang-format
        venv_cf = self.venv_mgr.find_executable("clang-format")
        if venv_cf:
            return venv_cf

        # install into venv
        print("clang-format not found. Installing into venv...")
        self.venv_mgr.install("clang-format")

        venv_cf = self.venv_mgr.find_executable("clang-format")
        if venv_cf:
            return venv_cf

        raise ToolNotFoundError("clang-format installation into venv failed")

    def ensure_config(self) -> Path:
        """Generate .clang-format in the project root if it doesn't exist."""
        config_path = self.project_dir / ".clang-format"
        if config_path.exists():
            return config_path

        if self.shell.dry_run:
            print(f"[dry-run] would generate {config_path} "
                  f"(base: {CLANG_FORMAT_BASE_STYLE})")
            return config_path

        cf_path = self.ensure()
        print(f"Generating .clang-format (base: {CLANG_FORMAT_BASE_STYLE})...")
        result = subprocess.run(
            [str(cf_path), f"-style={CLANG_FORMAT_BASE_STYLE}",
             "--dump-config"],
            capture_output=True, text=True, check=True,
        )
        config_path.write_text(result.stdout, encoding="utf-8")
        print(f"  Created {config_path}")
        return config_path
