import os
import subprocess

from buildtools.commands.base import Command
from buildtools.config import ProjectConfig
from buildtools.errors import BuildError

class RunCommand(Command):
    """Runs the built application with the correct environment."""

    name = "run"
    summary = "Run the built application"

    def __init__(self, config: ProjectConfig):
        self.config = config

    def execute(self) -> None:
        executable = self._find_executable()

        build_root = self.config.project_dir / "build" / self.config.cmake_preset

        env = os.environ.copy()
        env["QT_PLUGIN_PATH"] = str(self.config.qt_plugin_dir)
        env["QML2_IMPORT_PATH"] = str(self.config.qt_qml_dir) + os.pathsep + str(build_root)
        env["PATH"] = str(self.config.qt_bin_dir) + os.pathsep + env.get("PATH", "")

        print(f"=== Running ({self.config.build_type}) ===")
        print(f">>> {executable}")
        subprocess.run([executable], env=env)

    def _find_executable(self) -> str:
        build_root = self.config.project_dir / "build" / self.config.cmake_preset
        exe = f"{self.config.app_name}.exe" if self.config.is_windows else self.config.app_name

        # Multi-config generators (Visual Studio): build/<preset>/<Config>/app.exe
        multi_config = build_root / self.config.build_type / exe
        if multi_config.exists():
            return str(multi_config)

        # Single-config generators (Ninja, Make): build/<preset>/app.exe
        single_config = build_root / exe
        if single_config.exists():
            return str(single_config)

        raise BuildError(
            f"Executable not found. "
            f"Run `python bootstrap.py compile` first."
        )
