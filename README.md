# TranslucentTB Lite

> **This is an unofficial, stripped-down fork of [TranslucentTB](https://github.com/TranslucentTB/TranslucentTB).**
> It is **not affiliated with, sponsored by, or endorsed by** the TranslucentTB project or its authors.
> "TranslucentTB" is used only to describe where this code comes from.

A minimal, non-configurable build of TranslucentTB: the taskbar is transparent on the
desktop, acrylic when a window is maximized, transparent while Task View is open, and the
taskbar line is always hidden. There is no settings UI and no `settings.json` — everything
is fixed at build time.

## Behavior

| Situation | Appearance |
|---|---|
| Desktop / normal windows | transparent |
| A window is maximized | acrylic |
| Task View is open | transparent |
| Taskbar line | always hidden |

- Tray menu: a classic Win32 menu with a single **Exit** item.
- Open at boot: enabled through the package manifest (`StartupTask`); there is no toggle.
- Logging: file log at `warn` level only.
- Reset hotkey: `Ctrl+Alt+Shift+F1` re-initializes the taskbar state (useful after shell glitches).
- Languages: en-US and zh-CN.
- Supported systems: Windows 10 (classic taskbar) and Windows 11 22621+. Windows 11 21H2
  (build 22000) is **not** supported by this fork.

## Removed compared to upstream

- all appearance options (color picker, peek/line toggles, per-state accent selection)
- start menu, search and battery saver appearances
- per-window rules and the ignored-windows list
- `settings.json`, hot reload, the welcome page, the advanced/log-level tray menu
- the whole WinUI/XAML tray flyout (`Xaml` project) and the Microsoft.UI.Xaml dependency
- the Windows 11 21H2 taskbar code path
- all languages except en-US and zh-CN

## Install

Download the latest ZIP or MSIX from the [Releases page](../../releases). Step-by-step
instructions for both builds (including the one-time certificate trust required by the
MSIX) are in [INSTALL.md](INSTALL.md).

Quick version:

- **Portable:** extract the ZIP and run `TranslucentTB.exe`. No admin rights needed.
- **MSIX:** trust `TranslucentTB-Lite.cer` in *Local Machine -> Trusted Root Certification
  Authorities* (administrator), then install the `.msix`. See [INSTALL.md](INSTALL.md) for
  the exact commands.

## Build

Requirements:

- Visual Studio 2026 with the Desktop C++ workload, the Windows application packaging
  (UWP/MSIX) tooling, and the Windows 11 SDK (10.0.26100)
- C++/WinRT, NuGet and vcpkg (restored automatically by the build)
- x64 Windows 10 1809+ / Windows 11

Open [TranslucentTB.slnx](TranslucentTB.slnx), select **Release | x64**, build the
`AppPackage` project and deploy (F5). The build produces the MSIX under
`AppPackage/AppPackages/` and a loose layout under `AppPackage/bin/x64/Release/AppX`.

Command line equivalent:

```powershell
msbuild TranslucentTB.slnx /m /p:Configuration=Release /p:Platform=x64
```

If your Visual Studio installation has no UWP workload, the `WindowsUdk` project cannot be
built; install the "Windows application packaging" / UWP build tools first.

## Running and updating

- The tray icon GUID and the single-instance mutex are **unchanged from upstream**, so this
  build cannot run alongside the official TranslucentTB (or another copy of this fork).
  The package publisher is now `CN=eternity`, so the MSIX is a separate package from the
  official one, but the runtime still conflicts.
- When the app is updated while Explorer already has `ExplorerTAP.dll` loaded, Windows
  cannot replace the copy in the package's `TempState` folder and the app asks for a
  restart. Restart `explorer.exe` (or Windows) and start the app again.
- Logs are written to
  `%LOCALAPPDATA%\Packages\TranslucentTB_*<publisher-hash>\TempState\<process-start-time>.log`
  in packaged mode (or `%TEMP%\TranslucentTB` in portable mode) and only contain
  warnings/errors.

## Naming, icons and trademarks

- The name **TranslucentTB**, its icons, the tray icons and the Store artwork are taken
  from the upstream project and remain the property of their respective authors. They are
  used here only to describe the origin of this fork.
- This repository keeps the original icons and package artwork for now. If you redistribute
  this fork more widely, replace them with your own artwork to avoid implying that this is
  an official TranslucentTB release.
- No trademark or name license is granted by the GPL; nothing in this repository should be
  read as an endorsement by the upstream project.

## Releases and signing

Each release ships two artifacts:

- `TranslucentTB-Lite-<version>-portable-x64.zip` — unpack and run `TranslucentTB.exe`.
  No installation or signing is required; Windows SmartScreen may warn about an unknown
  publisher. The portable build needs the Microsoft Visual C++ 2015-2022 Redistributable if
  `MSVCP140.dll` is not already present. The startup task is not available in this mode.
- `TranslucentTB-Lite-<version>-x64.msix` — the packaged build. It is signed with a
  self-signed certificate (`CN=eternity`, valid until 2031) whose public half ships as
  `TranslucentTB-Lite.cer`.

**Installing the MSIX:** Windows requires the signing certificate to be trusted. Install
`TranslucentTB-Lite.cer` into *Local Machine* -> *Trusted Root Certification Authorities*
(administrator rights required; *Trusted People* can be added as well), then double-click
the `.msix` or run `Add-AppxPackage .\TranslucentTB-Lite-<version>-x64.msix`. Only trust the
certificate if the package came from this repository. A paid, publicly trusted
code-signing certificate would remove this step, but is not used here.

**Signing new releases:** the private key and password live in `certs/` (ignored by git,
never commit them) and `scripts/publish.ps1` builds, signs the binaries and the MSIX, and
writes everything to `dist/`:

```powershell
pwsh -File scripts/publish.ps1
```

If the certificate is lost, a new one must be created with the same subject (`CN=eternity`)
and users have to trust it again. Keep `certs/pfx-password.txt` safe; without it the PFX
cannot be used. Timestamping (`/tr`) keeps already-published signatures valid after the
certificate expires.

## License

This fork is distributed under the **GNU General Public License v3.0**, the same license as
the upstream project. See [LICENSE.md](LICENSE.md). Modified sources are covered by GPLv3
as well; if you distribute binaries, the corresponding source is this repository.

Upstream project: <https://github.com/TranslucentTB/TranslucentTB> (GPLv3).
This fork is based on upstream commit `d4636e4` plus local changes.

Third-party components and their licenses are listed in
[THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md).
