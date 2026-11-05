# Repository publication

## Steprant Android fork

This fork publishes to **`Steprant/LostOdysseyRecomp-Android`**, on `integration/v1.1.0-steprant-all`; its app version is **1.5**. The branch name identifies the upstream base, not the APK version. The current [release notes](STEPRANT_RELEASE_NOTES.md) own the download and checksum. Use [the Android packaging guide](notes/android-fork-build-inputs.md#steprant-android-15-packaging) for future builds. Its private-input requirement remains in force.

The inherited multi-platform release workflow targets the current repository and excludes automatic `v*-steprant.*` Android tags. It must not be used to overwrite an Android-only release with desktop packages. The `.forge.json` manifest remains an upstream desktop installer reference, not a Steprant Android download manifest.

## Upstream multi-platform procedure

Develop in an isolated worktree and verify remote names with `git remote -v` before a push. `origin` is the public GitHub repository, `freefrank/LostOdysseyRecomp`. Publish reviewed changes on a feature branch and open a pull request; follow the maintainer's review gate before merging. Reuse relevant passing checks; a push alone does not require another build or game run.

```powershell
$publishBranch = git branch --show-current
$publishRevision = git rev-parse HEAD
git -c push.followTags=false push origin "${publishRevision}:refs/heads/${publishBranch}"
git ls-remote origin "refs/heads/${publishBranch}"
```

The readback hash must equal `$publishRevision`. Stop if a push fails; resolve connection, authentication or divergent-history errors without force-pushing. Review the unpublished diff for private or generated content before publication.

All CI/CD runs on GitHub Actions using GitHub-hosted runners. Pushes and pull requests start the native checks when their path filters match; manual dispatch can run a skipped workflow on a branch. There is no second CI remote to push to. See [GitHub Actions checks and releases](notes/ci.md) for workflows, results, credentials and caches.

The legacy `tools/push_all.ps1` helper hardcodes `origin` and `github`. It needs a separate remote-selection update before use with this publication path. Do not rename remotes merely to follow an old example.

The pre-cleanup history was retained only on the former Gitea mirror in `archive/pre-public-cleanup-2026-09-05`. This is historical storage, not a publication remote. Never merge that branch into `main` or publish it to GitHub. Do not use `--all`, `--mirror` or automatic tag pushing.

Release tags must be created from reviewed public commits and pushed explicitly to `origin`. A `v*` tag starts the release pipeline, creates a draft and builds the Windows ZIP, AppImage, Flatpak, Android APK and macOS DMG. The DMG is built on GitHub's hosted Apple Silicon runner ([macOS releases](MACOS_RELEASE.md)). Once all package jobs succeed, publication verifies exactly these five assets; no manual package build or upload is required.

```powershell
git push origin refs/tags/vX.Y.Z
gh run list --repo freefrank/LostOdysseyRecomp --workflow release.yml
```

For a build-only rehearsal, dispatch the workflow on the intended branch with an empty `release_tag`:

```powershell
gh workflow run release.yml --repo freefrank/LostOdysseyRecomp --ref <branch> -f release_tag=
```

Check the GitHub run and published assets before announcing a release. The publish job dispatches the Discord announcement workflow after publication; build-only rehearsals do not publish or announce anything.

Personal game data, saves, generated sources and analysis outputs stay ignored. Dependency submodules retain their upstream histories and licenses.
