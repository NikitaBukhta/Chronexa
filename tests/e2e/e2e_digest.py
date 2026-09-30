"""End-to-end: a daily summary missed while Chronexa was closed goes out on launch.

Runs the real Chronexa.exe against throwaway profiles (--profile) whose
summary was due at midnight and never sent:

1. catching up on: the summary is sent, late, soon after launch;
2. the same profile again: it is not repeated -- the day is remembered;
3. catching up off: the missed summary is dropped, nothing is shown.

No window is driven and nothing is typed, but a real tray balloon appears. It
needs the Debug build: the evidence is at info level, which Release filters
out. Between midnight and 00:15 nothing is overdue yet, so it skips.

Usage: e2e_digest.py <path to Chronexa.exe>
"""

import os
import shutil
import subprocess
import sys
import tempfile
import time
from pathlib import Path

from e2e_categories import E2EFailure, check, check_log, close, wait_for_window

GOALS_JSON = '[{"category":"E2E Work","days":127,"kind":"target","minutes":60}]'
RULES_JSON = '[{"apps":["python"],"category":"E2E Work","title":""}]'

# The first look at the summary follows 3 s after start-up.
SETTLE_SECONDS = 8


def ini_string(value: str) -> str:
    return '"' + value.replace("\\", "\\\\").replace('"', '\\"') + '"'


def write_profile(profile: Path, catch_up: bool) -> None:
    ini = profile / "Chronexa" / "Chronexa.ini"
    ini.parent.mkdir(parents=True, exist_ok=True)
    ini.write_text(
        "[categories]\n"
        f"rules={ini_string(RULES_JSON)}\n"
        "[goals]\n"
        f"daily={ini_string(GOALS_JSON)}\n"
        "notify=true\n"
        "digestTime=custom\n"
        "digestMinutes=0\n"
        f"digestCatchUp={'true' if catch_up else 'false'}\n"
        # Not "[general]": QSettings reads that as its special [General]
        # section, i.e. no group at all, and the language is never found.
        "[%General]\n"
        "language=en\n",
        encoding="utf-8",
    )


def run_once(exe: Path, profile: Path) -> list:
    """Starts Chronexa, lets it settle, closes it; returns the new log lines."""
    before = set(profile.glob("log_*.log"))
    env = dict(os.environ)
    env.pop("QT_QPA_PLATFORM", None)
    process = subprocess.Popen([str(exe), "--profile", str(profile)], env=env,
                               stdout=subprocess.DEVNULL,
                               stderr=subprocess.DEVNULL)
    try:
        hwnd = wait_for_window(process, 30)
        time.sleep(SETTLE_SECONDS)
        close(process, hwnd)
    finally:
        if process.poll() is None:
            process.kill()
    # Log names carry the second they were opened in; keep runs apart.
    time.sleep(1.1)

    lines = []
    for log in sorted(set(profile.glob("log_*.log")) - before):
        lines += log.read_text(encoding="utf-8",
                               errors="replace").splitlines()
    check(any('applied as "en"' in line for line in lines),
          "the profile's language=en was not applied")
    return lines


def summaries(lines: list) -> list:
    return [line for line in lines if "Notification:" in line
            and "goals" in line.split("Notification:", 1)[1]]


def main() -> int:
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    now = time.localtime()
    if now.tm_hour == 0 and now.tm_min < 15:
        print("SKIP: the midnight summary is not overdue yet")
        return 0

    exe = Path(sys.argv[1])
    catching_up = Path(tempfile.mkdtemp(prefix="chronexa-e2e-"))
    dropping = Path(tempfile.mkdtemp(prefix="chronexa-e2e-"))
    write_profile(catching_up, catch_up=True)
    write_profile(dropping, catch_up=False)
    try:
        first = run_once(exe, catching_up)
        check(any("Summary sent late" in line for line in first),
              "the missed summary was not sent on launch")
        sent = summaries(first)
        check(len(sent) == 1,
              f"expected one summary in the tray, got {len(sent)}:\n"
              + "\n".join(sent))
        # An empty profile: yesterday's target was missed, 0 of 1.
        check("0 of 1 met" in sent[0], f"unexpected summary: {sent[0]}")

        again = run_once(exe, catching_up)
        check(not summaries(again),
              "the summary was sent again after a restart the same day")

        dropped = run_once(exe, dropping)
        check(any("catching up is off" in line for line in dropped),
              "with catching up off, the missed summary was not settled")
        check(not summaries(dropped),
              "with catching up off, a missed summary was still shown")

        for profile in (catching_up, dropping):
            check_log(profile)
    except E2EFailure as failure:
        print(f"FAIL: {failure}")
        print(f"Profiles kept for inspection: {catching_up}, {dropping}")
        return 1

    print(f"  {sent[0].split('Notification:', 1)[1].strip()}")
    print("PASS: a missed summary goes out once on launch, or is dropped "
          "when catching up is off")
    shutil.rmtree(catching_up)
    shutil.rmtree(dropping)
    return 0


if __name__ == "__main__":
    sys.exit(main())
