// 攻击组件抽象基类实现文件。
// 默认实现保持安全空行为，派生类按自身攻击类型覆盖具体逻辑。

#include "Component/AttackComponentBase.h"
#include "Engine/World.h"

/**
 * 初始化攻击组件默认属性。
 */
UAttackComponentBase::UAttackComponentBase()
{
	PrimaryComponentTick.bCanEverTick = false;
	LastAttackTime = -1000.0f;
}

void UAttackComponentBase::BeginPlay()
{
	Super::BeginPlay();
	ResolveWeaponData();
}

/**
 * 默认开始攻击实现。
 * 抽象基类不执行具体攻击，仅返回 false 表示未处理。
 */
void UAttackComponentBase::OnEquip(ACharacter* CharacterOwner)
{
	
}

/**
 * 默认卸下装备实现。
 * 抽象基类不维护额外装备状态，因此保持空行为。
 */
void UAttackComponentBase::OnUnEquip(ACharacter* CharacterOwner)
{
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
	const float AttackInterval = FMath::Max(0.0f, GetAttackInterval());
	// 小于0默认为无攻击间隔
	if (AttackInterval <= 0.0f)
		return true;

	const UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] CanAttack 判断失败：GetWorld失败，无法获取世界对象"), *GetNameSafe(this));	
		return true;
	}

	if (World->GetTimeSeconds() - LastAttackTime < AttackInterval)
		return false;

	return true;
}

/**
 * 返回当前攻击组件管理的攻击间隔。
 * 抽象基类默认无冷却，派生类按自身数据覆盖。
 */
float UAttackComponentBase::GetAttackInterval() const
{
	return 0.0f;
}

/**
 * 返回当前攻击组件管理的基础伤害。
 * 抽象基类默认返回 0，派生类按自身数据覆盖。
 */
float UAttackComponentBase::GetBaseDamage() const
{
	return 0.0f;
}

/**
 * 返回当前攻击组件管理的默认耐久消耗。
 * 抽象基类默认返回 0，派生类按自身数据覆盖。
 */
float UAttackComponentBase::GetDefaultDurabilityCost() const
{
	return 0.0f;
}

/**
 * 返回当前攻击组件是否已经正确加载自身数据。
 * 抽象基类默认未加载，派生类按自身缓存状态覆盖。
 */
bool UAttackComponentBase::IsDataLoaded() const
{
	return false;
}

/**
 * 默认数据解析实现。
 * 抽象基类没有数据资产，具体组件应覆盖并从 Owner 的 ItemDataAsset 中解析。
 */
void UAttackComponentBase::ResolveWeaponData()
{
}

/**
	 * 标记本轮首次攻击输入成功开始的时间戳。
 */
void UAttackComponentBase::MarkFirstAttackTime()
{
	UWorld* World = GetWorld();
	if (!World)
		return;

	LastAttackTime = World->GetTimeSeconds();
}
