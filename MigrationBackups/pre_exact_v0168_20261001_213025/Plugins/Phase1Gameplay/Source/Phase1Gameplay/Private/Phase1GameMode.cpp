#include "Phase1GameMode.h"
#include "Phase1Character.h"

APhase1GameMode::APhase1GameMode()
{
    DefaultPawnClass = APhase1Character::StaticClass();
}
