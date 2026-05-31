// 攻击组件抽象基类。为武器挂载的近战、远程等攻击组件提供统一攻击入口。
// WeaponBase 只依赖本基类转发输入生命周期，具体攻击方式由派生组件自行实现。

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AttackComponentBase.generated.h"

class ACharacter;

/**
 * 攻击组件抽象基类：统一所有武器攻击组件的开始攻击、结束攻击和可攻击判断接口。
 * 派生类负责实现具体攻击行为，例如远程射击、近战检测、蓄力攻击等。
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class BEARSURVIOR_API UAttackComponentBase : public UActorComponent
{
	GENERATED_BODY()

public:
	/** 构造函数，初始化组件默认状态。 */
	UAttackComponentBase();

	UFUNCTION(BlueprintCallable, Category = "Equip")
	virtual void OnEquip(ACharacter* CharacterOwner);

	UFUNCTION(BlueprintCallable, Category = "Equip")
	virtual void OnUnEquip(ACharacter* CharacterOwner);

	/**
	 * 开始攻击。
	 * @param AimLocation 攻击发起者的瞄准起点，通常来自角色相机或眼睛位置。
	 * @param AimDirection 攻击方向，通常来自角色视角前向量。
	 * @return 是否成功发起攻击。
	 */
	UFUNCTION(BlueprintCallable, Category = "Attack")
	virtual bool StartAttack(const FVector& AimLocation, const FVector& AimDirection);

	/**
	 * 结束攻击。
	 * 用于释放输入时停止自动射击、关闭近战攻击窗口或结束持续攻击行为。
	 */
	UFUNCTION(BlueprintCallable, Category = "Attack")
	virtual void StopAttack();

	/**
	 * 判断组件当前是否允许攻击。
	 * 派生类可检查弹药、装弹、攻击窗口、蓄力状态等具体条件。
	 */
	UFUNCTION(BlueprintPure, Category = "Attack")
	virtual bool CanAttack() const;

	/**
	 * 返回当前攻击组件管理的攻击间隔。
	 * WeaponBase 只负责转发攻击意图，具体节奏由派生攻击组件根据自身数据决定。
	 */
	virtual float GetAttackInterval() const;

	/**
	 * 返回当前攻击组件管理的基础伤害。
	 * WeaponBase 通过该接口读取展示或调试所需的统一伤害值。
	 */
	virtual float GetBaseDamage() const;

	/**
	 * 返回当前攻击组件管理的默认耐久消耗。
	 * 近战与远程组件各自决定一次攻击应消耗多少耐久。
	 */
	virtual float GetDefaultDurabilityCost() const;

	/**
	 * 返回当前攻击组件是否已经正确加载自身数据。
	 * 由派生类根据各自 DataTable 缓存状态实现。
	 */
	virtual bool IsDataLoaded() const;

	/**
	 * 解析并缓存组件自身的武器数据。
	 * WeaponBase 在 BeginPlay 时调用，派生类根据自己的 DataRow 完成初始化。
	 */
	virtual void ResolveWeaponData();

protected:

	// 上一次成功开始攻击的时间，由攻击组件自身用于控制攻击节奏。
	float LastAttackTime;

	/**
	 * 标记本轮首次攻击输入成功开始的时间戳。
	 * 派生类在真正接受本次攻击输入后调用，用于后续冷却判断。
	 */
	void MarkFirstAttackTime();

	virtual void BeginPlay() override;
};
