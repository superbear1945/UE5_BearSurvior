// 武器基类实现文件。

#include "WeaponBase.h"
#include "Engine/World.h"

/**
 * 初始化武器默认属性。
 */
AWeaponBase::AWeaponBase()
{
	// 武器默认不可堆叠。
	MaxStackCount = 1;
	StackCount = 1;

	// 初始化武器数据。
	BaseDamage = 10.0f;
	AttackInterval = 1.0f;

	// 初始化耐久度。
	MaxDurability = 100.0f;
	CurrentDurability = MaxDurability;
	DurabilityCostPerAttack = 1.0f;

	// 初始化攻击状态。
	bIsAttacking = false;
	LastAttackTime = -1000.0f;
}

/**
 * 在游戏开始时初始化武器状态。
 */
void AWeaponBase::BeginPlay()
{
	Super::BeginPlay();

	// 确保当前耐久度不超过最大耐久度（防止蓝图中误设）。
	if (CurrentDurability > MaxDurability)
	{
		CurrentDurability = MaxDurability;
	}
}

/**
 * 判断武器当前是否可以执行攻击。
 * 检查项目：耐久度是否大于0、是否处于攻击状态、攻击间隔是否已过。
 */
bool AWeaponBase::CanAttack() const
{
	// 武器已损坏，无法攻击。
	if (CurrentDurability <= 0.0f)
	{
		return false;
	}

	// 正在攻击中，不可重复发起。
	if (bIsAttacking)
	{
		return false;
	}

	// 检查攻击间隔：距离上次攻击的时间是否足够。
	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastAttackTime < AttackInterval)
	{
		return false;
	}

	return true;
}

/**
 * 开始攻击。基类实现检查 CanAttack() 并更新攻击状态标记。
 * 子类应覆盖此函数，在调用 Super::StartAttack() 后实现具体攻击流程。
 * @return 是否成功发起攻击。
 */
bool AWeaponBase::StartAttack()
{
	if (!CanAttack())
	{
		return false;
	}

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
 * @param Cost 要消耗的耐久值。传入负值时使用默认的 DurabilityCostPerAttack。
 * @return 实际消耗的耐久值。
 */
float AWeaponBase::ConsumeDurability(float Cost)
{
	// 传入负值时使用默认消耗值。
	const float ActualCost = (Cost >= 0.0f) ? Cost : DurabilityCostPerAttack;

	if (ActualCost <= 0.0f)
	{
		return 0.0f;
	}

	const float OldDurability = CurrentDurability;
	CurrentDurability = FMath::Max(0.0f, CurrentDurability - ActualCost);
	const float ActualConsumed = OldDurability - CurrentDurability;

	// 耐久度耗尽时触发处理逻辑。
	if (CurrentDurability <= 0.0f)
	{
		HandleDurabilityDepleted();
	}

	return ActualConsumed;
}

/**
 * 修复武器耐久度。
 * @param Amount 要修复的耐久值。
 * @return 实际修复的耐久值。
 */
float AWeaponBase::RepairDurability(float Amount)
{
	if (Amount <= 0.0f)
	{
		return 0.0f;
	}

	const float OldDurability = CurrentDurability;
	CurrentDurability = FMath::Min(MaxDurability, CurrentDurability + Amount);
	return CurrentDurability - OldDurability;
}

/**
 * 返回当前耐久度百分比。
 */
float AWeaponBase::GetDurabilityPercent() const
{
	if (MaxDurability <= 0.0f)
	{
		return 0.0f;
	}
	return FMath::Clamp(CurrentDurability / MaxDurability, 0.0f, 1.0f);
}

/**
 * 返回当前耐久度。
 */
float AWeaponBase::GetCurrentDurability() const
{
	return CurrentDurability;
}

/**
 * 返回最大耐久度。
 */
float AWeaponBase::GetMaxDurability() const
{
	return MaxDurability;
}

/**
 * 返回基础伤害值。
 */
float AWeaponBase::GetBaseDamage() const
{
	return BaseDamage;
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
 * 耐久耗尽时的处理逻辑，广播事件。
 */
void AWeaponBase::HandleDurabilityDepleted()
{
	OnDurabilityDepleted.Broadcast(this);
}
