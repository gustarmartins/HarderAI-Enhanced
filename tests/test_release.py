import hashlib
import json
from pathlib import Path
import struct
import unittest

from build import branch, patch_text
from ips import encode_ips, parse_ips
from patch_parameters import apply

ROOT = Path(__file__).resolve().parents[1]
sha = lambda raw: hashlib.sha256(raw).hexdigest()


class ReleaseTests(unittest.TestCase):
    def test_release_files_match_manifest(self):
        m = json.loads((ROOT/'release-manifest.json').read_text())
        actual = {p.relative_to(ROOT/'release').as_posix(): sha(p.read_bytes())
                  for p in (ROOT/'release').rglob('*') if p.is_file()}
        self.assertEqual(actual, m['files'])

    def test_published_sources_match_frozen_profile(self):
        profile = json.loads((ROOT/'profile.json').read_text())
        for name, digest in profile['sources'].items():
            with self.subTest(source=name):
                self.assertEqual(sha((ROOT/'src'/name).read_bytes()), digest)

    def test_ips_and_patch_text_describe_same_instructions(self):
        p = json.loads((ROOT/'profile.json').read_text())
        binary = (ROOT/'release'/(p['main_build_id']+'.ips')).read_bytes()
        memory = parse_ips(binary)
        self.assertEqual(len(memory), p['merged_patch_bytes'])
        self.assertEqual(encode_ips(memory), binary)
        m = json.loads((ROOT/'release-manifest.json').read_text())
        path = ROOT/'release'/m['mod_folder']/'exefs'/(p['main_build_id']+'.pchtxt')
        self.assertEqual(patch_text(memory, p['main_build_id']), path.read_bytes())
        for hook in p['hook_plan']:
            actual = bytes(memory[hook['main_offset']+i] for i in range(4))
            self.assertEqual(actual.hex(), hook['branch_hex'])

    def test_parameter_edits_reconstruct_both_released_files(self):
        recipes = json.loads((ROOT/'parameter_edits.json').read_text())
        for name, recipe in recipes.items():
            with self.subTest(file=name):
                m = json.loads((ROOT/'release-manifest.json').read_text())
                released = (ROOT/'release'/m['mod_folder']/'romfs/Common/Mush'/name).read_bytes()
                original = bytearray(released)
                for edit in recipe['edits']:
                    start = edit['offset'];after = bytes.fromhex(edit['after']);before = bytes.fromhex(edit['before'])
                    self.assertEqual(original[start:start+len(after)], after)
                    original[start:start+len(before)] = before
                self.assertEqual(sha(original), recipe['base_sha256'])
                self.assertEqual(apply(bytes(original), recipe), released)
                original[0] ^= 1
                with self.assertRaises(ValueError):
                    apply(bytes(original), recipe)

    def test_ips_rejects_truncated_and_overlapping_records(self):
        with self.assertRaises(ValueError):
            parse_ips(b'IPS32'+struct.pack('>IH', 0x100, 4)+b'xx')
        record = struct.pack('>IH', 0x100, 4)+b'1234'
        with self.assertRaises(ValueError):
            parse_ips(b'IPS32'+record+record+b'EEOF')

    def test_branch_rejects_invalid_placements(self):
        for source, target in [(0, 1), (0, 1 << 27), (1 << 28, 0)]:
            with self.subTest(target=target), self.assertRaises(ValueError):
                branch(source, target)


if __name__ == '__main__':
    unittest.main()
