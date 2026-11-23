#pragma once
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "BuildHomework2Commandlet.generated.h"
UCLASS()
class UBuildHomework2Commandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UBuildHomework2Commandlet();
    virtual int32 Main(const FString& Params) override;
};
