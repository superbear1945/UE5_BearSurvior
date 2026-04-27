// AK47 武器类实现文件。当前提供与武器基类一致的默认行为，后续可在此扩展 AK47 的专属攻击逻辑。

#include "Weapon/AK47.h"

/**
 * 构造函数，初始化 AK47 的默认运行参数。
 */
AAK47::AAK47()
{
	PrimaryActorTick.bCanEverTick = true;
}

/**
 * 判断 AK47 当前是否可以执行攻击。
 * 当前沿用武器基类的通用判定逻辑。
 */
bool AAK47::CanAttack() const
{
	return Super::CanAttack();
}

/**
 * 开始执行 AK47 攻击。
 * 当前沿用武器基类的通用攻击开始逻辑。
 */
bool AAK47::StartAttack()
{
	return Super::StartAttack();
}

/**
 * 停止 AK47 攻击。
 * 当前沿用武器基类的通用攻击结束逻辑。
 */
void AAK47::StopAttack()
{
	Super::StopAttack();
}

/**
 * 消耗 AK47 的耐久度。
 * 当前沿用武器基类的通用耐久消耗逻辑。
 */
float AAK47::ConsumeDurability(float Cost)
{
	return Super::ConsumeDurability(Cost);
}

/**
 * 修复 AK47 的耐久度。
 * 当前沿用武器基类的通用耐久修复逻辑。
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
 * 每帧更新 AK47 的运行逻辑。
 */
void AAK47::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

/**
 * 处理 AK47 耐久耗尽时的逻辑。
 * 当前沿用武器基类的默认耗尽处理流程。
 */
void AAK47::HandleDurabilityDepleted()
{
	Super::HandleDurabilityDepleted();
}
