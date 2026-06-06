import argparse
import subprocess
import sys
from pathlib import Path

from buildtools.commands import (
    BootstrapCommand,
    CleanCommand,
    Command,
    CompileCommand,
    DepsCommand,
    FormatCommand,
    HelpCommand,
    PackageCommand,
    RunCommand,
    TestCommand,
)
from buildtools.config import ProjectConfig
from buildtools.errors import BuildError, ToolNotFoundError
from buildtools.providers import (
    CMakeProvider,
    ClangFormatProvider,
    GitProvider,
    VcpkgProvider,
)
from buildtools.shell import Shell
from buildtools.venv_manager import VenvManager


class CommandRegistry:
    """Builds the command graph from providers and config."""

    def __init__(self, config: ProjectConfig):
        self.config = config
        self.shell = Shell(dry_run=config.dry_run)
        self._venv_mgr = VenvManager(config, self.shell)
        self._git = GitProvider(self.shell)
        self._cmake = CMakeProvider(self.shell, self._venv_mgr)
        self._vcpkg = VcpkgProvider(self.shell, config, self._git)
        self._clang_format = ClangFormatProvider(
            self.shell, self._venv_mgr, config.project_dir,
        )

    def build(self) -> dict[str, Command]:
        deps = DepsCommand(
            self.config, self.shell,
            self._venv_mgr, self._vcpkg, self._clang_format,
        )
        commands: dict[str, Command] = {
            "bootstrap": BootstrapCommand(
                self.config, self.shell, deps, self._cmake,
            ),
            "compile": CompileCommand(
                self.config, self.shell, self._cmake,
                self._clang_format,
            ),
            "deps": deps,
            "format": FormatCommand(
                self.config, self.shell, self._clang_format,
            ),
            "run": RunCommand(self.config),
            "test": TestCommand(self.config),
            "package": PackageCommand(self.config, self.shell),
            "clean": CleanCommand(self.config),
        }
        commands["help"] = HelpCommand(self.config, self.shell, commands)
        return commands


class CLI:
    """Parses arguments and dispatches commands."""

    def __init__(self):
        self.parser = argparse.ArgumentParser(
            description="Bootstrap and build the Library project.",
            add_help=False,
        )
        self._setup_subcommands()

    def _setup_subcommands(self) -> None:
        self.parser.add_argument(
            "-h", "--help", action="store_true", default=False,
        )

        help_parser = argparse.ArgumentParser(add_help=False)
        help_parser.add_argument(
            "-h", "--help", action="store_true", default=False,
        )

        release_parser = argparse.ArgumentParser(add_help=False)
        release_parser.add_argument(
            "--release", action="store_true",
            help="Use Release mode (default: Debug)",
        )

        subs = self.parser.add_subparsers(dest="command")

        p_bootstrap = subs.add_parser(
            "bootstrap", parents=[help_parser, release_parser],
            add_help=False,
            help="Install dependencies and configure",
        )
        p_bootstrap.add_argument(
            "-D", dest="cmake_defs", action="append", default=[],
            metavar="VAR=VALUE",
            help="Pass CMake cache variable (e.g. -DBUILD_TESTS=OFF)",
        )
        p_bootstrap.add_argument(
            "--dry-run", action="store_true",
            help="Show the commands that would run without executing them",
        )

        p_compile = subs.add_parser(
            "compile", parents=[help_parser, release_parser],
            add_help=False,
            help="Build the project",
        )
        p_compile.add_argument(
            "-j", "--jobs", type=int, default=None,
            help="Parallel build jobs",
        )

        subs.add_parser(
            "deps", parents=[help_parser],
            add_help=False,
            help="Ensure dependencies only (used by CMake auto-bootstrap)",
        )

        subs.add_parser(
            "run", parents=[help_parser, release_parser],
            add_help=False,
            help="Run the built application",
        )

        subs.add_parser(
            "package", parents=[help_parser],
            add_help=False,
            help="Create installer (Inno Setup)",
        )

        subs.add_parser(
            "test", parents=[help_parser, release_parser],
            add_help=False,
            help="Run unit tests",
        )

        subs.add_parser(
            "format", parents=[help_parser],
            add_help=False,
            help="Auto-format C++ source files",
        )

        subs.add_parser(
            "clean", parents=[help_parser],
            add_help=False,
            help="Remove dependencies, build dirs, and venv",
        )

        subs.add_parser("help", parents=[help_parser],
                         add_help=False,
                         help="Show detailed help and status")

    def run(self, project_dir: Path) -> None:
        args = self.parser.parse_args()
        if args.help:
            args.command = "help"
        kwargs: dict = {
            "project_dir": project_dir,
            "release": getattr(args, "release", False),
        }
        jobs = getattr(args, "jobs", None)
        if jobs is not None:
            kwargs["jobs"] = jobs
        cmake_defs = getattr(args, "cmake_defs", None)
        if cmake_defs:
            kwargs["cmake_defs"] = cmake_defs
        if getattr(args, "dry_run", False):
            kwargs["dry_run"] = True
        config = ProjectConfig(**kwargs)
        command_name = args.command or "help"
        registry = CommandRegistry(config)
        commands = registry.build()

        try:
            commands[command_name].execute()
        except (ToolNotFoundError, BuildError) as e:
            print(f"\nERROR: {e}")
            sys.exit(1)
        except subprocess.CalledProcessError as e:
            print(f"\nERROR: Command failed with exit code {e.returncode}")
            sys.exit(e.returncode)
