param(
    [string]$ProjectRoot = (Get-Location).Path
)

$ErrorActionPreference = "Stop"

Write-Host "=== MAGICPHS1 / PHASE 1 CLEAN FOUNDATION ===" -ForegroundColor Cyan
Write-Host "Project root: $ProjectRoot"

$ProjectFile = Get-ChildItem -Path $ProjectRoot -Filter *.uproject -File | Select-Object -First 1
if (-not $ProjectFile) {
    throw "No .uproject file found in $ProjectRoot"
}

$PluginFile = Join-Path $ProjectRoot "Plugins\Phase1Gameplay\Phase1Gameplay.uplugin"
if (-not (Test-Path $PluginFile)) {
    throw "Phase1Gameplay plugin not found. Extract this ZIP into the PROJECT ROOT first."
}

$Stamp = Get-Date -Format "yyyyMMdd_HHmmss"
$BackupRoot = Join-Path $ProjectRoot ".phase1_backups\$Stamp"
New-Item -ItemType Directory -Path $BackupRoot -Force | Out-Null

Copy-Item $ProjectFile.FullName (Join-Path $BackupRoot $ProjectFile.Name) -Force

$ConfigDir = Join-Path $ProjectRoot "Config"
$DefaultEngine = Join-Path $ConfigDir "DefaultEngine.ini"
if (Test-Path $DefaultEngine) {
    Copy-Item $DefaultEngine (Join-Path $BackupRoot "DefaultEngine.ini") -Force
}

Write-Host "Backup created: $BackupRoot" -ForegroundColor DarkGray

# Enable project plugin in the uproject JSON.
$ProjectJson = Get-Content $ProjectFile.FullName -Raw | ConvertFrom-Json
$Plugins = @()
if ($null -ne $ProjectJson.Plugins) {
    $Plugins = @($ProjectJson.Plugins)
}

$Existing = $Plugins | Where-Object { $_.Name -eq "Phase1Gameplay" }
if ($Existing) {
    $Existing.Enabled = $true
}
else {
    $Plugins += [PSCustomObject]@{
        Name = "Phase1Gameplay"
        Enabled = $true
    }
}

$ProjectJson | Add-Member -NotePropertyName Plugins -NotePropertyValue $Plugins -Force
$ProjectJson | ConvertTo-Json -Depth 100 | Set-Content $ProjectFile.FullName -Encoding UTF8

# Add a clearly marked config block. Re-running the installer replaces only this block.
New-Item -ItemType Directory -Path $ConfigDir -Force | Out-Null
if (-not (Test-Path $DefaultEngine)) {
    New-Item -ItemType File -Path $DefaultEngine -Force | Out-Null
}

$Ini = Get-Content $DefaultEngine -Raw
$BeginMarker = "; BEGIN MAGICPHS1 PHASE1 CLEAN FOUNDATION"
$EndMarker = "; END MAGICPHS1 PHASE1 CLEAN FOUNDATION"
$Pattern = [regex]::Escape($BeginMarker) + ".*?" + [regex]::Escape($EndMarker) + "\s*"
$Ini = [regex]::Replace($Ini, $Pattern, "", [System.Text.RegularExpressions.RegexOptions]::Singleline)

$Block = @"
$BeginMarker
[/Script/EngineSettings.GameMapsSettings]
GlobalDefaultGameMode=/Script/Phase1Gameplay.Phase1GameMode
$EndMarker
"@

$Ini = $Ini.TrimEnd() + "`r`n`r`n" + $Block.Trim() + "`r`n"
Set-Content $DefaultEngine $Ini -Encoding UTF8

Write-Host "Phase1Gameplay enabled in $($ProjectFile.Name)" -ForegroundColor Green
Write-Host "GlobalDefaultGameMode set to Phase1GameMode" -ForegroundColor Green
Write-Host ""
Write-Host "NEXT:" -ForegroundColor Yellow
Write-Host "1. Right-click the .uproject -> Generate Visual Studio project files (if that option exists)."
Write-Host "2. Open the project. Let Unreal rebuild the Phase1Gameplay module if prompted."
Write-Host "3. In the level, make sure there is a floor with collision and preferably a PlayerStart."
Write-Host "4. Play. Default mode is State 2. Up/Down arrows cycle the four states."
Write-Host ""
Write-Host "Do NOT import old Characters, Landscape, FirstPersonInteraction, InnerRealm or marketplace plugins yet." -ForegroundColor Cyan
