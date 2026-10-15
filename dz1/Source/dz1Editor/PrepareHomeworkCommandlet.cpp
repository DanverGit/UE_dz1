#include "PrepareHomeworkCommandlet.h"
#include "HomeworkCharacter.h"
#include "HomeworkAnimInstance.h"
#include "Engine/Blueprint.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimSequence.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_Event.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_AddDelegate.h"
#include "K2Node_CallFunction.h"
#include "K2Node_VariableGet.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "AnimGraphNode_StateMachine.h"
#include "AnimStateNode.h"
#include "AnimStateTransitionNode.h"
#include "AnimationStateMachineGraph.h"
#include "AnimationTransitionGraph.h"
#include "AnimGraphNode_SequencePlayer.h"
#include "AnimGraphNode_TransitionResult.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputCoreTypes.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "Misc/ScopeExit.h"

UPrepareHomeworkCommandlet::UPrepareHomeworkCommandlet()
{
    IsClient = false; IsServer = false; IsEditor = true; LogToConsole = true;
}

namespace HomeworkAssets
{
template<class T> T* Node(UEdGraph* Graph, int32 X, int32 Y)
{
    T* Result = NewObject<T>(Graph);
    Graph->AddNode(Result, false, false);
    Result->CreateNewGuid(); Result->NodePosX = X; Result->NodePosY = Y;
    return Result;
}
void Link(UEdGraphPin* A, UEdGraphPin* B)
{
    check(A && B);
    check(A->GetOwningNode()->GetSchema()->TryCreateConnection(A, B));
}
bool Save(UObject* Asset)
{
    UPackage* Package = Asset->GetOutermost();
    Package->MarkPackageDirty();
    FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone; Args.SaveFlags = SAVE_NoError;
    return UPackage::SavePackage(Package, Asset, *FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension()), Args);
}
bool CompileSave(UBlueprint* BP)
{
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    FKismetEditorUtilities::CompileBlueprint(BP);
    return BP->Status != BS_Error && Save(BP);
}
UInputAction* Action(const TCHAR* Name)
{
    FString Path = FString(TEXT("/Game/Input/Actions/")) + Name;
    UInputAction* A = LoadObject<UInputAction>(nullptr, *(Path + TEXT(".") + Name));
    if (!A) A = NewObject<UInputAction>(CreatePackage(*Path), Name, RF_Public | RF_Standalone);
    A->ValueType = EInputActionValueType::Boolean;
    check(Save(A)); return A;
}

void Transition(UAnimStateNode* From, UAnimStateNode* To, FName Flag, bool Negate, int32 Priority)
{
    UAnimStateTransitionNode* T = Node<UAnimStateTransitionNode>(From->GetGraph(), (From->NodePosX + To->NodePosX)/2, To->NodePosY - 100);
    T->PostPlacedNewNode(); T->AllocateDefaultPins();
    T->CreateConnections(From, To); T->CrossfadeDuration = 0.2f; T->PriorityOrder = Priority;
    UEdGraph* Rule = T->BoundGraph;
    UK2Node_VariableGet* Get = Node<UK2Node_VariableGet>(Rule, -450, 0);
    Get->VariableReference.SetSelfMember(Flag); Get->AllocateDefaultPins();
    UEdGraphPin* Value = Get->GetValuePin();
    if (Negate)
    {
        UK2Node_CallFunction* Not = Node<UK2Node_CallFunction>(Rule, -230, 0);
        Not->SetFromFunction(UKismetMathLibrary::StaticClass()->FindFunctionByName(TEXT("Not_PreBool")));
        Not->AllocateDefaultPins(); Link(Value, Not->FindPinChecked(TEXT("A")));
        Value = Not->GetReturnValuePin();
    }
    UAnimGraphNode_TransitionResult* Result = CastChecked<UAnimationTransitionGraph>(Rule)->GetResultNode();
    Link(Value, Result->FindPinChecked(TEXT("bCanEnterTransition")));
}
}

int32 UPrepareHomeworkCommandlet::Main(const FString& Params)
{
    using namespace HomeworkAssets;
    UBlueprint* Character = LoadObject<UBlueprint>(nullptr, TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter.BP_ThirdPersonCharacter"));
    UAnimBlueprint* Anim = LoadObject<UAnimBlueprint>(nullptr, TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed"));
    if (!Character || !Anim) return 1;
    if (Params.Contains(TEXT("Verify")))
    {
        UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
        FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
        Context.SetCurrentWorld(World);
        ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
        World->InitializeActorsForPlay(FURL());
        AHomeworkCharacter* Pawn = World->SpawnActor<AHomeworkCharacter>(Character->GeneratedClass, FVector(0, 0, 200), FRotator::ZeroRotator);
        APlayerController* Controller = World->SpawnActor<APlayerController>();
        if (!Pawn || !Controller) return 20;
        Controller->SetPlayer(NewObject<ULocalPlayer>(GEngine));
        Controller->Possess(Pawn); Pawn->PawnClientRestart(); Pawn->DispatchBeginPlay();
        UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(Pawn->InputComponent);
        UE_LOG(LogTemp, Display, TEXT("VERIFY Input=%s Sprint=%s Interact=%s Bound=%d Begun=%d"), *GetNameSafe(Input), *GetNameSafe(Pawn->SprintAction), *GetNameSafe(Pawn->InteractAction), Pawn->OnInteractionTriggered.IsBound(), Pawn->HasActorBegunPlay());
        if (!Input || !Pawn->SprintAction || !Pawn->InteractAction || !Pawn->OnInteractionTriggered.IsBound()) return 21;
        auto Trigger = [&](UInputAction* Action, ETriggerEvent Event)
        {
            int32 Count = 0;
            for (const auto& Binding : Input->GetActionEventBindings())
                if (Binding->GetAction() == Action && Binding->GetTriggerEvent() == Event)
                { Binding->Execute(FInputActionInstance(Action)); ++Count; }
            return Count == 1;
        };
        if (!Trigger(Pawn->SprintAction, ETriggerEvent::Started) || !Pawn->bIsSprinting || Pawn->GetCharacterMovement()->MaxWalkSpeed != 900.f) return 22;
        Pawn->GetMesh()->InitAnim(true);
        UHomeworkAnimInstance* Instance = Cast<UHomeworkAnimInstance>(Pawn->GetMesh()->GetAnimInstance());
        if (!Instance) return 23;
        Instance->NativeUpdateAnimation(1.f / 60.f);
        if (!Instance->bIsSprinting) return 24;
        if (!Trigger(Pawn->InteractAction, ETriggerEvent::Started)) return 25;
        if (!Trigger(Pawn->SprintAction, ETriggerEvent::Completed) || Pawn->bIsSprinting || Pawn->GetCharacterMovement()->MaxWalkSpeed != 500.f) return 26;
        Instance->NativeUpdateAnimation(1.f / 60.f);
        if (Instance->bIsSprinting) return 27;
        Trigger(Pawn->SprintAction, ETriggerEvent::Started);
        if (!Trigger(Pawn->SprintAction, ETriggerEvent::Canceled) || Pawn->bIsSprinting) return 28;
        Trigger(Pawn->SprintAction, ETriggerEvent::Started);
        Pawn->GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
        Pawn->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
        World->BeginPlay();
        auto Advance = [&]()
        {
            for (int32 Frame = 0; Frame < 45; ++Frame)
            {
                Pawn->AddMovementInput(FVector::ForwardVector, 1.f);
                World->Tick(LEVELTICK_All, 1.f / 60.f);
                ++GFrameCounter;
            }
        };
        Advance();
        const int32 Machine = Instance->GetStateMachineIndex(TEXT("Locomotion"));
        UE_LOG(LogTemp, Display, TEXT("VERIFY active sprint state: %s"), *Instance->GetCurrentStateName(Machine).ToString());
        if (Instance->GetCurrentStateName(Machine) != TEXT("Sprint")) return 30;
        Trigger(Pawn->SprintAction, ETriggerEvent::Completed);
        Advance();
        UE_LOG(LogTemp, Display, TEXT("VERIFY released sprint state: %s"), *Instance->GetCurrentStateName(Machine).ToString());
        if (Instance->GetCurrentStateName(Machine) != TEXT("Walk / Run")) return 31;
        Trigger(Pawn->SprintAction, ETriggerEvent::Started);
        Controller->UnPossess();
        if (Pawn->bIsSprinting || Pawn->GetCharacterMovement()->MaxWalkSpeed != 500.f) return 29;
        UE_LOG(LogTemp, Display, TEXT("HOMEWORK VERIFY PASS: Blueprint spawn, Enhanced Input bindings, 500/900 speeds, completed/canceled/unpossess reset, Blueprint delegate subscription and execution, animation flag propagation."));
        return 0;
    }
    if (Params.Contains(TEXT("Inspect"))) for (UBlueprint* BP : { Character, static_cast<UBlueprint*>(Anim) })
    {
        UE_LOG(LogTemp, Display, TEXT("HOMEWORK Blueprint %s parent %s"), *BP->GetName(), *GetNameSafe(BP->ParentClass));
        TArray<UEdGraph*> Graphs; BP->GetAllGraphs(Graphs);
        for (UEdGraph* Graph : Graphs)
        {
            UE_LOG(LogTemp, Display, TEXT("HOMEWORK Graph %s (%s)"), *Graph->GetName(), *Graph->GetClass()->GetName());
            for (UEdGraphNode* N : Graph->Nodes)
            {
                UE_LOG(LogTemp, Display, TEXT("HOMEWORK Node %s : %s"), *N->GetClass()->GetName(), *N->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
            }
        }
    }
    if (Params.Contains(TEXT("Inspect"))) return 0;

    UInputAction* Sprint = Action(TEXT("IA_Sprint"));
    UInputAction* Interact = Action(TEXT("IA_Interact"));
    UInputMappingContext* IMC = LoadObject<UInputMappingContext>(nullptr, TEXT("/Game/Input/IMC_Default.IMC_Default"));
    if (!IMC) return 2;
    IMC->UnmapAllKeysFromAction(Sprint); IMC->UnmapAllKeysFromAction(Interact);
    IMC->MapKey(Sprint, EKeys::LeftShift); IMC->MapKey(Interact, EKeys::E);
    if (!Save(IMC)) return 3;

    Character->ParentClass = AHomeworkCharacter::StaticClass();
    if (!CompileSave(Character)) return 4;
    AHomeworkCharacter* Defaults = CastChecked<AHomeworkCharacter>(Character->GeneratedClass->GetDefaultObject());
    Defaults->SprintAction = Sprint; Defaults->InteractAction = Interact;
    Defaults->WalkSpeed = 500.f; Defaults->SprintSpeed = 900.f;

    UEdGraph* Events = Character->UbergraphPages[0];
    bool HasBinding = false;
    for (UEdGraphNode* N : Events->Nodes)
        if (UK2Node_CustomEvent* E = Cast<UK2Node_CustomEvent>(N)) HasBinding |= E->CustomFunctionName == TEXT("HandleInteraction");
    if (!HasBinding)
    {
        UK2Node_Event* Begin = Node<UK2Node_Event>(Events, 0, 700);
        Begin->EventReference.SetExternalMember(TEXT("ReceiveBeginPlay"), AActor::StaticClass());
        Begin->bOverrideFunction = true; Begin->AllocateDefaultPins();
        UK2Node_AddDelegate* Bind = Node<UK2Node_AddDelegate>(Events, 300, 700);
        Bind->DelegateReference.SetExternalMember(TEXT("OnInteractionTriggered"), AHomeworkCharacter::StaticClass());
        Bind->AllocateDefaultPins();
        UK2Node_CustomEvent* Handler = Node<UK2Node_CustomEvent>(Events, 0, 1000);
        Handler->CustomFunctionName = TEXT("HandleInteraction"); Handler->AllocateDefaultPins();
        UK2Node_CallFunction* Print = Node<UK2Node_CallFunction>(Events, 350, 1000);
        Print->SetFromFunction(UKismetSystemLibrary::StaticClass()->FindFunctionByName(TEXT("PrintString")));
        Print->AllocateDefaultPins();
        Print->FindPinChecked(TEXT("InString"))->DefaultValue = TEXT("Interaction triggered!");
        Print->FindPinChecked(TEXT("Duration"))->DefaultValue = TEXT("2.0");
        Link(Begin->FindPinChecked(UEdGraphSchema_K2::PN_Then), Bind->FindPinChecked(UEdGraphSchema_K2::PN_Execute));
        Link(Handler->FindPinChecked(TEXT("OutputDelegate")), Bind->GetDelegatePin());
        Link(Handler->FindPinChecked(UEdGraphSchema_K2::PN_Then), Print->GetExecPin());
    }
    if (!CompileSave(Character)) return 5;

    Anim->ParentClass = UHomeworkAnimInstance::StaticClass();
    FBlueprintEditorUtils::RefreshAllNodes(Anim);
    FKismetEditorUtilities::CompileBlueprint(Anim);
    TArray<UEdGraph*> Graphs; Anim->GetAllGraphs(Graphs);
    UAnimStateNode* Walk = nullptr; UAnimStateNode* Idle = nullptr; UAnimStateNode* SprintState = nullptr;
    for (UEdGraph* Graph : Graphs)
        if (Graph->GetName() == TEXT("Locomotion") && Graph->IsA<UAnimationStateMachineGraph>())
            for (UEdGraphNode* N : Graph->Nodes)
                if (UAnimStateNode* State = Cast<UAnimStateNode>(N))
                {
                    if (State->GetStateName() == TEXT("Walk / Run")) Walk = State;
                    if (State->GetStateName() == TEXT("Idle")) Idle = State;
                    if (State->GetStateName() == TEXT("Sprint")) SprintState = State;
                }
    if (!Walk || !Idle) return 6;
    if (!SprintState)
    {
        SprintState = Node<UAnimStateNode>(Walk->GetGraph(), Walk->NodePosX + 420, Walk->NodePosY);
        SprintState->PostPlacedNewNode(); SprintState->AllocateDefaultPins();
        FBlueprintEditorUtils::RenameGraph(SprintState->BoundGraph, TEXT("Sprint"));
        UAnimSequence* Run = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Jog/MF_Unarmed_Jog_Fwd.MF_Unarmed_Jog_Fwd"));
        if (!Run) return 7;
        UAnimGraphNode_SequencePlayer* Player = Node<UAnimGraphNode_SequencePlayer>(SprintState->BoundGraph, -350, 0);
        Player->SetAnimationAsset(Run); Player->Node.SetLoopAnimation(true); Player->Node.SetPlayRate(1.35f);
        Player->AllocateDefaultPins();
        Link(Player->FindPinChecked(TEXT("Pose")), SprintState->GetPoseSinkPinInsideState());
        Transition(Walk, SprintState, TEXT("bIsSprinting"), false, 2);
        Transition(SprintState, Walk, TEXT("bIsSprinting"), true, 2);
        Transition(SprintState, Idle, TEXT("ShouldMove"), true, 1);
    }
    if (!CompileSave(Anim)) return 8;
    for (const TCHAR* Path : { TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonGameMode.BP_ThirdPersonGameMode"), TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonPlayerController.BP_ThirdPersonPlayerController") })
    {
        UBlueprint* BP = LoadObject<UBlueprint>(nullptr, Path);
        if (!BP || !CompileSave(BP)) return 9;
    }
    UE_LOG(LogTemp, Display, TEXT("HOMEWORK: assets prepared and all Blueprints compiled successfully."));
    return 0;
}
