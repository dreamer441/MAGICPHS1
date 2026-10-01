# MAGICPHS1 — Phase 1 Clean Foundation

This package is a clean, code-only reconstruction of the v0.16 four-state gameplay foundation.
It intentionally contains **no character mesh, animation blueprint, landscape, marketplace asset, InnerRealm, WorldCodex, or old FirstPersonInteraction plugin**.

## Install

1. Close Unreal Engine and Visual Studio.
2. Extract this ZIP directly into the project root, e.g.:
   `D:\UNREALPHS1MAGIC\MAGICPHS1`
3. Open PowerShell in the project root and run:
   `Set-ExecutionPolicy -Scope Process Bypass -Force`
   `.\Install_Phase1.ps1`
4. Regenerate Visual Studio project files if needed.
5. Open the `.uproject` and allow Unreal to rebuild the plugin.
6. Ensure the test level has a floor with collision and ideally a PlayerStart.

## Controls

- Up Arrow / Down Arrow: move between gameplay states.
- State 1 — First Person: WASD, mouse look, Space jump, Shift pulse sprint.
- State 2 — Third Person: WASD, mouse look, Space jump, RMB shoulder aim. LMB is intentionally left free for the upcoming spell execution layer.
- State 3 — Top Down: RMB move-to-cursor, LMB orbit camera, mouse-wheel-up sprint pulse.
- State 4 — Free Roam: WASD + Q/E fly the detached camera, LMB look, RMB tap moves the body, RMB hold 3 seconds teleports the body.

## Architecture rule

`Phase1ViewModeSubsystem` owns transitions.
Each mode is a separate node implementing the same `FPhase1ModeNode` contract.
Modes do not reference one another.
The gameplay plugin does not depend on magic.
Future magic plugins may depend on this plugin and query `GetCurrentMode()`, `GetCurrentCapabilities()`, and `GetGameplayCastRay()`.

This dependency direction is intentional and cleaner than the old v0.16 layout.
