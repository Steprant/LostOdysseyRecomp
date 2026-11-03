#!/usr/bin/env python3
"""Check the integrated Java/native boundary without a game image or an APK.

The actual RuntimeActivity declarations generate JNI headers with javac. The
production Tester JNI translation unit must agree with those declarations.
The JVM export tests use the production Java trace writer, not Android stubs.
"""
import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
JAVA = ROOT / 'packaging/android/runtime/src/main/java/io/github/freefrank/lostodyssey'


def run(*command):
    subprocess.run([str(part) for part in command], check=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--jdk', type=Path, default=os.environ.get('JAVA_HOME'))
    parser.add_argument('--cxx', default=os.environ.get('CXX', 'c++'))
    parser.add_argument('--ndk', type=Path)
    args = parser.parse_args()
    javac = args.jdk / 'bin/javac' if args.jdk else shutil.which('javac')
    java = args.jdk / 'bin/java' if args.jdk else shutil.which('java')
    jdk = args.jdk or (Path(javac).resolve().parents[1] if javac else None)
    if not javac or not java:
        parser.error('JDK 17 or later required')
    activity = (JAVA / 'RuntimeActivity.java').read_text()
    commands = dict((name, int(value, 16)) for name, value in re.findall(
        r'private static final int (COMMAND_\w+) = (0x[0-9a-fA-F]+);', activity))
    expected = {'COMMAND_OPEN_GAME_FOLDER': 0x8000, 'COMMAND_OPEN_TV_SETTINGS': 0x8001,
                'COMMAND_TESTER_START': 0x8002, 'COMMAND_TESTER_STOP': 0x8003,
                'COMMAND_TESTER_EXPORT': 0x8004}
    assert commands == expected, f'Java command contract changed: {commands}'
    assert len(set(commands.values())) == len(commands), 'colliding Android command IDs'
    for name in commands:
        assert re.search(r'if\s*\(command == ' + name + r'\)', activity), name + ' has no handler'
    menu = (ROOT / 'LostOdysseyRecomp/settings/menu.cpp').read_text()
    sent = [int(value, 16) for value in re.findall(r'SDL_SendAndroidMessage\((0x[0-9a-fA-F]+),', menu)]
    assert set(sent) == set(commands.values()), f'Unmatched native commands: {sent}'
    declarations = re.findall(r'static native ([\w\[\]]+) (nativeTester\w+)\(([^)]*)\);', activity)
    assert len(declarations) == 6, 'missing Tester JNI declarations'
    with tempfile.TemporaryDirectory(prefix='lo-android-integration-') as temporary:
        out = Path(temporary)
        run(javac, '-d', out, JAVA / 'TesterThreadStats.java', JAVA / 'TesterTraceFiles.java',
            ROOT / 'tools/tests/AndroidTesterTraceTest.java')
        run(java, '-cp', out, 'io.github.freefrank.lostodyssey.AndroidTesterTraceTest')
        source = out / 'RuntimeActivity.java'
        source.write_text('package io.github.freefrank.lostodyssey;\npublic final class RuntimeActivity {\n' +
                          '\n'.join(f'public static native {result} {name}({parameters});'
                                    for result, name, parameters in declarations) + '\n}\n')
        run(javac, '-h', out, '-d', out, source)
        header = out / 'io_github_freefrank_lostodyssey_RuntimeActivity.h'
        standard = out / 'standard.h'
        standard.write_text('#include <optional>\n#include <string>\n#include <vector>\n#include <span>\n')
        compiler = args.cxx
        includes = ['-I' + str(jdk / 'include'), '-I' + str(jdk / 'include/linux')]
        if args.ndk:
            compiler = args.ndk / 'toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android26-clang++'
            includes = []  # The NDK provides its own JNI headers.
        run(compiler, '-std=c++20', '-D__ANDROID__', '-fsyntax-only', '-include', standard,
            '-include', header, *includes, '-I' + str(ROOT / 'LostOdysseyRecomp'),
            ROOT / 'LostOdysseyRecomp/diagnostics/android_tester_jni.cpp')
        if args.ndk:
            run(compiler, '-std=c++20', '-c', '-I' + str(ROOT / 'LostOdysseyRecomp'),
                ROOT / 'tools/tests/android_tv_tester_test.cpp', '-o', out / 'tv-tester-arm64.o')
    print('PASS: five distinct Android commands, six javac-generated JNI signatures and JVM trace export')


if __name__ == '__main__':
    main()
