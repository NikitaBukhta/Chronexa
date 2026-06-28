import json
import shutil
from pathlib import Path

from buildtools.commands.base import Command
from buildtools.commands.deps import DepsCommand
from buildtools.config import ProjectConfig
from buildtools.providers.cmake import CMakeProvider
from buildtools.shell import Shell


def _fwd(p: Path) -> str:
    # Forward slashes survive CMake -> JSON parsing; a literal `C:\Users`
    # would get unescaped to `C:Users`. Mirrors RunCommand's path handling.
    return str(p).replace("\\", "/")


class BootstrapCommand(Command):
    """Downloads tools, installs vcpkg dependencies, configures the project."""

    name = "bootstrap"
    summary = "Install dependencies and configure the project"

    def __init__(self, config: ProjectConfig, shell: Shell,
                 deps: DepsCommand, cmake: CMakeProvider):
        self.config = config
        self.shell = shell
        self.deps = deps
        self.cmake = cmake

    def execute(self) -> None:
        # Ensure venv + vcpkg + clang-format via the shared deps logic, so
        # the CLI and the CMake auto-bootstrap path stay in sync.
        self.deps.ensure()
        cmake_path = self.cmake.ensure()

        print(f"\n=== Configuring ({self.config.build_type}) ===")
        cmake_cmd = [
            cmake_path,
            "--preset", self.config.cmake_preset,
            "-S", self.config.project_dir,
            f"-DVCPKG_INSTALLED_DIR={self.config.deps_dir}",
            # vcpkg is already ensured above; provide the toolchain directly
            # and skip the CMake-side auto-bootstrap to avoid a redundant
            # nested `bootstrap.py deps` call during configuration.
            f"-DCMAKE_TOOLCHAIN_FILE={self.config.vcpkg_toolchain.as_posix()}",
            "-DCHRONEXA_SKIP_AUTO_BOOTSTRAP=ON",
        ]
        for d in self.config.cmake_defs:
            cmake_cmd.append(f"-D{d}")
        self.shell.run(cmake_cmd)

        self._cleanup_vcpkg_temp()

        user_presets = self._write_user_presets()
        print(f"  IDE presets  : {user_presets}")
        run_dir = self._write_run_configs()
        print(f"  IDE run cfgs : {run_dir}")

        print("\nBootstrap complete.")
        print(f"  Dependencies : {self.config.deps_dir}")
        print(f"  Virtual env  : {self.config.venv_dir}")
        flag = " --release" if self.config.release else ""
        print(f"Run `python bootstrap.py compile{flag}` to build the project.")

    # (slug, CMake build type, pretty label) for each variant we emit.
    _BUILD_TYPES = (
        ("debug", "Debug", "debug"),
        ("release", "Release", "release"),
    )

    def _write_user_presets(self) -> Path:
        """Generate a git-ignored CMakeUserPresets.json for IDEs (CLion, VS).

        The committed CMakePresets.json relies on `cmake/AutoBootstrap.cmake`
        to provision vcpkg and wire the toolchain on the IDE's first Configure.
        We additionally materialize resolved profiles here -- honoring any
        project.json deps_dir/vcpkg_dir override -- so the IDE has ready-made
        Debug/Release profiles with the toolchain already wired (and the
        auto-bootstrap step skipped, since deps are installed). Regenerated on
        every bootstrap. Names are `amd64-*` to avoid colliding with the
        committed `debug`/`release` presets while inheriting their `vcpkg` base.

        Only configurePresets are emitted: IDEs derive the build step from the
        configure preset, and adding buildPresets just doubles every entry in
        CLion's profile list.
        """
        base = self.config.deps_dir / "x64-windows"
        toolchain = _fwd(self.config.vcpkg_toolchain)
        configure: list[dict] = []
        for slug, btype, label in self._BUILD_TYPES:
            # Debug links the debug triplet; Release uses the release tree.
            triplet = base / "debug" if slug == "debug" else base
            # The preset `environment` carries the Qt runtime env because CLion
            # applies a profile's environment to BOTH build and run -- so the
            # app finds its platform plugin and the QML plugins' dependency DLLs
            # without relying on qt.conf. Mirrors RunCommand's desktop env.
            env = {
                "VCPKG_ROOT": _fwd(self.config.vcpkg_dir),
                "VCPKG_INSTALLED_DIR": _fwd(self.config.deps_dir),
                "QT_PLUGIN_PATH": _fwd(triplet / "Qt6" / "plugins"),
                "QML2_IMPORT_PATH": _fwd(triplet / "Qt6" / "qml"),
                "PATH": f"{_fwd(triplet / 'bin')};$penv{{PATH}}",
            }
            name = f"amd64-{slug}"
            configure.append({
                "name": name,
                "displayName": f"amd64 {label}",
                "inherits": "vcpkg",
                "binaryDir": "${sourceDir}/build/" + name,
                "environment": env,
                "cacheVariables": {
                    "CMAKE_BUILD_TYPE": btype,
                    # Deps are already provisioned -- skip the CMake-side
                    # auto-bootstrap and wire the toolchain directly so the
                    # IDE's Configure doesn't re-run `bootstrap.py deps`.
                    "CHRONEXA_SKIP_AUTO_BOOTSTRAP": "ON",
                    "CMAKE_TOOLCHAIN_FILE": {
                        "type": "FILEPATH",
                        "value": toolchain,
                    },
                    "VCPKG_INSTALLED_DIR": {
                        "type": "PATH",
                        "value": _fwd(self.config.deps_dir),
                    },
                    "VCPKG_TARGET_TRIPLET": "x64-windows",
                },
            })

        presets = {"version": 6, "configurePresets": configure}
        path = self.config.project_dir / "CMakeUserPresets.json"
        path.write_text(json.dumps(presets, indent=2) + "\n", encoding="utf-8")
        return path

    def _write_run_configs(self) -> Path:
        """Generate CLion shared run configs (.run/) for the amd64 profiles.

        Each binds to its CMake profile and injects the Qt runtime env the app
        needs to find its platform plugin -- the desktop equivalent of what
        RunCommand sets up. Without this, launching from the IDE can die with
        "no Qt platform plugin could be initialized". Regenerated per bootstrap.
        """
        run_dir = self.config.project_dir / ".run"
        run_dir.mkdir(exist_ok=True)
        app = self.config.app_name
        base = self.config.deps_dir / "x64-windows"
        for slug, _btype, label in self._BUILD_TYPES:
            triplet = base / "debug" if slug == "debug" else base
            plugins = triplet / "Qt6" / "plugins"
            qml = triplet / "Qt6" / "qml"
            bin_dir = triplet / "bin"
            cfg_name = f"{app} (amd64 {label})"
            # CLion binds run configs by the preset *name* (CONFIG_NAME),
            # not its displayName.
            profile = f"amd64-{slug}"
            xml = (
                '<component name="ProjectRunConfigurationManager">\n'
                f'  <configuration default="false" name="{cfg_name}"'
                ' type="CMakeRunConfiguration" factoryName="Application"'
                f' PASS_PARENT_ENVS_2="true" PROJECT_NAME="{app}"'
                f' TARGET_NAME="{app}"'
                f' CONFIG_NAME="{profile}"'
                f' RUN_TARGET_PROJECT_NAME="{app}" RUN_TARGET_NAME="{app}">\n'
                "    <envs>\n"
                f'      <env name="QT_PLUGIN_PATH" value="{plugins}" />\n'
                f'      <env name="QML2_IMPORT_PATH" value="{qml}" />\n'
                f'      <env name="PATH" value="{bin_dir};$PATH$" />\n'
                "    </envs>\n"
                '    <method v="2">\n'
                '      <option name="com.jetbrains.cidr.execution.'
                'CidrBuildBeforeRunTaskProvider$BuildBeforeRunTask"'
                ' enabled="true" />\n'
                "    </method>\n"
                "  </configuration>\n"
                "</component>\n"
            )
            (run_dir / f"{app}_amd64_{slug}.run.xml").write_text(
                xml, encoding="utf-8")

        # Desktop "via script": the most reliable launch. run.py sets the full
        # Qt env in-process, so it can't hit the "no Qt platform plugin"
        # failure that bites when CLion doesn't apply a run config's env.
        for slug, _btype, label in self._BUILD_TYPES:
            flag = "" if slug == "debug" else " --release"
            cmd = (
                f"python bootstrap.py compile{flag}; "
                f"if ($?) {{ python bootstrap.py run{flag} }}"
            )
            (run_dir / f"{app}_amd64_{slug}_script.run.xml").write_text(
                self._shell_run_xml(f"{app} (amd64 {label} → script)", cmd),
                encoding="utf-8")
        return run_dir

    @staticmethod
    def _shell_run_xml(name: str, cmd: str) -> str:
        """A CLion Shell Script run config that runs `cmd` via PowerShell."""
        return (
            '<component name="ProjectRunConfigurationManager">\n'
            f'  <configuration default="false" name="{name}"'
            ' type="ShConfigurationType">\n'
            f'    <option name="SCRIPT_TEXT" value="{cmd}" />\n'
            '    <option name="INDEPENDENT_SCRIPT_PATH" value="true" />\n'
            '    <option name="SCRIPT_PATH" value="" />\n'
            '    <option name="SCRIPT_OPTIONS" value="" />\n'
            '    <option name="INDEPENDENT_SCRIPT_WORKING_DIRECTORY"'
            ' value="true" />\n'
            '    <option name="SCRIPT_WORKING_DIRECTORY"'
            ' value="$PROJECT_DIR$" />\n'
            '    <option name="INDEPENDENT_INTERPRETER_PATH" value="true" />\n'
            '    <option name="INTERPRETER_PATH" value="powershell.exe" />\n'
            '    <option name="INTERPRETER_OPTIONS" value="" />\n'
            '    <option name="EXECUTE_IN_TERMINAL" value="true" />\n'
            '    <option name="EXECUTE_SCRIPT_FILE" value="false" />\n'
            "    <envs />\n"
            '    <method v="2" />\n'
            "  </configuration>\n"
            "</component>\n"
        )

    def _cleanup_vcpkg_temp(self) -> None:
        for sub in ("buildtrees", "packages"):
            path = self.config.vcpkg_dir / sub
            if path.exists():
                if self.shell.dry_run:
                    print(f"[dry-run] would clean {path}")
                    continue
                print(f"Cleaning {path}...")
                shutil.rmtree(path, ignore_errors=True)
