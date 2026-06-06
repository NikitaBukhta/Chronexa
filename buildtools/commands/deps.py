from buildtools.commands.base import Command
from buildtools.config import ProjectConfig
from buildtools.providers.clang_format import ClangFormatProvider
from buildtools.providers.vcpkg import VcpkgProvider
from buildtools.shell import Shell
from buildtools.venv_manager import VenvManager


class DepsCommand(Command):
    """Ensures tooling and vcpkg are present WITHOUT running CMake.

    This is the dependency-only half of `bootstrap`. It exists so that CMake
    itself (cmake/AutoBootstrap.cmake) can invoke it during configuration to
    make a fresh checkout build out-of-the-box in any IDE. It deliberately
    does NOT run `cmake configure` -- that would recurse when called from
    within a configure run. The C++ libraries themselves are installed by
    vcpkg manifest mode during that same configure (driven by vcpkg.json).
    """

    name = "deps"
    summary = "Ensure venv, vcpkg, and clang-format (no CMake configure)"

    def __init__(self, config: ProjectConfig, shell: Shell,
                 venv_mgr: VenvManager, vcpkg: VcpkgProvider,
                 clang_format: ClangFormatProvider):
        self.config = config
        self.shell = shell
        self.venv_mgr = venv_mgr
        self.vcpkg = vcpkg
        self.clang_format = clang_format

    def ensure(self) -> None:
        """Ensure venv, vcpkg, and clang-format config exist.

        Shared by `bootstrap` so both entry points run identical logic.
        """
        self.venv_mgr.ensure()
        self.vcpkg.ensure()
        self.clang_format.ensure_config()

    def execute(self) -> None:
        self.ensure()

        # Machine-readable markers consumed by cmake/AutoBootstrap.cmake.
        toolchain = self.config.vcpkg_toolchain
        print(f"CHRONEXA_VCPKG_TOOLCHAIN={toolchain.as_posix()}")
        print(f"CHRONEXA_VCPKG_INSTALLED_DIR={self.config.deps_dir.as_posix()}")
