#include "MyAIController.h"
#include "BearSurviorCharacter.h"
#include "Components/StateTreeComponent.h"
#include "Misc/AssertionMacros.h"
#include "Engine/Engine.h"
#include "StateTreeReference.h"
#include "StructUtils/PropertyBag.h"

AMyAIController::AMyAIController()
{
	// 创建并持有 StateTree AI 组件，供后续行为树状态逻辑使用。
	StateTree = CreateDefaultSubobject<UStateTreeComponent>(TEXT("StateTree"));
	check(StateTree);
	// 创建并持有AI感知组件，供后续感知相关逻辑使用。
	AIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComponent"));
	check(AIPerceptionComponent);

	// 在控制器接管 Pawn 后自动启动 AI 逻辑。
	bStartAILogicOnPossess = true;

	// 将控制器附着到 Pawn，避免部分 AI/查询逻辑缺少宿主引用。
	bAttachToPawn = true;
}

void AMyAIController::BeginPlay()
{
	Super::BeginPlay();

	// 注册感知更新事件，确保 AI 在发现目标时能够立即收到回调。
	// check(AIPerceptionComponent);
	PerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &AMyAIController::OnSeeingPlayer);

	PerceptionComponent->OnTargetPerceptionForgotten.AddDynamic(this, &AMyAIController::OnForgetPlayer);
}

void AMyAIController::OnForgetPlayer(AActor* UpdatedActor)
{
	ABearSurviorCharacter *PlayerCharacter = Cast<ABearSurviorCharacter>(UpdatedActor);
	// 如果忘记的不是玩家则直接返回，避免误触发 AI 行为。
	if (PlayerCharacter == nullptr)
		return;

	// 设置对应状态变量
	bIsSeeingPlayer = false;
}

void AMyAIController::OnSeeingPlayer(AActor* UpdatedActor, FAIStimulus Stimulus)
{
	ABearSurviorCharacter *PlayerCharacter = Cast<ABearSurviorCharacter>(UpdatedActor);
	// 如果看到的不是玩家则直接返回，避免误触发 AI 行为。
	if (PlayerCharacter == nullptr)
		return;
		
	// 设置对应状态变量
	bIsSeeingPlayer = Stimulus.WasSuccessfullySensed();
	TargetActor = UpdatedActor;
}
