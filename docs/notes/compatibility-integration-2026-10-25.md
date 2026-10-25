# Compatibility integration review — 25 October 2026

This branch combines four independently submitted corrections against upstream
`2128383494a53071db658cbf550778beedc6240d` for maintainer review.

| Pull request | Change |
| --- | --- |
| [#707](https://github.com/freefrank/LostOdysseyRecomp/pull/707) | Bound driver-cache exports and contain exporter allocation failures. |
| [#709](https://github.com/freefrank/LostOdysseyRecomp/pull/709) | Provide one stage mask per generic Vulkan submission wait. |
| [#711](https://github.com/freefrank/LostOdysseyRecomp/pull/711) | Return texture image/view allocation failures to callers. |
| [#712](https://github.com/freefrank/LostOdysseyRecomp/pull/712) | Wrap XMA output ring cursors before accounting and writes. |

The individual branches overlap in the tracked Plume patch, fixture setup,
fixture registration and release notes. This branch retains all four fixtures,
applies the SDL3 overlay during fixture setup and resolves their shared-file
conflicts. The Plume patch was regenerated from the pinned dependency with both
Vulkan corrections; its hunk layout and blob identities reflect that combined
source. The dependency commit pins are unchanged.

Validation performed on the combined tree:

- A clean Plume replay reconstructs the complete intended dependency tree,
  `5118a6950aa3c51e192439f6dac491aa04f1655a`.
- The SDL3 and Android overlays apply. The macOS overlay passes its application
  check with SDL3, and with SDL3 plus the Android overlay.
- GCC 13.3 / C++23 checks the complete patched `plume_vulkan.cpp` with
  `-fsyntax-only`, the pinned Vulkan/VMA/volk/SDL headers and the SDL Vulkan
  integration enabled, without diagnostics.
- All 52 locally supported review-regression CTest entries pass with ASan/UBSan,
  including driver-cache export, Vulkan submission, Vulkan texture allocation
  and 63,493 XMA ring cases. The SDL fixtures reuse the same pinned SDL static
  library built for the independent XMA branch.
- The Linux home-layout namespace test is excluded locally because this host
  cannot create its isolated environment. Leak detection is disabled locally
  because this runner cannot inspect the required `/proc` state.
- `git diff --check` passes against the upstream base.

The independent XMA and corrected texture branches each passed all 50 review
regressions on GitHub Actions. The fork's Android APK workflow stops while
retrieving the private build-input repository, before Android compilation.
Full runtime/APK compilation, real GPU or Android hardware, gameplay, audible
playback and performance have not been verified. The XMA fixture uses synthetic
PCM and does not exercise FFmpeg packet decoding.

These are shared runtime changes for review. No upstream merge or release has
been performed, and no game assets are included.
