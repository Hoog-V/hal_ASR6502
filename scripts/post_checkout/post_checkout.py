#!/usr/bin/env python3
"""
post-checkout.py — Branch name validator with interactive rename prompt
Usage: python post-checkout.py <prev-head> <new-head> <is-branch-checkout>

Git passes:
  $1 — previous HEAD (commit SHA)
  $2 — new HEAD (commit SHA)
  $3 — 1 if branch checkout, 0 if file checkout
"""

import re
import subprocess
import sys


# test/chore are commit types only; branches are always work-item-linked (feat/fix/docs)
VALID_TYPES = ("feat", "fix", "docs", "qms")
PROTECTED = re.compile(r"^(main|dev)$")
# Description segment is lowercase-only to match what the sanitizer produces
NAMING_PATTERN = re.compile(r"^(feat|fix|docs|qms)/[0-9]+-[a-z0-9][a-z0-9_-]*$")


def run(cmd: list[str], check=True) -> subprocess.CompletedProcess:
    return subprocess.run(cmd, capture_output=True, text=True, check=check)


def current_branch() -> str:
    try:
        return run(["git", "symbolic-ref", "--short", "HEAD"]).stdout.strip()
    except subprocess.CalledProcessError:
        return ""  # detached HEAD


def rename_branch(new_name: str) -> bool:
    exists = run(["git", "show-ref", "--verify", "--quiet", f"refs/heads/{new_name}"], check=False)
    if exists.returncode == 0:
        print(f"  ERROR: Branch '{new_name}' already exists. Rename manually.")
        return False
    result = run(["git", "branch", "-m", new_name], check=False)
    return result.returncode == 0


def prompt(msg: str) -> str:
    """Read a line interactively. Exits cleanly in non-interactive environments."""
    try:
        if sys.platform == "win32":
            sys.stdout.write(msg)
            sys.stdout.flush()
            return input()
        else:
            with open("/dev/tty") as tty:
                sys.stdout.write(msg)
                sys.stdout.flush()
                return tty.readline().strip()
    except (OSError, EOFError):
        # No interactive terminal (CI, piped input); skip the rename silently.
        sys.exit(0)


def main():
    if len(sys.argv) < 4:
        sys.exit(0)

    is_branch_checkout = sys.argv[3]
    if is_branch_checkout != "1":
        sys.exit(0)

    branch = current_branch()
    if not branch:
        sys.exit(0)  # detached HEAD

    if PROTECTED.match(branch):
        sys.exit(0)

    if NAMING_PATTERN.match(branch):
        sys.exit(0)

    # Invalid branch name — prompt to fix
    print(f"\nERROR: Branch name '{branch}' does not follow the naming convention.")
    print("  Required format: type/<issue-num>-description  (e.g. feat/12-ble-reconnect)")
    print()

    # Prompt for issue number
    while True:
        issue_num = prompt("  Enter the issue number this branch relates to (digits only): ")
        if issue_num.isdigit():
            break
        print("  Invalid input — please enter a numeric issue number.")

    # Determine type from existing branch name or prompt
    match = re.match(r"^(feat|fix|docs|qms)", branch)
    if match:
        branch_type = match.group(1)
        # Strip type prefix and any stale issue number to get description
        description = re.sub(rf"^{branch_type}[/\-]*[0-9]*[-_]*", "", branch)
    else:
        print()
        print("  Branch type is also missing or invalid.")
        while True:
            branch_type = prompt(f"  Enter branch type ({'/'.join(VALID_TYPES)}): ")
            if branch_type in VALID_TYPES:
                break
            print(f"  Invalid — choose one of: {', '.join(VALID_TYPES)}")
        description = re.sub(r"^[a-z0-9_-]+[/\-]", "", branch)

    # Sanitize description — keep chars valid per NAMING_PATTERN ([a-z0-9_-])
    description = description.lower()
    description = re.sub(r"[^a-z0-9_-]", "-", description)
    description = re.sub(r"-+", "-", description).strip("-")
    if not description:
        description = "untitled"

    new_branch = f"{branch_type}/{issue_num}-{description}"

    print()
    print(f"  Renaming branch to: {new_branch}")

    if rename_branch(new_branch):
        print(f"  Done. You are now on branch '{new_branch}'.")
        print()
    else:
        print(f"\n  ERROR: Could not rename branch automatically.")
        print(f"  Please rename manually: git branch -m {new_branch}")
        sys.exit(1)


if __name__ == "__main__":
    main()
