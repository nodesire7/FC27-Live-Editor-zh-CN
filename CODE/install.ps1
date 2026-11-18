param([Parameter(Mandatory = $true)][string]$GameDirectory)
$ErrorActionPreference = 'Stop'
$gamePath = (Resolve-Path -LiteralPath $GameDirectory).Path
$packageRoot = Split-Path -Parent $PSScriptRoot
$targetDll = Join-Path $gamePath 'FCLiveEditor.DLL'
$summary = Get-Content -LiteralPath (Join-Path $packageRoot 'audit/validation_summary.json') -Raw | ConvertFrom-Json
if ((Get-FileHash -LiteralPath $targetDll -Algorithm SHA256).Hash.ToLowerInvariant() -ne $summary.target_sha256) {
    throw 'This FCLiveEditor.DLL version is not supported. No files were copied.'
}
$checksums = Get-Content -LiteralPath (Join-Path $packageRoot 'checksums.json') -Raw | ConvertFrom-Json
$files = @('FC27LEzh.dll', 'FC27LEzh_font.b85', 'FC27LEzh_ranges.bin', 'loc/eng_us/localize.json')
foreach ($name in $files) {
    $sourceFile = Join-Path $packageRoot $name
    if ((Get-FileHash -LiteralPath $sourceFile -Algorithm SHA256).Hash.ToLowerInvariant() -ne $checksums.$name) {
        throw "Payload checksum mismatch: $name"
    }
}
# Check that the existing helper can be replaced before copying any payload.
$helperPath = Join-Path $gamePath 'FC27LEzh.dll'
if (Test-Path -LiteralPath $helperPath) {
    try {
        $stream = [IO.File]::Open($helperPath, [IO.FileMode]::Open, [IO.FileAccess]::ReadWrite, [IO.FileShare]::None)
        $stream.Dispose()
    } catch {
        throw 'Close the game before installing. No files were copied.'
    }
}
$backupPath = Join-Path $gamePath ('FC27LEzh_backups/' + (Get-Date -Format 'yyyyMMdd_HHmmss_fff'))
foreach ($name in $files) {
    $destination = Join-Path $gamePath $name
    if (Test-Path -LiteralPath $destination) {
        $saved = Join-Path $backupPath $name
        New-Item -ItemType Directory -Path (Split-Path -Parent $saved) -Force | Out-Null
        Copy-Item -LiteralPath $destination -Destination $saved
    }
}
foreach ($name in $files) {
    $destination = Join-Path $gamePath $name
    New-Item -ItemType Directory -Path (Split-Path -Parent $destination) -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $packageRoot $name) -Destination $destination -Force
    if ((Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash.ToLowerInvariant() -ne $checksums.$name) {
        throw "Copy verification failed: $name. Backup: $backupPath"
    }
}
Write-Output "Installed. Backup: $backupPath"
Write-Output 'Load FC27LEzh.dll using your existing loader configuration, then restart the game.'
