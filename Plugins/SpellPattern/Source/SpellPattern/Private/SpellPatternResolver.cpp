#include "SpellPatternResolver.h"

namespace
{
    FVector SafeHorizontalForward(FVector Forward)
    {
        Forward.Z = 0.0f;
        if (!Forward.Normalize())
        {
            return FVector::ForwardVector;
        }
        return Forward;
    }

    FQuat MakeFacingDelta(
        const FVector& SharedForward,
        const FVector& DesiredForward)
    {
        const FVector From = SharedForward.GetSafeNormal();
        const FVector To = DesiredForward.GetSafeNormal();

        if (From.IsNearlyZero() || To.IsNearlyZero())
        {
            return FQuat::Identity;
        }

        const float Dot = FVector::DotProduct(From, To);

        if (Dot < -0.999f)
        {
            return FQuat(FVector::UpVector, PI);
        }

        return FQuat::FindBetweenNormals(From, To);
    }

    FVector ResolveLineAxis(
        const ESpellPatternAxis Axis,
        const FVector& Forward,
        const FVector& Right)
    {
        switch (Axis)
        {
            case ESpellPatternAxis::Forward: return Forward;
            case ESpellPatternAxis::Right:   return Right;
            case ESpellPatternAxis::Up:      return FVector::UpVector;
            default:                         return Right;
        }
    }

    FQuat ResolveCirclePatternRotation(
        const ESpellPatternOrientation Mode,
        const FVector& SharedForward,
        const FVector& RadialOutward)
    {
        switch (Mode)
        {
            case ESpellPatternOrientation::Outward:
                return MakeFacingDelta(SharedForward, RadialOutward);

            case ESpellPatternOrientation::Inward:
                return MakeFacingDelta(SharedForward, -RadialOutward);

            case ESpellPatternOrientation::Tangent:
            {
                FVector Tangent =
                    FVector::CrossProduct(FVector::UpVector, RadialOutward).GetSafeNormal();

                if (Tangent.IsNearlyZero())
                {
                    Tangent = SharedForward;
                }

                return MakeFacingDelta(SharedForward, Tangent);
            }

            case ESpellPatternOrientation::Shared:
            default:
                return FQuat::Identity;
        }
    }

    FQuat ResolveLinePatternRotation(
        const ESpellPatternOrientation Mode,
        const FVector& SharedForward,
        const FVector& Axis,
        const float OffsetSteps)
    {
        switch (Mode)
        {
            case ESpellPatternOrientation::Outward:
                if (FMath::IsNearlyZero(OffsetSteps))
                {
                    return FQuat::Identity;
                }
                return MakeFacingDelta(
                    SharedForward,
                    OffsetSteps < 0.0f ? -Axis : Axis);

            case ESpellPatternOrientation::Inward:
                if (FMath::IsNearlyZero(OffsetSteps))
                {
                    return FQuat::Identity;
                }
                return MakeFacingDelta(
                    SharedForward,
                    OffsetSteps < 0.0f ? Axis : -Axis);

            case ESpellPatternOrientation::Tangent:
                return MakeFacingDelta(SharedForward, Axis);

            case ESpellPatternOrientation::Shared:
            default:
                return FQuat::Identity;
        }
    }
}

void FSpellPatternResolver::ResolveInstances(
    const FSpellPatternDefinition& Pattern,
    const FVector& PatternCenter,
    const FVector& CastForward,
    TArray<FResolvedPatternInstance>& OutInstances)
{
    OutInstances.Reset();

    const int32 Amount = FMath::Clamp(
        Pattern.Amount,
        SpellPatternRanges::MinAmount,
        SpellPatternRanges::MaxAmount);

    const FVector Forward = SafeHorizontalForward(CastForward);

    FVector Right =
        FVector::CrossProduct(FVector::UpVector, Forward).GetSafeNormal();

    if (Right.IsNearlyZero())
    {
        Right = FVector::RightVector;
    }

    if (Amount <= 1)
    {
        FResolvedPatternInstance& Instance = OutInstances.AddDefaulted_GetRef();
        Instance.Location = PatternCenter;
        Instance.PatternRotation = FQuat::Identity;
        return;
    }

    if (Pattern.Arrangement == ESpellArrangement::Circle)
    {
        const float Radius = FMath::Clamp(
            Pattern.CircleRadiusCm,
            SpellPatternRanges::MinCircleRadiusCm,
            SpellPatternRanges::MaxCircleRadiusCm);

        OutInstances.Reserve(Amount);

        for (int32 Index = 0; Index < Amount; ++Index)
        {
            const float Angle =
                2.0f * PI * static_cast<float>(Index)
                / static_cast<float>(Amount);

            FVector Radial =
                Forward * FMath::Cos(Angle)
                + Right * FMath::Sin(Angle);

            Radial.Normalize();

            FResolvedPatternInstance& Instance =
                OutInstances.AddDefaulted_GetRef();

            Instance.Location = PatternCenter + Radial * Radius;
            Instance.PatternRotation = ResolveCirclePatternRotation(
                Pattern.InstanceOrientation,
                Forward,
                Radial);
        }

        return;
    }

    const FVector Axis =
        ResolveLineAxis(Pattern.LineAxis, Forward, Right).GetSafeNormal();

    const float Spacing = FMath::Clamp(
        Pattern.SpacingCm,
        SpellPatternRanges::MinSpacingCm,
        SpellPatternRanges::MaxSpacingCm);

    const float Middle =
        static_cast<float>(Amount - 1) * 0.5f;

    OutInstances.Reserve(Amount);

    for (int32 Index = 0; Index < Amount; ++Index)
    {
        const float OffsetSteps =
            static_cast<float>(Index) - Middle;

        FResolvedPatternInstance& Instance =
            OutInstances.AddDefaulted_GetRef();

        Instance.Location =
            PatternCenter + Axis * (OffsetSteps * Spacing);

        Instance.PatternRotation = ResolveLinePatternRotation(
            Pattern.InstanceOrientation,
            Forward,
            Axis,
            OffsetSteps);
    }
}

void FSpellPatternResolver::ResolveLocations(
    const FSpellPatternDefinition& Pattern,
    const FVector& PatternCenter,
    const FVector& CastForward,
    TArray<FVector>& OutLocations)
{
    TArray<FResolvedPatternInstance> Instances;
    ResolveInstances(Pattern, PatternCenter, CastForward, Instances);

    OutLocations.Reset(Instances.Num());

    for (const FResolvedPatternInstance& Instance : Instances)
    {
        OutLocations.Add(Instance.Location);
    }
}
