"""
Copyright 2009-2026 Olivier Belanger

This file is part of pyo, a Python module to help digital signal processing
script creation.

Load host ALSA and JACK libraries ahead of auditwheel-bundled copies.
"""

import ctypes
import glob
import os
import platform
import struct
import subprocess
import sys
from pathlib import Path


_LIBRARIES = (
    ("libasound", "libasound.so.2"),
    ("libjack", "libjack.so.0"),
)
_LOADED_LIBRARIES = []


def _package_libs_directory():
    """Return the wheel's .libs directory without recursively importing pyo."""
    package = sys.modules.get(__package__)
    package_file = getattr(package, "__file__", None)
    if package_file is None:
        return None

    try:
        directory = Path(package_file).resolve().parent / ".libs"
    except OSError:
        return None
    return directory if directory.is_dir() else None


def _bundled_library_name(directory, prefix):
    try:
        candidates = sorted(
            path.name
            for path in directory.iterdir()
            if path.name.startswith(prefix) and ".so" in path.name
        )
    except OSError:
        return None
    return candidates[0] if candidates else None


def _expected_elf_machine():
    machines = {
        "aarch64": 183,
        "amd64": 62,
        "arm": 40,
        "arm64": 183,
        "armv7l": 40,
        "i386": 3,
        "i686": 3,
        "ppc64": 21,
        "ppc64le": 21,
        "riscv64": 243,
        "s390x": 22,
        "x86_64": 62,
    }
    return machines.get(platform.machine().lower())


def _is_compatible_elf(path):
    """Return whether *path* is an ELF library for this Python architecture."""
    try:
        if not path.is_file():
            return False
        with path.open("rb") as file:
            header = file.read(20)
    except OSError:
        return False

    if len(header) < 20 or header[:4] != b"\x7fELF":
        return False

    elf_class, byte_order = header[4], header[5]
    expected_class = 2 if struct.calcsize("P") == 8 else 1
    expected_byte_order = 1 if sys.byteorder == "little" else 2
    if elf_class != expected_class or byte_order != expected_byte_order:
        return False

    expected_machine = _expected_elf_machine()
    if expected_machine is None:
        return True

    byte_order_name = "little" if byte_order == 1 else "big"
    return int.from_bytes(header[18:20], byte_order_name) == expected_machine


def _ldconfig_candidates(soname):
    """Yield native absolute paths recorded in the dynamic-linker cache."""
    try:
        result = subprocess.run(
            ["ldconfig", "-p"],
            check=False,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            text=True,
        )
    except (FileNotFoundError, OSError):
        return

    if result.returncode != 0:
        return

    for line in result.stdout.splitlines():
        name, separator, location = line.strip().partition("=>")
        if not separator or name.split(maxsplit=1)[0] != soname:
            continue
        path = Path(location.strip())
        if _is_compatible_elf(path):
            yield path


def _library_directories():
    """Yield native and multiarch library directories in priority order."""
    directories = []
    for path in os.environ.get("LD_LIBRARY_PATH", "").split(":"):
        # An empty segment means the current directory. Do not load a library
        # from an arbitrary working directory during import.
        if path:
            directories.append(Path(path))

    for variable in ("CONDA_PREFIX", "VIRTUAL_ENV"):
        value = os.environ.get(variable)
        if value:
            directories.append(Path(value) / "lib")

    directories.extend(
        [
            Path(sys.prefix) / "lib",
            Path("/usr/local/lib"),
            Path("/usr/local/lib64"),
            Path("/usr/lib"),
            Path("/usr/lib64"),
            Path("/lib"),
            Path("/lib64"),
            Path("/app/lib"),
            Path("/run/current-system/sw/lib"),
        ]
    )
    directories.extend(Path(path) for path in glob.glob("/usr/lib/*"))
    directories.extend(Path(path) for path in glob.glob("/lib/*"))

    seen = set()
    for directory in directories:
        if directory in seen or not directory.is_dir():
            continue
        seen.add(directory)
        yield directory


def _find_system_library(soname):
    """Find a compatible system library without recursively walking /usr."""
    seen = set()
    for directory in _library_directories():
        candidates = [directory / soname, *sorted(directory.glob("%s*" % soname))]
        for path in candidates:
            try:
                resolved = path.resolve()
            except OSError:
                continue
            if resolved in seen:
                continue
            seen.add(resolved)
            if _is_compatible_elf(path):
                return path

    for path in _ldconfig_candidates(soname):
        try:
            resolved = path.resolve()
        except OSError:
            continue
        if resolved not in seen:
            seen.add(resolved)
            return path
    return None


def _cache_directory():
    version = "%d.%d_%d" % (
        sys.version_info.major,
        sys.version_info.minor,
        struct.calcsize("P") * 8,
    )
    try:
        directory = Path.home() / ".pyo" / version / "libs"
        directory.mkdir(mode=0o700, parents=True, exist_ok=True)
    except (OSError, RuntimeError):
        return None
    return directory if directory.is_dir() else None


def _cache_link(directory, filename, target):
    """Create or repair a cache symlink without overwriting a regular file."""
    destination = directory / filename
    try:
        if os.path.lexists(destination):
            if destination.is_symlink() and destination.exists():
                try:
                    if os.path.samefile(destination, target):
                        return destination
                except OSError:
                    pass
            if not destination.is_symlink():
                return None
            destination.unlink()
        destination.symlink_to(target)
        return destination
    except OSError:
        return None


def _preload_library(libs_directory, prefix, soname):
    filename = _bundled_library_name(libs_directory, prefix)
    if filename is None:
        return

    system_library = _find_system_library(soname)
    if system_library is None:
        return

    load_path = system_library
    cache_directory = _cache_directory()
    if cache_directory is not None:
        cached_library = _cache_link(cache_directory, filename, system_library)
        if cached_library is not None:
            load_path = cached_library

    try:
        _LOADED_LIBRARIES.append(ctypes.CDLL(str(load_path), mode=ctypes.RTLD_GLOBAL))
    except OSError:
        # The bundled library remains available as a last-resort fallback.
        pass


def _preload_system_audio_libraries():
    libs_directory = _package_libs_directory()
    if libs_directory is None:
        return

    for prefix, soname in _LIBRARIES:
        _preload_library(libs_directory, prefix, soname)


_preload_system_audio_libraries()
