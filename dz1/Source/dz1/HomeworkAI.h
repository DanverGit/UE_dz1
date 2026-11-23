#pragma once
#include "CoreMinimal.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "BehaviorTree/BTTaskNode.h"
#include "HomeworkAI.generated.h"

UCLASS()
class DZ1_API AHomeworkAIController : public AAIController
{
    GENERATED_BODY()
public:
    AHomeworkAIController();
    virtual void OnPossess(APawn* Pawn) override;
    virtual void Tick(float DeltaSeconds) override;
    UPROPERTY(EditDefaultsOnly, Category="AI") float SightRadius = 1200.f;
    UPROPERTY(EditDefaultsOnly, Category="AI") float LoseRadius = 1700.f;
    UPROPERTY(EditDefaultsOnly, Category="AI") float PatrolRadius = 2400.f;
    UPROPERTY(EditDefaultsOnly, Category="AI") TObjectPtr<class UBehaviorTree> Behavior;
};

UCLASS()
class DZ1_API AHomeworkAICharacter : public ACharacter
{
    GENERATED_BODY()
public:
    AHomeworkAICharacter();
};

UCLASS()
class DZ1_API UBTTask_PatrolPoint : public UBTTaskNode
{
    GENERATED_BODY()
public:
    UBTTask_PatrolPoint();
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& Owner, uint8* Memory) override;
};
