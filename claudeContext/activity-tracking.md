# Chronexa — Activity Tracking (feature/TrackPereferiaActivity)

Knowledge dump of the project state and the changes made on this branch.
Date: 2026-06-06.

## Project overview

- **What**: Qt6 (Quick/QML) desktop app that tracks user activity (foreground
  apps, browser tabs) on Windows.
- **Stack**: C++17, Qt 6.8+ via vcpkg, CMake 3.16+, MSVC. Single executable
  target `Chronexa`. Build dirs: `build/debug`, `build/release`.
- **Architecture**: clean/layered under `src/`:
  - `core/` — AppInitializer (wires modules), AppEnvironment, FileLogger
    (own thread, mutex + condvar).
  - `domain/` — pure business logic. **Must never include Win32 headers**
    and must stay time-deterministic (timestamps always passed in, never
    `QDateTime::currentDateTime()` inside) so it is unit-testable.
  - `infrastructure/` — platform-specific code (Win32, UIA).
  - `application/` — services orchestrating the domain.
  - `ui/` — MVVM: `UserActivityModel` (QAbstractListModel) +
    `UserActivityController`, QML in `src/ui/qml`.
- **Conventions**: namespace `chronexa::<layer>`, members `_camelCase`,
  constants `kCamelCase`, logging categories `lcCamelCase` per file
  (`Q_LOGGING_CATEGORY`), `[[nodiscard]]` on getters.

## Data flow (the architecture implemented on this branch)

```
Win32 WinEvent hooks ──► WindowsActivityProvider (infrastructure, singleton)
        │ every foreground/title change
        ▼
ActivityCollector (domain) ── temp list + handling logic
        ▲ drainHandled(now) every 5-min boundary
ActivityService (application) ── QTimer tick, emits activityUpdated(QList<Activity>)
        ▼
UserActivityModel / QML (ui)
```

## Key classes

### `domain/activity/Activity.hpp`
Struct: `appId` (stable id, exe base name), `appName` (display name),
`title` (window title), `tabDomain` (active browser tab domain, empty for
non-browsers), `startedOn`, `endedOn`.

### `domain/activity/ActivityCollector` (new)
The "main collector". Thread-safe (QMutex). API:
- `notifyActivityChanged(appId, appName, title, tabDomain, when)` — dedups
  repeated notifications, closes the previous open activity.
- `drainHandled(now)` — closes the open activity at `now`, returns the
  handled list, re-opens the same activity from `now` (no lost time span).
- `reset()`.

Handling rules (in `closeCurrentLocked`):
- Activities shorter than `kMinActivityDurationMs` (1 s) are dropped
  (alt-tab glitches).
- Consecutive records of the same activity with a gap ≤ `kMergeGapMs`
  (2 s) are merged.
- Identity = `appId + title + tabDomain` (so a tab/domain change splits
  the activity).

### `infrastructure/activity/WindowsActivityProvider`
Singleton (`instance()`, private ctor) implementing `IUserActivityProvider`
(`start/stop/drainEvents`). Installs two WinEvent hooks
(`WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS`, fire on the Qt main
thread's message loop):
- `EVENT_SYSTEM_FOREGROUND` — app switches.
- `EVENT_OBJECT_NAMECHANGE` — title changes, filtered to the foreground
  window only (catches browser tab switches).

Each event → `captureForegroundActivity()` → AppInfoParser (+
BrowserInfoParser for browsers) → `_collector.notifyActivityChanged(...)`.
`drainEvents()` just forwards to `drainHandled(currentDateTime())`.

### `infrastructure/activity/AppInfoParser` (ported from
`C:\Users\nikit\Documents\projects\hotcode\qtime\piche_qtime_plugin_windows`)
Per-HWND parser:
- `processId()` — `GetWindowThreadProcessId`.
- `executablePath()` — `OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION)` +
  `QueryFullProcessImageNameW` (RAII `UniqueHandle`).
- `appId()` — exe base name without extension ("chrome"); falls back to
  `appName()`.
- `appName()` — version-info metadata: ProductName for standard Windows
  classes (#32770 etc.), FileDescription otherwise (`GetFileVersionInfoW`
  + `VerQueryValueW`, first translation language). Fallbacks: exe base
  name → window title.
- `windowTitle()`, `className()`.

### `infrastructure/activity/BrowserInfoParser` (new, UIA option chosen)
Resolves the active-tab **domain** (privacy: domain only, not full URL)
for supported browsers `chrome / msedge / firefox / opera`.
- UIA cost mitigation: the address-bar `IUIAutomationElement` is found once
  per HWND and **cached** (`QHash<HWND, ComPtr>`); subsequent reads are a
  single `GetCurrentPropertyValue(UIA_ValueValuePropertyId)`. Stale
  elements (empty read) are evicted and re-resolved.
- Find paths: Firefox → `nav-bar` toolbar then `urlbar-input`
  (AutomationId); Chromium-based → first Edit control descendant.
- `domainFromAddress()` strips scheme/`www.`/path and validates with a
  domain regex (+ localhost forms); returns empty for typed search queries.
- COM: `CoInitializeEx(STA)` tolerant of `S_FALSE`/`RPC_E_CHANGED_MODE`
  (Qt already initializes COM on the GUI thread).
- Rejected alternatives discussed: browser extension + native messaging
  (best long-term, requires per-browser install), CDP (needs debug flag),
  title parsing (no domain in Chrome titles).

### `infrastructure/activity/Win32Raii.hpp` (new)
RAII wrappers (move-only): `UniqueHandle` (CloseHandle),
`UniqueWinEventHook` (UnhookWinEvent), `ScopedVariant` (VariantClear, with
`fromBstr`/`fromInt`/`receive()` for out-params). Used everywhere instead
of manual cleanup.

### `application/activity/ActivityService`
Holds a **non-owning** raw pointer to the provider singleton (was
`unique_ptr` — would double-free the singleton). Ticks on 5-minute wall
clock boundaries (`kTickIntervalMinutes`), drains the provider, emits
`activityUpdated`. `requestClear()` drains + emits `cleared()`.

## Tests

- `tests/activity/tst_ActivityCollector.cpp` — Qt Test target
  `tst_ActivityCollector` (CMake `enable_testing()` + `add_test`), links
  only Qt6::Test + domain sources (no Win32). Table-driven with fabricated
  timestamps relative to a fixed `kT0`: switch closes previous, duplicate
  ignored, <1 s glitch dropped, same-activity merge across small gap,
  tabDomain change splits, drain re-opens current, reset drops all.
- **Open issue**: test target fails to link with
  `LNK2019: unresolved external symbol main` (MSVCRTD exe_main) despite
  `QTEST_APPLESS_MAIN`. Likely Qt entrypoint/qMain interplay with
  `qt_add_executable`; possible fixes to try: plain `add_executable`
  for the test, or `set_target_properties(... WIN32_EXECUTABLE FALSE)`,
  or explicit `int main` + check `QT_NEEDS_QMAIN`.
- Also: linking `Chronexa.exe` fails with LNK1168 while the app is running
  (file locked) — close the app before building.

## CMake notes

- New sources registered in `qt_add_executable(Chronexa ...)`:
  ActivityCollector, AppInfoParser, BrowserInfoParser, Win32Raii.hpp.
- `find_package(Qt6 ... COMPONENTS Quick Test)`.
- `target_link_libraries(Chronexa PRIVATE version)` on WIN32 — needed for
  `GetFileVersionInfoW`. UIA CLSIDs come from uuid.lib (linked by default).
- vcpkg auto-bootstrap via `cmake/AutoBootstrap.cmake`; qt.conf generated
  next to the exe to point at the vcpkg Qt tree.

## Possible next steps

- Fix the test target link error and run `ctest`.
- UI: expose `tabDomain` / `appId` roles in `UserActivityModel` (currently
  only appName/title/times are exposed).
- Debounce UIA reads on rapid tab switching (e.g. 150 ms singleShot).
- Idle detection (no input ⇒ close current activity) — not implemented.
- Browser-extension transport as a more reliable domain source later.
