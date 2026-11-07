# TranslucentTB Lite (unofficial fork)

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

- The package identity, publisher identity, tray icon GUID and single-instance mutex are
  **unchanged from upstream**. Do not install this build alongside the official
  TranslucentTB (or another copy of this fork); they will fight over the same taskbar and
  tray icon.
- When the app is updated while Explorer already has `ExplorerTAP.dll` loaded, Windows
  cannot replace the copy in the package's `TempState` folder and the app asks for a
  restart. Restart `explorer.exe` (or Windows) and start the app again.
- Logs are written to
  `%LOCALAPPDATA%\Packages\TranslucentTB_99myrp4edc0yc\TempState\<process-start-time>.log`
  and only contain warnings/errors.

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

An MSIX package must be signed before Windows will install it outside Developer Mode.
There are three practical options:

1. **Portable ZIP (simplest, no signing).** Ship `TranslucentTB.exe` together with
   `ExplorerHooks.dll`, `ExplorerTAP.dll` and `ProgramLog.dll`. Windows may show a
   SmartScreen "unknown publisher" prompt; users choose *More info* -> *Run anyway*.
   The tray icon and taskbar effects work; the MSIX startup task is not available in this
   mode.
2. **Self-signed MSIX (free).** Create a code-signing certificate, put its subject into
   `Publisher` in [Package.appxmanifest](AppPackage/Package.appxmanifest), sign the package
   with `signtool`, and tell users to trust your `.cer` first:

   ```powershell
   $cert = New-SelfSignedCertificate -Type Custom -Subject "CN=Your Name" `
       -KeyUsage DigitalSignature -FriendlyName "TranslucentTB Lite" `
       -CertStoreLocation Cert:\CurrentUser\My `
       -TextExtension @("2.5.29.37={text}1.3.6.1.5.5.7.3.3")
   Export-PfxCertificate -Cert $cert -FilePath ttb-lite.pfx -Password (Read-Host -AsSecureString)
   Export-Certificate  -Cert $cert -FilePath ttb-lite.cer

   signtool sign /fd SHA256 /f ttb-lite.pfx /tr http://timestamp.digicert.com /td SHA256 `
       AppPackage_1.0.0.0_x64.msix
   ```

   Users install the certificate once (double-click -> *Install Certificate* ->
   *Local Machine* -> *Trusted People*), then can install the MSIX. The certificate must be
   renewed when it expires; timestamping keeps old releases valid.
3. **Commercial code-signing certificate (paid).** Removes the "unknown publisher" warning
   entirely, but requires buying a certificate and keeping its private key in a hardware
   token/HSM. `Publisher` must match the certificate subject.

This repository currently keeps the upstream `Publisher` value, so it builds and runs via
F5/Developer Mode but is not signed for distribution. Change `Publisher` to your own
certificate subject before publishing a signed MSIX.

## License

This fork is distributed under the **GNU General Public License v3.0**, the same license as
the upstream project. See [LICENSE.md](LICENSE.md). Modified sources are covered by GPLv3
as well; if you distribute binaries, the corresponding source is this repository.

Upstream project: <https://github.com/TranslucentTB/TranslucentTB> (GPLv3).
This fork is based on upstream commit `d4636e4` plus local changes.

Third-party components and their licenses are listed in
[THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md).
