from pathlib import Path

from buildtools.errors import ToolNotFoundError
from buildtools.providers.base import ToolProvider
from buildtools.shell import Shell
from buildtools.venv_manager import VenvManager


class CMakeProvider(ToolProvider):

    def __init__(self, shell: Shell, venv_mgr: VenvManager):
        super().__init__(shell)
        self.venv_mgr = venv_mgr

    def ensure(self) -> Path:
        # system cmake
        found = self.shell.which("cmake")
        if found:
            return Path(found)

        # venv cmake
        venv_cmake = self.venv_mgr.find_executable("cmake")
        if venv_cmake:
            return venv_cmake

        # install into venv
        print("CMake not found. Installing into venv...")
        self.venv_mgr.install("cmake")

        venv_cmake = self.venv_mgr.find_executable("cmake")
        if venv_cmake:
            return venv_cmake

        raise ToolNotFoundError("cmake installation into venv failed")
