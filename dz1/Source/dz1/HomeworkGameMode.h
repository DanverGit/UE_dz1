// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "HomeworkGameMode.generated.h"

/**
 *  Simple GameMode for a third person game
 */
UCLASS(abstract)
class DZ1_API AHomeworkGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	
	/** Constructor */
	AHomeworkGameMode();
};
