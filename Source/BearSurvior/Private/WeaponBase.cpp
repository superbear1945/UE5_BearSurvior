// 武器基类实现文件。
// 物品公共数据由 AItemBase 解析，武器专属数据由各攻击组件从自身 DataRow 独立解析。

#include "Weapon/WeaponBase.h"
#include "Component/MeleeAttackComponent.h"
#include "Component/RangeAttackComponent.h"
#include "Engine/Engine.h"
#include "Logging/LogMacros.h"
#include "Engine/World.h"

/**
 * 初始化武器默认属性。
 * 设计期数值由 DataTable 提供，构造函数仅初始化运行时状态标记。
 */
AWeaponBase::AWeaponBase()
{
	// 武器默认不可堆叠。
	MaxStackCount = 1;
	StackCount = 1;

	// 运行时状态初始化。
	bIsAttacking = false;
	LastAttackTime = -1000.0f;
	CurrentDurability = 0.0f;

	// 武器专属数据缓存指针初始化。
	CachedMeleeData = nullptr;
	CachedRangedData = nullptr;
}

/**
 * 在游戏开始时初始化武器状态。
 * 先让 AItemBase 解析公共物品数据，再解析武器专属数据并通知攻击组件。
 */
void AWeaponBase::BeginPlay()
{
	Super::BeginPlay();

	ResolveWeaponData();
	InitializeFromData();
	InitializeAttackComponents();
}

/**
 * 解析 WeaponDataRow 指向的武器专属 DataTable 行。
 * ItemDataRow 已由 AItemBase 处理，这里只根据 WeaponType 缓存近战或远程数据。
 */
void AWeaponBase::ResolveWeaponData()
{
	if (!WeaponDataRow.DataTable || WeaponDataRow.RowName.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] WeaponDataRow 未配置或无效"), *GetNameSafe(this));
		return;
	}

	static const FString Context(TEXT("WeaponDataResolve"));

	// if (WeaponType == EWeaponType::Ranged)
	// {
	// 	CachedRangedData = WeaponDataRow.DataTable->FindRow<FRangedWeaponData>(WeaponDataRow.RowName, Context);
	// }
	// else
	// {
	// 	CachedMeleeData = WeaponDataRow.DataTable->FindRow<FMeleeWeaponData>(WeaponDataRow.RowName, Context);
	// }

	// if (!CachedMeleeData && !CachedRangedData)
	// {
	// 	UE_LOG(LogTemp, Error, TEXT("[%s] 解析 DataTable 行失败，表: %s，行: %s"),
	// 		*GetNameSafe(this),
	// 		*GetNameSafe(WeaponDataRow.DataTable),
	// 		*WeaponDataRow.RowName.ToString());
	// }
}

/**
 * 使用已解析的 DataTable 数据初始化运行时状态。
 */
void AWeaponBase::InitializeFromData()
{
	if (!CachedItemData)
		return;

	CurrentDurability = CachedItemData->MaxDurability;
}

/**
 * 通知挂载的攻击组件使用 DataTable 数据进行初始化。
 * 组件各自从自身的 DataRow 引用解析数据，不再由 WeaponBase 传递。
 */
void AWeaponBase::InitializeAttackComponents()
{
	if (WeaponType == EWeaponType::Ranged)
	{
		URangeAttackComponent* RangedComp = FindComponentByClass<URangeAttackComponent>();
		if (RangedComp)
			RangedComp->ResolveWeaponData();
	}
	else
	{
		UMeleeAttackComponent* MeleeComp = FindComponentByClass<UMeleeAttackComponent>();
		if (MeleeComp)
			MeleeComp->ResolveWeaponData();
	}
}

/**
 * 判断武器当前是否可以执行攻击。
 */
bool AWeaponBase::CanAttack() const
{
	if (CurrentDurability <= 0.0f)
		return false;

	if (bIsAttacking)
		return false;

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	const float Interval = GetAttackInterval();
	if (CurrentTime - LastAttackTime < Interval)
		return false;

	return true;
}

/**
 * 开始攻击。基类实现检查 CanAttack() 并更新攻击状态标记。
 */
bool AWeaponBase::StartAttack()
{
	if (!CanAttack())
		return false;

	bIsAttacking = true;
	LastAttackTime = GetWorld()->GetTimeSeconds();

	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Blue, FString::Printf(TEXT("StartAttack调用")));

	return true;
}

/**
 * 停止攻击。重置攻击状态标记。
 */
void AWeaponBase::StopAttack()
{
	bIsAttacking = false;

	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Blue, FString::Printf(TEXT("StopAttack调用")));
}

/**
 * 消耗武器耐久度。
 * @param Cost 要消耗的耐久值。传入负值时使用当前武器专属 DataTable 中配置的耐久消耗。
 */
float AWeaponBase::ConsumeDurability(float Cost)
{
	const float DefaultCost = GetDefaultDurabilityCost();
	const float ActualCost = (Cost >= 0.0f) ? Cost : DefaultCost;

	if (ActualCost <= 0.0f)
		return 0.0f;

	const float OldDurability = CurrentDurability;
	CurrentDurability = FMath::Max(0.0f, CurrentDurability - ActualCost);
	const float ActualConsumed = OldDurability - CurrentDurability;

	if (CurrentDurability <= 0.0f)
		HandleDurabilityDepleted();

	return ActualConsumed;
}

/**
 * 修复武器耐久度。
 */
float AWeaponBase::RepairDurability(float Amount)
{
	if (Amount <= 0.0f)
		return 0.0f;

	const float MaxDur = GetMaxDurability();
	const float OldDurability = CurrentDurability;
	CurrentDurability = FMath::Min(MaxDur, CurrentDurability + Amount);
	return CurrentDurability - OldDurability;
}

/**
 * 返回当前耐久度百分比。
 */
float AWeaponBase::GetDurabilityPercent() const
{
	const float MaxDur = GetMaxDurability();
	if (MaxDur <= 0.0f)
		return 0.0f;
	return FMath::Clamp(CurrentDurability / MaxDur, 0.0f, 1.0f);
}

/**
 * 返回当前耐久度。
 */
float AWeaponBase::GetCurrentDurability() const
{
	return CurrentDurability;
}

/**
 * 返回最大耐久度（从 DataTable 读取）。
 */
float AWeaponBase::GetMaxDurability() const
{
	return CachedItemData ? CachedItemData->MaxDurability : 0.0f;
}

/**
 * 返回基础伤害值（从当前武器专属 DataTable 读取）。
 */
float AWeaponBase::GetBaseDamage() const
{
	if (WeaponType == EWeaponType::Ranged && CachedRangedData)
		return CachedRangedData->BaseDamage;

	if (WeaponType == EWeaponType::Melee && CachedMeleeData)
		return CachedMeleeData->BaseDamage;

	return 0.0f;
}

/**
 * 返回攻击间隔（从当前武器专属 DataTable 读取）。
 */
float AWeaponBase::GetAttackInterval() const
{
	if (WeaponType == EWeaponType::Ranged && CachedRangedData)
		return CachedRangedData->AttackInterval;

	if (WeaponType == EWeaponType::Melee && CachedMeleeData)
		return CachedMeleeData->AttackInterval;

	return 1.0f;
}

/**
 * 返回默认耐久消耗（从当前武器专属 DataTable 读取）。
 */
float AWeaponBase::GetDefaultDurabilityCost() const
{
	if (WeaponType == EWeaponType::Ranged && CachedRangedData)
		return CachedRangedData->DurabilityCostPerShot;

	if (WeaponType == EWeaponType::Melee && CachedMeleeData)
		return CachedMeleeData->DurabilityCostPerAttack;

	return 1.0f;
}

/**
 * 返回武器是否已损坏。
 */
bool AWeaponBase::IsBroken() const
{
	return CurrentDurability <= 0.0f;
}

/**
 * 返回武器是否正在攻击中。
 */
bool AWeaponBase::IsAttacking() const
{
	return bIsAttacking;
}

// 空武器数据静态实例，用于空指针兜底。
static const FMeleeWeaponData EmptyMeleeWeaponData;
static const FRangedWeaponData EmptyRangedWeaponData;

/**
 * 返回缓存的近战武器数据引用。
 */
const FMeleeWeaponData& AWeaponBase::GetMeleeWeaponData() const
{
	return CachedMeleeData ? *CachedMeleeData : EmptyMeleeWeaponData;
}

/**
 * 返回缓存的远程武器数据引用。
 */
const FRangedWeaponData& AWeaponBase::GetRangedWeaponData() const
{
	return CachedRangedData ? *CachedRangedData : EmptyRangedWeaponData;
}

/**
 * 判断武器数据是否正确加载。
 */
bool AWeaponBase::IsDataLoaded() const
{
	const bool bItemLoaded = CachedItemData != nullptr;
	const bool bWeaponLoaded = (WeaponType == EWeaponType::Ranged) ? (CachedRangedData != nullptr) : (CachedMeleeData != nullptr);
	return bItemLoaded && bWeaponLoaded;
}

/**
 * 耐久耗尽时的处理逻辑，广播事件。
 */
void AWeaponBase::HandleDurabilityDepleted()
{
	OnDurabilityDepleted.Broadcast(this);
}

/**
 * 主要使用开始：武器默认在左键按下时尝试进入攻击状态。
 * AimLocation / AimDirection 由角色视角提供，子类可在覆盖时用于射线或弹道计算。
 */
void AWeaponBase::PrimaryUseStart_Implementation(const FVector& AimLocation, const FVector& AimDirection)
{
	StartAttack();
}

/**
 * 主要使用结束：武器默认在左键松开时停止攻击状态。
 */
void AWeaponBase::PrimaryUseEnd_Implementation()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Blue, FString::Printf(TEXT("停止攻击")));

	StopAttack();
}

/**
 * 次要使用开始：基类默认不处理右键按下，留给瞄准、格挡或互动型物品覆盖。
 */
void AWeaponBase::SecondaryUseStart_Implementation()
{
	// 默认没有次要使用开始行为，子类可覆盖实现。
}

/**
 * 次要使用结束：基类默认不处理右键松开，留给持续次要行为覆盖。
 */
void AWeaponBase::SecondaryUseEnd_Implementation()
{
	// 默认没有次要使用结束行为，子类可覆盖实现。
}
