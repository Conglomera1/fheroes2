"""Validate the beta overlay without modifying an existing installation."""
from pathlib import Path
import struct
import sys
import zipfile


def read_catalog(data):
    # fheroes2 consumes legacy-encoded byte strings. Python gettext assumes
    # UTF-8 for header metadata before processing the declared charset.
    assert len(data) >= 28, "Truncated MO header"
    magic = struct.unpack_from("<I", data)[0]
    assert magic in (0x950412DE, 0xDE120495), "Invalid MO magic"
    endian = "<" if magic == 0x950412DE else ">"
    revision, count, originals, translations = struct.unpack_from(endian + "4I", data, 4)
    assert revision >> 16 <= 1, "Unsupported MO revision"
    assert count > 0
    assert originals + count * 8 <= len(data)
    assert translations + count * 8 <= len(data)

    def entry(table, index):
        length, offset = struct.unpack_from(endian + "2I", data, table + index * 8)
        assert offset + length < len(data), "Truncated MO string"
        assert data[offset + length] == 0, "Missing MO string terminator"
        return data[offset:offset + length]

    catalog = {entry(originals, i): entry(translations, i) for i in range(count)}
    assert b"" in catalog, "Missing MO metadata"
    return catalog


def verify(package, architecture):
    executable = f"fheroes2TP{architecture}.exe"
    locales = {p.stem for p in Path("files/lang").glob("*.po")}
    expected = {executable, "README_PTP_1.0b.txt", "LICENSE"}
    expected.update(f"files/lang/{locale}_TP.mo" for locale in locales)
    with zipfile.ZipFile(package) as archive:
        names = [name for name in archive.namelist() if not name.endswith("/")]
        assert len(names) == len(set(names)), "Duplicate ZIP entries"
        assert set(names) == expected, (set(names) - expected, expected - set(names))
        image = archive.read(executable)
        assert image[:2] == b"MZ", "Not a Windows executable"
        pe_offset = struct.unpack_from("<I", image, 0x3C)[0]
        assert image[pe_offset:pe_offset + 4] == b"PE\0\0"
        machine = struct.unpack_from("<H", image, pe_offset + 4)[0]
        assert machine == {"x86": 0x14C, "x64": 0x8664}[architecture]
        for locale in locales:
            read_catalog(archive.read(f"files/lang/{locale}_TP.mo"))
        french = read_catalog(archive.read("files/lang/fr_TP.mo"))
        assert french[b"Taxpayer"] == b"Contribuable"
        farm = b"Each Peasant working at the farm yields %{tax_rate} gold in tax per day."
        assert french[farm] != farm
        assert b"%{tax_rate}" in french[farm]
        assert archive.read("README_PTP_1.0b.txt") == Path("docs/README_PTP_1.0b.txt").read_bytes()
        assert archive.read("LICENSE") == Path("LICENSE").read_bytes()
    print(f"Verified {package}: {architecture}, {len(locales)} renamed catalogs, no original files overwritten.")


if __name__ == "__main__":
    verify(sys.argv[1], sys.argv[2])
