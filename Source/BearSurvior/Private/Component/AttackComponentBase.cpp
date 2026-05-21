// 攻击组件抽象基类实现文件。
// 默认实现保持安全空行为，派生类按自身攻击类型覆盖具体逻辑。

#include "Component/AttackComponentBase.h"

/**
 * 初始化攻击组件默认属性。
 */
UAttackComponentBase::UAttackComponentBase()
{
	PrimaryComponentTick.bCanEverTick = false;
}

/**
 * 默认开始攻击实现。
 * 抽象基类不执行具体攻击，仅返回 false 表示未处理。
 */
bool UAttackComponentBase::StartAttack(const FVector& AimLocation, const FVector& AimDirection)
{
	return false;
}

/**
 * 默认结束攻击实现。
 * 抽象基类没有持续状态，因此保持空行为。
 */
void UAttackComponentBase::StopAttack()
{
}

/**
 * 默认可攻击判断。
 * 抽象基类无法确认具体条件，因此默认允许，具体组件应覆盖。
 */
bool UAttackComponentBase::CanAttack() const
{
	return true;
}

/**
 * 默认数据解析实现。
 * 抽象基类没有数据行，具体组件应覆盖并解析自身 DataRow。
 */
void UAttackComponentBase::ResolveWeaponData()
{
}
