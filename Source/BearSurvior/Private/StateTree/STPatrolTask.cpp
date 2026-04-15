// Copyright Epic Games, Inc. All Rights Reserved.

#include "STPatrolTask.h"
#include "EnemyBase.h"
#include "AIController.h"
#include "AITypes.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeExecutionTypes.h"

namespace
{
	/**
	 * 获取巡逻任务当前控制的角色。
	 */
	ACharacter* GetControlledCharacter(const FSTPatrolTaskInstanceData& InstanceData)
	{
		if (!InstanceData.Controller)
			return nullptr;

		return Cast<ACharacter>(InstanceData.Controller->GetPawn());
	}

	/**
	 * 发起一次随机巡逻移动请求。
	 */
	EStateTreeRunStatus StartPatrolMove(FSTPatrolTaskInstanceData& InstanceData)
	{
		if (!InstanceData.Controller)
			return EStateTreeRunStatus::Failed;

		APawn* ControlledPawn = InstanceData.Controller->GetPawn();
		if (!ControlledPawn)
			return EStateTreeRunStatus::Failed;

		UNavigationSystemV1* NavigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(ControlledPawn->GetWorld());
		if (!NavigationSystem)
			return EStateTreeRunStatus::Failed;

		FNavLocation PatrolLocation;
		if (!NavigationSystem->GetRandomReachablePointInRadius(InstanceData.PatrolCenterLocation, InstanceData.PatrolRadius, PatrolLocation))
			return EStateTreeRunStatus::Failed;

		InstanceData.CurrentPatrolLocation = PatrolLocation.Location;

		const EPathFollowingRequestResult::Type MoveResult = InstanceData.Controller->MoveToLocation(
			InstanceData.CurrentPatrolLocation,
			InstanceData.AcceptableRadius,
			true,
			true,
			true,
			true,
			nullptr,
			true);

		if (MoveResult == EPathFollowingRequestResult::Failed)
			return EStateTreeRunStatus::Failed;


		InstanceData.bHasActiveMoveRequest = MoveResult == EPathFollowingRequestResult::RequestSuccessful;
		return MoveResult == EPathFollowingRequestResult::AlreadyAtGoal
			? EStateTreeRunStatus::Succeeded
			: EStateTreeRunStatus::Running;
	}
}

EStateTreeRunStatus FSTPatrolTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// 仅在真正切入巡逻状态时初始化。
	if (Transition.ChangeType != EStateTreeStateChangeType::Changed)
		return EStateTreeRunStatus::Running;

	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	ACharacter* ControlledCharacter = GetControlledCharacter(InstanceData);
	if (!ControlledCharacter)
		return EStateTreeRunStatus::Failed;

	InstanceData.PatrolCenterLocation = ControlledCharacter->GetActorLocation();
	InstanceData.CurrentPatrolLocation = InstanceData.PatrolCenterLocation;
	InstanceData.bHasActiveMoveRequest = false;

	// 记录并设置巡逻速度，便于退出状态时恢复。
	InstanceData.PreviousSpeed = ControlledCharacter->GetCharacterMovement()->MaxWalkSpeed;
	ControlledCharacter->GetCharacterMovement()->MaxWalkSpeed = InstanceData.PatrolSpeed;

	// 设置敌人移动状态
	Cast<AEnemyBase>(InstanceData.Controller->GetPawn())->SwitchEnemyState(EEnemyState::Patrol);

	return StartPatrolMove(InstanceData);
}

EStateTreeRunStatus FSTPatrolTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	static_cast<void>(DeltaTime);

	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	ACharacter* ControlledCharacter = GetControlledCharacter(InstanceData);
	if (!ControlledCharacter || !InstanceData.Controller)
		return EStateTreeRunStatus::Failed;

	UPathFollowingComponent* PathFollowingComponent = InstanceData.Controller->GetPathFollowingComponent();
	if (!PathFollowingComponent)
		return EStateTreeRunStatus::Failed;

	if (!InstanceData.bHasActiveMoveRequest)
		return EStateTreeRunStatus::Failed;

	if (PathFollowingComponent->GetStatus() != EPathFollowingStatus::Idle)
		return EStateTreeRunStatus::Running;

	const float DistanceToTarget = FVector::Dist2D(ControlledCharacter->GetActorLocation(), InstanceData.CurrentPatrolLocation);
	if (DistanceToTarget <= InstanceData.AcceptableRadius)
		return EStateTreeRunStatus::Succeeded;

	return EStateTreeRunStatus::Failed;
}

void FSTPatrolTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// 仅在真正离开巡逻状态时清理。
	if (Transition.ChangeType != EStateTreeStateChangeType::Changed)
		return;

	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	ACharacter* ControlledCharacter = GetControlledCharacter(InstanceData);
	if (InstanceData.Controller)
		InstanceData.Controller->StopMovement();

	if (ControlledCharacter)
		ControlledCharacter->GetCharacterMovement()->MaxWalkSpeed = InstanceData.PreviousSpeed;

	InstanceData.bHasActiveMoveRequest = false;
}
#if WITH_EDITOR
FText FSTPatrolTask::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting /*= EStateTreeNodeFormatting::Text*/) const
{
	static_cast<void>(ID);
	static_cast<void>(InstanceDataView);
	static_cast<void>(BindingLookup);
	static_cast<void>(Formatting);

	return FText::FromString(TEXT("<b>Patrol</b>"));
}
#endif // WITH_EDITOR
