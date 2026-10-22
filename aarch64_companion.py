"""Compile an authored AArch64 function at its declared main-image address.

No game file or runtime memory is accessed.
"""
from pathlib import Path
import re
import subprocess
import tempfile


def compile_c(source: Path, symbol: str, address: int, *, extra_sources=(),
              compiler_args=(), include_data=False, linker_symbols=()) -> bytes:
    # Absolute link symbols produce relocations relative to the final guest
    # layout; assembler numeric branch operands are not equivalent addresses.
    declarations=[]
    for name,value in linker_symbols:
        if not re.fullmatch(r'[A-Za-z_][A-Za-z_0-9]*',name) or not isinstance(value,int) or value<0:
            raise ValueError('Invalid native linker symbol')
        declarations.append(f'{name} = {value:#x};\n')
    with tempfile.TemporaryDirectory(prefix='mk8-master-native-') as directory:
        temp = Path(directory)
        objects = []
        for index, item in enumerate((source, *extra_sources)):
            obj = temp / f'code{index}.o'
            subprocess.run(['clang', '--target=aarch64-none-elf', '-O2', '-ffreestanding',
                '-ffunction-sections', '-fno-builtin', '-fno-stack-protector', '-fno-strict-aliasing',
                '-Wall', '-Wextra', '-Werror', *compiler_args,
                '-c', str(item), '-o', str(obj)], check=True)
            objects.append(str(obj))
        # Keep compiler literal pools next to text. Extracting only .text loses
        # .rodata constants addressed by valid ADRP/LDR pairs in optimized C.
        (temp/'link.ld').write_text(
            ''.join(declarations)+f'SECTIONS {{ . = {address:#x}; .text : {{ *(.text.{symbol}) *(.text .text.*) }} '
            '.rodata : { *(.rodata .rodata.*) } '
            '.data : { *(.data .data.*) } '
            '/DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) } }\n')
        subprocess.run(['ld.lld', '-e', symbol, '-T', str(temp/'link.ld'),
            *objects, '-o', str(temp/'code.elf')], check=True)
        subprocess.run(['llvm-objcopy', '-O', 'binary', '--only-section=.text',
            '--only-section=.rodata',
            *(['--only-section=.data'] if include_data else []),
            str(temp/'code.elf'), str(temp/'code.bin')], check=True)
        return (temp/'code.bin').read_bytes()
