#include "BuildHomework2Commandlet.h"
#include "HomeworkAI.h"
#include "Editor.h"
#include "FileHelpers.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "BehaviorTree/Composites/BTComposite_Selector.h"
#include "BehaviorTree/Composites/BTComposite_Sequence.h"
#include "BehaviorTree/Tasks/BTTask_MoveTo.h"
#include "BehaviorTree/Tasks/BTTask_Wait.h"
#include "BehaviorTree/Decorators/BTDecorator_Blackboard.h"
#include "BehaviorTreeGraph.h"
#include "BehaviorTreeGraphNode_Root.h"
#include "EdGraphSchema_BehaviorTree.h"
#include "PCGGraph.h"
#include "PCGNode.h"
#include "PCGVolume.h"
#include "PCGComponent.h"
#include "PCGSubsystem.h"
#include "Elements/PCGCreatePointsGrid.h"
#include "Elements/PCGTransformPoints.h"
#include "Elements/PCGStaticMeshSpawner.h"
#include "MeshSelectors/PCGMeshSelectorWeighted.h"
#include "Builders/CubeBuilder.h"
#include "ActorFactories/ActorFactory.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "NavigationSystem.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/WorldSettings.h"
#include "GameFramework/GameModeBase.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#include "EngineUtils.h"
#include "Engine/LocalPlayer.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Misc/ScopeExit.h"
#include "NavigationPath.h"
#include "GameFramework/CharacterMovementComponent.h"

UBuildHomework2Commandlet::UBuildHomework2Commandlet() { IsEditor = true; LogToConsole = true; }

namespace
{
bool SaveAsset(UObject* Object)
{
    FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone;
    UPackage* Package = Object->GetOutermost();
    return UPackage::SavePackage(Package, Object, *FPackageName::LongPackageNameToFilename(Package->GetName(), TEXT(".uasset")), Args);
}
void Key(UObject* Node, FName Name)
{
    FStructProperty* Property = FindFProperty<FStructProperty>(Node->GetClass(), TEXT("BlackboardKey"));
    check(Property);
    Property->ContainerPtrToValuePtr<FBlackboardKeySelector>(Node)->SelectedKeyName = Name;
}
void Box(AVolume* Volume, float X, float Y, float Z)
{
    UCubeBuilder* Builder = NewObject<UCubeBuilder>();
    Builder->X = X; Builder->Y = Y; Builder->Z = Z;
    UActorFactory::CreateBrushForVolumeActor(Volume, Builder);
}
}

int32 UBuildHomework2Commandlet::Main(const FString& Params)
{
    if (Params.Contains(TEXT("Verify")))
    {
        FMath::RandInit(42);
        if (!FEditorFileUtils::LoadMap(TEXT("/Game/AI/Lvl_PCGPatrol"), false, true)) return 20;
        UWorld* TestWorld = GEditor->GetEditorWorldContext().World();
        UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(TestWorld);
        if (!Nav) return 21;
        UNavigationSystemV1::SetNavigationAutoUpdateEnabled(true, Nav);
        FlushAsyncLoading(); Nav->RemoveNavigationBuildLock(ENavigationBuildLock::AsyncLoadLock); Nav->OnWorldInitDone(FNavigationSystemRunMode::EditorMode); Nav->GetDefaultNavDataInstance(FNavigationSystem::Create); Nav->Tick(0.1f); Nav->Build();
        FNavLocation Start;
        if (!Nav->ProjectPointToNavigation(FVector(0,0,100), Start, FVector(200,200,300))) return 22;
        TestWorld->WorldType = EWorldType::Game;
        TestWorld->CreateAISystem();
        TestWorld->InitializeActorsForPlay(FURL());
        APlayerController* PC = TestWorld->SpawnActor<APlayerController>();
        PC->SetPlayer(NewObject<ULocalPlayer>(GEngine));
        ACharacter* Player = TestWorld->SpawnActor<ACharacter>(FVector(-10000,0,110), FRotator::ZeroRotator);
        PC->Possess(Player);
        AHomeworkAICharacter* Bot = nullptr;
        for (TActorIterator<AHomeworkAICharacter> It(TestWorld); It; ++It) { Bot = *It; break; }
        if (!Bot) return 23;
        const FTransform BotTransform = Bot->GetActorTransform();
        Bot->Destroy();
        Bot = TestWorld->SpawnActor<AHomeworkAICharacter>(AHomeworkAICharacter::StaticClass(), BotTransform);
        if (!Bot) return 23;
        Bot->SpawnDefaultController();
        TestWorld->BeginPlay();
        TestWorld->GetWorldSettings()->NotifyBeginPlay();
        TestWorld->GetWorldSettings()->NotifyMatchStarted();
        ON_SCOPE_EXIT { TestWorld->EndPlay(EEndPlayReason::Quit); };
        auto Step = [&](int32 Frames) { for (int32 I=0; I<Frames; ++I) { TestWorld->Tick(LEVELTICK_All, 1.f/30.f); ++GFrameCounter; } };
        AHomeworkAIController* AI = Cast<AHomeworkAIController>(Bot->GetController());
        if (!AI || !AI->GetBlackboardComponent()) return 24;
        if (!AI->Behavior || !AI->Behavior->RootNode) return 24;
        FVector Before = Bot->GetActorLocation(); Step(180);
        float PatrolDistance = FVector::Dist2D(Before, Bot->GetActorLocation());
        UE_LOG(LogTemp, Display, TEXT("VERIFY patrol displacement: %.1f"), PatrolDistance);
        if (PatrolDistance < 50.f) return 25;
        if (AI->GetBlackboardComponent()->GetValueAsObject(TEXT("Target"))) return 25;
        FNavLocation Nearby;
        if (!Nav->ProjectPointToNavigation(Bot->GetActorLocation()+FVector(650,0,0), Nearby, FVector(300,300,300))) return 26;
        Player->SetActorLocation(Nearby.Location+FVector(0,0,100));
        Before = Bot->GetActorLocation(); Step(15);
        if (AI->GetBlackboardComponent()->GetValueAsObject(TEXT("Target")) != Player) return 27;
        Step(60);
        UE_LOG(LogTemp, Display, TEXT("VERIFY chase displacement: %.1f"), FVector::Dist2D(Before,Bot->GetActorLocation()));
        if (FVector::Dist2D(Before,Bot->GetActorLocation()) < 50.f) return 28;
        Player->SetActorLocation(FVector(10000,10000,100)); Step(15);
        if (AI->GetBlackboardComponent()->GetValueAsObject(TEXT("Target"))) return 29;
        Before = Bot->GetActorLocation(); Step(150);
        if (FVector::Dist2D(Before,Bot->GetActorLocation()) < 50.f) return 30;
        int32 Detours = 0;
        for (TActorIterator<AActor> It(TestWorld); It; ++It)
        {
            TArray<UInstancedStaticMeshComponent*> Components; It->GetComponents(Components);
            for (auto* C : Components) for (int32 I=0; I<C->GetInstanceCount(); ++I)
            {
                FTransform T; C->GetInstanceTransform(I,T,true); FVector P=T.GetLocation(); P.Z=10;
                UNavigationPath* Path = Nav->FindPathToLocationSynchronously(TestWorld,P-FVector(300,0,0),P+FVector(300,0,0));
                if (Path && Path->IsValid() && !Path->IsPartial() && Path->PathPoints.Num()>2) ++Detours;
            }
        }
        UE_LOG(LogTemp, Display, TEXT("VERIFY obstacle detours: %d"),Detours);
        if (Detours < 1) return 31;
        UE_LOG(LogTemp, Display, TEXT("HOMEWORK2 VERIFY PASS: patrol, chase, loss, resumed patrol, NavMesh obstacle detours."));
        return 0;
    }
    for (const TCHAR* Path : { TEXT("/Game/AI/BB_Patrol"), TEXT("/Game/AI/BT_Patrol"), TEXT("/Game/AI/PCG_Obstacles") })
        if (FPackageName::DoesPackageExist(Path)) LoadPackage(nullptr, Path, LOAD_None)->FullyLoad();
    UBlackboardData* BB = NewObject<UBlackboardData>(CreatePackage(TEXT("/Game/AI/BB_Patrol")), TEXT("BB_Patrol"), RF_Public | RF_Standalone);
    for (FName Name : { FName("Home"), FName("PatrolPoint"), FName("Target") })
    {
        FBlackboardEntry Entry; Entry.EntryName = Name;
        Entry.KeyType = Name == TEXT("Target") ? static_cast<UBlackboardKeyType*>(NewObject<UBlackboardKeyType_Object>(BB)) : NewObject<UBlackboardKeyType_Vector>(BB);
        if (auto* ObjectKey = Cast<UBlackboardKeyType_Object>(Entry.KeyType)) ObjectKey->BaseClass = AActor::StaticClass();
        BB->Keys.Add(Entry);
    }
    if (!SaveAsset(BB)) return 1;
    UBehaviorTree* Tree = NewObject<UBehaviorTree>(CreatePackage(TEXT("/Game/AI/BT_Patrol")), TEXT("BT_Patrol"), RF_Public | RF_Standalone);
    Tree->BlackboardAsset = BB;
    UBTComposite_Selector* Root = NewObject<UBTComposite_Selector>(Tree); Tree->RootNode = Root;
    UBTTask_MoveTo* Chase = NewObject<UBTTask_MoveTo>(Tree); Key(Chase, TEXT("Target"));
    Chase->AcceptableRadius = 110.f;
    UBTDecorator_Blackboard* Seen = NewObject<UBTDecorator_Blackboard>(Tree); Key(Seen, TEXT("Target"));
    FByteProperty* Abort = FindFProperty<FByteProperty>(Seen->GetClass(), TEXT("FlowAbortMode"));
    Abort->SetPropertyValue_InContainer(Seen, EBTFlowAbortMode::Both);
    FBTCompositeChild ChaseChild; ChaseChild.ChildTask = Chase; ChaseChild.Decorators.Add(Seen); Root->Children.Add(ChaseChild);
    UBTComposite_Sequence* Patrol = NewObject<UBTComposite_Sequence>(Tree);
    FBTCompositeChild PatrolChild; PatrolChild.ChildComposite = Patrol; Root->Children.Add(PatrolChild);
    UBTTask_MoveTo* Move = NewObject<UBTTask_MoveTo>(Tree); Key(Move, TEXT("PatrolPoint")); Move->AcceptableRadius = 60.f;
    UBTTask_Wait* Wait = NewObject<UBTTask_Wait>(Tree); Wait->WaitTime = 1.0f;
    for (UBTTaskNode* Task : { static_cast<UBTTaskNode*>(NewObject<UBTTask_PatrolPoint>(Tree)), static_cast<UBTTaskNode*>(Move), static_cast<UBTTaskNode*>(Wait) })
    { FBTCompositeChild Child; Child.ChildTask = Task; Patrol->Children.Add(Child); }
    UBehaviorTreeGraph* BTGraph = CastChecked<UBehaviorTreeGraph>(FBlueprintEditorUtils::CreateNewGraph(Tree, TEXT("BehaviorTree"), UBehaviorTreeGraph::StaticClass(), UEdGraphSchema_BehaviorTree::StaticClass()));
    Tree->BTGraph = BTGraph;
    BTGraph->LockUpdates();
    auto* RootGraphNode = NewObject<UBehaviorTreeGraphNode_Root>(BTGraph); BTGraph->AddNode(RootGraphNode); RootGraphNode->CreateNewGuid(); RootGraphNode->AllocateDefaultPins();
    for (UEdGraphNode* N : BTGraph->Nodes)
        if (auto* R = Cast<UBehaviorTreeGraphNode_Root>(N)) { R->BlackboardAsset = BB; R->NodeInstance = BB; }
    Tree->RootNode = Root;
    BTGraph->OnCreated(); BTGraph->Initialize(); BTGraph->UnlockUpdates(); BTGraph->UpdateAsset();
    check(Tree->RootNode && Tree->RootNode->Children.Num() == 2);
    if (!SaveAsset(Tree)) return 2;

    UPCGGraph* Graph = NewObject<UPCGGraph>(CreatePackage(TEXT("/Game/AI/PCG_Obstacles")), TEXT("PCG_Obstacles"), RF_Public | RF_Standalone);
    UPCGCreatePointsGridSettings* Grid; UPCGNode* GridNode = Graph->AddNodeOfType(Grid);
    Grid->GridExtents = FVector(2800, 2800, 0); Grid->CellSize = FVector(700, 700, 100);
    UPCGTransformPointsSettings* Transform; UPCGNode* TransformNode = Graph->AddNodeOfType(Transform);
    Transform->OffsetMin = FVector(-160, -160, 150); Transform->OffsetMax = FVector(160, 160, 150);
    Transform->RotationMax = FRotator(0, 180, 0); Transform->bUniformScale = false;
    Transform->ScaleMin = FVector(1.5, 1.5, 3); Transform->ScaleMax = FVector(2.3, 2.3, 3);
    UPCGStaticMeshSpawnerSettings* Spawner; UPCGNode* SpawnNode = Graph->AddNodeOfType(Spawner);
    Spawner->SetMeshSelectorType(UPCGMeshSelectorWeighted::StaticClass()); Spawner->bSynchronousLoad = true;
    FPCGMeshSelectorWeightedEntry Entry;
    Entry.Descriptor.StaticMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Engine/BasicShapes/Cube.Cube")));
    Entry.Descriptor.BodyInstance.SetCollisionProfileName(TEXT("BlockAll")); Entry.Descriptor.bCanEverAffectNavigation = true;
    CastChecked<UPCGMeshSelectorWeighted>(Spawner->MeshSelectorParameters)->MeshEntries.Add(Entry);
    Graph->AddEdge(GridNode, TEXT("Out"), TransformNode, TEXT("In")); Graph->AddEdge(TransformNode, TEXT("Out"), SpawnNode, TEXT("In"));
    GridNode->SetNodePosition(0, 0); TransformNode->SetNodePosition(300, 0); SpawnNode->SetNodePosition(600, 0);
    if (!SaveAsset(Graph)) return 3;

    UWorld* World = GEditor->NewMap();
    World->GetWorldSettings()->DefaultGameMode = LoadClass<AGameModeBase>(nullptr, TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonGameMode.BP_ThirdPersonGameMode_C"));
    AStaticMeshActor* Floor = World->SpawnActor<AStaticMeshActor>(FVector(0, 0, -50), FRotator::ZeroRotator);
    Floor->SetActorLabel(TEXT("Arena floor")); Floor->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
    Floor->SetActorScale3D(FVector(80, 80, 1)); Floor->GetStaticMeshComponent()->SetCollisionProfileName(TEXT("BlockAll"));
    ADirectionalLight* Sun = World->SpawnActor<ADirectionalLight>(FVector(0,0,1000), FRotator(-50,-30,0));
    Sun->GetLightComponent()->SetIntensity(3.f);
    ASkyLight* Sky = World->SpawnActor<ASkyLight>(); Sky->GetLightComponent()->SetMobility(EComponentMobility::Movable); Sky->GetLightComponent()->SetIntensity(1.f);
    World->SpawnActor<APlayerStart>(FVector(-3400, 0, 110), FRotator::ZeroRotator);
    World->SpawnActor<AHomeworkAICharacter>(FVector(0, 0, 110), FRotator::ZeroRotator);
    ANavMeshBoundsVolume* Bounds = World->SpawnActor<ANavMeshBoundsVolume>(); Box(Bounds, 8000, 8000, 1600);
    APCGVolume* Volume = World->SpawnActor<APCGVolume>(); Box(Volume, 6000, 6000, 1200);
    Volume->SetActorLabel(TEXT("PCG obstacles - Generate to rebuild"));
    Volume->PCGComponent->SetGraph(Graph); Volume->PCGComponent->Seed = 42;
    Volume->PCGComponent->GenerateLocal(true);
    UPCGSubsystem* PCG = World->GetSubsystem<UPCGSubsystem>();
    const double Deadline = FPlatformTime::Seconds() + 60;
    while (Volume->PCGComponent->IsGenerating() && FPlatformTime::Seconds() < Deadline)
    { PCG->Tick(0.05f); FTaskGraphInterface::Get().ProcessThreadUntilIdle(ENamedThreads::GameThread); FPlatformProcess::Sleep(0.01f); }
    int32 Count = 0;
    for (TActorIterator<AActor> It(World); It; ++It)
    { TArray<UInstancedStaticMeshComponent*> Components; It->GetComponents(Components); for (auto* Component : Components) Count += Component->GetInstanceCount(); }
    UE_LOG(LogTemp, Display, TEXT("HOMEWORK2 PCG generated %d obstacles"), Count);
    if (Count < 20) return 4;
    UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
    if (!Nav) return 5;
    UNavigationSystemV1::SetNavigationAutoUpdateEnabled(true, Nav);
    Nav->OnNavigationBoundsUpdated(Bounds); FlushAsyncLoading(); Nav->RemoveNavigationBuildLock(ENavigationBuildLock::AsyncLoadLock); Nav->OnWorldInitDone(FNavigationSystemRunMode::EditorMode); Nav->GetDefaultNavDataInstance(FNavigationSystem::Create); Nav->Tick(0.1f); Nav->Build();
    if (!FEditorFileUtils::SaveLevel(World->PersistentLevel, FPackageName::LongPackageNameToFilename(TEXT("/Game/AI/Lvl_PCGPatrol"), TEXT(".umap")))) return 6;
    UE_LOG(LogTemp, Display, TEXT("HOMEWORK2 assets and level saved."));
    return 0;
}
