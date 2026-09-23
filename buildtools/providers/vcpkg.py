import os
from pathlib import Path

from buildtools.config import ProjectConfig
from buildtools.errors import BuildError
from buildtools.providers.base import ToolProvider
from buildtools.providers.git import GitProvider
from buildtools.shell import Shell

# Relative location of the CMake toolchain inside any vcpkg checkout.
TOOLCHAIN_RELPATH = Path("scripts") / "buildsystems" / "vcpkg.cmake"


class VcpkgProvider(ToolProvider):

    def __init__(self, shell: Shell, config: ProjectConfig, git: GitProvider):
        super().__init__(shell)
        self.config = config
        self.git = git
        self._root: Path | None = None

    @property
    def root(self) -> Path:
        """Root of the vcpkg checkout that ensure() actually resolved.

        This is NOT always config.vcpkg_dir -- a vcpkg already on PATH wins.
        """
        if self._root is None:
            raise BuildError("vcpkg root requested before VcpkgProvider.ensure()")
        return self._root

    @property
    def toolchain(self) -> Path:
        """Toolchain file of the resolved vcpkg (single source of truth)."""
        return self.root / TOOLCHAIN_RELPATH

    def ensure(self) -> Path:
        # Already on PATH -- but only usable if it is a real checkout. Some
        # vcpkg.exe on PATH (e.g. the Visual Studio shim) has no scripts/
        # tree next to it, and adopting its parent as VCPKG_ROOT would hand
        # CMake a toolchain path that does not exist.
        found = self.shell.which("vcpkg")
        if found and self._is_checkout(Path(found).parent):
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
    def _is_checkout(path: Path) -> bool:
        return (path / TOOLCHAIN_RELPATH).exists()

    def _set_root(self, path: Path) -> None:
        self._root = path
        os.environ["VCPKG_ROOT"] = str(path)
