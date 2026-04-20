#include "Component/HealthComponent.h"

#include "GameFramework/Actor.h"

UHealthComponent::UHealthComponent()
{
	// 生命值组件采用事件驱动即可，不需要逐帧 Tick。
	PrimaryComponentTick.bCanEverTick = false;
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	// 默认在游戏开始时将生命值初始化到满血状态，避免宿主忘记手动初始化。
	InitializeHP();
}

void UHealthComponent::HandleDeath(AActor* InstigatorActor)
{
	// 组件只负责统一广播死亡结果，具体表现交由宿主或监听方实现。
	OnDeath.Broadcast(this, InstigatorActor);
}

void UHealthComponent::InitializeHP()
{
	// 先确保最大生命值合法，防止编辑器或运行时传入负数。
	MaxHP = FMath::Max(0.0f, MaxHP);

	// 当前默认初始化策略为直接回满血量。
	CurrentHP = MaxHP;

	// 当最大生命值本身为 0 时，视为处于死亡状态。
	bIsDead = CurrentHP <= 0.0f;
}

float UHealthComponent::ApplyDamage(float DamageAmount, AActor* DamageCauser)
{
	// 已死亡或伤害值非法时，不进行任何结算。
	if (bIsDead || DamageAmount <= 0.0f)
	{
		return 0.0f;
	}

	// 先记录结算前血量，便于后续广播事件时提供变化前后的上下文。
	const float OldHP = CurrentHP;

	// 将伤害结算后的生命值钳制到合法范围内，避免出现负血。
	const float NewHP = FMath::Clamp(CurrentHP - DamageAmount, 0.0f, MaxHP);
	const float ActualDamage = OldHP - NewHP;

	// 若钳制后没有产生实际伤害，则直接返回。
	if (ActualDamage <= 0.0f)
	{
		return 0.0f;
	}

	CurrentHP = NewHP;

	// 记录死亡前状态，用于确保死亡事件只在状态首次切换时触发一次。
	const bool bWasDead = bIsDead;
	bIsDead = CurrentHP <= 0.0f;

	// 先广播受伤与数值变化，监听方能拿到结算后的最新血量。
	OnDamaged.Broadcast(this, ActualDamage, CurrentHP, DamageCauser);
	OnHPChanged.Broadcast(this, OldHP, CurrentHP, DamageCauser);

	// 只有从“活着”切换到“死亡”时才执行死亡收口逻辑。
	if (!bWasDead && bIsDead)
	{
		HandleDeath(DamageCauser);
	}

	return ActualDamage;
}

float UHealthComponent::ApplyHealing(float HealAmount, AActor* Healer)
{
	// 当前默认不允许对死亡对象继续治疗；同时过滤非法输入与无效上限。
	if (bIsDead || HealAmount <= 0.0f || MaxHP <= 0.0f)
	{
		return 0.0f;
	}

	// 记录变化前血量，用于生成精确事件参数。
	const float OldHP = CurrentHP;

	// 治疗结果同样需要钳制，避免超过最大生命值。
	const float NewHP = FMath::Clamp(CurrentHP + HealAmount, 0.0f, MaxHP);
	const float ActualHeal = NewHP - OldHP;

	// 若本次治疗没有真实生效，则不广播任何事件。
	if (ActualHeal <= 0.0f)
	{
		return 0.0f;
	}

	CurrentHP = NewHP;

	// 先广播治疗事件，再广播统一的生命值变化事件。
	OnHealed.Broadcast(this, ActualHeal, CurrentHP, Healer);
	OnHPChanged.Broadcast(this, OldHP, CurrentHP, Healer);
	return ActualHeal;
}

void UHealthComponent::Die(AActor* InstigatorActor)
{
	// 已死亡时不重复触发死亡流程，保持行为幂等。
	if (bIsDead)
	{
		return;
	}

	// 直接将血量归零，并标记进入死亡状态。
	const float OldHP = CurrentHP;
	CurrentHP = 0.0f;
	bIsDead = true;

	// 只有血量确实发生变化时才广播生命值变化事件。
	if (!FMath::IsNearlyEqual(OldHP, CurrentHP))
	{
		OnHPChanged.Broadcast(this, OldHP, CurrentHP, InstigatorActor);
	}

	// 统一走死亡收口逻辑，保证强制处死与扣血致死的外部行为一致。
	HandleDeath(InstigatorActor);
}

void UHealthComponent::SetMaxHP(float NewMaxHP, bool bScaleCurrentHP, AActor* InstigatorActor)
{
	// 先记录修改前状态，便于在必要时广播精确的血量变化。
	const float OldMaxHP = MaxHP;
	const float OldHP = CurrentHP;
	const bool bWasDead = bIsDead;

	// 最大生命值不允许小于 0。
	MaxHP = FMath::Max(0.0f, NewMaxHP);

	// 按配置决定是保留血量比例，还是仅在新区间内钳制当前血量。
	if (bScaleCurrentHP)
	{
		const float HealthPercent = OldMaxHP > 0.0f ? (OldHP / OldMaxHP) : 0.0f;
		CurrentHP = FMath::Clamp(MaxHP * HealthPercent, 0.0f, MaxHP);
	}
	else
	{
		CurrentHP = FMath::Clamp(CurrentHP, 0.0f, MaxHP);
	}

	// 根据更新后的生命值重新同步死亡状态。
	bIsDead = CurrentHP <= 0.0f;

	// 若当前生命值发生变化，则广播统一生命值变化事件。
	if (!FMath::IsNearlyEqual(OldHP, CurrentHP))
	{
		OnHPChanged.Broadcast(this, OldHP, CurrentHP, InstigatorActor);
	}

	// 仅当本次最大生命值调整首次导致对象死亡时，才触发死亡流程。
	if (!bWasDead && bIsDead)
	{
		HandleDeath(InstigatorActor);
	}
}

void UHealthComponent::Revive(float ReviveHP, AActor* InstigatorActor)
{
	// 先记录复活前生命值，便于在复活成功后广播生命值变化。
	const float OldHP = CurrentHP;

	// 清理历史死亡状态，准备重新根据目标生命值计算当前存活状态。
	bIsDead = false;

	// 当未指定有效复活血量时，默认按最大生命值满血复活。
	const float TargetHP = ReviveHP > 0.0f ? ReviveHP : MaxHP;
	CurrentHP = FMath::Clamp(TargetHP, 0.0f, MaxHP);
	bIsDead = CurrentHP <= 0.0f;

	// 复活导致生命值变化时，广播统一血量变化事件，便于 UI 和宿主同步状态。
	if (!FMath::IsNearlyEqual(OldHP, CurrentHP))
	{
		OnHPChanged.Broadcast(this, OldHP, CurrentHP, InstigatorActor);
	}

	// 若在无效上限下仍然无法获得生命值，则按规则重新进入死亡流程。
	if (bIsDead)
	{
		HandleDeath(InstigatorActor);
	}
}

bool UHealthComponent::IsDead() const
{
	// 直接返回当前缓存的死亡标记，供蓝图和 C++ 快速查询。
	return bIsDead;
}

float UHealthComponent::GetCurrentHP() const
{
	// 返回当前生命值，便于外部读取精确血量。
	return CurrentHP;
}

float UHealthComponent::GetMaxHP() const
{
	// 返回最大生命值，便于外部做数值展示或逻辑判断。
	return MaxHP;
}

float UHealthComponent::GetHPPercent() const
{
	// 防止最大生命值非法时发生除零。
	if (MaxHP <= 0.0f)
	{
		return 0.0f;
	}

	// 返回 0~1 区间的百分比结果，供血条等 UI 直接使用。
	return CurrentHP / MaxHP;
}
