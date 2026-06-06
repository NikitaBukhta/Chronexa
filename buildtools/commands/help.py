from pathlib import Path

from buildtools.commands.base import Command
from buildtools.config import ProjectConfig
from buildtools.shell import Shell


class StatusChecker:
    """Inspects the current state of tools and project directories."""

    def __init__(self, config: ProjectConfig, shell: Shell):
        self.config = config
        self.shell = shell

    def check_tool(self, name: str,
                   extra_paths: list[Path] | None = None) -> tuple[bool, str]:
        found = self.shell.which(name)
        if found:
            return True, found
        for p in (extra_paths or []):
            if p.exists():
                return True, str(p)
        return False, "not found"

    def check_dir(self, path: Path) -> tuple[bool, str]:
        if path.exists():
            return True, str(path)
        return False, "not created"

    def get_tool_version(self, executable: str) -> str:
        version = self.shell.get_output([executable, "--version"])
        return version or "unknown version"


class HelpCommand(Command):
    """Shows an overview of commands, paths, and tool status."""

    name = "help"
    summary = "Show this help message"

    def __init__(self, config: ProjectConfig, shell: Shell,
                 commands: dict[str, Command]):
        self.config = config
        self.checker = StatusChecker(config, shell)
        self.commands = commands

    def execute(self) -> None:
        self._print_header()
        self._print_commands()
        self._print_usage()
        self._print_paths()
        self._print_tools()

    def _print_header(self) -> None:
        print("Library Project — bootstrap & build helper")
        print("=" * 50)

    def _print_commands(self) -> None:
        print("\nCommands:")
        for cmd in self.commands.values():
            print(f"  {cmd.name:<12} {cmd.summary}")

    def _print_usage(self) -> None:
        print("\nUsage:")
        print("  python bootstrap.py <command> [flags]")

        print("\nFlags:")
        print("  --release        Use Release mode (default: Debug)")
        print("                   Applies to: bootstrap, compile, run")
        print("  -j, --jobs N     Limit parallel build jobs (default: all cores)")
        print("                   Applies to: compile")
        print("  --dry-run        Show commands without executing them")
        print("                   Applies to: bootstrap")

    def _print_paths(self) -> None:
        print("\nPaths:")
        dirs = {
            "Project":      self.config.project_dir,
            "Venv":         self.config.venv_dir,
            "Dependencies": self.config.deps_dir,
            "vcpkg":        self.config.vcpkg_dir,
        }
        for label, path in dirs.items():
            exists, loc = self.checker.check_dir(path)
            status = "OK" if exists else "MISSING"
            print(f"  {label:<14} {loc:<50} [{status}]")

    def _print_tools(self) -> None:
        cmake_venv = self.config.venv_executable("cmake")
        vcpkg_path = self.config.vcpkg_executable()

        tools: dict[str, list[Path]] = {
            "git":   [],
            "cmake": [cmake_venv],
            "vcpkg": [vcpkg_path],
        }

        print("\nTools:")
        for name, extra in tools.items():
            found, location = self.checker.check_tool(name, extra)
            if found:
                version = self.checker.get_tool_version(location)
                print(f"  {name:<14} {version:<40} [{location}]")
            else:
                print(f"  {name:<14} {'not installed':<40} "
                      "[will be installed on bootstrap]")
