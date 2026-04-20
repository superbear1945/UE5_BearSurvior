// 武器基类。继承自物品基类，提供武器通用属性（伤害、耐久度、攻击间隔）和攻击入口。
// 武器的近战/远程具体行为由子类附加的 MeleeComponent / RangedWeaponComponent 实现。

#pragma once

#include "CoreMinimal.h"
#include "ItemBase.h"
#include "WeaponBase.generated.h"

class UMeleeComponent;
class URangedWeaponComponent;

// 武器耐久耗尽事件。
// @param Weapon 耐久耗尽的武器。
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponDurabilityDepletedSignature, AWeaponBase*, Weapon);

/**
 * 武器基类：所有武器的公共父类。
 * 封装伤害、耐久度、攻击间隔等武器共性，并提供统一的攻击入口虚函数。
 */
UCLASS(Abstract)
class BEARSURVIOR_API AWeaponBase : public AItemBase
{
	GENERATED_BODY()

public:
	/** 构造函数，初始化武器默认属性。 */
	AWeaponBase();

// ────────────────────────────────────────── 数据 ──────────────────────────────────────────

public:

	// 基础伤害值，武器单次命中的基础伤害。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float BaseDamage;

	// 攻击间隔，两次攻击之间的最短等待时间（秒）。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float AttackInterval;

	// 最大耐久度，耐久度上限。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Durability", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MaxDurability;

	// 当前耐久度，武器每次攻击会消耗一定耐久，归零后武器无法使用。
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Durability")
	float CurrentDurability;

protected:

	// 每次攻击消耗的耐久值。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Durability", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DurabilityCostPerAttack;

	// 当前是否处于攻击状态（防止重复攻击）。
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	bool bIsAttacking;

	// 上次攻击完成的时间，用于攻击间隔判定。
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	float LastAttackTime;

// ────────────────────────────────────────── 事件 ──────────────────────────────────────────

public:

	// 武器耐久度耗尽时广播，可用于触发武器破碎特效、移除装备等。
	UPROPERTY(BlueprintAssignable, Category = "Weapon|Event")
	FOnWeaponDurabilityDepletedSignature OnDurabilityDepleted;

// ────────────────────────────────────────── 方法 ──────────────────────────────────────────

public:

	/**
	 * 判断武器当前是否可以执行攻击。
	 * 检查项目：耐久度是否大于0、是否处于攻击状态、攻击间隔是否已过。
	 */
	UFUNCTION(BlueprintPure, Category = "Weapon")
	virtual bool CanAttack() const;

	/**
	 * 开始攻击。子类应覆盖此函数实现具体的攻击流程（如播放动画、触发组件等）。
	 * 基类实现会检查 CanAttack() 并更新攻击状态标记。
	 * @return 是否成功发起攻击。
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual bool StartAttack();

	/**
	 * 停止攻击。重置攻击状态，子类可覆盖实现额外的收尾逻辑。
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void StopAttack();

	/**
	 * 消耗武器耐久度。
	 * @param Cost 要消耗的耐久值，默认使用 DurabilityCostPerAttack。
	 * @return 实际消耗的耐久值。
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Durability")
	virtual float ConsumeDurability(float Cost = -1.0f);

	/**
	 * 修复武器耐久度。
	 * @param Amount 要修复的耐久值。
	 * @return 实际修复的耐久值。
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Durability")
	virtual float RepairDurability(float Amount);

	/** 返回当前耐久度百分比，供UI使用。 */
	UFUNCTION(BlueprintPure, Category = "Weapon|Durability")
	float GetDurabilityPercent() const;

	/** 返回当前耐久度。 */
	UFUNCTION(BlueprintPure, Category = "Weapon|Durability")
	float GetCurrentDurability() const;

	/** 返回最大耐久度。 */
	UFUNCTION(BlueprintPure, Category = "Weapon|Durability")
	float GetMaxDurability() const;

	/** 返回基础伤害值。 */
	UFUNCTION(BlueprintPure, Category = "Weapon")
	float GetBaseDamage() const;

	/** 返回武器是否已损坏（耐久度 <= 0）。 */
	UFUNCTION(BlueprintPure, Category = "Weapon|Durability")
	bool IsBroken() const;

	/** 返回武器是否正在攻击中。 */
	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool IsAttacking() const;

protected:

	/** 在游戏开始时初始化武器状态。 */
	virtual void BeginPlay() override;

	/**
	 * 耐久耗尽时的处理逻辑，广播事件并标记武器不可用。
	 * 子类可覆盖实现额外效果（如破碎特效、禁用攻击等）。
	 */
	virtual void HandleDurabilityDepleted();
};
