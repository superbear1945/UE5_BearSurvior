// 近战攻击组件实现文件。

#include "Component/MeleeAttackComponent.h"
#include "Component/HealthComponent.h"
#include "Engine/World.h"
#include "Engine.h"
#include "DrawDebugHelpers.h"

/**
 * 初始化组件默认配置。
 */
UMeleeAttackComponent::UMeleeAttackComponent()
{
	// 近战组件不依赖逐帧 Tick，命中检测由窗口开闭事件驱动。
	PrimaryComponentTick.bCanEverTick = false;

	// 配置默认值。
	BaseDamage = 25.0f;
	AttackRange = 200.0f;
	AttackRadius = 30.0f;
	DurabilityCostPerAttack = 1.0f;
	bCanHitMultipleTargets = false;
	TraceChannel = ECC_Visibility;

	// 状态默认值。
	bIsInAttackWindow = false;
}

/**
 * 开启攻击窗口，允许命中检测。
 * 通常由动画通知在攻击动画的伤害帧调用。
 */
void UMeleeAttackComponent::BeginAttackWindow()
{
	// 清空上一次挥击的命中记录，准备新的一轮检测。
	HitActorsThisSwing.Empty();
	bIsInAttackWindow = true;
}

/**
 * 关闭攻击窗口，重置命中记录。
 * 通常由动画通知在攻击动画结束时调用。
 */
void UMeleeAttackComponent::EndAttackWindow()
{
	bIsInAttackWindow = false;
	HitActorsThisSwing.Empty();
}

/**
 * 获取当前是否处于攻击窗口内。
 */
bool UMeleeAttackComponent::IsInAttackWindow() const
{
	return bIsInAttackWindow;
}

/**
 * 执行近战命中检测。
 * 从宿主 Actor 的根组件位置出发，沿宿主朝向执行球形扫描。
 */
void UMeleeAttackComponent::PerformHitDetection()
{
	if (!bIsInAttackWindow)
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	// 获取射线起始位置和方向。
	FVector TraceOrigin;
	FVector TraceDirection;
	GetTraceOrigin(TraceOrigin, TraceDirection);

	const FVector TraceEnd = TraceOrigin + TraceDirection * AttackRange;

	// 执行球形扫描（SphereTrace），检测攻击范围内的目标。
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Owner);

	// 球形扫描参数：起始、结束位置、球体半径、碰撞通道、查询参数。
	FCollisionShape SphereShape;
	SphereShape.SetSphere(AttackRadius);

	TArray<FHitResult> HitResults;
	const bool bHit = GetWorld()->SweepMultiByChannel(
		HitResults,
		TraceOrigin,
		TraceEnd,
		FQuat::Identity,
		TraceChannel,
		SphereShape,
		QueryParams
	);

	if (!bHit)
	{
		return;
	}

	// 遍历所有命中结果，对未命中过的目标施加伤害。
	for (const FHitResult& HitResult : HitResults)
	{
		AActor* HitActor = HitResult.GetActor();
		if (!HitActor)
		{
			continue;
		}

		// 跳过本次挥击已经命中过的目标（防止重复伤害）。
		if (HitActorsThisSwing.Contains(HitActor))
		{
			continue;
		}

		// 记录已命中目标。
		HitActorsThisSwing.Add(HitActor);

		// 施加伤害。
		const float FinalDamage = ApplyHitDamage(HitResult);

		// 广播命中事件。
		OnMeleeHit.Broadcast(HitActor, HitResult, FinalDamage);

		// 如果不允许命中多个目标，则在命中第一个目标后停止检测。
		if (!bCanHitMultipleTargets)
		{
			break;
		}
	}
}

/**
 * 对命中的目标施加伤害。
 * 优先查找目标上的 UHealthComponent 调用 ApplyDamage。
 * 若目标无 HealthComponent 则不施加伤害并提示。
 */
float UMeleeAttackComponent::ApplyHitDamage(const FHitResult& HitResult)
{
	AActor* HitActor = HitResult.GetActor();
	if (!HitActor)
	{
		return 0.0f;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return 0.0f;
	}

	// 优先查找目标上的 HealthComponent，使用项目自定义的伤害接口。
	UHealthComponent* HealthComp = HitActor->FindComponentByClass<UHealthComponent>();
	if (HealthComp)
	{
		return HealthComp->ApplyDamage(BaseDamage, Owner);
	}

	// 若目标无 HealthComponent，则不施加伤害并输出警告。
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Blue, FString::Printf(TEXT("目标 %s 无 HealthComponent，未施加伤害"), *HitActor->GetName()));
	return 0.0f;
}

/**
 * 获取命中检测的起始位置和方向。
 * 从宿主 Actor 的根组件位置出发，方向为宿主朝向。
 */
void UMeleeAttackComponent::GetTraceOrigin(FVector& OutOrigin, FVector& OutDirection) const
{
	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		OutOrigin = FVector::ZeroVector;
		OutDirection = FVector::ForwardVector;
		return;
	}

	// 射线从宿主根组件位置出发。
	OutOrigin = Owner->GetActorLocation();

	// 射线方向沿宿主朝向（X轴正方向）。
	OutDirection = Owner->GetActorForwardVector();
}
