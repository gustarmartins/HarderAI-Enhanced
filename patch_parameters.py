#!/usr/bin/env python3
"""Reproduce released parameter files from matching original v3.0.5 files."""
import argparse
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent
sha = lambda raw: hashlib.sha256(raw).hexdigest()


def apply(raw, recipe):
    if len(raw) != recipe['size'] or sha(raw) != recipe['base_sha256']:
        raise ValueError('Input differs from the original v3.0.5 file')
    result = bytearray(raw)
    for edit in recipe['edits']:
        start = edit['offset'];before = bytes.fromhex(edit['before']);after = bytes.fromhex(edit['after'])
        if len(before) != len(after) or result[start:start+len(before)] != before:
            raise ValueError('Parameter edit preimage differs')
        result[start:start+len(after)] = after
    if sha(result) != recipe['result_sha256']:
        raise ValueError('Generated parameter hash differs')
    return bytes(result)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('kind', choices=['AIRace.bin', 'Performance.bin'])
    parser.add_argument('original', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    recipes = json.loads((ROOT/'parameter_edits.json').read_text())
    result = apply(args.original.read_bytes(), recipes[args.kind])
    with args.output.open('xb') as stream:
        stream.write(result)
    print(sha(result), args.output.name)
