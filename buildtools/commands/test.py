import os
import subprocess

from buildtools.commands.base import Command
from buildtools.config import ProjectConfig
from buildtools.errors import BuildError


class TestCommand(Command):
    """Runs unit tests with the correct environment."""

    name = "test"
    summary = "Run unit tests"

    # Tests with this ctest label drive the real app and take the foreground;
    # they only run through E2eCommand.
    E2E_LABEL = "e2e"

    def __init__(self, config: ProjectConfig):
        self.config = config

    def _label_args(self) -> list[str]:
        return ["-LE", self.E2E_LABEL]

    def _environment(self) -> dict[str, str]:
        env = os.environ.copy()
        env["PATH"] = str(self.config.qt_bin_dir) + os.pathsep + env.get("PATH", "")
        env["QT_PLUGIN_PATH"] = str(self.config.qt_plugin_dir)
        env["QT_QPA_PLATFORM"] = "offscreen"
        return env

    def execute(self) -> None:
        build_root = self.config.project_dir / "build" / self.config.cmake_preset

        # ctest ships next to the venv's cmake; a bare "ctest" is only found
        # when some other CMake happens to be on PATH.
        ctest = self.config.venv_executable("ctest")
        if not ctest.exists():
            ctest = "ctest"

        print(f"\n=== Running {self.name} ({self.config.build_type}) ===")
        result = subprocess.run(
            [
                str(ctest),
                "--test-dir", str(build_root),
                "--output-on-failure",
                "-C", self.config.build_type,
                *self._label_args(),
            ],
            env=self._environment(),
        )

        if result.returncode != 0:
            raise BuildError(f"Tests failed with exit code {result.returncode}")

        print("\nAll tests passed.")


class E2eCommand(TestCommand):
    """Runs the end-to-end tests: the real app, driven through real windows."""

    name = "e2e"
    summary = "Run end-to-end tests (takes the foreground for ~30 s)"

    def _label_args(self) -> list[str]:
        return ["-L", self.E2E_LABEL, "--verbose"]

    def _environment(self) -> dict[str, str]:
        env = super()._environment()
        env.pop("QT_QPA_PLATFORM", None)
        return env
