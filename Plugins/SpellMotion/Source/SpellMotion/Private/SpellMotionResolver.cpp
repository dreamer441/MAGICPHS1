#include "SpellMotionResolver.h"

namespace
{
    FVector SafeForward(const FVector& InForward)
    {
        const FVector Forward = InForward.GetSafeNormal();
        return Forward.IsNearlyZero() ? FVector::ForwardVector : Forward;
    }

    FVector SafeHorizontalForward(const FVector& InForward)
    {
        FVector Forward = InForward;
        Forward.Z = 0.0f;
        if (!Forward.Normalize())
        {
            return FVector::ForwardVector;
        }
        return Forward;
    }

    FVector ResolveLineAxis(
        const ESpellPatternAxis Axis,
        const FVector& CastForward)
    {
        const FVector HorizontalForward = SafeHorizontalForward(CastForward);
        FVector Right = FVector::CrossProduct(
            FVector::UpVector,
            HorizontalForward).GetSafeNormal();

        if (Right.IsNearlyZero())
        {
            Right = FVector::RightVector;
        }

        switch (Axis)
        {
            case ESpellPatternAxis::Forward: return HorizontalForward;
            case ESpellPatternAxis::Right:   return Right;
            case ESpellPatternAxis::Up:      return FVector::UpVector;
            default:                         return HorizontalForward;
        }
    }
}

FVector FSpellMotionResolver::ResolveDirection(
    const ESpellMotionDirection Direction,
    const FSpellPatternDefinition& Pattern,
    const FVector& InstanceLocation,
    const FVector& PatternCenter,
    const FVector& CastForward)
{
    const FVector Forward = SafeForward(CastForward);

    switch (Direction)
    {
        case ESpellMotionDirection::Forward:
            return Forward;

        case ESpellMotionDirection::Backward:
            return -Forward;

        case ESpellMotionDirection::Up:
            return FVector::UpVector;

        case ESpellMotionDirection::Down:
            return -FVector::UpVector;

        case ESpellMotionDirection::Outward:
        case ESpellMotionDirection::Inward:
        {
            FVector Radial = InstanceLocation - PatternCenter;

            // Circle patterns are horizontal by definition. Line patterns may
            // intentionally use Up, so keep Z for lines.
            if (Pattern.Arrangement == ESpellArrangement::Circle)
            {
                Radial.Z = 0.0f;
            }

            if (!Radial.Normalize())
            {
                return Forward;
            }

            return Direction == ESpellMotionDirection::Outward
                ? Radial
                : -Radial;
        }

        case ESpellMotionDirection::Tangent:
        {
            if (Pattern.Amount <= 1)
            {
                return Forward;
            }

            if (Pattern.Arrangement == ESpellArrangement::Circle)
            {
                FVector Radial = InstanceLocation - PatternCenter;
                Radial.Z = 0.0f;

                if (!Radial.Normalize())
                {
                    return Forward;
                }

                FVector Tangent =
                    FVector::CrossProduct(FVector::UpVector, Radial).GetSafeNormal();

                return Tangent.IsNearlyZero() ? Forward : Tangent;
            }

            const FVector Axis = ResolveLineAxis(Pattern.LineAxis, CastForward);
            return Axis.IsNearlyZero() ? Forward : Axis;
        }

        default:
            return Forward;
    }
}
