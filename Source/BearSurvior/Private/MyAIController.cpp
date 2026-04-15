#include "MyAIController.h"
#include "Engine.h"
#include "EnemyBase.h"
#include "BearSurviorCharacter.h"
#include "Components/StateTreeComponent.h"
#include "Misc/AssertionMacros.h"

namespace
{
	/** 获取当前控制器接管的敌人角色，避免重复的空指针解引用。 */
	AEnemyBase* GetControlledEnemy(const AMyAIController* Controller)
	{
		if (!IsValid(Controller))
			return nullptr;

		return Cast<AEnemyBase>(Controller->GetPawn());
	}
}

AMyAIController::AMyAIController()
{
	// 创建并持有 StateTree AI 组件，供后续行为树状态逻辑使用。
	StateTree = CreateDefaultSubobject<UStateTreeComponent>(TEXT("StateTree"));
	check(StateTree);
	// 使用父类预留的感知组件指针，避免子类与基类维护两份引用。
	PerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComponent"));
	check(PerceptionComponent);

	// 在控制器接管 Pawn 后自动启动 AI 逻辑。
	bStartAILogicOnPossess = true;

	// 将控制器附着到 Pawn，避免部分 AI/查询逻辑缺少宿主引用。
	bAttachToPawn = true;
	bIsSeeingPlayer = false;
	TargetCharacter = nullptr;
}

void AMyAIController::BeginPlay()
{
	Super::BeginPlay();

	// 注册感知更新事件，确保 AI 在发现目标时能够立即收到回调。
	if (!PerceptionComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("PerceptionComponent is invalid on %s."), *GetNameSafe(this));
		return;
	}

	PerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &AMyAIController::OnSeeingPlayer);
	PerceptionComponent->OnTargetPerceptionForgotten.AddDynamic(this, &AMyAIController::OnForgetPlayer);
}

void AMyAIController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 在控制器销毁前移除感知回调，避免 Pawn 或控制器生命周期结束后仍然触发事件。
	if (PerceptionComponent)
	{
		PerceptionComponent->OnTargetPerceptionUpdated.RemoveDynamic(this, &AMyAIController::OnSeeingPlayer);
		PerceptionComponent->OnTargetPerceptionForgotten.RemoveDynamic(this, &AMyAIController::OnForgetPlayer);
	}

	Super::EndPlay(EndPlayReason);
}

void AMyAIController::OnForgetPlayer(AActor* UpdatedActor)
{
	ABearSurviorCharacter *PlayerCharacter = Cast<ABearSurviorCharacter>(UpdatedActor);
	// 如果忘记的不是玩家则直接返回，避免误触发 AI 行为。
	if (PlayerCharacter == nullptr)
		return;

	// 设置对应状态变量
	bIsSeeingPlayer = false;
	TargetCharacter = nullptr;

	AEnemyBase* ControlledEnemy = GetControlledEnemy(this);
	if (!ControlledEnemy)
	{
		UE_LOG(LogTemp, Warning, TEXT("MyAIController %s has no controlled enemy when forgetting %s."), *GetNameSafe(this), *GetNameSafe(UpdatedActor));
		return;
	}

	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Blue, FString::Printf(TEXT("忘记玩家")));
	ControlledEnemy->SwitchEnemyState(EEnemyState::Idle);
}

void AMyAIController::OnSeeingPlayer(AActor* UpdatedActor, FAIStimulus Stimulus)
{
	ABearSurviorCharacter *PlayerCharacter = Cast<ABearSurviorCharacter>(UpdatedActor);
	// 如果看到的不是玩家则直接返回，避免误触发 AI 行为。
	if (PlayerCharacter == nullptr)
		return;
		
	// 设置对应状态变量
	if (Stimulus.WasSuccessfullySensed())
	{
		bIsSeeingPlayer = true;
		TargetCharacter = PlayerCharacter;
	}

	AEnemyBase* ControlledEnemy = GetControlledEnemy(this);
	if (!ControlledEnemy)
	{
		UE_LOG(LogTemp, Warning, TEXT("MyAIController %s has no controlled enemy when updating perception for %s."), *GetNameSafe(this), *GetNameSafe(UpdatedActor));
		return;
	}
}
