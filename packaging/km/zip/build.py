"""Build the manual package from the payload used by Knight-Mod-Setup."""

import argparse
import io
import re
import struct
from pathlib import Path
from zipfile import ZIP_DEFLATED, ZIP_STORED, ZipFile


def windows_text(path, encoding):
    return path.read_text(encoding="utf-8").replace("\n", "\r\n").encode(encoding)


def check_machine(executable, machine):
    header = struct.unpack_from("<I", executable, 0x3C)[0]
    if executable[:2] != b"MZ" or executable[header : header + 4] != b"PE\0\0":
        raise ValueError("Invalid Windows executable")
    if struct.unpack_from("<H", executable, header + 4)[0] != machine:
        raise ValueError("Windows executable architecture does not match the archive")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("payload", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()

    source = Path(__file__).resolve().parent
    common = args.payload / "common"
    payload = {file.relative_to(common).as_posix(): file.read_bytes() for file in common.rglob("*") if file.is_file()}
    check_machine(payload["KnightModLauncher.exe"], 0x14C)
    batch = windows_text(source / "Uninstall_KM.bat", "ascii")
    manifest = batch.decode("ascii").split(":mod_files\r\n")[1].split(":find\r\n")[0]
    batch_files = {name.replace("\\", "/") for name in re.findall(r'^    "([^"]+)"\r?$', manifest, re.M)}
    locale_line = re.search(r"for %%L in \(([^)]+)\)", manifest)
    batch_files.update(f"Knight Mod/files/lang/km_{locale}.mo" for locale in locale_line[1].split())
    if batch_files != payload.keys() | {"fheroes2KM.exe"}:
        raise ValueError("The batch file and the staged payload contain different files")
    payload["Uninstall_KM.bat"] = batch

    args.output.parent.mkdir(parents=True, exist_ok=True)
    with ZipFile(args.output, "w", ZIP_DEFLATED) as outer:
        for name in ("Installation.txt", "installation_fr.txt"):
            outer.writestr(name, windows_text(source / name, "utf-8-sig"))
        outer.writestr("LICENCE", payload["Knight Mod/docs/LICENCE.txt"])

        for arch, machine in (("x86", 0x14C), ("x64", 0x8664)):
            executable = (args.payload / arch / "fheroes2KM.exe").read_bytes()
            check_machine(executable, machine)
            with io.BytesIO() as buffer:
                with ZipFile(buffer, "w", ZIP_DEFLATED, compresslevel=8) as inner:
                    inner.writestr("fheroes2KM.exe", executable)
                    for name, content in sorted(payload.items()):
                        inner.writestr(name, content)
                outer.writestr(f"Knight-Mod-1.1b-Windows-{arch}.zip", buffer.getvalue(), ZIP_STORED)

    with ZipFile(args.output) as outer:
        if outer.testzip():
            raise ValueError("Invalid ZIP package")
        for arch in ("x86", "x64"):
            with ZipFile(io.BytesIO(outer.read(f"Knight-Mod-1.1b-Windows-{arch}.zip"))) as inner:
                if inner.testzip():
                    raise ValueError(f"Invalid {arch} archive")


if __name__ == "__main__":
    main()
