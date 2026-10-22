#!/usr/bin/env python3
"""Build the v3.0.5 AArch64 module and its matching main patch. No game access."""
import argparse
import hashlib
import io
import json
from pathlib import Path
import struct

from elftools.elf.elffile import ELFFile
import nso_builder
from ips import encode_ips, parse_ips
from aarch64_companion import compile_c

ROOT = Path(__file__).resolve().parent
sha = lambda data: hashlib.sha256(data).hexdigest()


def branch(source, target):
    delta = target - source
    if delta % 4 or not -(1 << 27) <= delta < (1 << 27):
        raise ValueError('AArch64 branch is unaligned or out of range')
    return 0x14000000 | ((delta // 4) & 0x3ffffff)


def encode_returns(raw, profile):
    elf = ELFFile(io.BytesIO(raw))
    text = elf.get_section_by_name('.text')
    symbols = {s.name: s['st_value'] for s in elf.get_section_by_name('.symtab').iter_symbols()}
    result = bytearray(raw)
    for name, destination in profile['returns']:
        address = symbols[name]
        if not text['sh_addr'] <= address <= text['sh_addr'] + text['sh_size'] - 4:
            raise ValueError('Return symbol outside executable section')
        offset = text['sh_offset'] + address - text['sh_addr']
        if raw[offset:offset+4] != bytes.fromhex('00000014'):
            raise ValueError('Return placeholder differs: ' + name)
        struct.pack_into('<I', result, offset,
            branch(profile['required_main_to_module_offset'] + address, destination))
    return bytes(result)


def patch_text(memory, build_id):
    lines = ['@nsobid-' + build_id, '@flag offset_shift 0x100', '@enabled']
    for address in sorted(memory)[::4]:
        if address % 4 or any(address+i not in memory for i in range(4)):
            raise ValueError('Patch must contain complete aligned instructions')
        lines.append(f'{address:08X} ' + bytes(memory[address+i] for i in range(4)).hex().upper())
    return ('\n'.join(lines + ['@stop']) + '\n').encode()


def build(output, verify=False):
    profile = json.loads((ROOT/'profile.json').read_text())
    if verify:
        for name, digest in profile['sources'].items():
            if sha((ROOT/'src'/name).read_bytes()) != digest:
                raise ValueError('Release source differs: ' + name)
    elf, nso, metadata = nso_builder.build(
        sources=[ROOT/'src'/name for name in profile['source_order']],
        compiler_args=profile['compiler_args'],
        source_compiler_args={str(ROOT/'src'/n):a for n,a in profile['source_compiler_args'].items()},
        linker_args=profile['linker_args'],
        elf_transform=lambda raw: encode_returns(raw, profile))
    memory = {}
    for hook in profile['hook_plan']:
        source = hook['main_offset']
        target = profile['required_main_to_module_offset'] + metadata['symbols'][hook['symbol']]
        word = branch(source, target)
        if int.from_bytes(bytes.fromhex(hook['branch_hex']), 'little') & 0xfc000000 == 0x94000000:
            word |= 0x80000000
        encoded = struct.pack('<I', word)
        if encoded.hex() != hook['branch_hex']:
            raise ValueError('Rebuilt native hook differs')
        for address, value in enumerate(encoded, source):
            if address in memory:
                raise ValueError('Overlapping hooks')
            memory[address] = value
    hook_ips = encode_ips(memory)
    if verify and sha(hook_ips) != profile['hook_patch_sha256']:
        raise ValueError('Rebuilt hook-only patch differs')
    overlays_raw = (ROOT/'main_overlays.json').read_bytes()
    if sha(overlays_raw) != profile['main_overlays_sha256']:
        raise ValueError('Companion main-image patch source differs')
    overlays = json.loads(overlays_raw)
    if overlays['game_version'] != profile['game_version'] or overlays['main_build_id'] != profile['main_build_id']:
        raise ValueError('Companion patch targets another game build')
    for word in overlays['words']:
        start = int(word['address'], 16)
        encoded = bytes.fromhex(word['hex'])
        if start < 0 or start % 4 or len(encoded) != 4:
            raise ValueError('Companion patch must contain aligned complete words')
        for address, value in enumerate(encoded, start):
            if address in memory:
                raise ValueError('Companion patch overlaps a compiled hook')
            memory[address] = value
    for companion in profile.get('compiled_companions', []):
        source = ROOT/companion['source']
        if sha(source.read_bytes()) != companion['source_sha256']:
            raise ValueError('Authored companion source differs')
        raw = compile_c(source, companion['symbol'], companion['address'],
                        linker_symbols=companion['symbols'])
        size = companion['size']
        if len(raw) > size or len(raw) % 4 or size % 4:
            raise ValueError('Compiled companion exceeds its native function')
        raw += bytes.fromhex('1f2003d5')*((size-len(raw))//4)
        if sha(raw) != companion['replacement_sha256']:
            raise ValueError('Compiled companion bytes differ')
        for at, value in enumerate(raw, companion['address']):
            if at in memory: raise ValueError('Companion overlaps an existing patch')
            memory[at] = value
    if len(memory) != profile['merged_patch_bytes']:
        raise ValueError('Merged patch extent differs')
    ips = encode_ips(memory)
    assert parse_ips(ips) == memory
    if verify and (sha(nso) != profile['nso_sha256'] or sha(ips) != profile['patch_sha256']):
        raise ValueError('Release hash differs; use the documented compiler versions')
    output = Path(output)
    output.mkdir(parents=True, exist_ok=True)
    files = {'module.elf': elf, 'subsdk9': nso, profile['main_build_id']+'.ips': ips,
             profile['main_build_id']+'.pchtxt': patch_text(memory, profile['main_build_id'])}
    for name, data in files.items():
        (output/name).write_bytes(data)
    receipt = {'version': profile['version'], 'game_version': profile['game_version'],
               'files': {name: sha(data) for name, data in files.items()},
               'matches_release': sha(nso) == profile['nso_sha256'] and sha(ips) == profile['patch_sha256']}
    (output/'build.json').write_text(json.dumps(receipt, indent=2)+'\n')
    return receipt


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', type=Path, default=ROOT/'build')
    parser.add_argument('--verify', action='store_true', help='Require exact release sources and binary hashes')
    args = parser.parse_args()
    print(json.dumps(build(args.out, args.verify), indent=2))
