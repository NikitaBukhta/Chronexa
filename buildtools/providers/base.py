from abc import ABC, abstractmethod
from pathlib import Path

from buildtools.shell import Shell


class ToolProvider(ABC):
    """Base class for tools that the project depends on."""

    def __init__(self, shell: Shell):
        self.shell = shell

    @abstractmethod
    def ensure(self) -> Path:
        """Ensure the tool is available and return path to its executable.

        Raises:
            ToolNotFoundError: If the tool cannot be found or installed.
        """
