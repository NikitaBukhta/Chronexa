from pathlib import Path

from buildtools.errors import ToolNotFoundError
from buildtools.providers.base import ToolProvider


class GitProvider(ToolProvider):

    def ensure(self) -> Path:
        found = self.shell.which("git")
        if found:
            return Path(found)
        raise ToolNotFoundError(
            "git is not installed. Install it from https://git-scm.com/downloads"
        )
