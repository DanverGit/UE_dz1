#include "HomeworkAI.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

AHomeworkAIController::AHomeworkAIController()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.15f;
    static ConstructorHelpers::FObjectFinder<UBehaviorTree> Tree(TEXT("/Game/AI/BT_Patrol.BT_Patrol"));
    Behavior = Tree.Object;
}

void AHomeworkAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
    if (Behavior && RunBehaviorTree(Behavior)) GetBlackboardComponent()->SetValueAsVector(TEXT("Home"), InPawn->GetActorLocation());
}

void AHomeworkAIController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    UBlackboardComponent* BB = GetBlackboardComponent();
    ACharacter* Self = Cast<ACharacter>(GetPawn());
    if (!BB || !Self) return;
    APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
    const float Radius = BB->GetValueAsObject(TEXT("Target")) ? LoseRadius : SightRadius;
    const bool bSeen = IsValid(Player) && FVector::DistSquared(Self->GetActorLocation(), Player->GetActorLocation()) <= FMath::Square(Radius);
    if (bSeen) BB->SetValueAsObject(TEXT("Target"), Player);
    else BB->ClearValue(TEXT("Target"));
    Self->GetCharacterMovement()->MaxWalkSpeed = bSeen ? 650.f : 250.f;
}

AHomeworkAICharacter::AHomeworkAICharacter()
{
    AIControllerClass = AHomeworkAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);
    bUseControllerRotationYaw = false;
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0, 400, 0);
    GetCharacterMovement()->MaxWalkSpeed = 250.f;
    GetCharacterMovement()->GetNavAgentPropertiesRef().AgentRadius = 42.f;
    GetCharacterMovement()->GetNavAgentPropertiesRef().AgentHeight = 192.f;
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> CharacterMesh(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));
    static ConstructorHelpers::FClassFinder<UAnimInstance> Anim(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed"));
    GetMesh()->SetSkeletalMesh(CharacterMesh.Object);
    GetMesh()->SetRelativeLocation(FVector(0, 0, -96));
    GetMesh()->SetRelativeRotation(FRotator(0, -90, 0));
    GetMesh()->SetAnimInstanceClass(Anim.Class);
    GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

UBTTask_PatrolPoint::UBTTask_PatrolPoint() { NodeName = TEXT("Random reachable point around Home"); }

EBTNodeResult::Type UBTTask_PatrolPoint::ExecuteTask(UBehaviorTreeComponent& Owner, uint8* Memory)
{
    AHomeworkAIController* AI = Cast<AHomeworkAIController>(Owner.GetAIOwner());
    UBlackboardComponent* BB = Owner.GetBlackboardComponent();
    UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(Owner.GetWorld());
    if (!AI || !AI->GetPawn() || !BB || !Nav) return EBTNodeResult::Failed;
    FNavLocation Point;
    if (!Nav->GetRandomReachablePointInRadius(BB->GetValueAsVector(TEXT("Home")), AI->PatrolRadius, Point)) return EBTNodeResult::Failed;
    const FVector Home = BB->GetValueAsVector(TEXT("Home"));
    if (FVector::DistSquared2D(Point.Location, Home) > FMath::Square(AI->PatrolRadius))
        if (!Nav->ProjectPointToNavigation(Home, Point)) return EBTNodeResult::Failed;
    BB->SetValueAsVector(TEXT("PatrolPoint"), Point.Location);
    return EBTNodeResult::Succeeded;
}
