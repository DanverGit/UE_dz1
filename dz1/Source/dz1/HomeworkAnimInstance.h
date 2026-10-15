#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "HomeworkAnimInstance.generated.h"

/** Supplies the character's C++ sprint flag to the template animation state machine. */
UCLASS()
class DZ1_API UHomeworkAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadOnly, Category="Sprint")
	bool bIsSprinting = false;

	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
};
