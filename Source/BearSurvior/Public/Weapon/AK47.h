// AK47 武器类声明。继承自武器基类，具体属性通过 DataTable 配置。

#pragma once

#include "CoreMinimal.h"
#include "Weapon/WeaponBase.h"
#include "AK47.generated.h"

class URangeAttackComponent;

/**
 * AK47 武器：所有属性由 DataTable 行（FRangedWeaponData）配置。
 * 蓝图子类只需设置 WeaponType=Ranged 和 WeaponDataRow 即可。
 */
UCLASS(Abstract)
class BEARSURVIOR_API AAK47 : public AWeaponBase
{
	GENERATED_BODY()

public:
	/** 构造函数。 */
	AAK47();

public:

	/**
	 * 判断武器当前是否可以执行攻击。
	 * 检查项目：耐久度是否大于0、是否处于攻击状态、攻击间隔是否已过。
	 */
	virtual bool CanAttack() const override;

	/**
	 * 开始执行 AK47 攻击。
	 * @return 是否成功发起攻击。
	 */
	virtual bool StartAttack() override;

	/**
	 * 停止 AK47 攻击。
	 */
	virtual void StopAttack() override;

	/**
	 * 消耗武器耐久度。
	 * @param Cost 要消耗的耐久值。
	 * @return 实际消耗的耐久值。
	 */
	virtual float ConsumeDurability(float Cost = -1.0f) override;

	/**
	 * 修复武器耐久度。
	 * @param Amount 要修复的耐久值。
	 * @return 实际修复的耐久值。
	 */
	virtual float RepairDurability(float Amount) override;

protected:

	/** 在游戏开始时初始化武器状态。 */
	virtual void BeginPlay() override;

	/** 每帧更新 AK47 的运行状态。 */
	virtual void Tick(float DeltaTime) override;

	/**
	 * 耐久耗尽时的处理逻辑，广播事件并标记武器不可用。
	 */
	virtual void HandleDurabilityDepleted() override;
};
