"""Compile/link and convert the authored AArch64 module to NSO."""
from __future__ import annotations
import hashlib
import io
from pathlib import Path
import struct
import subprocess
import tempfile

from elftools.elf.elffile import ELFFile

ROOT = Path(__file__).resolve().parent / "src"


def convert(elf_bytes):
    elf = ELFFile(io.BytesIO(elf_bytes))
    segments = [s for s in elf.iter_segments() if s['p_type'] == 'PT_LOAD']
    if len(segments) != 3 or [s['p_flags'] for s in segments] != [5,4,6]:
        raise ValueError('NSO must have exactly text RX, rodata R and data RW segments')
    if segments[0]['p_vaddr'] != 0:
        raise ValueError('Module header must start at zero')
    symbols = elf.get_section_by_name('.dynsym')
    if any(s.name and s['st_shndx'] == 'SHN_UNDEF' for s in symbols.iter_symbols()):
        raise ValueError('Observer must have no imported symbols')
    relocations = []
    rela = elf.get_section_by_name('.rela.dyn')
    if rela is not None:
        for entry in rela.iter_relocations():
            if entry['r_info_type'] != 1027 or entry['r_info_sym'] != 0:
                raise ValueError('Observer supports only local AArch64 RELATIVE relocations')
            relocations.append({'offset': entry['r_offset'], 'addend': entry['r_addend']})
    header = bytearray(0x100)
    header[:4] = b'NSO0'
    # No compression; all three segment hashes enabled.
    struct.pack_into('<I', header, 0xC, 0x38)
    content = bytearray()
    entries = []
    for index, segment in enumerate(segments):
        data = segment.data()
        address = segment['p_vaddr']
        if address % 0x1000:
            raise ValueError('Segment memory offsets must be page aligned')
        file_offset = 0x100 + len(content)
        struct.pack_into('<III', header, 0x10 + index*16, file_offset, address, len(data))
        struct.pack_into('<I', header, 0x60 + index*4, len(data))
        header[0xA0 + index*32:0xC0 + index*32] = hashlib.sha256(data).digest()
        content.extend(data)
        entries.append({'kind': ['text','rodata','data'][index],
                        'offset': address, 'size': len(data),
                        'memory_size': segment['p_memsz']})
    bss_size = segments[2]['p_memsz'] - segments[2]['p_filesz']
    struct.pack_into('<I', header, 0x3C, bss_size)
    build_id = hashlib.sha256(content).digest()
    header[0x40:0x60] = build_id
    addresses = {symbol.name: symbol['st_value'] for symbol in symbols.iter_symbols() if symbol.name}
    manifest = {'candidate': 'master-lab-observer-v1', 'build_id': build_id.hex(),
                'segments': entries, 'bss_size': bss_size, 'symbols': addresses,
                'relative_relocations': relocations,
                'status': 'built; not installed or loaded',
                'installs_hooks': False, 'modifies_game_objects': False,
                'replaces_npdm': False, 'enabled_experiments': [],
                'loader_gate': 'Require retail RTLD execution tests and clean-title loading in Eden'}
    return bytes(header + content), manifest


def build(sources=None, linker_args=(), compiler_args=(), elf_transform=None):
    with tempfile.TemporaryDirectory(prefix='mk8-master-module-') as directory:
        temp = Path(directory)
        if sources is None:
            raise ValueError('Pass the source list from profile.json')
        objects = []
        for index, source in enumerate(sources):
            output = temp/f'{index}.o'
            subprocess.run(['clang','--target=aarch64-none-elf','-O2','-ffreestanding',
                '-fno-builtin','-fno-stack-protector','-fno-strict-aliasing',
                '-fPIC','-fvisibility=hidden','-Wall','-Wextra','-Werror',
                *compiler_args,'-c',str(source),'-o',str(output)],check=True)
            objects.append(str(output))
        subprocess.run(['ld.lld','--shared','-Bsymbolic','--hash-style=sysv','--no-undefined',
            '-init=lab_module_init','-T',str(ROOT/'module/link.ld'),*linker_args,*objects,
            '-o',str(temp/'observer.elf')],check=True)
        elf = (temp/'observer.elf').read_bytes()
        if elf_transform is not None:
            elf = elf_transform(elf)
        nso, manifest = convert(elf)
        manifest['elf_sha256'] = hashlib.sha256(elf).hexdigest()
        manifest['nso_sha256'] = hashlib.sha256(nso).hexdigest()
        return elf, nso, manifest


