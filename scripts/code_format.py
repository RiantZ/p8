#!/usr/bin/env python3
import subprocess
import argparse
import os
import re
import sys
from pathlib import Path

git_project_root_path = Path(
    subprocess.check_output(["git", "rev-parse", "--show-toplevel"]).decode().strip()
)

# Configuration
INLCUDE_EXTENSIONS = (".cpp", ".h", ".hpp", ".c")
CLANG_FORMAT_FILE_PATH = os.path.join(git_project_root_path, ".clang-format")
EXCLUDE_PATTERNS = (
    ("dir", r"^_Build"),  # prefix: directory starts with "_Build"
    ("dir", r"^\.git$"),  # exact:  directory equals ".git"
    ("file", r"_lib$"),  # suffix: filename ends with "_lib"
)
FILES_PROCESSING_BATCH_SIZE = 8


# ***********************************************************************************************************************
def is_path_excluded(file_path: Path, patterns=None) -> bool:
    """Return True if any component of the path matches one of the exclude patterns.

    Each pattern is a tuple (kind, regex) where kind is:
      "file" - match against the filename only (last component)
      "dir"  - match against directory components only (all but last)
      "any"  - match against all components
    """
    if patterns is None:
        patterns = EXCLUDE_PATTERNS
    parts = file_path.parts
    file_part = parts[-1:]
    dir_parts = parts[:-1]

    for kind, pattern in patterns:
        if kind == "file":
            candidates = file_part
        elif kind == "dir":
            candidates = dir_parts
        else:
            candidates = parts
        if any(re.search(pattern, part) for part in candidates):
            return True
    return False


# ***********************************************************************************************************************
def get_git_staged_files() -> list[Path]:
    """Get only staged/changed files (added, modified, copied) tracked by Git."""
    try:
        output = subprocess.check_output(["git", "status", "--porcelain=1"], text=True)
    except subprocess.CalledProcessError as e:
        print("ERROR: running git status return exception: ", e)
        return []

    git_staged_files = []

    for line in output.splitlines():
        status = line[:2].strip()
        path = line[3:].strip()
        # Staged files status
        # T (Type)
        # M (Modified)
        # A (Added)
        # R (Renamed)
        # C (Copied)
        # ?? (Unknown)
        if re.match(r"^(T|M|A|R|C|\?\?)$", status):
            full_path = git_project_root_path / path
            if full_path.suffix in INLCUDE_EXTENSIONS and not is_path_excluded(
                full_path
            ):
                git_staged_files.append(full_path)
        else:
            print(f"ERROR: Unknown status: {status} for file {path}, ignored!")

    return git_staged_files


# ***********************************************************************************************************************
def get_project_files() -> list[Path]:
    return [
        p
        for p in git_project_root_path.rglob("*")
        if p.suffix in INLCUDE_EXTENSIONS and not is_path_excluded(p)
    ]


# ***********************************************************************************************************************
def check_formatting(files: list, clang_path: str = CLANG_FORMAT_FILE_PATH) -> bool:
    err_count = 0
    style = "-style=file:" + clang_path
    for i in range(0, len(files), FILES_PROCESSING_BATCH_SIZE):
        result = subprocess.run(
            ["clang-format", "--Werror", "--dry-run", style]
            + [str(f) for f in files[i : i + FILES_PROCESSING_BATCH_SIZE]],
            encoding="utf-8",
            text=True,
        )
        err_count += result.returncode
    return 0 == err_count


# ***********************************************************************************************************************
def apply_formatting(files: list, clang_path: str = CLANG_FORMAT_FILE_PATH) -> bool:
    err_count = 0
    style = "-style=file:" + clang_path
    for i in range(0, len(files), FILES_PROCESSING_BATCH_SIZE):
        result = subprocess.run(
            ["clang-format", "--verbose", style, "-i"]
            + [str(f) for f in files[i : i + FILES_PROCESSING_BATCH_SIZE]],
            encoding="utf-8",
            text=True,
        )
        err_count += result.returncode
    return 0 == err_count


# ***********************************************************************************************************************
def check_clang_format_version(min_major: int = 20) -> None:
    """Check that clang-format is installed and meets the minimum version requirement."""
    requirements = (
        f"  For commit clang-format is required, version >= {min_major}\n"
        f"  Install:  https://releases.llvm.org/download.html\n"
        f"            macOS:  brew install clang-format\n"
        f"            Ubuntu: sudo apt install clang-format-{min_major}"
    )

    try:
        result = subprocess.run(
            ["clang-format", "--version"], capture_output=True, text=True, check=True
        )
    except FileNotFoundError:
        print(
            f"Error: clang-format is not installed or not found in PATH.\n{requirements}"
        )
        sys.exit(1)

    match = re.search(r"clang-format version (\d+)", result.stdout)
    if not match:
        print(
            f"Error: could not parse clang-format version from: {result.stdout.strip()}\n{requirements}"
        )
        sys.exit(1)

    major = int(match.group(1))
    if major < min_major:
        print(f"Error: clang-format version {major} is too old.\n{requirements}")
        sys.exit(1)


# ***********************************************************************************************************************
def main() -> None:
    check_clang_format_version()

    parser = argparse.ArgumentParser(
        description="""Clang format helper script.\n
                       Scanning project and helping to identify files need to be formatted.\n
                       Script is used as hook for git\n"""
    )
    parser.add_argument(
        "--check",
        action="store_true",
        help="Check files formatting  using Clang format file",
    )
    parser.add_argument(
        "--format",
        action="store_true",
        help="Apply format using Clang format file",
    )

    parser.add_argument(
        "--all", action="store_true", help="Run on all source files in the repo"
    )
    parser.add_argument(
        "--staged", action="store_true", help="Run only on staged/changed files"
    )

    args = parser.parse_args()

    if not args.check and not args.apply:
        print("Error: Please use --format or --apply options")
        sys.exit(1)

    if args.all:
        files = get_project_files()
    else:
        files = get_git_staged_files()

    if not files:
        print("No files for processing found :-(")
        sys.exit(0)

    success = False
    if args.check:
        success = check_formatting(files, CLANG_FORMAT_FILE_PATH)
    elif args.apply:
        success = apply_formatting(files, CLANG_FORMAT_FILE_PATH)

    if not success:
        print("Found improperly formatted files.\n")
        sys.exit(1)


if __name__ == "__main__":
    main()
