#pragma once
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "PrepareHomeworkCommandlet.generated.h"

UCLASS()
class UPrepareHomeworkCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UPrepareHomeworkCommandlet();
    virtual int32 Main(const FString& Params) override;
};
