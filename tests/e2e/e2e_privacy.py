"""End-to-end: Chronexa keeps excluded windows and hidden titles off the disk.

Runs the real Chronexa.exe against a throwaway profile (--profile) whose
privacy rules exclude one window title and hide another, walks a real window
through both, then closes Chronexa and checks what reached the profile: the
recorded rows, and the raw bytes of the database and log files.

It takes focus and presses Alt, as any UI automation does -- run it with
`python bootstrap.py e2e`, not while typing. Your own history and settings are
never touched: everything lives in a temporary directory.

Usage: e2e_privacy.py <path to Chronexa.exe>
"""

import contextlib
import os
import shutil
import sqlite3
import subprocess
import sys
import tempfile
import tkinter as tk
from pathlib import Path

from e2e_categories import (E2EFailure, bring_to_front, check, check_log,
                            close, pump, user32, wait_for_fresh_minute,
                            wait_for_window)

MARKER = "chronexa-e2e-privacy"
# Neither may appear anywhere in the profile once Chronexa has run.
EXCLUDED_SECRET = "Vault of Aunt Mariia"
HIDDEN_SECRET = "Olena: the door code is 4411"

# The window below belongs to this Python process, whose recorded app name is
# the interpreter's file description ("Python") or, without one, "python".
RULES_JSON = (
    '[{"action":"exclude","apps":["python"],"title":"Vault"},'
    '{"action":"hideTitle","apps":["python"],"title":"Olena"}]'
)


def write_profile(profile: Path) -> None:
    ini = profile / "Chronexa" / "Chronexa.ini"
    ini.parent.mkdir(parents=True, exist_ok=True)
    escaped = RULES_JSON.replace("\\", "\\\\").replace('"', '\\"')
    ini.write_text(
        "[privacy]\n"
        f'rules="{escaped}"\n'
        "[general]\n"
        "language=en\n",
        encoding="utf-8",
    )


def drive_window() -> None:
    root = tk.Tk()
    root.title(f"Docs - {MARKER}")
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
              f"the test window lost the foreground during '{title}'")

    try:
        hold(f"Docs - {MARKER}", 5)
        hold(f"{EXCLUDED_SECRET} - {MARKER}", 5)
        hold(f"{HIDDEN_SECRET} - {MARKER}", 5)
        hold(f"Docs - {MARKER}", 5)
    finally:
        root.destroy()


def recorded_rows(database: Path) -> list:
    check(database.exists(), f"no database was written at {database}")
    with contextlib.closing(sqlite3.connect(database)) as db:
        return db.execute(
            "SELECT title, started_on, ended_on FROM activity "
            "WHERE lower(app_name) LIKE '%python%' ORDER BY started_on"
        ).fetchall()


def verify(rows: list) -> None:
    listing = "\n".join(
        f"  {title!r}: {(end - start) / 1000:.1f} s" for title, start, end in rows)
    docs = f"Docs - {MARKER}"
    # Docs, the excluded window (absent), the hidden one (empty), Docs.
    check([title for title, _, _ in rows] == [docs, "", docs],
          f"expected Docs, a title-less session and Docs, got:\n{listing}")
    for title, start, end in rows:
        check((end - start) / 1000 >= 4,
              f"{title!r} lasted {(end - start) / 1000:.1f} s, expected >= 4 s")

    gap = rows[1][1] - rows[0][2]
    check(4000 <= gap <= 6500,
          f"the excluded window should leave a ~5 s hole, got {gap / 1000:.1f} s")
    check(abs(rows[2][1] - rows[1][2]) <= 1500,
          "the Docs session should follow straight on from the hidden one")


def check_no_trace(profile: Path) -> None:
    for path in profile.rglob("*"):
        if not path.is_file():
            continue
        data = path.read_bytes()
        for secret in (EXCLUDED_SECRET, HIDDEN_SECRET):
            for encoded in (secret.encode("utf-8"), secret.encode("utf-16-le")):
                check(encoded not in data,
                      f"{secret!r} was found in {path.name}")


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
        drive_window()
        close(process, hwnd)

        rows = recorded_rows(profile / "chronexa.db")
        verify(rows)
        check_no_trace(profile)
        check_log(profile)
    except E2EFailure as failure:
        print(f"FAIL: {failure}")
        print(f"Profile kept for inspection: {profile}")
        return 1
    finally:
        if process.poll() is None:
            process.kill()

    for title, start, end in rows:
        print(f"  recorded {title!r}: {(end - start) / 1000:.1f} s")
    print("PASS: excluded window absent, hidden title empty, no trace on disk")
    shutil.rmtree(profile)
    return 0


if __name__ == "__main__":
    sys.exit(main())
