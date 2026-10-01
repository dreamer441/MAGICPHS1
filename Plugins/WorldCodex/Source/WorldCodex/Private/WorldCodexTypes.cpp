#include "WorldCodexTypes.h"

FText CodexCategoryToText(const ECodexCategory Category)
{
    switch (Category)
    {
        case ECodexCategory::Element:          return FText::FromString(TEXT("ELEMENTS"));
        case ECodexCategory::Shape:            return FText::FromString(TEXT("SHAPES"));
        case ECodexCategory::ShapeParameter:   return FText::FromString(TEXT("SHAPE PARAMETERS"));
        case ECodexCategory::MaterialProperty: return FText::FromString(TEXT("MATERIAL PROPERTIES"));
        case ECodexCategory::Spatial:          return FText::FromString(TEXT("SPATIAL"));
        case ECodexCategory::Pattern:          return FText::FromString(TEXT("PATTERNS"));
        case ECodexCategory::Motion:           return FText::FromString(TEXT("MOTION"));
        case ECodexCategory::Action:           return FText::FromString(TEXT("ACTIONS"));
        case ECodexCategory::WorldObject:      return FText::FromString(TEXT("WORLD OBJECTS"));
        case ECodexCategory::WorldState:       return FText::FromString(TEXT("WORLD STATES"));
        case ECodexCategory::Event:            return FText::FromString(TEXT("EVENTS"));
        case ECodexCategory::Logic:            return FText::FromString(TEXT("LOGIC"));
        case ECodexCategory::Value:            return FText::FromString(TEXT("VALUES"));
        case ECodexCategory::PhysicsConcept:   return FText::FromString(TEXT("WORLD PHYSICS"));
        default:                               return FText::FromString(TEXT("CODEX"));
    }
}

FText CodexTierToText(const ECodexTier Tier)
{
    switch (Tier)
    {
        case ECodexTier::TierI:
            return FText::FromString(TEXT("TIER I / FOUNDATION"));
        case ECodexTier::TierIII:
            return FText::FromString(TEXT("TIER III / TERMINAL"));
        case ECodexTier::TierII:
        default:
            return FText::FromString(TEXT("TIER II / OPERATOR"));
    }
}

FText CodexImplementationStateToText(const ECodexImplementationState State)
{
    switch (State)
    {
        case ECodexImplementationState::Implemented:
            return FText::FromString(TEXT("IMPLEMENTED"));
        case ECodexImplementationState::Planned:
            return FText::FromString(TEXT("PLANNED"));
        case ECodexImplementationState::CodexOnly:
        default:
            return FText::FromString(TEXT("CODEX ONLY"));
    }
}
