// 武器基类。继承自物品基类，提供武器运行时状态与攻击输入转发入口。
// 武器的近战/远程具体行为、攻击节奏和武器专属数据均由挂载的 AttackComponentBase
// 派生组件自行管理。 物品公共数据由 AItemBase 读取，WeaponBase
// 只负责公共耐久状态和攻击意图转发。

#pragma once

#include "CoreMinimal.h"
#include "EquippableItem.h"
#include "ItemBase.h"
#include "Weapon/IUseableItem.h"
#include "Weapon/WeaponDataTypes.h"
#include "WeaponBase.generated.h"

class UAttackComponentBase;

// 武器耐久耗尽事件。
// @param Weapon 耐久耗尽的武器。
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponDurabilityDepletedSignature, AWeaponBase *, Weapon);

// 瞄准状态切换事件。比如进入/退出右键瞄准状态
// @param bIsAiming 当前是否处于瞄准状态。
// @param TargetFOV 当前瞄准状态下的目标视野范围（FOV）。
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAimStateChangedSignature, bool, bIsAiming, float, TargetFOV);

/**
 * 武器基类：所有武器的公共父类。
 * 物品公共数据与武器专属数据分别从不同 DataTable 行读取，运行时状态由本类管理。
 */
UCLASS(Abstract)
class BEARSURVIOR_API AWeaponBase : public AItemBase,
                                    public IUseableItem,
                                    public IEquippableItem {
  GENERATED_BODY()

public:
  /** 构造函数，初始化武器默认属性。 */
  AWeaponBase();

  // ────────────────────────────────────────── 数据表引用
  // ──────────────────────────────────────────

public:
  // ────────────────────────────────────────── 运行时状态
  // ──────────────────────────────────────────

public:
  // 当前耐久度，武器每次攻击会消耗一定耐久，归零后武器无法使用。
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Durability")
  float CurrentDurability;

protected:
  // 当前是否处于攻击状态（防止重复攻击）。
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
  bool bIsAttacking;

  // 当前武器实际用于执行攻击的组件。WeaponBase
  // 只负责输入转发，不关心具体是近战还是远程。
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Attack",
            meta = (AllowPrivateAccess = "true"))
  TObjectPtr<UAttackComponentBase> ActiveAttackComponent;

  // ────────────────────────────────────────── 事件
  // ──────────────────────────────────────────

public:
  // 武器耐久度耗尽时广播，可用于触发武器破碎特效、移除装备等。
  UPROPERTY(BlueprintAssignable, Category = "Weapon|Event")
  FOnWeaponDurabilityDepletedSignature OnDurabilityDepleted;

  // ────────────────────────────────────────── 方法
  // ──────────────────────────────────────────

public:
  /**
   * 装备武器并附着到角色指定插槽。
   * @param CharacterOwner 装备该武器的角色。
   * @param AttachSocketName 武器需要附着到的角色骨骼插槽名称。
   */
  UFUNCTION(BlueprintCallable, Category = "Weapon")
  void Equip_Implementation(ACharacter *CharacterOwner, FName AttachSocketName) override;

  UFUNCTION(BlueprintCallable, Category = "Weapon")
  void UnEquip_Implementation(ACharacter *CharacterOwner) override;

  /**
   * 判断武器当前是否可以执行攻击。
   * WeaponBase 只检查公共状态；具体攻击间隔与武器专属条件由攻击组件负责。
   */
  UFUNCTION(BlueprintPure, Category = "Weapon")
  virtual bool CanAttack() const;

  /**
   * 开始攻击。基类会检查武器状态，并把角色传入的瞄准信息转发给当前攻击组件。
   * @param AimLocation 角色视角或相机位置，用于远程射线等需要瞄准起点的攻击。
   * @param AimDirection 角色视角方向，用于远程射线、投射物或朝向型攻击。
   * @return 是否成功发起攻击。
   */
  UFUNCTION(BlueprintCallable, Category = "Weapon")
  virtual bool StartAttack(const FVector &AimLocation, const FVector &AimDirection);

  /**
   * 停止攻击。重置攻击状态，子类可覆盖实现额外的收尾逻辑。
   */
  UFUNCTION(BlueprintCallable, Category = "Weapon")
  virtual void StopAttack();

  /**
   * 消耗武器耐久度。
   * @param Cost 要消耗的耐久值，默认使用当前武器专属 DataTable
   * 中配置的耐久消耗。
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

  /** 返回基础伤害值（由当前攻击组件提供）。 */
  UFUNCTION(BlueprintPure, Category = "Weapon")
  float GetBaseDamage() const;

  /** 返回攻击间隔（由当前攻击组件提供）。 */
  UFUNCTION(BlueprintPure, Category = "Weapon")
  float GetAttackInterval() const;

  /** 返回默认耐久消耗（由当前攻击组件提供）。 */
  UFUNCTION(BlueprintPure, Category = "Weapon|Durability")
  float GetDefaultDurabilityCost() const;

  /** 返回武器是否已损坏（耐久度 <= 0）。 */
  UFUNCTION(BlueprintPure, Category = "Weapon|Durability")
  bool IsBroken() const;

  /** 返回武器是否正在攻击中。 */
  UFUNCTION(BlueprintPure, Category = "Weapon")
  bool IsAttacking() const;

  /** 判断物品公共数据与攻击组件数据是否都已正确加载。 */
  UFUNCTION(BlueprintPure, Category = "Weapon|DataTable")
  bool IsDataLoaded() const;

  // IUseableItem
  // 接口实现：武器默认将主要使用映射为攻击生命周期，次要使用留给子类扩展。
  virtual void PrimaryUseStart_Implementation(const FVector &AimLocation, const FVector &AimDirection) override;
  virtual void PrimaryUseEnd_Implementation() override;
  virtual void SecondaryUseStart_Implementation() override;
  virtual void SecondaryUseEnd_Implementation() override;

protected:
  /** 在游戏开始时初始化武器状态。 */
  virtual void BeginPlay() override;

  /**
   * 使用物品公共数据初始化运行时状态。
   */
  void InitializeFromData();

  /**
   * 通知挂载的 AttackComponent 解析并初始化自身武器数据。
   */
  void InitializeAttackComponents();

  /**
   * 耐久耗尽时的处理逻辑，广播事件并标记武器不可用。
   * 子类可覆盖实现额外效果（如破碎特效、禁用攻击等）。
   */
  virtual void HandleDurabilityDepleted();
};
