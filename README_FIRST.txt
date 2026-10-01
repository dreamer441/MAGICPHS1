V0.16.8 EXACT GAMEPLAY MIGRATOR — V2
======================================

This fixes the path mistake in V1.

The recovery scan proved that the four-state source files DO still exist in
D:\TESTUNREALPROJECT. Their real layout is:

  Public\Core\PlayerGameplayModeNode.h
  Public\Core\PlayerGameplayModeTypes.h

  Private\Modes\FirstPerson\...
  Private\Modes\ThirdPerson\...
  Private\Modes\TopDown\...
  Private\Modes\FreeRoam\...

V1 incorrectly looked for Core\ and Modes\ directly under the module root.

This installer:
- uses Git tag v0.16.8-stable / commit 241edfba...
- copies those missing files BYTE-FOR-BYTE from the old project
- does not rebuild or redesign any gameplay function
- removes the experimental Phase1Gameplay plugin from the fresh project
- brings only the internal gameplay plugin dependency closure
- brings the default Unreal character/template assets
- brings the required AMADEUS Earth material
- excludes landscapes, old maps, external actors, imported characters,
  environment packs, and marketplace/external plugins

RUN
---
Close Unreal, extract this ZIP, then:

  Set-ExecutionPolicy -Scope Process Bypass -Force
  .\INSTALL_V0168_EXACT_GAMEPLAY_V2.ps1

Default paths:
  Old: D:\TESTUNREALPROJECT
  New: D:\UNREALPHS1MAGIC\MAGICPHS1

The script creates a backup before modifying the new project.
