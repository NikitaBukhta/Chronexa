from abc import ABC, abstractmethod


class Command(ABC):
    """Base class for CLI commands."""

    name: str = ""
    summary: str = ""

    @abstractmethod
    def execute(self) -> None: ...
