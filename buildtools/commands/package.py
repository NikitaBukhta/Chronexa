import shutil
from pathlib import Path

from buildtools.commands.base import Command
from buildtools.config import ProjectConfig
from buildtools.errors import BuildError, ToolNotFoundError
from buildtools.shell import Shell

APP_NAME = "BeeLibrary"

# Qt plugins needed at runtime (subdirs of Qt6/plugins/)
QT_PLUGIN_DIRS = ["platforms", "sqldrivers", "imageformats", "iconengines", "styles", "tls"]

# QML modules needed at runtime (subdirs of Qt6/qml/)
QT_QML_DIRS = ["QtCore", "QtQml", "QtQuick"]


class PackageCommand(Command):
    """Builds a release, stages files, and creates an Inno Setup installer."""

    name = "package"
    summary = "Create installer (Inno Setup)"

    def __init__(self, config: ProjectConfig, shell: Shell):
        self.config = config
        self.shell = shell
        self.staging_dir = config.project_dir / "staging"

    def execute(self) -> None:
        self._check_release()
        iscc = self._find_iscc()
        self._prepare_staging()
        self._run_iscc(iscc)

    def _check_release(self) -> None:
        exe = self._find_exe()
        if not exe.exists():
            raise BuildError(
                "Release build not found. Run:\n"
                "  python bootstrap.py bootstrap --release\n"
                "  python bootstrap.py compile --release"
            )

    def _find_exe(self) -> Path:
        build_root = self.config.project_dir / "build" / "release"
        name = f"{APP_NAME}.exe" if self.config.is_windows else APP_NAME
        # Multi-config (Visual Studio)
        candidate = build_root / "Release" / name
        if candidate.exists():
            return candidate
        # Single-config
        return build_root / name

    def _find_iscc(self) -> Path:
        path = self._locate_iscc()
        if path:
            return path

        self._offer_install()

        path = self._locate_iscc()
        if path:
            return path

        raise ToolNotFoundError(
            "Inno Setup is still not found after installation.\n"
            "Try installing manually: https://jrsoftware.org/isdl.php"
        )

    @property
    def _inno_dir(self) -> Path:
        return self.config.deps_dir / "other" / "InnoSetup"

    def _locate_iscc(self) -> Path | None:
        # Our managed install
        local = self._inno_dir / "ISCC.exe"
        if local.exists():
            return local

        # System PATH
        found = self.shell.which("ISCC")
        if found:
            return Path(found)

        # Standard install locations
        for path in [
            Path("C:/Program Files (x86)/Inno Setup 6/ISCC.exe"),
            Path("C:/Program Files/Inno Setup 6/ISCC.exe"),
        ]:
            if path.exists():
                return path
        return None

    def _offer_install(self) -> None:
        print("Inno Setup is required to create the installer.")
        print("It is only needed for packaging, not for development.\n")

        answer = input("Install Inno Setup via winget? [y/N] ").strip().lower()
        if answer != "y":
            raise ToolNotFoundError(
                "Inno Setup not installed. Install manually:\n"
                "  winget install JRSoftware.InnoSetup\n"
                "  https://jrsoftware.org/isdl.php"
            )

        print()
        self.shell.run([
            "winget", "install", "JRSoftware.InnoSetup",
            "--location", str(self._inno_dir),
            "--accept-source-agreements", "--accept-package-agreements",
        ])

    def _prepare_staging(self) -> None:
        if self.staging_dir.exists():
            shutil.rmtree(self.staging_dir)
        self.staging_dir.mkdir()

        print("=== Staging files ===")
        exe_src = self._find_exe()
        exe_dir = exe_src.parent

        # Executable
        shutil.copy2(exe_src, self.staging_dir / exe_src.name)

        # DLLs from vcpkg bin (all runtime dependencies)
        vcpkg_bin = self.config.deps_dir / "x64-windows" / "bin"
        for dll in vcpkg_bin.glob("*.dll"):
            shutil.copy2(dll, self.staging_dir / dll.name)

        # qt.conf
        qt_conf = self.config.project_dir / "installer" / "qt.conf"
        shutil.copy2(qt_conf, self.staging_dir / "qt.conf")

        # Qt plugins
        vcpkg_plugins = self.config.deps_dir / "x64-windows" / "Qt6" / "plugins"
        for subdir in QT_PLUGIN_DIRS:
            src = vcpkg_plugins / subdir
            if src.exists():
                dst = self.staging_dir / "plugins" / subdir
                shutil.copytree(src, dst, ignore=shutil.ignore_patterns("*.pdb"))

        # QML modules (vcpkg)
        vcpkg_qml = self.config.deps_dir / "x64-windows" / "Qt6" / "qml"
        for subdir in QT_QML_DIRS:
            src = vcpkg_qml / subdir
            if src.exists():
                dst = self.staging_dir / "qml" / subdir
                shutil.copytree(src, dst, ignore=shutil.ignore_patterns("*.pdb"))

        # QML modules (project — the Library module from build)
        build_qml = self.config.project_dir / "build" / "release" / "Library"
        if build_qml.exists():
            dst = self.staging_dir / "qml" / "Library"
            shutil.copytree(build_qml, dst,
                            ignore=shutil.ignore_patterns("*.pdb", "*.qrc"))

        file_count = sum(1 for _ in self.staging_dir.rglob("*") if _.is_file())
        print(f"  Staged {file_count} files into {self.staging_dir}")

    def _run_iscc(self, iscc: Path) -> None:
        iss_file = self.config.project_dir / "installer" / "BeeLibrary.iss"
        dist_dir = self.config.project_dir / "dist"
        dist_dir.mkdir(exist_ok=True)

        print("\n=== Building installer ===")
        self.shell.run([iscc, str(iss_file)])

        print(f"\nInstaller created in {dist_dir}")
