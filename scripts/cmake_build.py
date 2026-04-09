#!/usr/bin/env python3
"""
Cross-platform CMake build helper.

Usage:
  python scripts/cmake_build.py                                    # configure only
  python scripts/cmake_build.py --build                            # configure + build
  python scripts/cmake_build.py --clean                            # wipe build dir, then configure
  python scripts/cmake_build.py --clean --build                    # wipe, configure, build
  python scripts/cmake_build.py --preset macos-local-kit --build   # use a custom preset
"""

import argparse
import shutil
import subprocess
import sys
from pathlib import Path

SOURCE_DIR = Path(__file__).parent.parent.resolve()


def get_platform() -> str:
    if sys.platform.startswith("win"):
        return "windows"
    if sys.platform.startswith("darwin"):
        return "macos"
    return "linux"


def get_build_dir(platform: str) -> Path:
    suffix = {"windows": "_Build_wnd", "macos": "_Build_mac", "linux": "_Build_lnx"}[
        platform
    ]
    return SOURCE_DIR / suffix


def run(cmd: list[str]) -> None:
    print(f"+ {' '.join(cmd)}")
    result = subprocess.run(cmd)
    if result.returncode != 0:
        sys.exit(result.returncode)


def install_git_hook() -> None:
    """Copy scripts/git_hooks/pre-commit to .git/hooks/pre-commit, overwriting if exists."""
    hook_src = SOURCE_DIR / "scripts" / "git_hooks" / "pre-commit"
    hook_dst = SOURCE_DIR / ".git" / "hooks" / "pre-commit"

    if not hook_src.exists():
        print(f"Warning: hook source not found: {hook_src}")
        return

    hook_dst.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(hook_src, hook_dst)

    if sys.platform != "win32":
        hook_dst.chmod(hook_dst.stat().st_mode | 0o111)

    action = "Updated" if hook_dst.exists() else "Installed"
    print(f"{action} git pre-commit hook: {hook_dst}")


def main() -> None:
    parser = argparse.ArgumentParser(
        description="CMake configure/build helper. Works on Windows, Linux and macOS."
    )
    parser.add_argument("--build", action="store_true", help="Build after configuring")
    parser.add_argument(
        "--clean", action="store_true", help="Remove build directory before configuring"
    )
    parser.add_argument(
        "--preset", default=None, help="CMake preset name (default: auto-detect by platform)"
    )
    args = parser.parse_args()

    platform = get_platform()
    preset = args.preset or platform
    build_dir = get_build_dir(platform)

    if args.clean and build_dir.exists():
        print(f"Removing {build_dir} ...")
        shutil.rmtree(build_dir)

    run(["cmake", "--preset", preset, "-S", str(SOURCE_DIR)])

    if args.build:
        run(["cmake", "--build", str(build_dir), "--config", "Release"])

    install_git_hook()


if __name__ == "__main__":
    main()
