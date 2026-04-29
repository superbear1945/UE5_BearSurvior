// 武器基类。继承自物品基类，提供武器通用属性（伤害、耐久度、攻击间隔）和攻击入口。
// 武器的近战/远程具体行为由子类附加的 MeleeAttackComponent / RangeAttackComponent 实现。
// 设计期数据（伤害、射速等）通过 DataTable 配置，运行时状态保存在本类中。

#pragma once

#include "CoreMinimal.h"
#include "ItemBase.h"
#include "Weapon/WeaponDataTypes.h"
#include "WeaponBase.generated.h"

class UMeleeAttackComponent;
class URangeAttackComponent;

// 武器耐久耗尽事件。
// @param Weapon 耐久耗尽的武器。
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponDurabilityDepletedSignature, AWeaponBase*, Weapon);

/**
 * 武器基类：所有武器的公共父类。
 * 设计期数据从 DataTable 行读取，运行时状态由本类管理。
 */
UCLASS(Abstract)
class BEARSURVIOR_API AWeaponBase : public AItemBase
{
	GENERATED_BODY()

public:
	/** 构造函数，初始化武器默认属性。 */
	AWeaponBase();

// ────────────────────────────────────────── 数据表引用 ──────────────────────────────────────────

public:

	// 武器类型，决定从哪张 DataTable 中解析数据。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|DataTable")
	EWeaponType WeaponType;

	// 武器数据表行引用。在编辑器中选中行后，BeginPlay 时自动解析并初始化武器。
	// 近战武器指向 DT_MeleeWeapons，远程武器指向 DT_RangedWeapons。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|DataTable", meta = (RequiredAssetDataTags = "RowStructure=/Script/BearSurvior.ItemData"))
	FDataTableRowHandle WeaponDataRow;

// ────────────────────────────────────────── 运行时状态 ──────────────────────────────────────────

public:

	// 当前耐久度，武器每次攻击会消耗一定耐久，归零后武器无法使用。
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Durability")
	float CurrentDurability;

protected:

	// 当前是否处于攻击状态（防止重复攻击）。
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	bool bIsAttacking;

	// 上次攻击完成的时间，用于攻击间隔判定。
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	float LastAttackTime;

	// 缓存的物品公共数据指针，BeginPlay 中解析 DataTable 后设置。
	const FItemData* CachedItemData;

	// 缓存的近战武器数据指针，仅近战类型有效。
	const FMeleeWeaponData* CachedMeleeData;

	// 缓存的远程武器数据指针，仅远程类型有效。
	const FRangedWeaponData* CachedRangedData;

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
	 * @param Cost 要消耗的耐久值，默认使用 DataTable 中配置的 DurabilityCostPerAttack。
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

	/** 返回最大耐久度（从 DataTable 读取）。 */
	UFUNCTION(BlueprintPure, Category = "Weapon|Durability")
	float GetMaxDurability() const;

	/** 返回基础伤害值（从 DataTable 读取）。 */
	UFUNCTION(BlueprintPure, Category = "Weapon")
	float GetBaseDamage() const;

	/** 返回武器是否已损坏（耐久度 <= 0）。 */
	UFUNCTION(BlueprintPure, Category = "Weapon|Durability")
	bool IsBroken() const;

	/** 返回武器是否正在攻击中。 */
	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool IsAttacking() const;

	/** 返回缓存的物品公共数据引用。数据未加载时返回空默认值。 */
	UFUNCTION(BlueprintPure, Category = "Weapon|DataTable")
	const FItemData& GetItemData() const;

	/** 返回缓存的具体武器数据引用（近战或远程，取决于 WeaponType）。 */
	UFUNCTION(BlueprintPure, Category = "Weapon|DataTable")
	const FItemData& GetWeaponData() const;

	/** 判断武器数据是否正确加载。 */
	UFUNCTION(BlueprintPure, Category = "Weapon|DataTable")
	bool IsDataLoaded() const;

protected:

	/** 在游戏开始时初始化武器状态。 */
	virtual void BeginPlay() override;

	/**
	 * 解析 WeaponDataRow 指向的 DataTable 行，缓存到 CachedItemData / CachedMeleeData / CachedRangedData 中。
	 * 同时将物品级数据同步到父类 AItemBase 的对应字段。
	 */
	void ResolveWeaponData();

	/**
	 * 使用已解析的 DataTable 数据初始化运行时状态（耐久度、弹匣等）。
	 */
	void InitializeFromData();

	/**
	 * 将 DataTable 中的物品级数据同步到父类字段。
	 */
	void SyncItemProperties();

	/**
	 * 通知挂载的 AttackComponent 使用 DataTable 数据进行初始化。
	 */
	void InitializeAttackComponents();

	/**
	 * 耐久耗尽时的处理逻辑，广播事件并标记武器不可用。
	 * 子类可覆盖实现额外效果（如破碎特效、禁用攻击等）。
	 */
	virtual void HandleDurabilityDepleted();
};
