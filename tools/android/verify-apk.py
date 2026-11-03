#!/usr/bin/env python3
"""Check the packaged ARM64 runtime; signature/alignment use Android tools."""
from pathlib import Path, PurePosixPath
import struct
import sys
import zipfile

apk = Path(sys.argv[1])
required = {'libmain.so', 'libSDL3.so', 'libc++_shared.so', 'libdxcompiler.so'}
with zipfile.ZipFile(apk) as archive:
    assert archive.testzip() is None, 'corrupt APK ZIP entry'
    names = archive.namelist()
    assert 'AndroidManifest.xml' in names and 'classes.dex' in names, 'incomplete Android package'
    libraries = [name for name in names if name.startswith('lib/') and name.endswith('.so')]
    assert required <= {PurePosixPath(name).name for name in libraries}, 'missing runtime library'
    for name in libraries:
        assert name.startswith('lib/arm64-v8a/'), 'unexpected ABI: ' + name
        header = archive.read(name)[:20]
        assert header[:6] == b'\x7fELF\x02\x01' and struct.unpack_from('<H', header, 18)[0] == 183, 'not ARM64 ELF: ' + name
    for name in names:
        assert PurePosixPath(name).suffix.lower() not in {'.xex', '.iso', '.xpr', '.xma', '.wmv'}, 'game input packaged: ' + name
print('PASS: intact APK, Android manifest/DEX and complete ARM64 runtime libraries')
