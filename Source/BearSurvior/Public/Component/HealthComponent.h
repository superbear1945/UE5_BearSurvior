#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

class AActor;
class UHealthComponent;

/**
 * 生命值变化事件。
 *
 * @param HealthComponent 触发事件的生命值组件。
 * @param OldHP 变化前的生命值。
 * @param NewHP 变化后的生命值。
 * @param InstigatorActor 造成这次变化的行为发起者，可为空。
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnHealthChangedSignature, UHealthComponent*, HealthComponent, float, OldHP, float, NewHP, AActor*, InstigatorActor);

/**
 * 受到伤害事件。
 *
 * @param HealthComponent 触发事件的生命值组件。
 * @param DamageAmount 本次实际生效的伤害值。
 * @param NewHP 扣血后的剩余生命值。
 * @param DamageCauser 伤害来源，可为空。
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnHealthDamagedSignature, UHealthComponent*, HealthComponent, float, DamageAmount, float, NewHP, AActor*, DamageCauser);

/**
 * 受到治疗事件。
 *
 * @param HealthComponent 触发事件的生命值组件。
 * @param HealAmount 本次实际生效的治疗值。
 * @param NewHP 治疗后的生命值。
 * @param Healer 治疗来源，可为空。
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnHealthHealedSignature, UHealthComponent*, HealthComponent, float, HealAmount, float, NewHP, AActor*, Healer);

/**
 * 死亡事件。
 *
 * @param HealthComponent 触发事件的生命值组件。
 * @param InstigatorActor 导致死亡的行为发起者，可为空。
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthDeathSignature, UHealthComponent*, HealthComponent, AActor*, InstigatorActor);

/**
 * 生命值组件：负责管理宿主 Actor 的生命值、死亡判定与相关事件广播。
 *
 * 该组件聚焦于“数值状态”本身，不直接耦合角色动画、AI 停止、掉落物生成等具体业务表现。
 * 外部系统可以通过监听组件事件来驱动 UI、特效、状态切换与销毁流程。
 */
UCLASS(ClassGroup=(Gameplay), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class BEARSURVIOR_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** 构造函数，初始化组件默认配置。 */
	UHealthComponent();

protected:
	/** 最大生命值，决定组件能够持有的血量上限。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Health", meta=(ClampMin="0.0", UIMin="0.0"))
	float MaxHP = 100.0f;

	/**
	 * 当前生命值，仅允许组件内部统一维护。
	 *
	 * 该值默认会在 BeginPlay 阶段通过 InitializeHP() 按 MaxHP 初始化。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Health")
	float CurrentHP = 0.0f;

	/**
	 * 当前是否已经死亡，用于阻止重复死亡和死亡后继续结算数值。
	 *
	 * 一旦进入死亡状态，ApplyDamage()、ApplyHealing() 与 Die() 都会按规则做幂等保护。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Health")
	bool bIsDead = false;

protected:
	/**
	 * 在游戏开始时初始化生命值状态。
	 *
	 * 当前默认策略为：将 CurrentHP 重置为 MaxHP，并根据结果同步 bIsDead。
	 */
	virtual void BeginPlay() override;

	/**
	 * 处理死亡收口逻辑，默认会广播死亡事件，可由子类扩展。
	 *
	 * 如需在 C++ 层扩展死亡时的统一行为，可继承组件并重写此函数。
	 */
	virtual void HandleDeath(AActor* InstigatorActor);

public:
	/** 生命值发生变化时广播，便于 UI 或表现层同步刷新。 */
	UPROPERTY(BlueprintAssignable, Category="Health|Event")
	FOnHealthChangedSignature OnHPChanged;

	/** 成功受到伤害时广播。 */
	UPROPERTY(BlueprintAssignable, Category="Health|Event")
	FOnHealthDamagedSignature OnDamaged;

	/** 成功受到治疗时广播。 */
	UPROPERTY(BlueprintAssignable, Category="Health|Event")
	FOnHealthHealedSignature OnHealed;

	/** 生命值归零并进入死亡状态时广播，仅触发一次。 */
	UPROPERTY(BlueprintAssignable, Category="Health|Event")
	FOnHealthDeathSignature OnDeath;

public:
	/**
	 * 按最大生命值重置当前生命值，通常在 BeginPlay 或重生时调用。
	 *
	 * 当前实现会直接把 CurrentHP 设为 MaxHP，并重置死亡标记。
	 * 该函数本身不会主动广播事件，适合作为初始化或重生准备阶段调用。
	 */
	UFUNCTION(BlueprintCallable, Category="Health")
	virtual void InitializeHP();

	/**
	 * 扣减生命值并返回实际生效的伤害值。
	 *
	 * 该函数会自动处理非法输入、数值钳制、伤害事件广播、生命值变化广播，以及归零后的死亡判定。
	 *
	 * @param DamageAmount 期望施加的伤害值。
	 * @param DamageCauser 伤害来源，可为空。
	 * @return 本次真正从 CurrentHP 中扣除的伤害值。
	 */
	UFUNCTION(BlueprintCallable, Category="Health")
	virtual float ApplyDamage(float DamageAmount, AActor* DamageCauser);

	/**
	 * 恢复生命值并返回实际生效的治疗值。
	 *
	 * 该函数会自动处理非法输入、数值钳制、治疗事件广播与生命值变化广播。
	 * 当前默认策略下，死亡状态不允许继续接受治疗。
	 *
	 * @param HealAmount 期望恢复的生命值。
	 * @param Healer 治疗来源，可为空。
	 * @return 本次真正恢复到 CurrentHP 的生命值。
	 */
	UFUNCTION(BlueprintCallable, Category="Health")
	virtual float ApplyHealing(float HealAmount, AActor* Healer);

	/**
	 * 直接进入死亡状态，适用于脚本强制处死等场景。
	 *
	 * 该函数会将 CurrentHP 置为 0，并在首次死亡时触发死亡收口逻辑。
	 * 若当前已经死亡，则不会重复广播死亡事件。
	 */
	UFUNCTION(BlueprintCallable, Category="Health")
	virtual void Die(AActor* InstigatorActor);

	/**
	 * 设置最大生命值，并根据参数决定是否同步调整当前生命值。
	 *
	 * 该函数会先将输入值钳制到不小于 0 的范围，再更新 MaxHP。
	 * 当 bScaleCurrentHP 为 true 时，会按旧生命百分比缩放当前生命值；否则仅将当前生命值钳制到新的合法区间。
	 * 若更新后生命值发生变化，会广播生命值变化事件。
	 * 若新上限导致对象进入死亡状态，也会触发统一死亡流程。
	 *
	 * @param NewMaxHP 新的最大生命值。
	 * @param bScaleCurrentHP 是否按原百分比同步缩放当前生命值。
	 * @param InstigatorActor 触发本次修改的行为发起者，可为空。
	 */
	UFUNCTION(BlueprintCallable, Category="Health")
	virtual void SetMaxHP(float NewMaxHP, bool bScaleCurrentHP, AActor* InstigatorActor);

	/**
	 * 从死亡状态恢复，并按指定生命值重新激活组件。
	 *
	 * 该函数会清除死亡标记，并将当前生命值恢复到合法范围内。
	 * 若传入生命值小于等于 0，则会使用 MaxHP 作为默认复活血量。
	 * 若 MaxHP 本身无效，则复活后仍会保持 0 血并重新进入死亡状态。
	 * 只要生命值发生变化，就会广播生命值变化事件。
	 *
	 * @param ReviveHP 复活后的目标生命值；小于等于 0 时默认取 MaxHP。
	 * @param InstigatorActor 触发本次复活的行为发起者，可为空。
	 */
	UFUNCTION(BlueprintCallable, Category="Health")
	virtual void Revive(float ReviveHP, AActor* InstigatorActor);

	/** 返回当前是否已经死亡。 */
	UFUNCTION(BlueprintPure, Category="Health")
	bool IsDead() const;

	/** 返回当前生命值。 */
	UFUNCTION(BlueprintPure, Category="Health")
	float GetCurrentHP() const;

	/** 返回最大生命值。 */
	UFUNCTION(BlueprintPure, Category="Health")
	float GetMaxHP() const;

	/**
	 * 返回当前生命值百分比，供血条等 UI 直接读取。
	 *
	 * 当 MaxHP 小于等于 0 时，返回 0，避免除零错误。
	 */
	UFUNCTION(BlueprintPure, Category="Health")
	float GetHPPercent() const;
};
