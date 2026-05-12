// 远程攻击组件。挂载在武器Actor上，负责远程射击的命中检测、弹药管理与开火控制。
// 使用射线扫描（LineTrace）实现弹道命中检测，支持弹匣、装弹、自动/半自动射击模式。
// 设计期数据（射速、弹匣等）由武器的 DataTable 提供，通过 InitializeFromWeaponData 初始化。

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/HitResult.h"
#include "RangeAttackComponent.generated.h"

struct FRangedWeaponData;
struct FHitResult;

// 开火事件。
// @param ImpactPoint 命中位置。
// @param HitActor 被命中的Actor，未命中时为空。
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRangeWeaponFireSignature, FVector, ImpactPoint, AActor*, HitActor);

// 开始装弹事件。
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnReloadStartedSignature);

// 装弹完成事件。
// @param NewAmmoCount 装弹完成后的弹药数。
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnReloadFinishedSignature, int32, NewAmmoCount);

// 弹药变化事件。
// @param CurrentMag 当前弹匣内弹药数。
// @param MaxMag 弹匣最大容量。
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAmmoChangedSignature, int32, CurrentMag, int32, MaxMag);

/**
 * 远程攻击组件：管理远程武器的开火、命中检测、弹药消耗与装弹流程。
 *
 * 工作流程：
 *   1. 外部调用 StartFire() 开始射击（组件自动处理开火节奏与弹药消耗）。
 *   2. 组件按 FireRate 执行射线扫描（LineTrace）检测命中。
 *   3. 弹药耗尽时自动或手动触发 Reload() 进行装弹。
 *   4. 外部调用 StopFire() 停止射击。
 *
 * 支持半自动（点射）和全自动（按住不放连续射击）模式。
 */
UCLASS(ClassGroup = (Combat), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class BEARSURVIOR_API URangeAttackComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** 构造函数，初始化组件默认配置。 */
	URangeAttackComponent();

// ────────────────────────────────────────── 配置 ──────────────────────────────────────────

public:

	// 可选的瞄准目标。设置后优先朝该目标方向射击；为空时沿宿主前向射击。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Range|Config")
	TObjectPtr<AActor> AimTarget;

	// 用于射线检测的碰撞通道。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Range|Config")
	TEnumAsByte<ECollisionChannel> TraceChannel;

// ────────────────────────────────────────── 状态 ──────────────────────────────────────────

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

	// 开火定时器句柄，用于控制射速节奏。
	FTimerHandle FireTimerHandle;

	// 装弹定时器句柄，用于控制装弹时长。
	FTimerHandle ReloadTimerHandle;

	// ────── 缓存的设计期数据（由 InitializeFromWeaponData 设置） ──────

	// 基础伤害值，从武器 DataTable 读取。
	float CachedBaseDamage;

	// 每秒射击次数，从武器 DataTable 读取。
	float CachedFireRate;

	// 弹匣容量，从武器 DataTable 读取。
	int32 CachedMagazineCapacity;

	// 装弹所需时间，从武器 DataTable 读取。
	float CachedReloadTime;

	// 子弹散布角度，从武器 DataTable 读取。
	float CachedSpreadAngle;

	// 是否为全自动射击模式，从武器 DataTable 读取。
	bool bCachedAutomaticFire;

	// 最大有效射程，从武器 DataTable 读取。
	float CachedMaxRange;

	// 每次射击消耗的耐久值，从武器 DataTable 读取。
	float CachedDurabilityCostPerShot;

	// 储备弹药总数，从武器 DataTable 读取。-1 表示无限弹药。
	int32 CachedReserveAmmo;

// ────────────────────────────────────────── 事件 ──────────────────────────────────────────

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

// ────────────────────────────────────────── 方法 ──────────────────────────────────────────

public:

	/**
	 * 从远程武器 DataTable 行数据初始化组件配置。
	 * 由宿主 AWeaponBase::InitializeAttackComponents 在 BeginPlay 中调用。
	 * @param Data DataTable 行中解析出的远程武器数据。
	 */
	void InitializeFromWeaponData(const FRangedWeaponData& Data);

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
	void SetAimTarget(AActor* NewAimTarget);

protected:

	/**
	 * 获取射击的起始位置和方向。
	 * 优先通过持有者（角色）的 GetActorEyesViewPoint 获取视角信息：
	 *   - 玩家角色返回摄像头位置和朝向。
	 *   - NPC 角色返回眼睛位置和控制器旋转。
	 * 当 AimTarget 有效时方向指向目标。无持有者时回退到武器位置和朝向。
	 */
	void GetTraceOriginAndDirection(FVector& OutStart, FVector& OutDirection) const;

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
	float ApplyHitDamage(const FHitResult& HitResult);

	/** 消耗一发弹药，广播弹药变化事件。 */
	void ConsumeAmmo();

	/** 补充弹药到弹匣。 */
	void RefillMagazine();
};
