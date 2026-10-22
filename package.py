#!/usr/bin/env python3
"""Create the install ZIP from pinned release files without running the mod."""
import hashlib
import json
from pathlib import Path
import zipfile

ROOT = Path(__file__).resolve().parent


def package():
    manifest = json.loads((ROOT/'release-manifest.json').read_text())
    files = {name: (ROOT/'release'/name).read_bytes() for name in manifest['files']}
    for name, data in files.items():
        if hashlib.sha256(data).hexdigest() != manifest['files'][name]:
            raise ValueError('Release file changed: '+name)
    bid = manifest['main_build_id']+'.ips'
    files['patch-source/'+bid] = files.pop(bid)
    for name in ['README.md', 'TECHNICAL.md', 'PARAMETERS.md', 'VALIDATION.md', 'LICENSE', 'release-manifest.json']:
        files[name] = (ROOT/name).read_bytes()
    dest = ROOT/'dist';dest.mkdir(exist_ok=True)
    output = dest/'HarderAI-Enhanced-v0.1.0-MK8DX-3.0.5.zip'
    with zipfile.ZipFile(output, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for name, data in sorted(files.items()):
            info = zipfile.ZipInfo(name, date_time=(2026, 10, 2, 0, 0, 0))
            info.create_system = 3;info.external_attr = 0o100644 << 16
            info.compress_type = zipfile.ZIP_DEFLATED
            archive.writestr(info, data)
    checksum = hashlib.sha256(output.read_bytes()).hexdigest()
    (dest/'SHA256SUMS').write_text(f'{checksum}  {output.name}\n')
    print(checksum, output.name)


if __name__ == '__main__':
    package()
