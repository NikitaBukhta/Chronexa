# BeeLibrary

Qt 6.8 QML desktop application for managing a personal book library.

## Prerequisites

- **Visual Studio 2022** (with "Desktop development with C++" workload)
- **Python 3.12+**
- **Git**

CMake and vcpkg will be installed automatically by the bootstrap script if not found on the system.

## IDE configuration (CLion)

Just **open the project folder in CLion** — no manual setup. The project ships
everything needed as committed files:

- **`CMakePresets.json`** — CLion auto-detects it and offers the `debug` /
  `release` profiles (no per-user configuration).
- **`cmake/AutoBootstrap.cmake`** — runs on the first *Configure*: it executes
  `python bootstrap.py deps` (which clones/bootstraps **vcpkg** and creates the
  venv + clang-format), then wires the vcpkg toolchain. **vcpkg manifest mode**
  then installs the C++ dependencies from `vcpkg.json` during configuration.
- **Runtime deploy** — `CMakeLists.txt` generates a `qt.conf` next to the
  executable and copies the Qt module DLLs beside it after each build, so
  **Run / Debug works straight from the IDE** without setting `QT_PLUGIN_PATH`
  or `PATH` by hand.
- **Code style** — `.idea/codeStyles/` enables ClangFormat using the project's
  `.clang-format`, so the IDE formats exactly like the CLI `format` command.

A new developer only needs the [prerequisites](#prerequisites) above; opening
the project does the rest. (Visual Studio reads the same presets; other IDEs can
point at `CMakePresets.json` too.)

## Build manually with CMake (no scripts, no IDE)

If you prefer to drive CMake yourself — without `bootstrap.py` and without an
IDE — set `-DCHRONEXA_SKIP_AUTO_BOOTSTRAP=ON` (so CMake does *not* call the
bootstrap script) and provide the vcpkg toolchain yourself. vcpkg still installs
the C++ dependencies automatically via manifest mode during configuration.

```bash
# 1. Get vcpkg once (anywhere on disk), then bootstrap it
git clone https://github.com/microsoft/vcpkg C:/dev/vcpkg
C:/dev/vcpkg/bootstrap-vcpkg.bat

# 2. Configure — vcpkg reads vcpkg.json and installs deps into the build tree
cmake -S . -B build/manual \
  -DCHRONEXA_SKIP_AUTO_BOOTSTRAP=ON \
  -DCMAKE_TOOLCHAIN_FILE=C:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DVCPKG_TARGET_TRIPLET=x64-windows

# 3. Build (Visual Studio is a multi-config generator -> pass --config)
cmake --build build/manual --config Debug

# 4. Run — qt.conf and the Qt DLLs are deployed next to the .exe
build/manual/Debug/Chronexa.exe
```

Notes:
- The **first** configure may take a long time: if vcpkg has no cached binaries
  it compiles Qt from source. Subsequent configures reuse the vcpkg cache.
- Use `-DCMAKE_BUILD_TYPE=Release` (single-config generators like Ninja) or
  `--config Release` (Visual Studio) for a release build.
- To control where deps land, add `-DVCPKG_INSTALLED_DIR=<path>` (defaults to
  `build/manual/vcpkg_installed`).
- Requires the [prerequisites](#prerequisites): a C++ compiler (VS 2022) and
  Git; Python is **not** needed on this path.

## Build & Run (CLI)

```bash
python bootstrap.py bootstrap
python bootstrap.py compile
python bootstrap.py run
```

Use `--release` for a release build:

```bash
python bootstrap.py bootstrap --release
python bootstrap.py compile --release
python bootstrap.py run --release
```

## Tests

```bash
python bootstrap.py test
```

## CMake Flags

Pass CMake variables via `-D` during bootstrap:

```bash
python bootstrap.py bootstrap -DBUILD_TESTS=OFF
```

## Create Installer

```bash
python bootstrap.py package
```

Inno Setup will be installed automatically if not found on the system.

## Clean

Remove all dependencies, build directories, and venv:

```bash
python bootstrap.py clean
```

Run `python bootstrap.py help` for full command reference and status.
