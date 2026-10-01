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

function Step([string]$Text) {
    Write-Host "`n=== $Text ===" -ForegroundColor Cyan
}

function Ensure-Parent([string]$Path) {
    $Parent = Split-Path $Path -Parent
    if ($Parent) {
        New-Item -ItemType Directory -Force -Path $Parent | Out-Null
    }
}

function Copy-TreeExact([string]$Source, [string]$Destination) {
    if (!(Test-Path $Source)) {
        throw "Required source path missing: $Source"
    }
    if (Test-Path $Destination) {
        Remove-Item $Destination -Recurse -Force
    }
    Ensure-Parent $Destination
    Copy-Item $Source $Destination -Recurse -Force
}

function Copy-FileExact([string]$Source, [string]$Destination) {
    if (!(Test-Path $Source)) {
        throw "Required source file missing: $Source"
    }
    Ensure-Parent $Destination
    Copy-Item $Source $Destination -Force
}

function Set-DefaultGameMode([string]$IniPath) {
    $Section = "[/Script/EngineSettings.GameMapsSettings]"
    $Line = "GlobalDefaultGameMode=/Game/ThirdPerson/Blueprints/BP_ThirdPersonGameMode.BP_ThirdPersonGameMode_C"

    if (!(Test-Path $IniPath)) {
        Ensure-Parent $IniPath
        Set-Content -Path $IniPath -Value "$Section`r`n$Line`r`n" -Encoding UTF8
        return
    }

    $Raw = Get-Content $IniPath -Raw

    if ($Raw -match '(?m)^GlobalDefaultGameMode=.*$') {
        $Raw = [regex]::Replace($Raw, '(?m)^GlobalDefaultGameMode=.*$', $Line)
    }
    elseif ($Raw.Contains($Section)) {
        $Raw = $Raw.Replace($Section, "$Section`r`n$Line")
    }
    else {
        $Raw = $Raw.TrimEnd() + "`r`n`r`n$Section`r`n$Line`r`n"
    }

    Set-Content -Path $IniPath -Value $Raw -Encoding UTF8
}

Step "PRE-FLIGHT"

if (Get-Process UnrealEditor -ErrorAction SilentlyContinue) {
    throw "Close Unreal Editor before running this migration."
}

if (!(Test-Path $OldProject)) {
    throw "Old project not found: $OldProject"
}
if (!(Test-Path (Join-Path $OldProject ".git"))) {
    throw "Old project is not a Git working copy: $OldProject"
}

$NewUProject = Join-Path $NewProject "MAGICPHS1.uproject"
if (!(Test-Path $NewUProject)) {
    throw "Fresh project not found: $NewUProject"
}

git -C $OldProject cat-file -e "$StableCommit^{commit}" 2>$null
if ($LASTEXITCODE -ne 0) {
    throw "Stable commit $StableCommit is not available in the old repository. Run: git -C `"$OldProject`" fetch --tags origin"
}

Write-Host "Stable checkpoint: $StableTag" -ForegroundColor Green
Write-Host "Commit:            $StableCommit"
Write-Host "Old project:       $OldProject"
Write-Host "New project:       $NewProject"

Step "VALIDATE THE EXACT LOCALLY-PRESERVED FOUR-STATE SOURCE"

# IMPORTANT:
# These are the REAL paths discovered by the recovery scan.
# Git missed these files, but they still exist in the old project.
$PVM = Join-Path $OldProject "Plugins\PlayerViewModes\Source\PlayerViewModes"

$LocalExactFiles = @(
    "Public\Core\PlayerGameplayModeNode.h",
    "Public\Core\PlayerGameplayModeTypes.h",

    "Private\Modes\FirstPerson\FirstPersonModeNode.h",
    "Private\Modes\FirstPerson\FirstPersonModeNode.cpp",

    "Private\Modes\ThirdPerson\ThirdPersonModeNode.h",
    "Private\Modes\ThirdPerson\ThirdPersonModeNode.cpp",

    "Private\Modes\TopDown\TopDownModeNode.h",
    "Private\Modes\TopDown\TopDownModeNode.cpp",

    "Private\Modes\FreeRoam\FreeRoamModeNode.h",
    "Private\Modes\FreeRoam\FreeRoamModeNode.cpp"
)

$Missing = @()
foreach ($Relative in $LocalExactFiles) {
    $Full = Join-Path $PVM $Relative
    if (!(Test-Path $Full)) {
        $Missing += $Full
    }
}

if ($Missing.Count -gt 0) {
    Write-Host "`nMissing locally-preserved files:" -ForegroundColor Red
    $Missing | ForEach-Object { Write-Host "  $_" }
    throw "Exact V0.16.8 migration stopped before modifying MAGICPHS1."
}

Write-Host "All 10 missing V0.16 mode-node files found." -ForegroundColor Green

Step "CREATE TEMPORARY EXACT STAGING AREA"

$TempRoot = Join-Path $env:TEMP ("MAGICPHS1_V0168_" + [guid]::NewGuid().ToString("N"))
$Archive = Join-Path $TempRoot "stable.zip"
$Extracted = Join-Path $TempRoot "stable"
$Stage = Join-Path $TempRoot "stage"

New-Item -ItemType Directory -Force -Path $TempRoot, $Extracted, $Stage | Out-Null

git -C $OldProject archive --format=zip --output="$Archive" $StableCommit
if ($LASTEXITCODE -ne 0) {
    throw "git archive failed."
}

Expand-Archive -Path $Archive -DestinationPath $Extracted -Force

Step "STAGE THE ORIGINAL V0.16.8 GAMEPLAY PLUGINS"

foreach ($Plugin in $GameplayPlugins) {
    Copy-TreeExact `
        (Join-Path $Extracted ("Plugins\" + $Plugin)) `
        (Join-Path $Stage ("Plugins\" + $Plugin))
}

# Overlay exactly the source files that were present locally but accidentally
# absent from Git. Nothing is rewritten or generated.
$StagePVM = Join-Path $Stage "Plugins\PlayerViewModes\Source\PlayerViewModes"

Copy-TreeExact `
    (Join-Path $PVM "Public\Core") `
    (Join-Path $StagePVM "Public\Core")

Copy-TreeExact `
    (Join-Path $PVM "Private\Modes") `
    (Join-Path $StagePVM "Private\Modes")

Step "STAGE ONLY REQUIRED GAMEPLAY CONTENT"

# Unreal template mannequin + animations.
Copy-TreeExact `
    (Join-Path $Extracted "Content\Characters") `
    (Join-Path $Stage "Content\Characters")

# Unreal template input assets.
Copy-TreeExact `
    (Join-Path $Extracted "Content\Input") `
    (Join-Path $Stage "Content\Input")

# Only the gameplay Blueprints from ThirdPerson.
# Deliberately NO old map and NO World Partition external actors.
Copy-TreeExact `
    (Join-Path $Extracted "Content\ThirdPerson\Blueprints") `
    (Join-Path $Stage "Content\ThirdPerson\Blueprints")

# EarthMagic hard-references this exact material.
Copy-FileExact `
    (Join-Path $Extracted "Content\AMADEUS\Materials\M_AMADEUS_Earth_v13.uasset") `
    (Join-Path $Stage "Content\AMADEUS\Materials\M_AMADEUS_Earth_v13.uasset")

# Exact input config from V0.16.8.
Copy-FileExact `
    (Join-Path $Extracted "Config\DefaultInput.ini") `
    (Join-Path $Stage "Config\DefaultInput.ini")

Step "BACK UP THE FRESH PROJECT BEFORE INSTALLATION"

$Stamp = Get-Date -Format "yyyyMMdd_HHmmss"
$BackupRoot = Join-Path $NewProject ("MigrationBackups\pre_exact_v0168_" + $Stamp)
New-Item -ItemType Directory -Force -Path $BackupRoot | Out-Null

$BackupTargets = @(
    "Plugins",
    "Content\Characters",
    "Content\Input",
    "Content\ThirdPerson",
    "Content\AMADEUS",
    "Config\DefaultInput.ini",
    "Config\DefaultEngine.ini",
    "MAGICPHS1.uproject"
)

foreach ($Relative in $BackupTargets) {
    $Src = Join-Path $NewProject $Relative
    if (Test-Path $Src) {
        $Dst = Join-Path $BackupRoot $Relative
        Ensure-Parent $Dst
        Copy-Item $Src $Dst -Recurse -Force
    }
}

Write-Host "Backup created: $BackupRoot" -ForegroundColor Green

Step "REMOVE THE EXPERIMENTAL PHASE1GAMEPLAY REBUILD"

$ExperimentalPlugin = Join-Path $NewProject "Plugins\Phase1Gameplay"
if (Test-Path $ExperimentalPlugin) {
    Remove-Item $ExperimentalPlugin -Recurse -Force
    Write-Host "Removed Plugins\Phase1Gameplay" -ForegroundColor Yellow
}

Step "INSTALL EXACT V0.16.8 GAMEPLAY"

foreach ($Plugin in $GameplayPlugins) {
    $Target = Join-Path $NewProject ("Plugins\" + $Plugin)
    if (Test-Path $Target) {
        Remove-Item $Target -Recurse -Force
    }
}

New-Item -ItemType Directory -Force -Path (Join-Path $NewProject "Plugins") | Out-Null
Copy-Item (Join-Path $Stage "Plugins\*") (Join-Path $NewProject "Plugins") -Recurse -Force

foreach ($Relative in @("Characters", "Input", "ThirdPerson", "AMADEUS")) {
    $Target = Join-Path $NewProject ("Content\" + $Relative)
    if (Test-Path $Target) {
        Remove-Item $Target -Recurse -Force
    }
}

New-Item -ItemType Directory -Force -Path (Join-Path $NewProject "Content") | Out-Null
Copy-Item (Join-Path $Stage "Content\*") (Join-Path $NewProject "Content") -Recurse -Force

Copy-FileExact `
    (Join-Path $Stage "Config\DefaultInput.ini") `
    (Join-Path $NewProject "Config\DefaultInput.ini")

Step "ENABLE THE ORIGINAL INTERNAL GAMEPLAY PLUGINS"

$Project = Get-Content $NewUProject -Raw | ConvertFrom-Json

$ExistingPlugins = @()
if ($null -ne $Project.Plugins) {
    $ExistingPlugins = @($Project.Plugins)
}

# Remove stale entries for our migrated modules and the experimental rebuild.
$NamesToReplace = @($GameplayPlugins + "Phase1Gameplay")

$Preserved = @(
    $ExistingPlugins | Where-Object {
        $NamesToReplace -notcontains $_.Name
    }
)

$Added = foreach ($Plugin in $GameplayPlugins) {
    [pscustomobject]@{
        Name = $Plugin
        Enabled = $true
    }
}

$Project.Plugins = @($Preserved + $Added)
$Project | ConvertTo-Json -Depth 30 | Set-Content $NewUProject -Encoding UTF8

Step "USE THE DEFAULT UNREAL THIRD-PERSON CHARACTER"

Set-DefaultGameMode (Join-Path $NewProject "Config\DefaultEngine.ini")

Step "WRITE SOURCE VERIFICATION MANIFEST"

$ManifestPath = Join-Path $NewProject "V0168_EXACT_SOURCE_MANIFEST.txt"

$Manifest = @()
$Manifest += "EXACT V0.16.8 GAMEPLAY RESTORE"
$Manifest += "Tag:    $StableTag"
$Manifest += "Commit: $StableCommit"
$Manifest += ""
$Manifest += "Tracked plugin source: Git commit above"
$Manifest += "Untracked four-state source: copied byte-for-byte from old working project"
$Manifest += ""
$Manifest += "LOCAL FOUR-STATE SOURCE SHA256:"
foreach ($Relative in $LocalExactFiles) {
    $Full = Join-Path $PVM $Relative
    $Hash = (Get-FileHash $Full -Algorithm SHA256).Hash
    $Manifest += "$Hash  PlayerViewModes\$Relative"
}
$Manifest += ""
$Manifest += "EXCLUDED:"
$Manifest += "LevelPrototyping"
$Manifest += "__ExternalActors__"
$Manifest += "__ExternalObjects__"
$Manifest += "old maps"
$Manifest += "landscapes"
$Manifest += "environment packs"
$Manifest += "Echo/imported characters"
$Manifest += "marketplace/external plugins"
$Manifest += "experimental Phase1Gameplay rewrite"

$Manifest | Set-Content $ManifestPath -Encoding UTF8

Step "REMOVE GENERATED BUILD OUTPUT"

foreach ($Generated in @("Binaries", "Intermediate")) {
    $Path = Join-Path $NewProject $Generated
    if (Test-Path $Path) {
        Remove-Item $Path -Recurse -Force
    }
}

Get-ChildItem (Join-Path $NewProject "Plugins") -Directory -ErrorAction SilentlyContinue |
ForEach-Object {
    foreach ($Generated in @("Binaries", "Intermediate")) {
        $Path = Join-Path $_.FullName $Generated
        if (Test-Path $Path) {
            Remove-Item $Path -Recurse -Force
        }
    }
}

Remove-Item $TempRoot -Recurse -Force -ErrorAction SilentlyContinue

Step "DONE"

Write-Host "Exact V0.16.8 gameplay files installed." -ForegroundColor Green
Write-Host "NO gameplay implementation was rewritten." -ForegroundColor Green
Write-Host ""
Write-Host "Included:"
Write-Host "  - original four-state system"
Write-Host "  - original V0.16 magic/gameplay plugin dependency closure"
Write-Host "  - default Unreal mannequin/animations"
Write-Host "  - default ThirdPerson gameplay Blueprints"
Write-Host "  - required Earth material"
Write-Host ""
Write-Host "Excluded:"
Write-Host "  - landscapes / environment"
Write-Host "  - old maps / external actors"
Write-Host "  - imported characters"
Write-Host "  - external marketplace plugins"
Write-Host "  - Phase1Gameplay rewrite"
Write-Host ""
Write-Host "Backup:   $BackupRoot"
Write-Host "Manifest: $ManifestPath"
Write-Host ""
Write-Host "NEXT:"
Write-Host "1. Right-click MAGICPHS1.uproject -> Generate Visual Studio project files"
Write-Host "2. Build MAGICPHS1Editor / Development Editor / Win64"
Write-Host "3. Open MAGICPHS1.uproject"
