#!/usr/bin/env python3
"""Package a verified build with parameter files reproduced from local originals."""
import argparse
import hashlib
import json
from pathlib import Path
import zipfile
from patch_parameters import apply

ROOT = Path(__file__).resolve().parent


def package(airace=None, performance=None):
    manifest = json.loads((ROOT/'release-manifest.json').read_text())
    if (airace is None) != (performance is None):
        raise ValueError('Supply both original parameter files')
    if airace is not None:
        recipes = json.loads((ROOT/'parameter_edits.json').read_text())
        prefix = manifest['mod_folder']
        bid = manifest['main_build_id']
        generated = {
            f'{prefix}/exefs/subsdk9': (ROOT/'build/subsdk9').read_bytes(),
            f'{prefix}/exefs/{bid}.pchtxt': (ROOT/'build'/(bid+'.pchtxt')).read_bytes(),
            bid+'.ips': (ROOT/'build'/(bid+'.ips')).read_bytes(),
            f'{prefix}/romfs/Common/Mush/AIRace.bin': apply(Path(airace).read_bytes(), recipes['AIRace.bin']),
            f'{prefix}/romfs/Common/Mush/Performance.bin': apply(Path(performance).read_bytes(), recipes['Performance.bin']),
        }
        if set(generated) != set(manifest['files']):
            raise ValueError('Generated file set differs from the release manifest')
        for name, raw in generated.items():
            if hashlib.sha256(raw).hexdigest() != manifest['files'][name]:
                raise ValueError('Generated release file differs: '+name)
        for name, raw in generated.items():
            target = ROOT/'release'/name
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(raw)
    files = {name: (ROOT/'release'/name).read_bytes() for name in manifest['files']}
    for name, data in files.items():
        if hashlib.sha256(data).hexdigest() != manifest['files'][name]:
            raise ValueError('Release file changed: '+name)
    bid = manifest['main_build_id']+'.ips'
    files['patch-source/'+bid] = files.pop(bid)
    for name in ['README.md', 'TECHNICAL.md', 'PARAMETERS.md', 'VALIDATION.md', 'LICENSE', 'release-manifest.json', 'RELEASE_NOTES.md']:
        files[name] = (ROOT/name).read_bytes()
    dest = ROOT/'dist';dest.mkdir(exist_ok=True)
    output = dest/f"HarderAI-Enhanced-v{manifest['version']}-MK8DX-{manifest['game_version']}.zip"
    with zipfile.ZipFile(output, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for name, data in sorted(files.items()):
            info = zipfile.ZipInfo(name, date_time=(2026, 10, 22, 0, 0, 0))
            info.create_system = 3;info.external_attr = 0o100644 << 16
            info.compress_type = zipfile.ZIP_DEFLATED
            archive.writestr(info, data)
    checksum = hashlib.sha256(output.read_bytes()).hexdigest()
    (dest/'SHA256SUMS').write_text(f'{checksum}  {output.name}\n')
    print(checksum, output.name)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--airace', type=Path, help='Original MK8DX 3.0.5 AIRace.bin')
    parser.add_argument('--performance', type=Path, help='Original MK8DX 3.0.5 Performance.bin')
    args = parser.parse_args()
    package(args.airace, args.performance)
