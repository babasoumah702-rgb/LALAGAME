#!/usr/bin/env python3
"""Create a macOS ZIP while preserving executable Unix modes."""

from __future__ import annotations

import hashlib
import json
import stat
import sys
import zipfile
from pathlib import Path


EXECUTABLE_NAMES = {
    "node",
    "node-arm64",
    "node-x64",
    "Lalaland",
    "TuanjieCrashHandler64",
}


def is_executable(path: Path) -> bool:
    return (
        path.name in EXECUTABLE_NAMES
        or path.suffix == ".command"
        or "/Contents/MacOS/" in path.as_posix()
    )


def add_directory(archive: zipfile.ZipFile, name: str) -> None:
    info = zipfile.ZipInfo(name.rstrip("/") + "/")
    info.create_system = 3
    info.external_attr = (stat.S_IFDIR | 0o755) << 16
    archive.writestr(info, b"")


def add_file(archive: zipfile.ZipFile, source: Path, name: str) -> None:
    info = zipfile.ZipInfo.from_file(source, name)
    info.create_system = 3
    mode = 0o755 if is_executable(source) else 0o644
    info.external_attr = (stat.S_IFREG | mode) << 16
    info.compress_type = zipfile.ZIP_DEFLATED
    info._compresslevel = 9
    with source.open("rb") as input_file, archive.open(info, "w") as output_file:
        while chunk := input_file.read(1024 * 1024):
            output_file.write(chunk)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as file:
        while chunk := file.read(1024 * 1024):
            digest.update(chunk)
    return digest.hexdigest()


def main() -> int:
    if len(sys.argv) != 3:
        print("usage: package_lalaland_macos.py STAGING_DIR OUTPUT_ZIP", file=sys.stderr)
        return 2

    staging = Path(sys.argv[1]).resolve()
    output = Path(sys.argv[2]).resolve()
    if not staging.is_dir():
        raise SystemExit(f"staging directory does not exist: {staging}")
    required = [
        staging / "Lalaland.app" / "Contents" / "MacOS" / "Lalaland",
        staging / "Server" / "node",
        staging / "Server" / "node-arm64",
        staging / "Server" / "node-x64",
        staging / "Server" / "dist" / "server.js",
        staging / "启动 Lalaland.command",
        staging / "MACOS_启动说明.md",
    ]
    missing = [str(path) for path in required if not path.is_file()]
    if missing:
        raise SystemExit("missing required macOS runtime files: " + ", ".join(missing))

    excluded = {"node.exe", "node-windows-unused.exe"}
    files = sorted(
        path for path in staging.rglob("*")
        if path.is_file() and path.name not in excluded
    )
    if any(path.suffix in {".log", ".db"} or path.name.startswith(".env") for path in files):
        raise SystemExit("staging contains logs, databases, or private environment files")

    output.parent.mkdir(parents=True, exist_ok=True)
    if output.exists():
        raise SystemExit(f"output already exists: {output}")
    root = staging.name
    directories = sorted({path.parent for path in files} | {staging})
    with zipfile.ZipFile(output, "w", allowZip64=True) as archive:
        for directory in directories:
            relative = directory.relative_to(staging).as_posix()
            add_directory(archive, root if relative == "." else f"{root}/{relative}")
        for source in files:
            relative = source.relative_to(staging).as_posix()
            add_file(archive, source, f"{root}/{relative}")

    verified = 0
    with zipfile.ZipFile(output, "r") as archive:
        for info in archive.infolist():
            if info.is_dir():
                continue
            relative = Path(*Path(info.filename).parts[1:])
            source = staging / relative
            with archive.open(info) as zipped:
                digest = hashlib.sha256()
                while chunk := zipped.read(1024 * 1024):
                    digest.update(chunk)
            if digest.hexdigest() != sha256(source):
                raise SystemExit(f"archive verification failed: {relative}")
            verified += 1
        executable_entries = {
            info.filename: (info.external_attr >> 16) & 0o777
            for info in archive.infolist()
            if not info.is_dir() and is_executable(Path(info.filename))
        }
    if verified != len(files) or any(mode != 0o755 for mode in executable_entries.values()):
        raise SystemExit("archive file count or executable modes are invalid")

    digest = sha256(output)
    sidecar = output.with_suffix(output.suffix + ".sha256")
    sidecar.write_text(f"{digest}  {output.name}\n", encoding="utf-8")
    report = {
        "passed": True,
        "archive": str(output),
        "sha256": digest,
        "bytes": output.stat().st_size,
        "verifiedFiles": verified,
        "executableEntries": len(executable_entries),
        "actualMacSmokeTest": False,
    }
    print(json.dumps(report, ensure_ascii=False, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
