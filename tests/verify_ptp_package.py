"""Validate the beta overlay without modifying an existing installation."""
import gettext
import io
from pathlib import Path
import struct
import sys
import zipfile


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
            gettext.GNUTranslations(io.BytesIO(archive.read(f"files/lang/{locale}_TP.mo")))
        french = gettext.GNUTranslations(io.BytesIO(archive.read("files/lang/fr_TP.mo")))
        assert french.gettext("Taxpayer") == "Contribuable"
        farm = "Each Peasant working at the farm yields %{tax_rate} gold in tax per day."
        assert french.gettext(farm) != farm
        assert "%{tax_rate}" in french.gettext(farm)
        assert archive.read("README_PTP_1.0b.txt") == Path("docs/README_PTP_1.0b.txt").read_bytes()
        assert archive.read("LICENSE") == Path("LICENSE").read_bytes()
    print(f"Verified {package}: {architecture}, {len(locales)} renamed catalogs, no original files overwritten.")


if __name__ == "__main__":
    verify(sys.argv[1], sys.argv[2])
