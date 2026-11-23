#!/usr/bin/env python3
"""
check_commit_sign_off.py — Validates git config and auto-appends Signed-off-by
Usage: python check_commit_sign_off.py <commit-msg-file> [<commit-source>]
"""

import subprocess
import sys
from pathlib import Path


SKIP_SOURCES = {"merge", "squash", "rebase"}

# Characters that would break or inject content into the Signed-off-by line.
_NAME_FORBIDDEN = frozenset('\n\r<>')
_EMAIL_FORBIDDEN = frozenset('\n\r>')


def git_config(key: str) -> str:
    try:
        result = subprocess.run(
            ["git", "config", key],
            capture_output=True, text=True, check=True
        )
        return result.stdout.strip()
    except subprocess.CalledProcessError:
        return ""


def _validate_git_field(value: str, key: str, forbidden: frozenset) -> None:
    bad = [repr(c) for c in forbidden if c in value]
    if bad:
        print(f"\nERROR: git {key} contains invalid character(s): {', '.join(bad)}")
        print(f"  Fix with: git config --global {key} 'correct value'")
        sys.exit(1)


def main():
    if len(sys.argv) < 2:
        print("Usage: python check_commit_sign_off.py <commit-msg-file> [source]")
        sys.exit(1)

    commit_file = Path(sys.argv[1])
    commit_source = sys.argv[2] if len(sys.argv) > 2 else ""

    # Skip merge and squash commits
    if commit_source in SKIP_SOURCES:
        sys.exit(0)

    # Validate git config
    name = git_config("user.name")
    email = git_config("user.email")

    if not name:
        print("\nERROR: git user.name is not set.")
        print("  Fix with: git config --global user.name 'First Last'")
        sys.exit(1)

    if not email:
        print("\nERROR: git user.email is not set.")
        print("  Fix with: git config --global user.email 'you@example.com'")
        sys.exit(1)

    if len(name.split()) < 2:
        print(f"\nERROR: git user.name '{name}' does not look like a full name.")
        print("  The Developer Certificate of Origin requires a real first and last name.")
        print("  Fix with: git config --global user.name 'First Last'")
        sys.exit(1)

    # Reject values that would inject newlines or break the Signed-off-by format.
    _validate_git_field(name, "user.name", _NAME_FORBIDDEN)
    _validate_git_field(email, "user.email", _EMAIL_FORBIDDEN)

    # Append sign-off if not already present
    signoff = f"Signed-off-by: {name} <{email}>"
    msg = commit_file.read_text(encoding="utf-8")

    if signoff not in msg:
        if not msg.endswith("\n"):
            msg += "\n"
        commit_file.write_text(f"{msg}{signoff}\n", encoding="utf-8")

    sys.exit(0)


if __name__ == "__main__":
    main()
