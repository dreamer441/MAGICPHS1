param(
    [string]$OldProject = "D:\TESTUNREALPROJECT",
    [string]$NewProject = "D:\UNREALPHS1MAGIC\MAGICPHS1"
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$StableCommit = "241edfba8a3fce64b2e73c01fa1a404206c36334"
$StableTag = "v0.16.8-stable"

$GameplayPlugins = @(
    "MaterialCore",
    "PhysicalBody",
    "ImpactSystem",
    "EarthFoundation",
    "EarthMagic",
    "LiveSpellCasting",
    "SpellCreation",
    "SpellExecution",
    "SpellPreview",
    "SpellCastingBindings",
    "SpellMotion",
    "SpellPattern",
    "SpellLoadout",
    "PlayerViewModes",
    "WorldCodex",
    "SpellGraph",
    "InnerRealm",
    "FirstPersonInteraction"
)

function Write-Step([string]$Text) {
    Write-Host "`n=== $Text ===" -ForegroundColor Cyan
}

function Copy-TreeExact([string]$Source, [string]$Destination) {
    if (!(Test-Path $Source)) {
        throw "Required source path is missing: $Source"
    }
    if (Test-Path $Destination) {
        Remove-Item $Destination -Recurse -Force
    }
    $parent = Split-Path $Destination -Parent
    New-Item -ItemType Directory -Force -Path $parent | Out-Null
    Copy-Item $Source $Destination -Recurse -Force
}

function Copy-FileExact([string]$Source, [string]$Destination) {
    if (!(Test-Path $Source)) {
        throw "Required source file is missing: $Source"
    }
    $parent = Split-Path $Destination -Parent
    New-Item -ItemType Directory -Force -Path $parent | Out-Null
    Copy-Item $Source $Destination -Force
}

function Set-GameModeConfig([string]$IniPath) {
    $line = "GlobalDefaultGameMode=/Game/ThirdPerson/Blueprints/BP_ThirdPersonGameMode.BP_ThirdPersonGameMode_C"
    $section = "[/Script/EngineSettings.GameMapsSettings]"

    if (!(Test-Path $IniPath)) {
        New-Item -ItemType Directory -Force -Path (Split-Path $IniPath -Parent) | Out-Null
        Set-Content -Path $IniPath -Value "$section`r`n$line`r`n" -Encoding UTF8
        return
    }

    $raw = Get-Content $IniPath -Raw

    if ($raw -match [regex]::Escape($section)) {
        # Replace an existing GlobalDefaultGameMode anywhere in the file.
        if ($raw -match '(?m)^GlobalDefaultGameMode=.*$') {
            $raw = [regex]::Replace(
                $raw,
                '(?m)^GlobalDefaultGameMode=.*$',
                $line
            )
        }
        else {
            # Insert immediately after the GameMapsSettings section header.
            $raw = $raw -replace (
                [regex]::Escape($section),
                "$section`r`n$line"
            )
        }
    }
    else {
        $raw = $raw.TrimEnd() + "`r`n`r`n$section`r`n$line`r`n"
    }

    Set-Content -Path $IniPath -Value $raw -Encoding UTF8
}

Write-Step "Preflight"

if (Get-Process UnrealEditor -ErrorAction SilentlyContinue) {
    throw "Unreal Editor is running. Close Unreal before installing V0.16.8."
}

if (!(Test-Path $OldProject)) {
    throw "Old project not found: $OldProject"
}

if (!(Test-Path (Join-Path $OldProject ".git"))) {
    throw "The old project is not a Git working copy: $OldProject"
}

$NewUProject = Join-Path $NewProject "MAGICPHS1.uproject"
if (!(Test-Path $NewUProject)) {
    throw "Fresh MAGICPHS1 project not found: $NewUProject"
}

git -C $OldProject cat-file -e "$StableCommit^{commit}" 2>$null
if ($LASTEXITCODE -ne 0) {
    throw @"
The local repository does not contain commit $StableCommit ($StableTag).

Run this first:
    git -C "$OldProject" fetch --tags origin

Then run this installer again.
"@
}

Write-Host "Source checkpoint: $StableTag / $StableCommit" -ForegroundColor Green
Write-Host "Old project:       $OldProject"
Write-Host "New project:       $NewProject"

Write-Step "Validate locally-preserved V0.16 mode-node source"

# These files were used by the V0.16.8 stable coordinator but were not committed
# into the Git tree. We copy them byte-for-byte from the old local project.
$ModeRoot = Join-Path $OldProject "Plugins\PlayerViewModes\Source\PlayerViewModes"

$RequiredLocalModeFiles = @(
    "Core\PlayerGameplayModeNode.h",
    "Core\PlayerGameplayModeTypes.h",
    "Modes\FirstPerson\FirstPersonModeNode.h",
    "Modes\FirstPerson\FirstPersonModeNode.cpp",
    "Modes\ThirdPerson\ThirdPersonModeNode.h",
    "Modes\ThirdPerson\ThirdPersonModeNode.cpp",
    "Modes\TopDown\TopDownModeNode.h",
    "Modes\TopDown\TopDownModeNode.cpp",
    "Modes\FreeRoam\FreeRoamModeNode.h",
    "Modes\FreeRoam\FreeRoamModeNode.cpp"
)

$MissingLocal = @()
foreach ($relative in $RequiredLocalModeFiles) {
    $full = Join-Path $ModeRoot $relative
    if (!(Test-Path $full)) {
        $MissingLocal += $full
    }
}

if ($MissingLocal.Count -gt 0) {
    Write-Host "`nThe exact four-state source is not fully available locally." -ForegroundColor Red
    Write-Host "Nothing has been changed in MAGICPHS1." -ForegroundColor Yellow
    Write-Host "`nMissing:"
    $MissingLocal | ForEach-Object { Write-Host "  $_" }
    throw "Cannot make an exact V0.16 migration without these locally-preserved files."
}

Write-Step "Extract exact tracked V0.16.8 checkpoint"

$TempRoot = Join-Path $env:TEMP ("MAGICPHS1_V0168_" + [guid]::NewGuid().ToString("N"))
$Archive = Join-Path $TempRoot "v0168.zip"
$Extracted = Join-Path $TempRoot "checkpoint"
$Stage = Join-Path $TempRoot "stage"

New-Item -ItemType Directory -Force -Path $TempRoot, $Extracted, $Stage | Out-Null

git -C $OldProject archive --format=zip --output="$Archive" $StableCommit
if ($LASTEXITCODE -ne 0 -or !(Test-Path $Archive)) {
    throw "git archive failed for $StableCommit"
}

Expand-Archive -Path $Archive -DestinationPath $Extracted -Force

Write-Step "Stage only gameplay plugins"

foreach ($Plugin in $GameplayPlugins) {
    $src = Join-Path $Extracted ("Plugins\" + $Plugin)
    $dst = Join-Path $Stage ("Plugins\" + $Plugin)
    Copy-TreeExact $src $dst
}

# Overlay ONLY the V0.16 mode-node files that Git failed to track.
# No implementation is rewritten or regenerated.
$StageModeRoot = Join-Path $Stage "Plugins\PlayerViewModes\Source\PlayerViewModes"
Copy-TreeExact (Join-Path $ModeRoot "Core")  (Join-Path $StageModeRoot "Core")
Copy-TreeExact (Join-Path $ModeRoot "Modes") (Join-Path $StageModeRoot "Modes")

Write-Step "Stage default Unreal character + gameplay content"

# Default Unreal mannequin / animations.
Copy-TreeExact `
    (Join-Path $Extracted "Content\Characters") `
    (Join-Path $Stage "Content\Characters")

# Default template input assets.
Copy-TreeExact `
    (Join-Path $Extracted "Content\Input") `
    (Join-Path $Stage "Content\Input")

# Default ThirdPerson template gameplay assets. We intentionally do not bring its map.
Copy-TreeExact `
    (Join-Path $Extracted "Content\ThirdPerson") `
    (Join-Path $Stage "Content\ThirdPerson")

$OldTemplateMap = Join-Path $Stage "Content\ThirdPerson\Lvl_ThirdPerson.umap"
if (Test-Path $OldTemplateMap) {
    Remove-Item $OldTemplateMap -Force
}

# Only the Earth material actually hard-referenced by EarthMagic.
Copy-FileExact `
    (Join-Path $Extracted "Content\AMADEUS\Materials\M_AMADEUS_Earth_v13.uasset") `
    (Join-Path $Stage "Content\AMADEUS\Materials\M_AMADEUS_Earth_v13.uasset")

# Exact V0.16 input configuration.
Copy-FileExact `
    (Join-Path $Extracted "Config\DefaultInput.ini") `
    (Join-Path $Stage "Config\DefaultInput.ini")

Write-Step "Create safety backup"

$Stamp = Get-Date -Format "yyyyMMdd_HHmmss"
$BackupRoot = Join-Path $NewProject ("MigrationBackups\pre_v0168_" + $Stamp)
New-Item -ItemType Directory -Force -Path $BackupRoot | Out-Null

foreach ($relative in @("Plugins", "Content\Characters", "Content\Input", "Content\ThirdPerson", "Content\AMADEUS", "Config\DefaultInput.ini", "Config\DefaultEngine.ini", "MAGICPHS1.uproject")) {
    $src = Join-Path $NewProject $relative
    if (Test-Path $src) {
        $dst = Join-Path $BackupRoot $relative
        $parent = Split-Path $dst -Parent
        New-Item -ItemType Directory -Force -Path $parent | Out-Null
        Copy-Item $src $dst -Recurse -Force
    }
}

Write-Step "Install V0.16.8 gameplay into MAGICPHS1"

# Remove only the selected custom gameplay plugin folders.
foreach ($Plugin in $GameplayPlugins) {
    $targetPlugin = Join-Path $NewProject ("Plugins\" + $Plugin)
    if (Test-Path $targetPlugin) {
        Remove-Item $targetPlugin -Recurse -Force
    }
}

New-Item -ItemType Directory -Force -Path (Join-Path $NewProject "Plugins") | Out-Null
Copy-Item (Join-Path $Stage "Plugins\*") (Join-Path $NewProject "Plugins") -Recurse -Force

# Replace the selected gameplay content only.
foreach ($relative in @("Characters", "Input", "ThirdPerson", "AMADEUS")) {
    $target = Join-Path $NewProject ("Content\" + $relative)
    if (Test-Path $target) {
        Remove-Item $target -Recurse -Force
    }
}
New-Item -ItemType Directory -Force -Path (Join-Path $NewProject "Content") | Out-Null
Copy-Item (Join-Path $Stage "Content\*") (Join-Path $NewProject "Content") -Recurse -Force

Copy-FileExact `
    (Join-Path $Stage "Config\DefaultInput.ini") `
    (Join-Path $NewProject "Config\DefaultInput.ini")

Write-Step "Enable only the internal V0.16 gameplay plugins"

$ProjectJson = Get-Content $NewUProject -Raw | ConvertFrom-Json

$ExistingPlugins = @()
if ($null -ne $ProjectJson.Plugins) {
    $ExistingPlugins = @($ProjectJson.Plugins)
}

# Preserve unrelated built-in project settings, but replace entries for our internal modules.
$FilteredPlugins = @(
    $ExistingPlugins | Where-Object {
        $GameplayPlugins -notcontains $_.Name
    }
)

$NewPluginEntries = @()
foreach ($Plugin in $GameplayPlugins) {
    $NewPluginEntries += [pscustomobject]@{
        Name = $Plugin
        Enabled = $true
    }
}

$ProjectJson.Plugins = @($FilteredPlugins + $NewPluginEntries)
$ProjectJson | ConvertTo-Json -Depth 20 | Set-Content $NewUProject -Encoding UTF8

Write-Step "Point the fresh project at the original default Unreal character GameMode"

$EngineIni = Join-Path $NewProject "Config\DefaultEngine.ini"
Set-GameModeConfig $EngineIni

Write-Step "Write exact-source manifest"

$ManifestPath = Join-Path $NewProject "V0168_GAMEPLAY_MIGRATION_MANIFEST.txt"

$Manifest = @()
$Manifest += "V0.16.8 exact gameplay migration"
$Manifest += "Stable tag:    $StableTag"
$Manifest += "Stable commit: $StableCommit"
$Manifest += "Installed:     $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')"
$Manifest += ""
$Manifest += "INCLUDED CUSTOM GAMEPLAY PLUGINS:"
$Manifest += ($GameplayPlugins | ForEach-Object { "  $_" })
$Manifest += ""
$Manifest += "INCLUDED CONTENT:"
$Manifest += "  Content\Characters      (default Unreal mannequin/animations)"
$Manifest += "  Content\Input           (default template input)"
$Manifest += "  Content\ThirdPerson     (default gameplay BPs; template map excluded)"
$Manifest += "  Content\AMADEUS\Materials\M_AMADEUS_Earth_v13.uasset"
$Manifest += ""
$Manifest += "EXCLUDED:"
$Manifest += "  Content\LevelPrototyping"
$Manifest += "  Content\__ExternalActors__"
$Manifest += "  Content\__ExternalObjects__"
$Manifest += "  old maps / landscapes / environment packs"
$Manifest += "  marketplace character/environment assets"
$Manifest += "  unrelated external plugins"
$Manifest += ""
$Manifest += "LOCAL V0.16 MODE FILE HASHES:"
foreach ($relative in $RequiredLocalModeFiles) {
    $full = Join-Path $ModeRoot $relative
    $hash = (Get-FileHash $full -Algorithm SHA256).Hash
    $Manifest += "  $hash  PlayerViewModes\$relative"
}

$Manifest | Set-Content $ManifestPath -Encoding UTF8

Write-Step "Clear generated build state"

foreach ($generated in @("Binaries", "Intermediate")) {
    $p = Join-Path $NewProject $generated
    if (Test-Path $p) {
        Remove-Item $p -Recurse -Force
    }
}

# Plugin binaries/intermediate are generated and should not migrate.
Get-ChildItem (Join-Path $NewProject "Plugins") -Directory -ErrorAction SilentlyContinue | ForEach-Object {
    foreach ($generated in @("Binaries", "Intermediate")) {
        $p = Join-Path $_.FullName $generated
        if (Test-Path $p) {
            Remove-Item $p -Recurse -Force
        }
    }
}

Remove-Item $TempRoot -Recurse -Force -ErrorAction SilentlyContinue

Write-Host ""
Write-Host "V0.16.8 GAMEPLAY MIGRATION COMPLETE" -ForegroundColor Green
Write-Host ""
Write-Host "No gameplay implementation was rewritten." -ForegroundColor Green
Write-Host "Tracked source came directly from commit $StableCommit." -ForegroundColor Green
Write-Host "The missing four-state node source was copied byte-for-byte from the old local project." -ForegroundColor Green
Write-Host ""
Write-Host "Backup:   $BackupRoot"
Write-Host "Manifest: $ManifestPath"
Write-Host ""
Write-Host "Next:"
Write-Host "  1. Right-click MAGICPHS1.uproject -> Generate Visual Studio project files"
Write-Host "  2. Build Development Editor / Win64"
Write-Host "  3. Open MAGICPHS1.uproject"
Write-Host ""
