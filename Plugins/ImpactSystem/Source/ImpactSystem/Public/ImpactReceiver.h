#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ImpactTypes.h"
#include "ImpactReceiver.generated.h"
UINTERFACE(Blueprintable)
class IMPACTSYSTEM_API UImpactReceiver : public UInterface { GENERATED_BODY() };
class IMPACTSYSTEM_API IImpactReceiver
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Impact")
    bool ReceiveImpact(const FImpactRequest& Request, FImpactResult& OutResult);
};
