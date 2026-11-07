# Installing TranslucentTB Lite

Two builds are published on the [Releases page](../../releases):

| Artifact | Use it if |
|---|---|
| `TranslucentTB-Lite-<version>-portable-x64.zip` | you want a no-install build (no admin rights) |
| `TranslucentTB-Lite-<version>-x64.msix` | you want a normal installed app with the boot task |

You only need one of them. Do not run both at the same time.

## Requirements

- Windows 10 version 1809 or newer (classic taskbar), or Windows 11 build 22621 or newer.
  Windows 11 21H2 (build 22000) is not supported.
- x64 Windows.
- Portable build only: the Microsoft Visual C++ 2015-2022 Redistributable if
  `MSVCP140.dll` is not already present.
- MSIX only: administrator rights once, to trust the self-signed certificate.

## Option 1: portable ZIP (no installation)

1. Download `TranslucentTB-Lite-<version>-portable-x64.zip` from the Releases page.
2. Extract it anywhere, for example `D:\Tools\TranslucentTB Lite`.
3. Run `TranslucentTB.exe`.
   - If Windows SmartScreen shows "Windows protected your PC", choose **More info** ->
     **Run anyway**.
   - If the app reports a missing `MSVCP140.dll`, install the
     [Microsoft Visual C++ 2015-2022 Redistributable (x64)](https://aka.ms/vs/17/release/vc_redist.x64.exe)
     and try again.
4. To start it with Windows, put a shortcut to `TranslucentTB.exe` into the folder opened by
   `Win+R` -> `shell:startup`. (The MSIX startup task is not available in portable mode.)
5. To quit, right-click the tray icon and choose **Exit**.

The portable build writes logs to `%TEMP%\TranslucentTB` (warnings and errors only).

## Option 2: MSIX (installed app)

The package is signed with a self-signed certificate (`CN=eternity`), so Windows only
installs it after that certificate is trusted. This is a one-time administrator step.

> **Coming from a development build?** These steps are all a normal user needs. Only if
> you previously installed this project through Visual Studio (F5) or an older development
> package with the same version, remove it first, otherwise the install can fail with
> `0x80073D06`:
>
> ```powershell
> Get-AppxPackage TranslucentTB | Remove-AppxPackage
> ```

### 1. Trust the certificate (administrator)

Open **PowerShell as Administrator**, go to the folder with the downloaded files and run:

```powershell
certutil -addstore -f Root .\TranslucentTB-Lite.cer
certutil -addstore -f TrustedPeople .\TranslucentTB-Lite.cer
```

Or with the certificate wizard:

1. Double-click `TranslucentTB-Lite.cer`.
2. **Install Certificate** -> **Local Machine**.
3. **Place all certificates in the following store** ->
   **Trusted Root Certification Authorities** -> Next -> Finish.
4. Optionally repeat for **Trusted People**.

Only trust the certificate if the files came from this repository. If a publicly trusted
code-signing certificate is used in the future, this step will disappear.

### 2. Install the package

Double-click `TranslucentTB-Lite-<version>-x64.msix`, or in an administrator or normal
PowerShell:

```powershell
Add-AppxPackage .\TranslucentTB-Lite-<version>-x64.msix
```

### 3. Launch

Start **TranslucentTB Lite** from the Start menu. The tray icon appears and the package
enables its startup task automatically (visible under Settings -> Apps -> Startup).

### Verify / uninstall

```powershell
Get-AppxPackage TranslucentTB | Select-Object PackageFullName, Version, Publisher
```

Uninstall through **Settings -> Apps -> Installed apps -> TranslucentTB Lite**, or:

```powershell
Get-AppxPackage TranslucentTB | Remove-AppxPackage
```

Removing the certificate afterwards is optional: open `certmgr.msc` -> *Trusted Root
Certification Authorities* / *Trusted People* -> delete `eternity`.

## Updating

- **MSIX:** the new package must have a higher `Identity Version` than the installed one.
  Install it the same way as above; the certificate must stay the same (`CN=eternity`).
- **Portable:** exit the app, replace the files in the folder, start it again.

If the app asks to restart Windows after an update, Explorer was still holding
`ExplorerTAP.dll`: restart `explorer.exe` (or Windows) and start the app again.

## Troubleshooting

| Symptom | Cause / fix |
|---|---|
| `0x800B0109` during MSIX install | certificate not in *Local Machine -> Trusted Root Certification Authorities*; repeat step 1 as administrator |
| `0x80073D06` or "newer version already installed" | an older development build (for example from F5) is registered; remove it with `Get-AppxPackage TranslucentTB \| Remove-AppxPackage` |
| SmartScreen "unknown publisher" for the portable build | expected for a self-signed build; *More info* -> *Run anyway*, or use the MSIX |
| Tray icon disappears after an Explorer restart | wait a few seconds; if it does not come back, start the app again |
| Taskbar has no effect | make sure the official TranslucentTB (or another copy) is not running; both share the tray GUID and single-instance mutex |
| "restart required" prompt after an update | restart `explorer.exe` (or Windows) |
| Need to reset the taskbar state | `Ctrl+Alt+Shift+F1` |

## 中文安装步骤

**便携版（推荐，不需要管理员）**

1. 下载 `TranslucentTB-Lite-<版本>-portable-x64.zip` 并解压；
2. 双击 `TranslucentTB.exe`，SmartScreen 提示时选「更多信息 → 仍要运行」；
3. 想开机自启，把 exe 快捷方式放进 `Win+R` → `shell:startup` 打开的文件夹；
4. 退出：右键托盘图标 → **Exit**。

**MSIX 版（需要管理员一次）**

> 如果你之前用 Visual Studio F5 部署过旧开发包（或装过同版本的开发包），先执行
> `Get-AppxPackage TranslucentTB | Remove-AppxPackage`。从 Release 正常安装的用户**不需要**这一步。

1. 以管理员打开 PowerShell，执行：
   ```powershell
   certutil -addstore -f Root .\TranslucentTB-Lite.cer
   certutil -addstore -f TrustedPeople .\TranslucentTB-Lite.cer
   ```
2. 双击 `.msix`，或执行 `Add-AppxPackage .\TranslucentTB-Lite-<版本>-x64.msix`；
3. 从开始菜单启动 **TranslucentTB Lite**。

常见错误：`0x800B0109` 是证书没装进「本地计算机 → 受信任的根证书颁发机构」；
`0x80073D06` 是旧包没卸载。不要同时运行官方版，它们共用托盘 GUID 和单实例互斥。
