#!/usr/bin/env bash
set -euo pipefail

repo=$(cd "$(dirname "$0")/../.." && pwd)
sdk=${ANDROID_HOME:-${ANDROID_SDK_ROOT:-}}
if [[ -z "$sdk" ]]; then
    echo "Set ANDROID_HOME to the Android SDK directory" >&2
    exit 1
fi

ndk="$sdk/ndk/28.2.13676358"
cmake="$sdk/cmake/3.22.1/bin/cmake"
llvm_bin="$ndk/toolchains/llvm/prebuilt/linux-x86_64/bin"
for executable in "$cmake" "$llvm_bin/llvm-readelf" "$llvm_bin/llvm-nm" "$llvm_bin/llvm-strip" "$llvm_bin/llvm-objcopy"; do
    if [[ ! -x "$executable" ]]; then
        echo "Required Android build tool is missing: $executable" >&2
        exit 1
    fi
done
command -v ninja >/dev/null || { echo "Ninja is required" >&2; exit 1; }

cache=${LO_DXC_CACHE_DIR:-${XDG_CACHE_HOME:-$HOME/.cache}/lostodysseyrecomp/android-dxc}
mkdir -p "$cache"
cache=$(cd "$cache" && pwd -P)
source="$cache/source"
build="$cache/build-android-cmake322"
artifact="$cache/artifacts/libdxcompiler.so"
output=${LO_DXC_OUTPUT:-$repo/out/android-dxc/libdxcompiler.so}
commit=416fab6b5c4ba956a320d9131102304da995edfc
release_base=737a12a663f1697d3755a522d8fbf30481ecd2f6

# Android API level and NDK of the ELF's .note.android.ident, e.g.
# "26 r28c 13676358". `file` decodes this note only with newer magic databases
# (Ubuntu 22.04's file 5.41 does not), which sent CI runs into source builds
# whose output then failed this check too.
android_ident() {
    local note copy
    note=$(mktemp)
    copy=$(mktemp)
    if ! "$llvm_bin/llvm-objcopy" --dump-section=.note.android.ident="$note" "$1" "$copy" 2>/dev/null; then
        rm -f "$note" "$copy"
        return 1
    fi
    rm -f "$copy"
    python3 - "$note" <<'PY'
import struct, sys
data = open(sys.argv[1], "rb").read()
namesz, descsz, kind = struct.unpack_from("<III", data, 0)
name = data[12:12 + namesz].rstrip(b"\0")
desc = data[12 + ((namesz + 3) & ~3):][:descsz]
if name != b"Android" or kind != 1 or len(desc) < 4:
    sys.exit(1)
api = struct.unpack_from("<i", desc, 0)[0]
fields = [desc[4:68], desc[68:132]] if len(desc) >= 132 else [b"", b""]
print(api, *(f.split(b"\0")[0].decode() or "-" for f in fields))
PY
    local status=$?
    rm -f "$note"
    return $status
}

verify_binary() {
    local library=$1 ident
    [[ -s "$library" ]] || { echo "  missing or empty: $library" >&2; return 1; }
    ident=$(android_ident "$library") || { echo "  no Android ELF note: $library" >&2; return 1; }
    [[ "$ident" == "26 r28c 13676358" ]] || { echo "  Android note '$ident', expected API 26 / NDK r28c (13676358)" >&2; return 1; }
    "$llvm_bin/llvm-readelf" --file-header "$library" | grep -F AArch64 >/dev/null || { echo "  not AArch64" >&2; return 1; }
    "$llvm_bin/llvm-nm" -D --defined-only "$library" | grep -E ' T DxcCreateInstance$' >/dev/null || { echo "  DxcCreateInstance not exported" >&2; return 1; }
    strings "$library" | grep -F 'dxcoob 1.8.2407.7 (416fab6b)' >/dev/null || { echo "  version string dxcoob 1.8.2407.7 (416fab6b) not found" >&2; return 1; }
}

if verify_binary "$output"; then
    echo "Reusing Android DXC: $output"
    sha256sum "$output"
    exit 0
elif [[ -s "$output" ]]; then
    # Say why an existing library is rebuilt; the `file` description varies by host.
    echo "Existing $output failed the ABI/API/version/export checks:" >&2
    file "$output" >&2 || true
fi
if verify_binary "$artifact"; then
    mkdir -p "$(dirname "$output")"
    if ! cmp -s "$artifact" "$output"; then
        cp "$artifact" "$output"
    fi
    echo "Reusing Android DXC: $output"
    sha256sum "$output"
    exit 0
fi

if [[ ! -e "$source" ]]; then
    git clone --depth 1 --branch v1.8.2407 --single-branch \
        https://github.com/microsoft/DirectXShaderCompiler.git "$source"
fi
if [[ ! -d "$source/.git" || $(git -C "$source" rev-parse HEAD) != "$commit" ]]; then
    echo "DXC cache source is not the pinned v1.8.2407 commit: $source" >&2
    exit 1
fi
if ! git -C "$source" cat-file -e "$release_base^{commit}" 2>/dev/null; then
    # The official version is 7 commits after this base. Avoid a full-history clone.
    git -C "$source" fetch --deepen=10 origin refs/tags/v1.8.2407
fi
git -C "$source" merge-base --is-ancestor "$release_base" HEAD
if [[ $(git -C "$source" rev-list --count "$release_base..HEAD") != 7 ]]; then
    echo "DXC release history is incomplete; cannot generate version 1.8.2407.7" >&2
    exit 1
fi
git -C "$source" submodule update --init --depth 1 \
    external/DirectX-Headers external/SPIRV-Headers external/SPIRV-Tools
if [[ -n $(git -C "$source" status --porcelain --untracked-files=no) ]]; then
    echo "DXC cached source has local edits: $source" >&2
    exit 1
fi

"$cmake" -S "$source" -B "$build" -G Ninja \
    -C "$source/cmake/caches/PredefinedParams.cmake" \
    -DCMAKE_TOOLCHAIN_FILE="$ndk/build/cmake/android.toolchain.cmake" \
    -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-26 \
    -DCMAKE_BUILD_TYPE=Release -DHLSL_OFFICIAL_BUILD=ON \
    -DENABLE_SPIRV_CODEGEN=ON -DSPIRV_BUILD_TESTS=OFF \
    -DHLSL_INCLUDE_TESTS=OFF -DLLVM_INCLUDE_TESTS=OFF \
    -DLLVM_BUILD_TESTS=OFF -DLLVM_BUILD_TOOLS=OFF \
    -DLLVM_ENABLE_ZLIB=OFF -DLLVM_ENABLE_TERMINFO=OFF \
    -DLLVM_ENABLE_ASSERTIONS=OFF

# The upstream cross build creates native table generators with EH/RTTI off,
# although its LLVM Support code uses try/throw. Configure that host build first.
"$cmake" -S "$source" -B "$build/NATIVE" -G Ninja \
    -DLLVM_ENABLE_EH=ON -DLLVM_ENABLE_RTTI=ON \
    -DHLSL_INCLUDE_TESTS=OFF -DSPIRV_BUILD_TESTS=OFF
"$cmake" --build "$build" --target dxcompiler --parallel "${LO_DXC_JOBS:-8}"

library="$build/lib/libdxcompiler.so"
if [[ ! -f "$library" ]]; then
    echo "DXC did not produce $library" >&2
    exit 1
fi
mkdir -p "$(dirname "$artifact")" "$(dirname "$output")"
"$llvm_bin/llvm-strip" --strip-debug -o "$artifact" "$library"
if ! verify_binary "$artifact"; then
    echo "Android DXC artifact failed ABI, API, version, or export checks" >&2
    exit 1
fi
cp "$artifact" "$output"
echo "Built Android DXC: $output"
sha256sum "$output"
