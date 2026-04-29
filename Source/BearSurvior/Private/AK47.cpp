// AK47 武器实现文件。武器属性由 DataTable 配置，本类提供 AK47 专属逻辑的扩展点。

#include "Weapon/AK47.h"
#include "Component/RangeAttackComponent.h"

/**
 * 构造函数，初始化 AK47 的默认运行参数。
 */
AAK47::AAK47()
{
	WeaponType = EWeaponType::Ranged;
}

/**
 * 判断 AK47 当前是否可以执行攻击。
 */
bool AAK47::CanAttack() const
{
	return Super::CanAttack();
}

/**
 * 开始执行 AK47 攻击。
 */
bool AAK47::StartAttack()
{
	return Super::StartAttack();
}

/**
 * 停止 AK47 攻击。
 */
void AAK47::StopAttack()
{
	Super::StopAttack();
}

/**
 * 消耗 AK47 的耐久度。
 */
float AAK47::ConsumeDurability(float Cost)
{
	return Super::ConsumeDurability(Cost);
}

/**
 * 修复 AK47 的耐久度。
 */
float AAK47::RepairDurability(float Amount)
{
	return Super::RepairDurability(Amount);
}

/**
 * 在游戏开始时初始化 AK47 状态。
 */
void AAK47::BeginPlay()
{
	Super::BeginPlay();
}

/**
 * 每帧更新 AK47 的运行状态。
 */
void AAK47::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

/**
 * 处理 AK47 耐久耗尽时的逻辑。
 */
void AAK47::HandleDurabilityDepleted()
{
	Super::HandleDurabilityDepleted();
}
