// 武器基类实现文件。
// 设计期数据从 DataTable 读取，运行时状态由本类管理。

#include "Weapon/WeaponBase.h"
#include "Component/MeleeAttackComponent.h"
#include "Component/RangeAttackComponent.h"
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

	// 缓存指针初始化。
	CachedItemData = nullptr;
	CachedMeleeData = nullptr;
	CachedRangedData = nullptr;
}

/**
 * 在游戏开始时初始化武器状态。
 * 解析 DataTable 行数据，同步物品属性，初始化运行时状态并通知攻击组件。
 */
void AWeaponBase::BeginPlay()
{
	Super::BeginPlay();

	ResolveWeaponData();
	SyncItemProperties();
	InitializeFromData();
	InitializeAttackComponents();
}

/**
 * 解析 WeaponDataRow 指向的 DataTable 行。
 * 根据 WeaponType 区分近战/远程，分别缓存到对应的指针。
 */
void AWeaponBase::ResolveWeaponData()
{
	if (!WeaponDataRow.DataTable || WeaponDataRow.RowName.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] WeaponDataRow 未配置或无效"), *GetNameSafe(this));
		return;
	}

	static const FString Context(TEXT("WeaponDataResolve"));

	if (WeaponType == EWeaponType::Ranged)
	{
		CachedRangedData = WeaponDataRow.DataTable->FindRow<FRangedWeaponData>(WeaponDataRow.RowName, Context);
		CachedItemData = CachedRangedData;
	}
	else
	{
		CachedMeleeData = WeaponDataRow.DataTable->FindRow<FMeleeWeaponData>(WeaponDataRow.RowName, Context);
		CachedItemData = CachedMeleeData;
	}

	if (!CachedItemData)
	{
		UE_LOG(LogTemp, Error, TEXT("[%s] 解析 DataTable 行失败，表: %s，行: %s"),
			*GetNameSafe(this),
			*GetNameSafe(WeaponDataRow.DataTable),
			*WeaponDataRow.RowName.ToString());
	}
}

/**
 * 将 DataTable 中的物品级数据同步到父类 AItemBase 字段。
 * 供背包UI、拾取提示等系统使用。
 */
void AWeaponBase::SyncItemProperties()
{
	if (!CachedItemData)
		return;

	ItemDisplayName = CachedItemData->DisplayName;
	ItemDescription = CachedItemData->Description;
	ItemIcon = CachedItemData->Icon.LoadSynchronous();
	Weight = CachedItemData->Weight;
	Rarity = CachedItemData->Rarity;

	// 加载武器网格到 ItemMesh 组件上。
	if (!CachedItemData->WeaponMesh.IsNull() && ItemMesh)
	{
		UStaticMesh* LoadedMesh = CachedItemData->WeaponMesh.LoadSynchronous();
		if (LoadedMesh)
			ItemMesh->SetStaticMesh(LoadedMesh);
	}
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
 */
void AWeaponBase::InitializeAttackComponents()
{
	if (WeaponType == EWeaponType::Ranged && CachedRangedData)
	{
		URangeAttackComponent* RangedComp = FindComponentByClass<URangeAttackComponent>();
		if (RangedComp)
			RangedComp->InitializeFromWeaponData(*CachedRangedData);
	}
	else if (WeaponType == EWeaponType::Melee && CachedMeleeData)
	{
		UMeleeAttackComponent* MeleeComp = FindComponentByClass<UMeleeAttackComponent>();
		if (MeleeComp)
			MeleeComp->InitializeFromWeaponData(*CachedMeleeData);
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
	const float Interval = CachedItemData ? CachedItemData->AttackInterval : 1.0f;
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

	return true;
}

/**
 * 停止攻击。重置攻击状态标记。
 */
void AWeaponBase::StopAttack()
{
	bIsAttacking = false;
}

/**
 * 消耗武器耐久度。
 * @param Cost 要消耗的耐久值。传入负值时使用 DataTable 中配置的 DurabilityCostPerAttack。
 */
float AWeaponBase::ConsumeDurability(float Cost)
{
	const float DefaultCost = CachedItemData ? CachedItemData->DurabilityCostPerAttack : 1.0f;
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
 * 返回基础伤害值（从 DataTable 读取）。
 */
float AWeaponBase::GetBaseDamage() const
{
	return CachedItemData ? CachedItemData->BaseDamage : 0.0f;
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

// 空物品数据静态实例，用于空指针兜底。
static const FItemData EmptyItemData;

/**
 * 返回缓存的物品公共数据引用。
 */
const FItemData& AWeaponBase::GetItemData() const
{
	return CachedItemData ? *CachedItemData : EmptyItemData;
}

/**
 * 返回缓存的具体武器数据引用。
 */
const FItemData& AWeaponBase::GetWeaponData() const
{
	return CachedItemData ? *CachedItemData : EmptyItemData;
}

/**
 * 判断武器数据是否正确加载。
 */
bool AWeaponBase::IsDataLoaded() const
{
	return CachedItemData != nullptr;
}

/**
 * 耐久耗尽时的处理逻辑，广播事件。
 */
void AWeaponBase::HandleDurabilityDepleted()
{
	OnDurabilityDepleted.Broadcast(this);
}
