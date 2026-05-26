// 武器基类实现文件。
// 物品公共数据由 AItemBase 解析，武器专属数据与攻击节奏由各攻击组件从自身 DataRow 独立解析。

#include "Weapon/WeaponBase.h"
#include "Component/AttackComponentBase.h"
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
	CurrentDurability = 0.0f;

	// 攻击组件缓存指针初始化。
	ActiveAttackComponent = nullptr;
}

/**
 * 在游戏开始时初始化武器状态。
 * 先让 AItemBase 解析公共物品数据，再初始化 WeaponBase 公共状态并通知攻击组件解析自身数据。
 */
void AWeaponBase::BeginPlay()
{
	Super::BeginPlay();

	InitializeFromData();
	InitializeAttackComponents();
}

/**
 * 使用物品公共数据初始化运行时状态。
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
	ActiveAttackComponent = FindComponentByClass<UAttackComponentBase>();
	if (!ActiveAttackComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] 未找到 AttackComponentBase 派生攻击组件，武器无法执行攻击"), *GetNameSafe(this));
		return;
	}

	// 攻击组件各自解析自己的 DataRow，WeaponBase 不再关心近战或远程的具体数据结构。
	ActiveAttackComponent->ResolveWeaponData();
}

/**
 * 判断武器当前是否可以执行攻击。
 * WeaponBase 只检查公共耐久和输入状态，攻击节奏交给攻击组件自身判断。
 */
bool AWeaponBase::CanAttack() const
{
	if (CurrentDurability <= 0.0f)
		return false;

	if (bIsAttacking)
		return false;

	return true;
}

/**
 * 开始攻击。检查武器公共状态后，将瞄准信息转发给当前攻击组件。
 */
bool AWeaponBase::StartAttack(const FVector& AimLocation, const FVector& AimDirection)
{
	if (!CanAttack())
		return false;

	if (!ActiveAttackComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] StartAttack 失败：未缓存攻击组件"), *GetNameSafe(this));
		return false;
	}

	if (!ActiveAttackComponent->CanAttack())
		return false;

	// 转发攻击事件给攻击组件
	if (!ActiveAttackComponent->StartAttack(AimLocation, AimDirection))
		return false;

	bIsAttacking = true;

	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Blue, FString::Printf(TEXT("StartAttack调用")));

	return true;
}

/**
 * 停止攻击。重置攻击状态标记。
 */
void AWeaponBase::StopAttack()
{
	if (ActiveAttackComponent)
		ActiveAttackComponent->StopAttack();

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
 * 返回基础伤害值（由当前攻击组件提供）。
 */
float AWeaponBase::GetBaseDamage() const
{
	return ActiveAttackComponent ? ActiveAttackComponent->GetBaseDamage() : 0.0f;
}

/**
 * 返回攻击间隔（由当前攻击组件提供）。
 */
float AWeaponBase::GetAttackInterval() const
{
	return ActiveAttackComponent ? ActiveAttackComponent->GetAttackInterval() : 0.0f;
}

/**
 * 返回默认耐久消耗（由当前攻击组件提供）。
 */
float AWeaponBase::GetDefaultDurabilityCost() const
{
	return ActiveAttackComponent ? ActiveAttackComponent->GetDefaultDurabilityCost() : 0.0f;
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

/**
 * 判断物品公共数据与攻击组件数据是否正确加载。
 */
bool AWeaponBase::IsDataLoaded() const
{
	const bool bItemLoaded = CachedItemData != nullptr;
	const bool bWeaponLoaded = ActiveAttackComponent != nullptr && ActiveAttackComponent->IsDataLoaded();
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
	StartAttack(AimLocation, AimDirection);
}

/**
 * 主要使用结束：武器默认在左键松开时停止攻击状态。
 */
void AWeaponBase::PrimaryUseEnd_Implementation()
{
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
