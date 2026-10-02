"""IPS32 uses NSO file offsets (main-relative address + 0x100)."""
import struct

def parse_ips(blob: bytes) -> dict[int, int]:
    if blob[:5] != b'IPS32':
        raise ValueError('Expected IPS32')
    pos, result = 5, {}
    while blob[pos:pos + 4] != b'EEOF':
        if pos + 6 > len(blob):
            raise ValueError('Truncated record')
        raw, size = struct.unpack_from('>IH', blob, pos)
        pos += 6
        if raw < 0x100:
            raise ValueError('Record before NSO text')
        if size:
            data = blob[pos:pos + size]
            pos += size
            if len(data) != size:
                raise ValueError('Truncated payload')
        else:
            if pos + 3 > len(blob):
                raise ValueError('Truncated RLE')
            size, value = struct.unpack_from('>HB', blob, pos)
            pos += 3
            if not size:
                raise ValueError('Empty RLE')
            data = bytes([value]) * size
        for addr, value in enumerate(data, raw - 0x100):
            if addr in result:
                raise ValueError(f'Overlapping record at {addr:#x}')
            result[addr] = value
    if pos + 4 != len(blob):
        raise ValueError('Unexpected trailer')
    return result


def encode_ips(memory: dict[int, int]) -> bytes:
    records: list[tuple[int, bytearray]] = []
    for addr, value in sorted(memory.items()):
        if records and addr == records[-1][0] + len(records[-1][1]) and len(records[-1][1]) < 65535:
            records[-1][1].append(value)
        else:
            records.append((addr, bytearray([value])))
    return b'IPS32' + b''.join(struct.pack('>IH', addr + 0x100, len(data)) + data
                             for addr, data in records) + b'EEOF'

