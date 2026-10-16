// Copyright Epic Games, Inc. All Rights Reserved.


#include "HomeworkPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "dz1.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include "PrikolOverlay.h"
#include "PrikolMedia.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"

void AHomeworkPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// only spawn touch controls on local player controllers
	if (IsLocalPlayerController() && ShouldUseTouchControls())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogTemp, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}
}

void AHomeworkPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		if (UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(InputComponent))
		{
			PrikolMapping = NewObject<UInputMappingContext>(this);
			PrikolActions.Reset();
			for (FKey Key : { EKeys::One, EKeys::Two, EKeys::Three })
			{
				UInputAction* Action = NewObject<UInputAction>(this);
				Action->ValueType = EInputActionValueType::Boolean;
				PrikolActions.Add(Action);
				PrikolMapping->MapKey(Action, Key);
			}
			Input->BindAction(PrikolActions[0], ETriggerEvent::Started, this, &AHomeworkPlayerController::HidePrikol);
			Input->BindAction(PrikolActions[1], ETriggerEvent::Started, this, &AHomeworkPlayerController::ShowPrikolImage);
			Input->BindAction(PrikolActions[2], ETriggerEvent::Started, this, &AHomeworkPlayerController::ShowPrikolAnimation);
		}
		// Add Input Mapping Contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			if (PrikolMapping) Subsystem->AddMappingContext(PrikolMapping, 1);
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}
}

bool AHomeworkPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

void AHomeworkPlayerController::EnsurePrikolOverlay()
{
	if (PrikolOverlay) return;
	PrikolMedia = LoadObject<UPrikolMedia>(nullptr, TEXT("/Game/Prikol/DA_Prikol.DA_Prikol"));
	if (!ensureMsgf(PrikolMedia, TEXT("Prikol media asset is missing"))) return;
	PrikolOverlay = CreateWidget<UPrikolOverlay>(this);
	if (PrikolOverlay)
	{
		PrikolOverlay->SetMedia(PrikolMedia);
		PrikolOverlay->Show(EPrikolMode::Hidden);
		PrikolOverlay->AddToViewport(1000);
	}
}

void AHomeworkPlayerController::ShowPrikolImage()
{
	EnsurePrikolOverlay();
	if (PrikolOverlay) PrikolOverlay->Show(EPrikolMode::Fullscreen);
}

void AHomeworkPlayerController::ShowPrikolAnimation()
{
	EnsurePrikolOverlay();
	if (PrikolOverlay) PrikolOverlay->Show(EPrikolMode::Animation);
}

void AHomeworkPlayerController::HidePrikol()
{
	if (PrikolOverlay) PrikolOverlay->Show(EPrikolMode::Hidden);
}
