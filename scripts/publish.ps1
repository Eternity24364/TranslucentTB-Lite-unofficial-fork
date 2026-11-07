<#
.SYNOPSIS
    Build, sign and package TranslucentTB Lite for release.

.DESCRIPTION
    Builds the solution in Release, signs the MSIX and the portable binaries with the
    certificate in certs/, then writes the .msix, the portable .zip, the public .cer and a
    SHA256SUMS.txt into dist/.

    The certificate password is read from certs/pfx-password.txt (both files are ignored by
    git and must never be committed).

.PARAMETER SkipBuild
    Reuse the existing build output instead of running MSBuild.

.EXAMPLE
    pwsh -File scripts/publish.ps1
#>
param(
    [string]$Configuration = 'Release',
    [string]$Platform = 'x64',
    [string]$OutputDir = (Join-Path $PSScriptRoot '..\dist'),
    [string]$Certificate = (Join-Path $PSScriptRoot '..\certs\TranslucentTBLite.pfx'),
    [string]$PasswordFile = (Join-Path $PSScriptRoot '..\certs\pfx-password.txt'),
    [switch]$SkipBuild
)

$ErrorActionPreference = 'Stop'

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$version = '1.0.0.1'
$portableFiles = @(
    'TranslucentTB.exe',
    'ExplorerHooks.dll',
    'ExplorerTAP.dll',
    'ProgramLog.dll'
)

function Find-MSBuild {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (Test-Path $vswhere) {
        $path = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe' 2>$null
        if ($path) { return $path }
    }

    $candidates = @(
        'D:\Program Files\VisualStudio\MSBuild\Current\Bin\MSBuild.exe',
        'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe',
        'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\MSBuild\Current\Bin\MSBuild.exe'
    )
    foreach ($candidate in $candidates) {
        if (Test-Path $candidate) { return $candidate }
    }

    throw 'MSBuild.exe not found. Install Visual Studio with the C++ desktop workload.'
}

function Find-SignTool {
    $kits = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10\bin'
    $tool = Get-ChildItem $kits -Recurse -Filter signtool.exe -ErrorAction SilentlyContinue |
        Where-Object { $_.FullName -like '*\x64\signtool.exe' } |
        Sort-Object FullName -Descending |
        Select-Object -First 1
    if ($tool) { return $tool.FullName }
    throw 'signtool.exe not found. Install the Windows SDK signing tools.'
}

if (-not (Test-Path $Certificate)) { throw "Certificate not found: $Certificate" }
if (-not (Test-Path $PasswordFile)) { throw "Certificate password file not found: $PasswordFile" }

$msbuild = Find-MSBuild
$signtool = Find-SignTool
$password = (Get-Content $PasswordFile -Raw).Trim()

if (-not $SkipBuild) {
    Write-Host "Building $Configuration|$Platform..."
    & $msbuild (Join-Path $root 'TranslucentTB.slnx') /m /t:Build /p:Configuration=$Configuration /p:Platform=$Platform /v:m /nologo
    if ($LASTEXITCODE -ne 0) { throw "Build failed with exit code $LASTEXITCODE" }
}

$outDir = if ([System.IO.Path]::IsPathRooted($OutputDir)) {
    [System.IO.Path]::GetFullPath($OutputDir)
} else {
    [System.IO.Path]::GetFullPath((Join-Path $root $OutputDir))
}
New-Item -ItemType Directory -Force $outDir | Out-Null

$binDir = Join-Path $root "x64\$Configuration"
$msix = Get-ChildItem (Join-Path $root 'AppPackage\AppPackages') -Recurse -Filter '*.msix' |
    Sort-Object LastWriteTime -Descending |
    Select-Object -First 1
if (-not $msix) { throw 'No MSIX found. Build the AppPackage project first.' }

Write-Host "Signing MSIX $($msix.Name)..."
& $signtool sign /fd SHA256 /f $Certificate /p $password /tr http://timestamp.digicert.com /td SHA256 $msix.FullName
if ($LASTEXITCODE -ne 0) { throw 'MSIX signing failed' }

foreach ($file in $portableFiles) {
    $path = Join-Path $binDir $file
    if (-not (Test-Path $path)) { throw "Missing build output: $path" }
    Write-Host "Signing $file..."
    & $signtool sign /fd SHA256 /f $Certificate /p $password /tr http://timestamp.digicert.com /td SHA256 $path
    if ($LASTEXITCODE -ne 0) { throw "Signing failed for $file" }
}

$msixOut = Join-Path $outDir "TranslucentTB-Lite-$version-$Platform.msix"
Copy-Item $msix.FullName $msixOut -Force
Copy-Item (Join-Path $root 'certs\TranslucentTBLite.cer') (Join-Path $outDir 'TranslucentTB-Lite.cer') -Force

$staging = Join-Path ([System.IO.Path]::GetTempPath()) ("ttb-lite-portable-" + [guid]::NewGuid())
New-Item -ItemType Directory -Force $staging | Out-Null
try {
    foreach ($file in $portableFiles) {
        Copy-Item (Join-Path $binDir $file) (Join-Path $staging $file) -Force
    }
    Copy-Item (Join-Path $root 'LICENSE.md') $staging -Force
    Copy-Item (Join-Path $root 'THIRD-PARTY-NOTICES.md') $staging -Force

    $installText = @"
TranslucentTB Lite $version - portable build ($Platform)

1. Run TranslucentTB.exe. Right-click the tray icon and choose Exit to quit.
2. Requires Windows 10 1809+ or Windows 11 22621+. If the app fails to start because
   MSVCP140.dll is missing, install the Microsoft Visual C++ 2015-2022 Redistributable.
3. This build shares its tray icon GUID and single-instance mutex with the upstream
   TranslucentTB. Do not run it alongside the official app.
4. This is an unofficial fork, not affiliated with the TranslucentTB project.
5. Ctrl+Alt+Shift+F1 resets the taskbar state. Portable mode logs warnings to
   %TEMP%\TranslucentTB.
"@
    Set-Content (Join-Path $staging 'INSTALL.txt') -Value $installText -Encoding UTF8

    $zipOut = Join-Path $outDir "TranslucentTB-Lite-$version-portable-$Platform.zip"
    if (Test-Path $zipOut) { Remove-Item $zipOut -Force }
    Compress-Archive -Path (Join-Path $staging '*') -DestinationPath $zipOut -Force
}
finally {
    Remove-Item $staging -Recurse -Force -ErrorAction SilentlyContinue
}

$msixInstall = @"
TranslucentTB Lite $version - MSIX installation

The package is signed by a self-signed certificate (Publisher: CN=eternity).
Windows only installs it after that certificate is trusted.

1. Double-click TranslucentTB-Lite.cer and choose Install Certificate.
2. Select Local Machine, then "Place all certificates in the following store".
3. Choose "Trusted Root Certification Authorities" (administrator rights required).
   Optionally repeat for "Trusted People".
4. Double-click TranslucentTB-Lite-$version-$Platform.msix, or run:
   Add-AppxPackage .\TranslucentTB-Lite-$version-$Platform.msix

If double-clicking does nothing, right-click the .msix -> Open with -> App Installer.

Only trust this certificate if you downloaded this release from the official repository.
"@
Set-Content (Join-Path $outDir 'INSTALL-MSIX.txt') -Value $msixInstall -Encoding UTF8

$hashFile = Join-Path $outDir 'SHA256SUMS.txt'
Remove-Item $hashFile -Force -ErrorAction SilentlyContinue
Get-ChildItem $outDir -File | Where-Object Name -ne 'SHA256SUMS.txt' | ForEach-Object {
    $hash = (Get-FileHash $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
    "$hash  $($_.Name)"
} | Set-Content $hashFile -Encoding ASCII

Write-Host ''
Write-Host "Release artifacts in ${outDir}:"
Get-ChildItem $outDir | Select-Object Name, Length | Format-Table -AutoSize
