#!/usr/bin/env python3
"""
template_checker.py — Commit message validator (Zephyr style)

Enforces the Zephyr commit convention:
  * subject line prefixed with an area, e.g. "drivers: gpio: Add open-drain mode"
  * subject line no longer than 72 characters
  * a Developer Certificate of Origin sign-off (Signed-off-by:)
An issue reference (Fixes #NNNN) is optional, so it is not required here.

Usage: python template_checker.py <commit-msg-file> [<commit-source>]
"""

import re
import sys
from pathlib import Path


# "<area>: <summary>" — one or more colon-separated area segments then the summary.
SUBJECT_PATTERN = re.compile(r"^[A-Za-z0-9_][A-Za-z0-9 _./-]*: .+")
SUBJECT_MAX_LEN = 72
SIGNOFF_PATTERN = re.compile(
    r"^Signed-off-by: [A-Za-z][\w .'-]+ <[^@\s]+@[^>\s]+>$", re.MULTILINE
)
SKIP_SOURCES = {"merge", "squash", "revert"}


def main():
    if len(sys.argv) < 2:
        print("Usage: python template_checker.py <commit-msg-file> [source]")
        sys.exit(1)

    commit_file = Path(sys.argv[1])
    commit_source = sys.argv[2] if len(sys.argv) > 2 else ""

    if commit_source in SKIP_SOURCES:
        sys.exit(0)

    if not commit_file.is_file():
        print(f"ERROR: File not found: {commit_file}")
        sys.exit(1)

    msg = commit_file.read_text(encoding="utf-8")
    subject = msg.splitlines()[0] if msg.splitlines() else ""

    # Skip git-generated revert/merge commits (source arg not always populated)
    if subject.startswith("Revert ") or subject.startswith("Merge "):
        sys.exit(0)

    # 1. Subject line format
    if not SUBJECT_PATTERN.match(subject):
        print("ERROR: Invalid commit subject line.")
        print(f"  Got : {subject}")
        print("  Want: <area>: <summary>   (imperative, capitalized, no trailing period)")
        print("  Examples:")
        print("    drivers: gpio: Add open-drain drive mode")
        print("    bsp: clocks: Program IMO frequency from SFLASH trim")
        print("    doc: Document the pin map")
        sys.exit(1)

    # 2. Subject length
    if len(subject) > SUBJECT_MAX_LEN:
        print(f"ERROR: Subject line is {len(subject)} characters; keep it <= {SUBJECT_MAX_LEN}.")
        print(f"  Got : {subject}")
        sys.exit(1)

    # 3. Sign-off (Developer Certificate of Origin)
    if not SIGNOFF_PATTERN.search(msg):
        print("ERROR: Commit message must contain a valid Signed-off-by with a real name.")
        print("  Required format:")
        print("    Signed-off-by: First Last <email@example.com>")
        print("  A single name or missing email is not accepted.")
        sys.exit(1)

    sys.exit(0)


if __name__ == "__main__":
    main()
