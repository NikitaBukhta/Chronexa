class ToolNotFoundError(RuntimeError):
    """A required external tool is not available."""


class BuildError(RuntimeError):
    """A build step failed."""
