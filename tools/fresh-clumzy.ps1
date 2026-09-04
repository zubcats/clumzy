# Fresh Clumzy install for a stuck/old zip.
# Kills running copies, deletes leftover zips/folders, downloads latest to Desktop\Clumzy.
#Requires -Version 5
$ErrorActionPreference = 'Stop'

$ReleaseUrl = 'https://github.com/zubcats/clumzy/releases/latest/download/Clumzy-windows-x64.zip'
$ExpectBuild = '090426b'

function Test-Admin {
    $id = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = New-Object Security.Principal.WindowsPrincipal($id)
    return $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

function Test-SafeToDelete([string]$path) {
    if ([string]::IsNullOrWhiteSpace($path)) { return $false }
    $full = [System.IO.Path]::GetFullPath($path)
    if ($full -match '\\GitHub\\') { return $false }
    if (Test-Path -LiteralPath (Join-Path $full '.git')) { return $false }
    return $true
}

if (-not (Test-Admin)) {
    $self = $PSCommandPath
    if (-not $self) {
        $self = Join-Path $env:TEMP 'fresh-clumzy.ps1'
        Set-Content -LiteralPath $self -Value $MyInvocation.MyCommand.ScriptBlock.ToString() -Encoding UTF8
    }
    Start-Process -FilePath 'powershell.exe' -Verb RunAs -ArgumentList @(
        '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', "`"$self`""
    )
    exit
}

$Host.UI.RawUI.WindowTitle = 'Clumzy fresh install'
Write-Host ''
Write-Host 'Clumzy fresh install' -ForegroundColor Cyan
Write-Host 'This closes old Clumzy/Clumsy, deletes stuck copies, and installs a new one on your Desktop.'
Write-Host ''

Write-Host 'Stopping old Clumzy / Clumsy...'
Get-Process -Name 'Clumzy', 'clumsy', 'Clumsy' -ErrorAction SilentlyContinue |
    Stop-Process -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 1

$searchRoots = @(
    (Join-Path $env:USERPROFILE 'Downloads'),
    (Join-Path $env:USERPROFILE 'Desktop'),
    (Join-Path $env:USERPROFILE 'Documents')
)
if ($env:OneDrive) {
    $searchRoots += @(
        (Join-Path $env:OneDrive 'Downloads'),
        (Join-Path $env:OneDrive 'Desktop'),
        (Join-Path $env:OneDrive 'Documents')
    )
}
$searchRoots = $searchRoots | Where-Object { $_ -and (Test-Path -LiteralPath $_) } | Select-Object -Unique

Write-Host 'Deleting stuck zip files...'
foreach ($root in $searchRoots) {
    Get-ChildItem -LiteralPath $root -File -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -match '(?i)clumzy|clumsy' -and $_.Extension -match '(?i)\.zip$' } |
        ForEach-Object {
            if (Test-SafeToDelete $_.DirectoryName) {
                Write-Host ("  zip  " + $_.FullName)
                Remove-Item -LiteralPath $_.FullName -Force -ErrorAction SilentlyContinue
            }
        }
}

Write-Host 'Deleting old extracted copies...'
$removed = @{}
foreach ($root in $searchRoots) {
    Get-ChildItem -LiteralPath $root -Recurse -File -Filter '*.exe' -Depth 4 -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -match '^(?i)(clumzy|clumsy)\.exe$' } |
        ForEach-Object {
            $dir = $_.DirectoryName
            $divert = Join-Path $dir 'WinDivert.dll'
            $divert64 = Join-Path $dir 'WinDivert64.dll'
            if (-not ((Test-Path -LiteralPath $divert) -or (Test-Path -LiteralPath $divert64))) {
                return
            }
            if (-not (Test-SafeToDelete $dir)) { return }
            if ($removed.ContainsKey($dir)) { return }
            $removed[$dir] = $true
            Write-Host ("  folder  " + $dir)
            Remove-Item -LiteralPath $dir -Recurse -Force -ErrorAction SilentlyContinue
        }
}

$dest = Join-Path $env:USERPROFILE 'Desktop\Clumzy'
if (Test-Path -LiteralPath $dest) {
    Write-Host ("Clearing " + $dest)
    Remove-Item -LiteralPath $dest -Recurse -Force -ErrorAction SilentlyContinue
}
New-Item -ItemType Directory -Path $dest -Force | Out-Null

$stamp = Get-Date -Format 'yyyyMMddHHmmss'
$zip = Join-Path $env:TEMP ("Clumzy-fresh-" + $stamp + ".zip")
Write-Host ''
Write-Host 'Downloading latest Clumzy (cache-busting filename)...'
$curl = Join-Path $env:SystemRoot 'System32\curl.exe'
if (Test-Path -LiteralPath $curl) {
    & $curl -L --fail --noproxy "*" -H "Cache-Control: no-cache" -H "Pragma: no-cache" -o $zip $ReleaseUrl
    if ($LASTEXITCODE -ne 0) { throw "Download failed (curl $LASTEXITCODE)." }
} else {
    Invoke-WebRequest -Uri $ReleaseUrl -OutFile $zip -UseBasicParsing -Headers @{
        'Cache-Control' = 'no-cache'
        'Pragma' = 'no-cache'
    }
}

$fs = [System.IO.File]::OpenRead($zip)
try {
    $b0 = $fs.ReadByte()
    $b1 = $fs.ReadByte()
} finally {
    $fs.Close()
}
if ($b0 -ne 0x50 -or $b1 -ne 0x4B) {
    throw 'Download was not a zip (GitHub may still be building). Wait a minute and run this again.'
}

Write-Host 'Unpacking to Desktop\Clumzy ...'
Expand-Archive -LiteralPath $zip -DestinationPath $dest -Force
Remove-Item -LiteralPath $zip -Force -ErrorAction SilentlyContinue

$exe = Get-ChildItem -LiteralPath $dest -Recurse -Filter 'Clumzy.exe' -ErrorAction SilentlyContinue |
    Select-Object -First 1
if (-not $exe) {
    throw 'Clumzy.exe missing after unpack. The zip may still be an old cached build.'
}

Write-Host ''
Write-Host 'Done. New copy:' -ForegroundColor Green
Write-Host ("  " + $exe.FullName)
Write-Host ''
Write-Host ("When it opens, the title must say  Build " + $ExpectBuild) -ForegroundColor Yellow
Write-Host 'If it still says 090326, this machine is still using a cached zip.'
Write-Host ''

Start-Process -FilePath $exe.FullName -WorkingDirectory $exe.DirectoryName -Verb RunAs
Write-Host 'Press Enter to close this window.'
Read-Host | Out-Null
