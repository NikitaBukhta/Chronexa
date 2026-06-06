import shutil

from buildtools.commands.base import Command
from buildtools.commands.deps import DepsCommand
from buildtools.config import ProjectConfig
from buildtools.providers.cmake import CMakeProvider
from buildtools.shell import Shell


class BootstrapCommand(Command):
    """Downloads tools, installs vcpkg dependencies, configures the project."""

    name = "bootstrap"
    summary = "Install dependencies and configure the project"

    def __init__(self, config: ProjectConfig, shell: Shell,
                 deps: DepsCommand, cmake: CMakeProvider):
        self.config = config
        self.shell = shell
        self.deps = deps
        self.cmake = cmake

    def execute(self) -> None:
        # Ensure venv + vcpkg + clang-format via the shared deps logic, so
        # the CLI and the CMake auto-bootstrap path stay in sync.
        self.deps.ensure()
        cmake_path = self.cmake.ensure()

        print(f"\n=== Configuring ({self.config.build_type}) ===")
        cmake_cmd = [
            cmake_path,
            "--preset", self.config.cmake_preset,
            "-S", self.config.project_dir,
            f"-DVCPKG_INSTALLED_DIR={self.config.deps_dir}",
            # vcpkg is already ensured above; provide the toolchain directly
            # and skip the CMake-side auto-bootstrap to avoid a redundant
            # nested `bootstrap.py deps` call during configuration.
            f"-DCMAKE_TOOLCHAIN_FILE={self.config.vcpkg_toolchain.as_posix()}",
            "-DCHRONEXA_SKIP_AUTO_BOOTSTRAP=ON",
        ]
        for d in self.config.cmake_defs:
            cmake_cmd.append(f"-D{d}")
        self.shell.run(cmake_cmd)

        self._cleanup_vcpkg_temp()

        print("\nBootstrap complete.")
        print(f"  Dependencies : {self.config.deps_dir}")
        print(f"  Virtual env  : {self.config.venv_dir}")
        flag = " --release" if self.config.release else ""
        print(f"Run `python bootstrap.py compile{flag}` to build the project.")

    def _cleanup_vcpkg_temp(self) -> None:
        for sub in ("buildtrees", "packages"):
            path = self.config.vcpkg_dir / sub
            if path.exists():
                if self.shell.dry_run:
                    print(f"[dry-run] would clean {path}")
                    continue
                print(f"Cleaning {path}...")
                shutil.rmtree(path, ignore_errors=True)
