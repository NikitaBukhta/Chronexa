"""End-to-end: a daily limit passed while Chronexa runs is announced in the tray, once.

Runs the real Chronexa.exe against a throwaway profile (--profile) holding one
category rule and a one-minute limit on it, keeps a matching window in the
foreground across three minute flushes, then closes Chronexa and reads its
log: the limit must be found passed after the second flush and announced
exactly once, not again on the third. A real tray balloon appears meanwhile.

It needs the Debug build (the evidence is at info level, which Release filters
out), takes focus and presses Alt -- run it with `python bootstrap.py e2e`,
not while typing. It holds the window for a little over three minutes.

Usage: e2e_goals.py <path to Chronexa.exe>
"""

import contextlib
import os
import shutil
import sqlite3
import subprocess
import sys
import tempfile
import time
import tkinter as tk
from pathlib import Path

from e2e_categories import (E2EFailure, bring_to_front, check, check_log,
                            close, foreground_description, pump, user32, wait_for_fresh_minute,
                            wait_for_window)

MARKER = "chronexa-e2e-goals"
CATEGORY = "E2E Distraction"

# The window below belongs to this Python process, whose recorded app name is
# the interpreter's file description ("Python") or, without one, "python".
RULES_JSON = f'[{{"apps":["python"],"category":"{CATEGORY}","title":"YouTube"}}]'
GOALS_JSON = f'[{{"category":"{CATEGORY}","days":127,"kind":"limit","minutes":1}}]'

# Three flushes: ~55 s (within the limit), ~115 s (past it), ~175 s (still
# past it, and must stay quiet). Held in slices: Chronexa counts a user idle
# after 120 s without input, and each slice presses a key.
HOLD_SLICES = [30] * 6 + [8]


def ini_string(value: str) -> str:
    # QSettings' INI dialect: a quoted value is one string even with commas in
    # it, and embedded quotes are backslash-escaped.
    return '"' + value.replace("\\", "\\\\").replace('"', '\\"') + '"'


def write_profile(profile: Path) -> None:
    ini = profile / "Chronexa" / "Chronexa.ini"
    ini.parent.mkdir(parents=True, exist_ok=True)
    ini.write_text(
        "[categories]\n"
        f"rules={ini_string(RULES_JSON)}\n"
        "[goals]\n"
        f"daily={ini_string(GOALS_JSON)}\n"
        "notify=true\n"
        # The daily summary has its own scenario (e2e_digest); here it would
        # add a second balloon on launch.
        "digestTime=off\n"
        # Not "[general]": QSettings reads that as its special [General]
        # section, i.e. no group at all, and the language is never found.
        "[%General]\n"
        "language=en\n",
        encoding="utf-8",
    )


def drive_window() -> float:
    """Keeps a distraction in front; returns how long it was held."""
    root = tk.Tk()
    title = f"YouTube - cats - {MARKER}"
    root.title(title)
    root.geometry("520x120+200+200")
    root.attributes("-topmost", True)
    tk.Label(root, text="Chronexa end-to-end test - do not type").pack(
        expand=True)
    root.update()
    hwnd = int(root.wm_frame(), 16)

    start = time.monotonic()
    try:
        for seconds in HOLD_SLICES:
            check(bring_to_front(hwnd),
                  "could not bring the test window to the front")
            pump(root, seconds)
            check(user32.GetForegroundWindow() == hwnd,
                  "the test window lost the foreground to "
                  f"{foreground_description()}")
    finally:
        root.destroy()
    return time.monotonic() - start


def recorded_seconds(database: Path) -> float:
    check(database.exists(), f"no database was written at {database}")
    with contextlib.closing(sqlite3.connect(database)) as db:
        total = db.execute(
            "SELECT coalesce(sum(ended_on - started_on), 0) FROM activity "
            "WHERE lower(app_name) LIKE '%python%' AND title LIKE ?",
            (f"%{MARKER}%",),
        ).fetchone()[0]
    return total / 1000


def log_lines(profile: Path) -> list:
    lines = []
    for log in sorted(profile.glob("log_*.log")):
        lines += log.read_text(encoding="utf-8",
                               errors="replace").splitlines()
    return lines


def verify_log(lines: list) -> None:
    check(any("Tray icon shown" in line for line in lines),
          "the log never says the tray icon was shown -- no system tray, or "
          "info logging is off (a Release build?)")

    check(any('applied as "en"' in line for line in lines),
          "the profile's language=en was not applied; the messages below "
          "would be in the system language")

    crossed = [line for line in lines
               if "Goal crossed:" in line and CATEGORY in line]
    notified = [line for line in lines
                if "Notification:" in line
                and f"Limit passed: {CATEGORY}" in line]
    listing = "\n".join("  " + line for line in crossed + notified)
    check(len(crossed) == 1,
          f"expected the limit to be found passed once, got {len(crossed)}:"
          f"\n{listing}")
    check(len(notified) == 1,
          f"expected one tray notification, got {len(notified)}:\n{listing}")


def verify_goals_kept(profile: Path) -> None:
    ini = (profile / "Chronexa" / "Chronexa.ini").read_text(encoding="utf-8")
    check(CATEGORY in ini.split("[goals]", 1)[-1],
          "the profile's goal was replaced by the first-run defaults")


def main() -> int:
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    exe = Path(sys.argv[1])
    profile = Path(tempfile.mkdtemp(prefix="chronexa-e2e-"))
    write_profile(profile)

    env = dict(os.environ)
    env.pop("QT_QPA_PLATFORM", None)
    process = subprocess.Popen([str(exe), "--profile", str(profile)], env=env,
                               stdout=subprocess.DEVNULL,
                               stderr=subprocess.DEVNULL)
    try:
        hwnd = wait_for_window(process, 30)
        wait_for_fresh_minute()
        held = drive_window()
        close(process, hwnd)

        seconds = recorded_seconds(profile / "chronexa.db")
        check(seconds > 120,
              f"only {seconds:.0f} s of the {held:.0f} s hold were recorded")
        verify_log(log_lines(profile))
        verify_goals_kept(profile)
        check_log(profile)
    except E2EFailure as failure:
        print(f"FAIL: {failure}")
        print(f"Profile kept for inspection: {profile}")
        return 1
    finally:
        if process.poll() is None:
            process.kill()

    print(f"  recorded {seconds:.0f} s against a 60 s limit")
    print("PASS: the limit was announced in the tray once, and only once")
    shutil.rmtree(profile)
    return 0


if __name__ == "__main__":
    sys.exit(main())
