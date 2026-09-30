"""End-to-end: Chronexa splits a session where the window's category changes.

Runs the real Chronexa.exe against a throwaway profile (--profile), puts a
real window in the foreground and walks its title through category changes,
then closes Chronexa and reads what it recorded.

It takes focus and presses Alt, as any UI automation does -- run it with
`python bootstrap.py e2e`, not while typing. Your own history and settings are
never touched: everything lives in a temporary directory.

Usage: e2e_categories.py <path to Chronexa.exe>
"""

import contextlib
import ctypes
import ctypes.wintypes as wt
import os
import shutil
import sqlite3
import subprocess
import sys
import tempfile
import time
import tkinter as tk
from pathlib import Path

MARKER = "chronexa-e2e"

# The window below belongs to this Python process, whose recorded app name is
# the interpreter's file description ("Python") or, without one, "python".
RULES_JSON = (
    '[{"apps":["python"],"category":"E2E Distraction","title":"YouTube"},'
    '{"apps":["python"],"category":"E2E Work","title":""}]'
)

user32 = ctypes.WinDLL("user32", use_last_error=True)
user32.GetForegroundWindow.restype = wt.HWND
user32.SetForegroundWindow.argtypes = [wt.HWND]
user32.PostMessageW.argtypes = [wt.HWND, wt.UINT, wt.WPARAM, wt.LPARAM]
user32.GetWindowThreadProcessId.argtypes = [wt.HWND, ctypes.POINTER(wt.DWORD)]
user32.IsWindowVisible.argtypes = [wt.HWND]
user32.GetWindowTextW.argtypes = [wt.HWND, wt.LPWSTR, ctypes.c_int]
user32.GetClassNameW.argtypes = [wt.HWND, wt.LPWSTR, ctypes.c_int]
WNDENUMPROC = ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)
user32.EnumWindows.argtypes = [WNDENUMPROC, wt.LPARAM]

VK_MENU = 0x12
VK_F24 = 0x87
KEYEVENTF_KEYUP = 0x2
WM_CLOSE = 0x0010


class E2EFailure(Exception):
    pass


def foreground_description() -> str:
    """Who holds the foreground: for a failure message, not for control."""
    hwnd = user32.GetForegroundWindow()
    if not hwnd:
        return "nothing (the desktop is locked or switching)"
    title = ctypes.create_unicode_buffer(256)
    user32.GetWindowTextW(hwnd, title, 256)
    klass = ctypes.create_unicode_buffer(256)
    user32.GetClassNameW(hwnd, klass, 256)
    pid = wt.DWORD()
    user32.GetWindowThreadProcessId(hwnd, ctypes.byref(pid))
    exe = "?"
    kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
    handle = kernel32.OpenProcess(0x1000, False, pid.value)
    if handle:
        buffer = ctypes.create_unicode_buffer(512)
        size = wt.DWORD(512)
        if kernel32.QueryFullProcessImageNameW(handle, 0, buffer,
                                                ctypes.byref(size)):
            exe = Path(buffer.value).name
        kernel32.CloseHandle(handle)
    return f"'{title.value}' (class {klass.value}, {exe}, pid {pid.value})"


def check(condition, message):
    if not condition:
        raise E2EFailure(message)


def write_profile(profile: Path) -> None:
    ini = profile / "Chronexa" / "Chronexa.ini"
    ini.parent.mkdir(parents=True, exist_ok=True)
    # QSettings' INI dialect: a quoted value is one string even with commas in
    # it, and embedded quotes are backslash-escaped.
    escaped = RULES_JSON.replace("\\", "\\\\").replace('"', '\\"')
    ini.write_text(
        "[categories]\n"
        f'rules="{escaped}"\n'
        # Not "[general]": QSettings reads that as its special [General]
        # section, i.e. no group at all, and the language is never found.
        "[%General]\n"
        "language=en\n",
        encoding="utf-8",
    )


def windows_of(pid: int) -> list:
    found = []

    def collect(hwnd, _):
        owner = wt.DWORD()
        user32.GetWindowThreadProcessId(hwnd, ctypes.byref(owner))
        if owner.value == pid and user32.IsWindowVisible(hwnd):
            buffer = ctypes.create_unicode_buffer(256)
            user32.GetWindowTextW(hwnd, buffer, 256)
            if buffer.value:
                found.append(hwnd)
        return True

    user32.EnumWindows(WNDENUMPROC(collect), 0)
    return found


def wait_for_window(process: subprocess.Popen, seconds: float):
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        check(process.poll() is None,
              f"Chronexa exited during start-up with code {process.returncode}")
        found = windows_of(process.pid)
        if found:
            return found[0]
        time.sleep(0.25)
    raise E2EFailure("Chronexa's window did not appear within "
                     f"{seconds:.0f} s")


def bring_to_front(hwnd) -> bool:
    # Windows refuses SetForegroundWindow from a background process, except
    # while Alt is held. The injected keys also reset the idle timer Chronexa
    # checks -- otherwise an away user would record nothing.
    #
    # F24 is tapped before Alt is released: a bare Alt press-and-release puts
    # the receiving window into system-menu mode, a modal loop that blocks
    # Tk's update() until something else is clicked. F24 has no character, so
    # it triggers no mnemonic and no beep.
    for _ in range(5):
        user32.keybd_event(VK_MENU, 0, 0, 0)
        user32.SetForegroundWindow(hwnd)
        user32.keybd_event(VK_F24, 0, 0, 0)
        user32.keybd_event(VK_F24, 0, KEYEVENTF_KEYUP, 0)
        user32.keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0)
        time.sleep(0.1)
        if user32.GetForegroundWindow() == hwnd:
            return True
    return False


def pump(root: tk.Tk, seconds: float) -> None:
    start = time.monotonic()
    while time.monotonic() - start < seconds:
        root.update()
        time.sleep(0.02)
    # A modal loop inside update() (a window menu, a drag) would stretch the
    # hold and quietly invalidate the expected durations.
    check(time.monotonic() - start < seconds + 2,
          f"the window was blocked for {time.monotonic() - start:.0f} s "
          f"during a {seconds} s hold")


def wait_for_fresh_minute() -> None:
    # Chronexa flushes on every wall-clock minute, and a flush hands over the
    # open session in two parts. The drive below takes ~22 s, so starting it
    # just after a minute boundary keeps every run in one piece.
    second = time.localtime().tm_sec
    if second > 5:
        time.sleep(62 - second)


def drive_window() -> list:
    """Shows the titles Chronexa should record; returns the expected rows."""
    root = tk.Tk()
    root.title(f"YouTube - cats - {MARKER}")
    root.geometry("520x120+200+200")
    root.attributes("-topmost", True)
    tk.Label(root, text="Chronexa end-to-end test - do not type").pack(
        expand=True)
    root.update()
    hwnd = int(root.wm_frame(), 16)

    def hold(title, seconds):
        root.title(title)
        root.update()
        check(bring_to_front(hwnd),
              f"could not bring the test window to the front for '{title}'")
        pump(root, seconds)
        check(user32.GetForegroundWindow() == hwnd,
              f"the test window lost the foreground during '{title}' to "
              f"{foreground_description()}")

    try:
        hold(f"YouTube - cats - {MARKER}", 5)
        hold(f"Docs - spec - {MARKER}", 5)
        # Same category with a title rewritten every second: one session.
        for tick in range(1, 5):
            hold(f"Docs - tick {tick} - {MARKER}", 1)
        hold(f"YouTube - dogs - {MARKER}", 5)
    finally:
        root.destroy()

    return [
        (f"YouTube - cats - {MARKER}", 4),
        (f"Docs - tick 4 - {MARKER}", 8),
        (f"YouTube - dogs - {MARKER}", 4),
    ]


def close(process: subprocess.Popen, hwnd) -> None:
    user32.PostMessageW(hwnd, WM_CLOSE, 0, 0)
    try:
        process.wait(timeout=20)
    except subprocess.TimeoutExpired:
        process.kill()
        raise E2EFailure("Chronexa did not exit within 20 s of WM_CLOSE")
    check(process.returncode == 0,
          f"Chronexa exited with code {process.returncode}")


def recorded_rows(database: Path) -> list:
    check(database.exists(), f"no database was written at {database}")
    # closing(): a sqlite3 connection used as a context manager only commits,
    # and the open handle would keep the profile from being deleted.
    with contextlib.closing(sqlite3.connect(database)) as db:
        return db.execute(
            "SELECT title, started_on, ended_on FROM activity "
            "WHERE lower(app_name) LIKE '%python%' AND title LIKE ? "
            "ORDER BY started_on",
            (f"%{MARKER}%",),
        ).fetchall()


def verify(rows: list, expected: list) -> None:
    listing = "\n".join(
        f"  {title!r}: {(end - start) / 1000:.1f} s" for title, start, end in rows)
    check(len(rows) == len(expected),
          f"expected {len(expected)} sessions, one per category run, "
          f"got {len(rows)}:\n{listing}")

    for (title, start, end), (want_title, min_seconds) in zip(rows, expected):
        check(title == want_title,
              f"expected a session titled {want_title!r}, got {title!r}")
        check((end - start) / 1000 >= min_seconds,
              f"{title!r} lasted {(end - start) / 1000:.1f} s, "
              f"expected at least {min_seconds} s")

    for (_, _, previous_end), (title, start, _) in zip(rows, rows[1:]):
        check(abs(start - previous_end) <= 1500,
              f"{title!r} does not follow on from the previous session "
              f"({(start - previous_end) / 1000:+.1f} s gap)")


def check_log(profile: Path) -> None:
    for log in profile.glob("log_*.log"):
        for line in log.read_text(encoding="utf-8", errors="replace").splitlines():
            if ".qml" in line and ("[WARN" in line or "[CRIT" in line):
                raise E2EFailure(f"QML reported a problem: {line}")


def check_real_history_untouched(since_ms: int) -> None:
    # Only this run's rows: a real Chronexa left open during an earlier run
    # recorded the test windows like any other, and those rows stay behind.
    real = Path(os.environ.get("APPDATA", "")) / "Chronexa" / "Chronexa" / "chronexa.db"
    if not real.exists():
        return
    with contextlib.closing(
            sqlite3.connect(f"file:{real}?mode=ro", uri=True)) as db:
        leaked = db.execute("SELECT count(*) FROM activity "
                            "WHERE title LIKE ? AND ended_on >= ?",
                            (f"%{MARKER}%", since_ms)).fetchone()[0]
    check(leaked == 0,
          f"{leaked} test sessions reached {real} during this run -- a leak "
          "past --profile, or your own Chronexa was running and recorded them")


def main() -> int:
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    exe = Path(sys.argv[1])
    started_ms = int(time.time() * 1000)
    profile = Path(tempfile.mkdtemp(prefix="chronexa-e2e-"))
    write_profile(profile)

    env = dict(os.environ)
    env.pop("QT_QPA_PLATFORM", None)  # a real window, like a real user's
    # Its log is written into the profile; the console only needs our verdict.
    process = subprocess.Popen([str(exe), "--profile", str(profile)], env=env,
                               stdout=subprocess.DEVNULL,
                               stderr=subprocess.DEVNULL)
    try:
        hwnd = wait_for_window(process, 30)
        wait_for_fresh_minute()
        expected = drive_window()
        close(process, hwnd)

        rows = recorded_rows(profile / "chronexa.db")
        verify(rows, expected)
        check_log(profile)
        check_real_history_untouched(started_ms)
    except E2EFailure as failure:
        print(f"FAIL: {failure}")
        print(f"Profile kept for inspection: {profile}")
        return 1
    finally:
        if process.poll() is None:
            process.kill()

    for title, start, end in rows:
        print(f"  recorded {title!r}: {(end - start) / 1000:.1f} s")
    print("PASS: one session per category run, ticking title kept whole")
    shutil.rmtree(profile)
    return 0


if __name__ == "__main__":
    sys.exit(main())
