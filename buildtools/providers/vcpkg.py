import os
from pathlib import Path

from buildtools.config import ProjectConfig
from buildtools.providers.base import ToolProvider
from buildtools.providers.git import GitProvider
from buildtools.shell import Shell


class VcpkgProvider(ToolProvider):

    def __init__(self, shell: Shell, config: ProjectConfig, git: GitProvider):
        super().__init__(shell)
        self.config = config
        self.git = git

    def ensure(self) -> Path:
        # already on PATH
        found = self.shell.which("vcpkg")
        if found:
            self._set_root(Path(found).parent)
            return Path(found)

        # already cloned
        candidate = self.config.vcpkg_executable()
        if candidate.exists():
            self._set_root(self.config.vcpkg_dir)
            return candidate

        # clone & bootstrap
        self.git.ensure()
        self._clone_and_bootstrap()
        return candidate

    def _clone_and_bootstrap(self) -> None:
        vcpkg_dir = self.config.vcpkg_dir
        print(f"Cloning vcpkg into {vcpkg_dir}...")
        self.shell.run([
            "git", "clone",
            "https://github.com/microsoft/vcpkg.git", str(vcpkg_dir),
        ])

        script = "bootstrap-vcpkg.bat" if self.config.is_windows else "bootstrap-vcpkg.sh"
        self.shell.run([vcpkg_dir / script, "-disableMetrics"])
        self._set_root(vcpkg_dir)

    @staticmethod
    def _set_root(path: Path) -> None:
        os.environ["VCPKG_ROOT"] = str(path)
