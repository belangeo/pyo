import importlib.util
from pathlib import Path
import subprocess
import unittest
from unittest.mock import patch


spec = importlib.util.spec_from_file_location(
    "find_python", Path(__file__).resolve().parents[1] / "find_python.py"
)
finder = importlib.util.module_from_spec(spec)
spec.loader.exec_module(finder)


class FindPythonTests(unittest.TestCase):
    def discover(self, paths, results):
        with (
            patch.object(finder.os, "get_exec_path", return_value=paths),
            patch.object(finder.Path, "is_file", return_value=True),
            patch.object(finder.subprocess, "run", side_effect=results) as run,
        ):
            return finder.find_python(), run.call_args_list

    def test_skips_unsuitable_python_and_stops_at_first_usable_one(self):
        selected, calls = self.discover(
            ["C:/msys/bin", "C:/Native Python", "C:/OtherPython"],
            [
                subprocess.CompletedProcess([], 1, "", "No module named pyo"),
                subprocess.CompletedProcess([], 0, "C:\\Native Python\\python.exe\n", ""),
            ],
        )
        self.assertEqual(selected, "C:/Native Python/python.exe")
        self.assertEqual(len(calls), 2)
        self.assertEqual(calls[1].args[0][0], str(Path("C:/Native Python/python.exe")))

    def test_does_not_launch_windows_store_alias(self):
        selected, calls = self.discover(
            ["C:/WindowsApps", "C:/Python"],
            [subprocess.CompletedProcess([], 0, "C:/Python/python.exe\n", "")],
        )
        self.assertEqual(selected, "C:/Python/python.exe")
        self.assertEqual(len(calls), 1)
        self.assertEqual(calls[0].args[0][0], str(Path("C:/Python/python.exe")))

    def test_skips_a_hung_candidate(self):
        selected, calls = self.discover(
            ["C:/BrokenPython", "C:/Python"],
            [
                subprocess.TimeoutExpired("python.exe", 10),
                subprocess.CompletedProcess([], 0, "C:/Python/python.exe\n", ""),
            ],
        )
        self.assertEqual(selected, "C:/Python/python.exe")
        self.assertEqual(len(calls), 2)

    def test_no_usable_python_and_duplicate_path_entries(self):
        selected, calls = self.discover(
            ["C:/Python", "C:/Python"],
            [subprocess.CompletedProcess([], 1, "", "")],
        )
        self.assertIsNone(selected)
        self.assertEqual(len(calls), 1)


if __name__ == "__main__":
    unittest.main()
