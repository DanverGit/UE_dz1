#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "HomeworkAnimInstance.generated.h"

UCLASS()
class DZ1_API UHomeworkAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadOnly, Category="Sprint")
	bool bIsSprinting = false;

	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
};
