// 近战攻击组件实现文件。
// 设计期数据由独立 FMeleeWeaponData 提供，通过 InitializeFromWeaponData 设置。

#include "Component/MeleeAttackComponent.h"
#include "Component/HealthComponent.h"
#include "Weapon/WeaponDataTypes.h"
#include "Engine/World.h"
#include "Engine.h"

/**
 * 初始化组件默认配置。
 * 设计期数值由 InitializeFromWeaponData 在 BeginPlay 时设置。
 */
UMeleeAttackComponent::UMeleeAttackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// 配置默认值（防止 DataTable 未加载时引用未初始化数据）。
	CachedBaseDamage = 25.0f;
	CachedAttackInterval = 1.0f;
	CachedAttackRange = 200.0f;
	CachedAttackRadius = 30.0f;
	CachedDurabilityCostPerAttack = 1.0f;
	bCachedCanHitMultipleTargets = false;
	TraceChannel = ECC_Visibility;

	// 数据缓存指针初始化。
	CachedMeleeWeaponData = nullptr;

	// 运行时状态默认值。
	bIsInAttackWindow = false;
}

/**
 * 从独立近战武器 DataTable 行数据初始化组件配置。
 * 由宿主 AWeaponBase::InitializeAttackComponents 在 BeginPlay 中调用。
 */
void UMeleeAttackComponent::InitializeFromWeaponData(const FMeleeWeaponData& Data)
{
	CachedBaseDamage = Data.BaseDamage;
	CachedAttackInterval = Data.AttackInterval;
	CachedAttackRange = Data.AttackRange;
	CachedAttackRadius = Data.AttackRadius;
	CachedDurabilityCostPerAttack = Data.DurabilityCostPerAttack;
	bCachedCanHitMultipleTargets = Data.bCanHitMultipleTargets;
}

/**
 * 解析 MeleeWeaponDataRow 指向的近战武器 DataTable 行，并缓存到 CachedMeleeWeaponData。
 * 数据无效时保留构造函数中的默认值，避免影响未接入 DataTable 的测试武器。
 */
void UMeleeAttackComponent::ResolveWeaponData()
{
	if (!MeleeWeaponDataRow.DataTable || MeleeWeaponDataRow.RowName.IsNone())
		return;

	static const FString Context(TEXT("MeleeWeaponDataResolve"));
	CachedMeleeWeaponData = MeleeWeaponDataRow.DataTable->FindRow<FMeleeWeaponData>(MeleeWeaponDataRow.RowName, Context);

	if (!CachedMeleeWeaponData)
	{
		UE_LOG(LogTemp, Error, TEXT("[%s] 解析 MeleeWeaponDataRow 失败，表: %s，行: %s"),
			*GetNameSafe(this),
			*GetNameSafe(MeleeWeaponDataRow.DataTable),
			*MeleeWeaponDataRow.RowName.ToString());
		return;
	}

	// 将解析出的数据同步到组件缓存字段。
	InitializeFromWeaponData(*CachedMeleeWeaponData);
}

// 空近战武器数据静态实例，用于空指针兜底。
static const FMeleeWeaponData EmptyMeleeWeaponData;

/**
 * 返回缓存的近战武器数据引用。
 */
const FMeleeWeaponData& UMeleeAttackComponent::GetMeleeWeaponData() const
{
	return CachedMeleeWeaponData ? *CachedMeleeWeaponData : EmptyMeleeWeaponData;
}

/**
 * 开始近战攻击。
 * 当前阶段没有动画通知接入时，先开启攻击窗口并立即执行一次检测，后续可由动画通知拆分窗口与检测时机。
 */
bool UMeleeAttackComponent::StartAttack(const FVector& AimLocation, const FVector& AimDirection)
{
	if (!CanAttack())
		return false;

	MarkFirstAttackTime();
	BeginAttackWindow();
	ExecuteAttack();
	return true;
}

/**
 * 结束近战攻击。
 * 松开输入或攻击流程结束时关闭窗口，避免后续检测沿用旧命中记录。
 */
void UMeleeAttackComponent::StopAttack()
{
	EndAttackWindow();
}

/**
 * 判断近战组件当前是否可以发起攻击。
 */
bool UMeleeAttackComponent::CanAttack() const
{
	if (!Super::CanAttack())
		return false;

	return GetOwner() != nullptr;
}

/**
 * 返回当前近战组件管理的攻击间隔。
 */
float UMeleeAttackComponent::GetAttackInterval() const
{
	return CachedAttackInterval;
}

/**
 * 返回当前近战组件管理的基础伤害。
 */
float UMeleeAttackComponent::GetBaseDamage() const
{
	return CachedBaseDamage;
}

/**
 * 返回当前近战组件管理的默认耐久消耗。
 */
float UMeleeAttackComponent::GetDefaultDurabilityCost() const
{
	return CachedDurabilityCostPerAttack;
}

/**
 * 返回当前近战组件是否已经成功加载 DataTable 数据。
 */
bool UMeleeAttackComponent::IsDataLoaded() const
{
	return CachedMeleeWeaponData != nullptr;
}

/**
 * 开启攻击窗口，允许命中检测。
 */
void UMeleeAttackComponent::BeginAttackWindow()
{
	HitActorsThisSwing.Empty();
	bIsInAttackWindow = true;
}

/**
 * 关闭攻击窗口，重置命中记录。
 */
void UMeleeAttackComponent::EndAttackWindow()
{
	bIsInAttackWindow = false;
	HitActorsThisSwing.Empty();
}

/**
 * 主动执行一次近战攻击检测。
 */
void UMeleeAttackComponent::ExecuteAttack()
{
	const bool bWasInAttackWindow = bIsInAttackWindow;

	if (!bWasInAttackWindow)
		HitActorsThisSwing.Empty();

	PerformHitDetection();

	if (!bWasInAttackWindow)
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
 */
void UMeleeAttackComponent::PerformHitDetection()
{
	AActor* Owner = GetOwner();
	if (!Owner)
		return;

	FVector TraceOrigin;
	FVector TraceDirection;
	GetTraceOrigin(TraceOrigin, TraceDirection);

	const FVector TraceEnd = TraceOrigin + TraceDirection * CachedAttackRange;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Owner);

	FCollisionShape SphereShape;
	SphereShape.SetSphere(CachedAttackRadius);

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
		return;

	for (const FHitResult& HitResult : HitResults)
	{
		AActor* HitActor = HitResult.GetActor();
		if (!HitActor)
			continue;

		if (HitActorsThisSwing.Contains(HitActor))
			continue;

		HitActorsThisSwing.Add(HitActor);

		const float FinalDamage = ApplyHitDamage(HitResult);
		OnMeleeHit.Broadcast(HitActor, HitResult, FinalDamage);

		if (!bCachedCanHitMultipleTargets)
			break;
	}
}

/**
 * 对命中的目标施加伤害。
 * 伤害值从组件的缓存数据获取。
 */
float UMeleeAttackComponent::ApplyHitDamage(const FHitResult& HitResult)
{
	AActor* HitActor = HitResult.GetActor();
	if (!HitActor)
		return 0.0f;

	AActor* Owner = GetOwner();
	if (!Owner)
		return 0.0f;

	UHealthComponent* HealthComp = HitActor->FindComponentByClass<UHealthComponent>();
	if (HealthComp)
		return HealthComp->ApplyDamage(CachedBaseDamage, Owner);

	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Blue, FString::Printf(TEXT("目标 %s 无 HealthComponent，未施加伤害"), *HitActor->GetName()));
	return 0.0f;
}

/**
 * 获取命中检测的起始位置和方向。
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

	OutOrigin = Owner->GetActorLocation();
	OutDirection = Owner->GetActorForwardVector();
}
