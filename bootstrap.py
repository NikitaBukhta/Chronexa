#!/usr/bin/env python3
"""Entry point for the Library build system."""

from pathlib import Path

from buildtools.cli import CLI


def main():
    project_dir = Path(__file__).resolve().parent
    CLI().run(project_dir)


if __name__ == "__main__":
    main()
