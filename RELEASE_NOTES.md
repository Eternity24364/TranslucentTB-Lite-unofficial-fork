# TranslucentTB Lite 1.0.0.1

**Unofficial, stripped-down fork of [TranslucentTB](https://github.com/TranslucentTB/TranslucentTB).**
Not affiliated with, sponsored by, or endorsed by the upstream project or its authors.
"TranslucentTB" is used only to describe where this code comes from.

This build has no settings and no configuration file: the taskbar appearance is fixed at
build time.

| Situation | Appearance |
|---|---|
| Desktop / normal windows | transparent |
| A window is maximized | acrylic |
| Task View is open | transparent |
| Taskbar line | always hidden |

- Tray menu: a classic Win32 menu with a single **Exit** item.
- Open at boot: enabled by the package manifest (no UI toggle).
- Logging: file log at `warn` level only.
- Reset hotkey: `Ctrl+Alt+Shift+F1`.
- Languages: English and Simplified Chinese.
- Supported systems: Windows 10 (classic taskbar) and Windows 11 22621+.
  Windows 11 21H2 (build 22000) is **not** supported.

## Downloads

| File | Size | SHA-256 |
|---|---|---|
| `TranslucentTB-Lite-1.0.0.1-portable-x64.zip` | 770 KB | `f2400c061e733b74d402e349ce7730ec439e735f5c08137092c6298dffc43b26` |
| `TranslucentTB-Lite-1.0.0.1-x64.msix` | 1.37 MB | `190ff45e47de41501be840588a775fc4f0f8a441de580ff6f4bc185d5e2b4a96` |
| `TranslucentTB-Lite.cer` | 776 B | `d931492f6773eade1247d7341acdd2342187f1c0dc00f41c00c6ad63eedc0b57` |

The source archive attached by GitHub is the corresponding source for these binaries.

## Installing

### Portable ZIP (recommended, no admin required)

1. Extract `TranslucentTB-Lite-1.0.0.1-portable-x64.zip`.
2. Run `TranslucentTB.exe`. If `MSVCP140.dll` is missing, install the
   Microsoft Visual C++ 2015-2022 Redistributable first.
3. Right-click the tray icon and choose **Exit** to quit.

The portable build does not include the MSIX startup task; use a shortcut in
`shell:startup` if you want it to start with Windows.

### MSIX (signed with a self-signed certificate)

Windows only installs the package after the signing certificate is trusted:

1. Double-click `TranslucentTB-Lite.cer` -> **Install Certificate**.
2. Select **Local Machine**, choose **Place all certificates in the following store** and
   select **Trusted Root Certification Authorities** (administrator rights required).
3. Double-click `TranslucentTB-Lite-1.0.0.1-x64.msix`, or run
   `Add-AppxPackage .\TranslucentTB-Lite-1.0.0.1-x64.msix`.

Publisher: `CN=eternity` (certificate valid until 2031). Only trust this certificate if the
files came from this repository. A publicly trusted code-signing certificate is not used
yet, so the certificate step is unavoidable for the MSIX.

## Notes

- This build shares the tray icon GUID and single-instance mutex with the upstream app.
  **Do not run it alongside the official TranslucentTB** (or another copy of this fork).
- After updating while Explorer already loaded `ExplorerTAP.dll`, the app may ask for a
  restart: restart `explorer.exe` (or Windows) and start it again.
- Logs: `%LOCALAPPDATA%\Packages\TranslucentTB_*<publisher-hash>\TempState\*.log` in
  packaged mode, `%TEMP%\TranslucentTB` in portable mode. Only warnings and errors.

## Verify downloads

```powershell
Get-FileHash .\TranslucentTB-Lite-1.0.0.1-portable-x64.zip -Algorithm SHA256
Get-FileHash .\TranslucentTB-Lite-1.0.0.1-x64.msix -Algorithm SHA256
```

Compare the hashes with the table above and with `SHA256SUMS.txt`.

## Removed compared to upstream

- all appearance settings (color picker, peek/line toggles, per-state accent selection)
- start menu, search and battery saver appearances
- per-window rules and the ignored-windows list
- `settings.json`, hot reload, welcome page, advanced/log-level tray menu
- the WinUI/XAML tray flyout and the Microsoft.UI.Xaml dependency
- the Windows 11 21H2 taskbar code path
- all languages except en-US and zh-CN

## License

GPLv3, same as the upstream project. See `LICENSE.md`; third-party notices are in
`THIRD-PARTY-NOTICES.md`. The icons and Store artwork are taken from the upstream project
and remain subject to their authors' rights; see the naming/trademark section in the
README.
