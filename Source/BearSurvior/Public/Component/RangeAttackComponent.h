// 远程攻击组件。挂载在武器Actor上，负责远程射击的命中检测、弹药管理与开火控制。
// 使用射线扫描（LineTrace）实现弹道命中检测，支持弹匣、装弹、自动/半自动射击模式。
// 设计期数据（伤害、射速、弹匣、枪声等）由 Owner 的 URangedWeaponDataAsset 提供，
// 通过 ResolveWeaponData 解析缓存。

#pragma once

#include "Component/AttackComponentBase.h"
#include "CoreMinimal.h"
#include "Weapon/RangedWeaponDataAsset.h"
#include "Engine/HitResult.h"
#include "RangeAttackComponent.generated.h"

struct FHitResult;
class UStaticMeshComponent;
class USoundBase;
class ACharacter;
class APlayerCameraManager;

// 开火事件。
// @param ImpactPoint 命中位置。
// @param HitActor 被命中的Actor，未命中时为空。
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRangeWeaponFireSignature,
                                             FVector, ImpactPoint, AActor *,
                                             HitActor);

// 开始装弹事件。
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnReloadStartedSignature);

// 装弹完成事件。
// @param NewAmmoCount 装弹完成后的弹药数。
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnReloadFinishedSignature, int32,
                                            NewAmmoCount);

// 弹药变化事件。
// @param CurrentMag 当前弹匣内弹药数。
// @param MaxMag 弹匣最大容量。
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAmmoChangedSignature, int32,
                                             CurrentMag, int32, MaxMag);

/**
 * 远程攻击组件：管理远程武器的开火、命中检测、弹药消耗与装弹流程。
 *
 * 工作流程：
 *   1. 外部调用 StartFire() 开始射击（组件自动处理开火节奏与弹药消耗）。
 *   2. 组件按
 * FireRate（每分钟射击次数）换算射击间隔，并执行射线扫描（LineTrace）检测命中。
 *   3. 弹药耗尽时自动或手动触发 Reload() 进行装弹。
 *   4. 外部调用 StopFire() 停止射击。
 *
 * 支持半自动（点射）和全自动（按住不放连续射击）模式。
 */
UCLASS(ClassGroup = (Combat), BlueprintType, Blueprintable,
       meta = (BlueprintSpawnableComponent))
class BEARSURVIOR_API URangeAttackComponent : public UAttackComponentBase {
  GENERATED_BODY()

public:
  /** 构造函数，初始化组件默认配置。 */
  URangeAttackComponent();

  // ────────────────────────────────────────── 配置
  // ──────────────────────────────────────────

public:
  // 装备与解除装备时调用的函数
  void OnEquip(ACharacter *CharacterOwner) override;
  void OnUnEquip(ACharacter *CharacterOwner) override;
  
  // 可选的瞄准目标。设置后优先朝该目标方向射击；为空时沿宿主前向射击。
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Range|Config")
  TObjectPtr<AActor> AimTarget;

  // 远程武器数据资产引用。运行时通过 Owner 的 AWeaponBase::GetItemDataAsset() 获取并 Cast。
  // 组件自身不直接持有 DataAsset 引用，统一由武器 Actor 管理，保证单点配置。

  // 用于射线检测的碰撞通道。
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Range|Config")
  TEnumAsByte<ECollisionChannel> TraceChannel;


  // ────────────────────────────────────────── 状态
  // ──────────────────────────────────────────

protected:
  // 当前是否正在射击（开火按钮是否按住）。
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Range|State")
  bool bIsFiring = false;

  // 当前是否正在装弹。
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Range|State")
  bool bIsReloading = false;

  // 当前弹匣内弹药数。
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Range|State")
  int32 CurrentAmmoInMagazine = 0;

  // 当前武器的弹匣网格,用于在世界中显示弹匣外观（可选）。
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Range|Mesh")
  TObjectPtr<UStaticMeshComponent> MagazineMeshComponent;

  // 本次攻击传入的瞄准起点，通常为角色相机或眼睛位置。
  FVector CachedAimLocation;

  // 本次攻击传入的瞄准方向，通常为角色相机或眼睛前向量。
  FVector CachedAimDirection;

  // 开火定时器句柄，用于控制射速节奏。
  FTimerHandle FireTimerHandle;

  // 装弹定时器句柄，用于控制装弹时长。
  FTimerHandle ReloadTimerHandle;

  // ────── 缓存的设计期数据（由 ResolveWeaponData 设置） ──────

  // 基础伤害值，从远程武器 DataAsset 读取。
  float CachedBaseDamage;

  // 单次开火需要施加到玩家视角上的后坐力旋转量，从远程武器 DataAsset 读取。
  FRotator CachedRecoilRotation;

  // 两次攻击输入之间的最短间隔，从远程武器 DataAsset 读取。
  float CachedAttackInterval;

  // 每分钟射击次数（RPM, Rounds Per Minute），从远程武器 DataAsset 读取。
  float CachedFireRate;

  // 弹匣容量，从远程武器 DataAsset 读取。
  int32 CachedMagazineCapacity;

  // 装弹所需时间，从远程武器 DataAsset 读取。
  float CachedReloadTime;

  // 子弹散布角度，从远程武器 DataAsset 读取。
  float CachedSpreadAngle;

  // 是否为全自动射击模式，从远程武器 DataAsset 读取。
  bool bCachedAutomaticFire;

  // 最大有效射程，从远程武器 DataAsset 读取。
  float CachedMaxRange;

  // 每次射击消耗的耐久值，从远程武器 DataAsset 读取。
  float CachedDurabilityCostPerShot;

  // 储备弹药总数，从远程武器 DataAsset 读取。-1 表示无限弹药。
  int32 CachedReserveAmmo;

  // 枪声资源，从远程武器 DataAsset 读取，供外部系统查询和播放。
  TSoftObjectPtr<USoundBase> CachedGunshotSound;

  // 装备时解析并缓存的枪声硬引用，避免后续实际播放时再同步加载资源。
  UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Range|Audio")
  TObjectPtr<USoundBase> LoadedGunshotSound = nullptr;

  // ────────────────────────── 事件 ──────────────────────────

public:

  // 射击时广播，包含命中位置和被命中的Actor（可能为空）。
  UPROPERTY(BlueprintAssignable, Category = "Range|Event")
  FOnRangeWeaponFireSignature OnFire;

  // 开始装弹时广播。
  UPROPERTY(BlueprintAssignable, Category = "Range|Event")
  FOnReloadStartedSignature OnReloadStarted;

  // 装弹完成时广播。
  UPROPERTY(BlueprintAssignable, Category = "Range|Event")
  FOnReloadFinishedSignature OnReloadFinished;

  // 弹药变化时广播，供 UI 同步弹药显示。
  UPROPERTY(BlueprintAssignable, Category = "Range|Event")
  FOnAmmoChangedSignature OnAmmoChanged;

  // ────────────────────────────────────────── 方法
  // ──────────────────────────────────────────

public:
  /**
   * 从 Owner 的 URangedWeaponDataAsset 解析并缓存远程武器数据。
   * 由宿主 AWeaponBase::InitializeAttackComponents 在 BeginPlay 中调用。
   * 从 Owner → AWeaponBase → GetItemDataAsset() → Cast<URangedWeaponDataAsset> 获取数据。
   * 数据无效时保留构造函数中的默认值。
   */
  virtual void ResolveWeaponData() override;

  /** 开始远程攻击。全自动武器会持续开火，半自动武器只在本次输入开始时射击一次。
   */
  virtual bool StartAttack(const FVector &AimLocation,
                           const FVector &AimDirection) override;

  /** 结束远程攻击。松开输入时停止全自动开火定时器，并重置射击状态。 */
  virtual void StopAttack() override;

  /** 返回远程组件当前是否可以发起攻击。 */
  virtual bool CanAttack() const override;

  /** 返回当前远程组件管理的攻击间隔。 */
  virtual float GetAttackInterval() const override;

  /** 返回当前远程组件管理的基础伤害。 */
  virtual float GetBaseDamage() const override;

  /** 返回当前远程组件管理的默认耐久消耗。 */
  virtual float GetDefaultDurabilityCost() const override;

  /** 返回当前远程组件是否已经成功加载 DataAsset 数据。 */
  virtual bool IsDataLoaded() const override;

  /** 返回当前缓存的枪声资源引用。未配置时返回空引用。 */
  UFUNCTION(BlueprintPure, Category = "Range|Audio")
  TSoftObjectPtr<USoundBase> GetGunshotSound() const;

  /** 返回当前已解析并缓存的枪声资源硬引用。未装备或未配置时返回空指针。 */
  UFUNCTION(BlueprintPure, Category = "Range|Audio")
  USoundBase *GetLoadedGunshotSound() const;

  /** 开始射击。按配置的射击模式处理开火节奏。 */
  UFUNCTION(BlueprintCallable, Category = "Range")
  void StartFire();

  /** 停止射击，取消开火定时器。 */
  UFUNCTION(BlueprintCallable, Category = "Range")
  void StopFire();

  /** 触发装弹。弹匣已满或正在装弹时不会重复触发。 */
  UFUNCTION(BlueprintCallable, Category = "Range")
  void Reload();

  /** 取消装弹。 */
  UFUNCTION(BlueprintCallable, Category = "Range")
  void CancelReload();

  // ────── 状态查询 ──────

  /** 返回武器当前是否可以射击（弹药充足、未装弹、未损坏）。 */
  UFUNCTION(BlueprintPure, Category = "Range")
  bool CanFire() const;

  /** 返回当前是否正在射击。 */
  UFUNCTION(BlueprintPure, Category = "Range")
  bool IsFiring() const;

  /** 返回当前是否正在装弹。 */
  UFUNCTION(BlueprintPure, Category = "Range")
  bool IsReloading() const;

  /** 返回弹匣是否为空。 */
  UFUNCTION(BlueprintPure, Category = "Range")
  bool IsMagazineEmpty() const;

  /** 返回弹匣是否已满。 */
  UFUNCTION(BlueprintPure, Category = "Range")
  bool IsMagazineFull() const;

  /** 返回当前弹匣内弹药数。 */
  UFUNCTION(BlueprintPure, Category = "Range")
  int32 GetCurrentAmmo() const;

  /** 返回弹匣容量。 */
  UFUNCTION(BlueprintPure, Category = "Range")
  int32 GetMagazineCapacity() const;

  /** 返回弹药百分比（0~1），供 UI 弹药条使用。 */
  UFUNCTION(BlueprintPure, Category = "Range")
  float GetAmmoPercent() const;

  /** 添加弹药到储备中。 */
  UFUNCTION(BlueprintCallable, Category = "Range|Ammo")
  void AddReserveAmmo(int32 Amount);

  /**
   * 设置当前瞄准目标。
   * 当目标有效时，组件优先朝目标方向射击；传入空值可恢复沿宿主前向射击。
   */
  UFUNCTION(BlueprintCallable, Category = "Range")
  void SetAimTarget(AActor *NewAimTarget);

  /**
   * 对玩家摄像机施加一次单发后坐力旋转。
   * 该函数用于在每次开火完成命中检测后，推动玩家视角产生一次瞬时抬枪/偏转效果。
   * 当外部未传入 PlayerCameraManager 时，函数会尝试从当前武器持有者反查本地玩家控制器。
   */
  UFUNCTION(BlueprintCallable, Category = "Range|Camera")
  void ApplyRecoilToCamera(APlayerCameraManager *PlayerCameraManager = nullptr);

protected:
  virtual void BeginPlay() override;

  /** 执行一次射击：消耗弹药、执行射线检测、施加伤害、广播事件。 */
  void FireOnce();

  /** 射速定时器回调，每次触发一次射击。 */
  void FireTimerCallback();

  /** 装弹定时器回调，装弹完成时补充弹药。 */
  void ReloadTimerCallback();

  /**
   * 执行射线扫描检测命中目标。
   * 射线起点和方向从持有者视角获取（玩家=摄像头，NPC=眼睛位置），
   * 当 AimTarget 有效时优先朝目标方向发射。
   */
  FHitResult PerformLineTrace();

  /**
   * 对命中的目标施加伤害。
   * 伤害值从组件缓存数据获取。
   * @param HitResult 命中结果。
   * @return 实际生效的伤害值。
   */
  float ApplyHitDamage(const FHitResult &HitResult);

  /** 消耗一发弹药，广播弹药变化事件。 */
  void ConsumeAmmo();

  /** 补充弹药到弹匣。 */
  void RefillMagazine();
};
