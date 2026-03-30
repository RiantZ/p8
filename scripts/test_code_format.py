#!/usr/bin/env python3

import subprocess
import pytest
from pathlib import Path
from unittest.mock import patch, MagicMock
import code_format as cf


def make_tree(root, paths):
    """Create files from a list of relative path strings under root."""
    for rel in paths:
        p = root / rel
        p.parent.mkdir(parents=True, exist_ok=True)
        p.touch()


# ---------------------------------------------------------------------------
# get_all_source_files
# ---------------------------------------------------------------------------
class TestGetAllSourceFiles:
    def test_returns_source_files(self, tmp_path, monkeypatch):
        make_tree(tmp_path, ["src/main.cpp", "src/utils.h"])
        monkeypatch.setattr(cf, "git_project_root_path", tmp_path)

        result = cf.get_project_files()

        names = [f.name for f in result]
        assert "main.cpp" in names
        assert "utils.h" in names

    def test_excludes_non_source_extensions(self, tmp_path, monkeypatch):
        make_tree(tmp_path, ["src/main.cpp", "docs/readme.txt", "CMakeLists.txt"])
        monkeypatch.setattr(cf, "git_project_root_path", tmp_path)

        result = cf.get_project_files()

        assert all(f.suffix != ".txt" for f in result)

    def test_excludes_directory_matching_pattern(self, tmp_path, monkeypatch):
        make_tree(tmp_path, ["src/main.cpp", "_Build_debug/generated.cpp"])
        monkeypatch.setattr(cf, "git_project_root_path", tmp_path)

        result = cf.get_project_files()

        names = [f.name for f in result]
        assert "main.cpp" in names
        assert "generated.cpp" not in names

    def test_excludes_git_directory(self, tmp_path, monkeypatch):
        make_tree(tmp_path, ["src/main.cpp", ".git/hooks/pre-commit.cpp"])
        monkeypatch.setattr(cf, "git_project_root_path", tmp_path)

        result = cf.get_project_files()

        assert not any(".git" in f.parts for f in result)

    def test_excludes_file_matching_custom_pattern(self, tmp_path, monkeypatch):
        make_tree(tmp_path, ["src/main.cpp", "src/foo_lib.cpp"])
        monkeypatch.setattr(cf, "git_project_root_path", tmp_path)
        monkeypatch.setattr(cf, "EXCLUDE_PATTERNS", (("file", r"_lib\.cpp$"),))

        result = cf.get_project_files()

        names = [f.name for f in result]
        assert "main.cpp" in names
        assert "foo_lib.cpp" not in names

    def test_returns_empty_for_empty_directory(self, tmp_path, monkeypatch):
        monkeypatch.setattr(cf, "git_project_root_path", tmp_path)

        result = cf.get_project_files()

        assert result == []

    def test_returns_path_objects(self, tmp_path, monkeypatch):
        make_tree(tmp_path, ["src/main.cpp"])
        monkeypatch.setattr(cf, "git_project_root_path", tmp_path)

        result = cf.get_project_files()

        assert all(isinstance(f, Path) for f in result)


# ---------------------------------------------------------------------------
# get_git_staged_files
# ---------------------------------------------------------------------------


def git_status(lines):
    """Build a fake git status --porcelain=1 output from a list of lines."""
    return "\n".join(lines) + "\n"


class TestGetGitStagedFiles:
    def _run(self, tmp_path, monkeypatch, porcelain_output):
        monkeypatch.setattr(cf, "git_project_root_path", tmp_path)
        with patch("subprocess.check_output", return_value=porcelain_output):
            return cf.get_git_staged_files()

    def test_includes_modified_source_file(self, tmp_path, monkeypatch):
        result = self._run(tmp_path, monkeypatch, git_status(["M  src/main.cpp"]))

        assert any("main.cpp" in str(f) for f in result)

    def test_includes_added_source_file(self, tmp_path, monkeypatch):
        result = self._run(tmp_path, monkeypatch, git_status(["A  src/new.h"]))

        assert any("new.h" in str(f) for f in result)

    def test_includes_all_valid_statuses(self, tmp_path, monkeypatch):
        lines = [
            "M  src/a.cpp",
            "A  src/b.h",
            "T  src/c.hpp",
            "R  src/d.c",
            "C  src/e.cpp",
            "?? src/f.cpp",
        ]
        result = self._run(tmp_path, monkeypatch, git_status(lines))

        assert len(result) == 6

    def test_excludes_non_source_extensions(self, tmp_path, monkeypatch):
        result = self._run(tmp_path, monkeypatch, git_status(["M  CMakeLists.txt"]))

        assert result == []

    def test_excludes_path_matching_exclude_pattern(self, tmp_path, monkeypatch):
        result = self._run(
            tmp_path, monkeypatch, git_status(["M  _Build/generated.cpp"])
        )

        assert result == []

    def test_ignores_unknown_status_and_prints_error(
        self, tmp_path, monkeypatch, capsys
    ):
        result = self._run(tmp_path, monkeypatch, git_status(["X  src/main.cpp"]))

        assert result == []
        assert "ERROR" in capsys.readouterr().out

    def test_returns_empty_on_git_error(self, tmp_path, monkeypatch):
        monkeypatch.setattr(cf, "git_project_root_path", tmp_path)
        with patch(
            "subprocess.check_output",
            side_effect=subprocess.CalledProcessError(1, "git"),
        ):
            result = cf.get_git_staged_files()

        assert result == []

    def test_returns_path_objects(self, tmp_path, monkeypatch):
        result = self._run(tmp_path, monkeypatch, git_status(["M  src/main.cpp"]))

        assert all(isinstance(f, Path) for f in result)


# ---------------------------------------------------------------------------
# is_path_excluded
# ---------------------------------------------------------------------------


class TestIsPathExcluded:
    def test_returns_false_for_normal_path(self):
        assert cf.is_path_excluded(Path("/project/src/main.cpp")) is False

    def test_dir_prefix_pattern_matches(self):
        assert cf.is_path_excluded(Path("/project/_Build_debug/gen.cpp")) is True

    def test_dir_prefix_pattern_does_not_match_file(self):
        # "_Build" in filename — should NOT be excluded by ("dir", ...) pattern
        assert cf.is_path_excluded(Path("/project/src/_Build_generated.cpp")) is False

    def test_dir_exact_pattern_matches(self):
        assert cf.is_path_excluded(Path("/project/.git/hooks/pre-commit.cpp")) is True

    def test_file_suffix_pattern_matches(self):
        patterns = (("file", r"_lib$"),)
        assert cf.is_path_excluded(Path("/project/src/foo_lib"), patterns) is True

    def test_file_pattern_does_not_match_directory(self):
        # "_lib" suffix in a dir name — should NOT be excluded by ("file", ...) pattern
        patterns = (("file", r"_lib$"),)
        assert (
            cf.is_path_excluded(Path("/project/vendor_lib/main.cpp"), patterns) is False
        )

    def test_any_pattern_matches_file(self):
        patterns = (("any", r"forbidden"),)
        assert cf.is_path_excluded(Path("/project/src/forbidden.cpp"), patterns) is True

    def test_any_pattern_matches_directory(self):
        patterns = (("any", r"forbidden"),)
        assert (
            cf.is_path_excluded(Path("/project/forbidden/main.cpp"), patterns) is True
        )

    def test_custom_patterns_override_defaults(self):
        patterns = (("dir", r"^custom_exclude$"),)
        assert cf.is_path_excluded(Path("/project/_Build/gen.cpp"), patterns) is False
        assert (
            cf.is_path_excluded(Path("/project/custom_exclude/gen.cpp"), patterns)
            is True
        )


# ---------------------------------------------------------------------------
# check_formatting
# ---------------------------------------------------------------------------


class TestCheckFormatting:
    def _mock_run(self, returncode):
        mock = MagicMock()
        mock.returncode = returncode
        return mock

    def test_returns_true_when_all_files_formatted(self):
        with patch("subprocess.run", return_value=self._mock_run(0)):
            assert cf.check_formatting([Path("a.cpp"), Path("b.cpp")]) is True

    def test_returns_false_when_any_file_not_formatted(self):
        with patch("subprocess.run", return_value=self._mock_run(1)):
            assert cf.check_formatting([Path("a.cpp")]) is False

    def test_accumulates_errors_across_batches(self, monkeypatch):
        monkeypatch.setattr(cf, "FILES_PROCESSING_BATCH_SIZE", 1)
        results = [self._mock_run(0), self._mock_run(1)]
        with patch("subprocess.run", side_effect=results):
            assert cf.check_formatting([Path("a.cpp"), Path("b.cpp")]) is False

    def test_returns_true_for_empty_file_list(self):
        with patch("subprocess.run") as mock_run:
            assert cf.check_formatting([]) is True
            mock_run.assert_not_called()


# ---------------------------------------------------------------------------
# apply_formatting
# ---------------------------------------------------------------------------


class TestApplyFormatting:
    def _mock_run(self, returncode):
        mock = MagicMock()
        mock.returncode = returncode
        return mock

    def test_returns_true_on_success(self):
        with patch("subprocess.run", return_value=self._mock_run(0)):
            assert cf.apply_formatting([Path("a.cpp")]) is True

    def test_returns_false_on_failure(self):
        with patch("subprocess.run", return_value=self._mock_run(1)):
            assert cf.apply_formatting([Path("a.cpp")]) is False

    def test_returns_true_for_empty_file_list(self):
        with patch("subprocess.run") as mock_run:
            assert cf.apply_formatting([]) is True
            mock_run.assert_not_called()


# ---------------------------------------------------------------------------
# check_clang_format_version
# ---------------------------------------------------------------------------


def make_version_result(version_str):
    mock = MagicMock()
    mock.stdout = version_str
    return mock


class TestCheckClangFormatVersion:
    def test_passes_when_version_meets_minimum(self):
        with patch(
            "subprocess.run",
            return_value=make_version_result("clang-format version 20.0.0"),
        ):
            cf.check_clang_format_version(min_major=20)  # should not raise

    def test_passes_when_version_exceeds_minimum(self):
        with patch(
            "subprocess.run",
            return_value=make_version_result("clang-format version 21.1.0"),
        ):
            cf.check_clang_format_version(min_major=20)  # should not raise

    def test_exits_when_version_too_old(self):
        with patch(
            "subprocess.run",
            return_value=make_version_result("clang-format version 18.0.0"),
        ):
            with pytest.raises(SystemExit) as exc:
                cf.check_clang_format_version(min_major=20)
            assert exc.value.code == 1

    def test_exits_when_not_installed(self):
        with patch("subprocess.run", side_effect=FileNotFoundError):
            with pytest.raises(SystemExit) as exc:
                cf.check_clang_format_version()
            assert exc.value.code == 1

    def test_exits_when_version_not_parseable(self):
        with patch(
            "subprocess.run", return_value=make_version_result("unexpected output")
        ):
            with pytest.raises(SystemExit) as exc:
                cf.check_clang_format_version()
            assert exc.value.code == 1

    def test_prints_requirements_when_not_installed(self, capsys):
        with patch("subprocess.run", side_effect=FileNotFoundError):
            with pytest.raises(SystemExit):
                cf.check_clang_format_version(min_major=20)
        assert "20" in capsys.readouterr().out

    def test_prints_requirements_when_version_too_old(self, capsys):
        with patch(
            "subprocess.run",
            return_value=make_version_result("clang-format version 17.0.0"),
        ):
            with pytest.raises(SystemExit):
                cf.check_clang_format_version(min_major=20)
        assert "20" in capsys.readouterr().out
