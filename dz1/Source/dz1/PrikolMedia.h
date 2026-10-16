#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PrikolMedia.generated.h"

class UTexture2D;

/** Cookable images extracted from the supplied PNG and GIF, including GIF timing. */
UCLASS()
class DZ1_API UPrikolMedia : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere) TObjectPtr<UTexture2D> FullscreenImage;
    UPROPERTY(EditAnywhere) TArray<TObjectPtr<UTexture2D>> Frames;
    UPROPERTY(EditAnywhere) TArray<float> FrameDurations;
};
