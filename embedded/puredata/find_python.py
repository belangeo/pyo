"""Find a native Windows Python with pyo and embedding files on PATH."""

import os
from pathlib import Path
import subprocess
import sys


PROBE = """
import contextlib
import io
from pathlib import Path
import sys
import sysconfig

root = Path(sys.base_prefix)
library = 'python%d%d%s.lib' % (*sys.version_info[:2], '_d' if sysconfig.get_config_var('Py_DEBUG') else '')
if not (root / 'include' / 'Python.h').is_file() or not (root / 'libs' / library).is_file():
    raise SystemExit(1)
with contextlib.redirect_stdout(io.StringIO()):
    import pyo
print(sys.executable)
"""


def find_python():
    env = dict(os.environ, PYO_GUI_WX="0", PYO_SERVER_AUDIO="embedded")
    seen = set()
    for directory in os.get_exec_path():
        candidate = Path(directory) / "python.exe"
        key = os.path.normcase(os.path.abspath(candidate))
        # WindowsApps aliases can launch the Store instead of an interpreter.
        if key in seen or candidate.parent.name.lower() == "windowsapps":
            continue
        seen.add(key)
        if not candidate.is_file():
            continue
        try:
            result = subprocess.run(
                [str(candidate), "-c", PROBE],
                cwd=Path(__file__).parent,
                env=env,
                capture_output=True,
                text=True,
                timeout=10,
                creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0),
            )
        except (OSError, subprocess.TimeoutExpired):
            continue
        if result.returncode == 0 and result.stdout.strip():
            return result.stdout.strip().replace("\\", "/")
    return None


if __name__ == "__main__":
    executable = find_python()
    if executable is None:
        print("No native Python with pyo and embedding files found on PATH.", file=sys.stderr)
        raise SystemExit(1)
    print(executable)
