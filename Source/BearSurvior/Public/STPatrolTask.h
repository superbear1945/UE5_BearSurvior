// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * StateTree 巡逻任务节点：负责为敌人选择一个随机巡逻点并移动到目标位置。
 */

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "STPatrolTask.generated.h"

class AAIController;

/**
 * 巡逻任务实例数据：保存巡逻参数与一次巡逻过程中的运行时状态。
 */
USTRUCT()
struct FSTPatrolTaskInstanceData
{
	GENERATED_BODY()

	/** 执行巡逻任务的 AI 控制器。 */
	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AAIController> Controller = nullptr;

	/** 随机巡逻点搜索半径。 */
	UPROPERTY(EditAnywhere, Category = Parameter, meta = (ClampMin = 0, Units = "cm"))
	float PatrolRadius = 1200.0f;

	/** 判定到达巡逻点的接受半径。 */
	UPROPERTY(EditAnywhere, Category = Parameter, meta = (ClampMin = 0, Units = "cm"))
	float AcceptableRadius = 75.0f;

	/** 巡逻移动速度。 */
	UPROPERTY(EditAnywhere, Category = Parameter, meta = (ClampMin = 0, ClampMax = 2000, Units = "cm/s"))
	float PatrolSpeed = 300.0f;

	/** 本次巡逻开始时记录的巡逻中心点。 */
	UPROPERTY(VisibleAnywhere, Category = Output)
	FVector PatrolCenterLocation = FVector::ZeroVector;

	/** 当前随机到的巡逻目标点。 */
	UPROPERTY(VisibleAnywhere, Category = Output)
	FVector CurrentPatrolLocation = FVector::ZeroVector;

	/** 退出巡逻状态时需要恢复的移动速度。 */
	UPROPERTY(VisibleAnywhere, Category = Output)
	float PreviousSpeed = 0.0f;

	/** 当前是否已成功发起移动请求。 */
	UPROPERTY(VisibleAnywhere, Category = Output)
	bool bHasActiveMoveRequest = false;
};

/**
 * StateTree 巡逻任务：选择一个导航可达的随机点并在到达后返回成功。
 */
USTRUCT(meta = (DisplayName = "Patrol", Category = "AI"))
struct FSTPatrolTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	/** 声明当前任务使用的实例数据结构。 */
	using FInstanceDataType = FSTPatrolTaskInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	/** 进入巡逻状态时初始化数据并发起移动。 */
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

	/** 巡逻状态激活期间持续检测是否已经到达巡逻点。 */
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

	/** 退出巡逻状态时停止移动并恢复角色速度。 */
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	/** 返回编辑器中显示的任务描述。 */
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};
