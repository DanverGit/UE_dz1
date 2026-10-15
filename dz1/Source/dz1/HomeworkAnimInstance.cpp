#include "HomeworkAnimInstance.h"
#include "HomeworkCharacter.h"

void UHomeworkAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	const AHomeworkCharacter* Character = Cast<AHomeworkCharacter>(TryGetPawnOwner());
	bIsSprinting = Character && Character->bIsSprinting;
}
