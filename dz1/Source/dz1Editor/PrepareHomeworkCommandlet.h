#pragma once
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "PrepareHomeworkCommandlet.generated.h"

/** Reproducible asset preparation and validation; never required during gameplay. */
UCLASS()
class UPrepareHomeworkCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UPrepareHomeworkCommandlet();
    virtual int32 Main(const FString& Params) override;
};
