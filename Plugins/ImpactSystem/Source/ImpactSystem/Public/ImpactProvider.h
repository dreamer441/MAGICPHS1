#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ImpactTypes.h"
#include "ImpactProvider.generated.h"
UINTERFACE(Blueprintable)
class IMPACTSYSTEM_API UImpactProvider : public UInterface { GENERATED_BODY() };
class IMPACTSYSTEM_API IImpactProvider
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Impact")
    bool BuildImpactRequest(const FHitResult& Hit, FImpactRequest& OutRequest) const;
};
