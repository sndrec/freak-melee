"""Exercise disc validation and section-bounded extraction without game data."""

import hashlib
import importlib.util
from pathlib import Path
import struct
import tempfile
import unittest


SPEC = importlib.util.spec_from_file_location(
    "prepare_native_assets", Path(__file__).parents[1] / "prepare_native_assets.py"
)
assets = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(assets)


class DiscExtractionTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.iso = Path(self.directory.name) / "fixture.iso"
        self.payload = bytes(range(64))
        self.dol = bytearray(0x100) + self.payload
        struct.pack_into(">I", self.dol, 7 * 4, 0x100)
        struct.pack_into(">I", self.dol, 0x48 + 7 * 4, 0x80400000)
        struct.pack_into(">I", self.dol, 0x90 + 7 * 4, len(self.payload))
        image = bytearray(0x800)
        image[:6] = b"GALE01"
        image[7] = 2
        struct.pack_into(">I", image, 0x1C, 0xC2339F3D)
        struct.pack_into(">I", image, 0x420, 0x800)
        self.iso.write_bytes(image + self.dol)
        self.sha1 = hashlib.sha1(self.dol).hexdigest()

    def test_data_symbol_uses_virtual_address_and_preserves_bytes(self):
        dol, sections = assets.extract_dol(self.iso, self.sha1)
        symbols = "Font = .data:0x80400010; // type:object size:0x20 scope:global\n"
        self.assertEqual(
            assets.extract_symbol(dol, sections, symbols, "Font"),
            bytes(range(16, 48)),
        )

    def test_modified_executable_is_rejected(self):
        image = bytearray(self.iso.read_bytes())
        image[-1] ^= 1
        self.iso.write_bytes(image)
        with self.assertRaisesRegex(ValueError, "SHA-1 mismatch"):
            assets.extract_dol(self.iso, self.sha1)

    def test_truncated_section_is_rejected(self):
        self.iso.write_bytes(self.iso.read_bytes()[:-1])
        with self.assertRaisesRegex(ValueError, "beyond the disc image"):
            assets.extract_dol(self.iso, self.sha1)

    def test_wrong_revision_is_rejected(self):
        image = bytearray(self.iso.read_bytes())
        image[7] = 1
        self.iso.write_bytes(image)
        with self.assertRaisesRegex(ValueError, "revision 2"):
            assets.extract_dol(self.iso, self.sha1)

    def test_symbol_cannot_cross_section_end(self):
        dol, sections = assets.extract_dol(self.iso, self.sha1)
        symbols = "Font = .data:0x80400030; // type:object size:0x20 scope:global\n"
        with self.assertRaisesRegex(ValueError, "does not fit"):
            assets.extract_symbol(dol, sections, symbols, "Font")


if __name__ == "__main__":
    unittest.main()
