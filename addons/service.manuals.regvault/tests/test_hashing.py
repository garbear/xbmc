# -*- coding: utf-8 -*-
"""The hash a dump produces, and the copier headers that must not go into it."""

import hashlib
import io
import os
import sys
import unittest
import zipfile

# resources/lib is a package, so it is imported as one: hashing reaches its
# sibling through a relative import that only resolves inside the package
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "resources"))

from lib import hashing  # noqa: E402


def md5(data):
    return hashlib.md5(data).hexdigest()


class HeaderSize(unittest.TestCase):
    def test_an_ines_header_is_recognised_by_its_magic(self):
        self.assertEqual(hashing.header_size("game.nes", 40976, b"NES\x1a"), 16)

    def test_a_file_without_the_magic_keeps_every_byte(self):
        self.assertEqual(hashing.header_size("game.md", 524288, b"\x00\xff\xff\xf8"), 0)

    def test_a_snes_copier_header_is_recognised_by_the_size_it_leaves(self):
        self.assertEqual(hashing.header_size("game.smc", 524800, b"\x00\x00\x00\x00"), 512)

    def test_a_headerless_snes_dump_keeps_every_byte(self):
        self.assertEqual(hashing.header_size("game.sfc", 524288, b"\x00\x00\x00\x00"), 0)

    def test_the_snes_rule_is_not_applied_to_other_machines(self):
        # 512 over a kilobyte is only meaningful where cartridge data is whole
        self.assertEqual(hashing.header_size("game.md", 524800, b"\x00\x00\x00\x00"), 0)


class HashFile(unittest.TestCase):
    def setUp(self):
        self.files = {}

    def _open(self, path):
        return io.BytesIO(self.files[path])

    def _size(self, path):
        return len(self.files[path])

    def _hash(self, path):
        return hashing.hash_file(path, opener=self._open, sizer=self._size)

    def test_an_ines_header_is_left_out_of_the_hash(self):
        payload = b"\x01" * 40960
        self.files["game.nes"] = b"NES\x1a" + b"\x00" * 12 + payload

        self.assertEqual(self._hash("game.nes"), md5(payload))

    def test_a_snes_copier_header_is_left_out_of_the_hash(self):
        payload = b"\x02" * (512 * 1024)
        self.files["game.smc"] = b"\x00" * 512 + payload

        self.assertEqual(self._hash("game.smc"), md5(payload))

    def test_a_dump_with_no_header_hashes_whole(self):
        payload = b"\x03" * (512 * 1024)
        self.files["game.md"] = payload

        self.assertEqual(self._hash("game.md"), md5(payload))

    def test_a_header_spanning_the_first_chunk_is_still_dropped(self):
        # The header is read with the first chunk, so the skip has to survive
        # the loop moving on to the second
        payload = b"\x04" * (hashing.CHUNK_SIZE * 2)
        self.files["game.nes"] = b"NES\x1a" + b"\x00" * 12 + payload

        self.assertEqual(self._hash("game.nes"), md5(payload))

    def test_a_zipped_rom_is_hashed_by_what_is_inside_it(self):
        payload = b"\x05" * 40960
        buffer = io.BytesIO()
        with zipfile.ZipFile(buffer, "w") as archive:
            archive.writestr("game.nes", b"NES\x1a" + b"\x00" * 12 + payload)
        self.files["game.zip"] = buffer.getvalue()

        self.assertEqual(self._hash("game.zip"), md5(payload))


if __name__ == "__main__":
    unittest.main()
