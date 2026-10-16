# Installs (or -Uninstall) the Picasa no-buffering proxy version.dll.
# Expects version.dll next to this script.
param([string]$PicasaDir = "${env:ProgramFiles(x86)}\Google\Picasa3", [switch]$Uninstall)
$ErrorActionPreference = 'Stop'

if (-not ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole('Administrators')) {
    $argList = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', "`"$PSCommandPath`"", '-PicasaDir', "`"$PicasaDir`"")
    if ($Uninstall) { $argList += '-Uninstall' }
    Start-Process powershell -Verb RunAs -ArgumentList $argList
    exit
}

if (-not (Test-Path "$PicasaDir\Picasa3.exe")) { throw "Picasa3.exe not found in $PicasaDir (use -PicasaDir)" }
$proxy = Join-Path $PicasaDir 'version.dll'
$orig  = Join-Path $PicasaDir 'version_orig.dll'

Get-Process Picasa3, PicasaPhotoViewer -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Milliseconds 500

if ($Uninstall) {
    Remove-Item $proxy, $orig -Force -ErrorAction SilentlyContinue
    Remove-MpPreference -ExclusionPath $proxy -ErrorAction SilentlyContinue
    Write-Host "Removed proxy from $PicasaDir"
} else {
    $src = Join-Path $PSScriptRoot 'version.dll'
    if (-not (Test-Path $src)) { throw "version.dll not found next to this script" }
    Unblock-File $src                             # strip downloaded-file mark
    Add-MpPreference -ExclusionPath $proxy       # before copying, so Defender doesn't quarantine it
    Copy-Item "$env:SystemRoot\SysWOW64\version.dll" $orig -Force
    Copy-Item $src $proxy -Force
    Write-Host "Installed proxy in $PicasaDir (Defender exclusion added for $proxy)"
}
Read-Host "Done - press Enter"
