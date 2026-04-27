// 近战攻击组件。挂载在武器Actor上，负责近战攻击的命中检测与伤害结算。
// 使用球形扫描（SphereTrace）检测范围内目标，通过 UE5 标准 ApplyDamage 施加伤害。

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MeleeAttackComponent.generated.h"

// 近战命中事件。
// @param HitActor 被命中的Actor。
// @param HitResult 命中结果，包含命中位置、法线等信息。
// @param FinalDamage 实际生效的伤害值。
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnMeleeHitSignature, AActor*, HitActor, const FHitResult&, HitResult, float, FinalDamage);

/**
 * 近战攻击组件：管理近战攻击窗口、命中检测、伤害结算。
 *
 * 工作流程：
 *   1. 外部调用 BeginAttackWindow() 开启命中去重窗口。
 *   2. 外部在需要的时机主动调用 ExecuteAttack() 执行一次近战检测。
 *   3. 命中后通过 UE5 标准 ApplyDamage 接口施加伤害。
 *   4. 外部调用 EndAttackWindow() 关闭窗口，重置命中记录。
 *
 * 建议与动画通知（AnimNotify）配合使用：动画通知控制窗口开闭。
 */
UCLASS(ClassGroup = (Combat), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class BEARSURVIOR_API UMeleeAttackComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** 构造函数，初始化组件默认配置。 */
	UMeleeAttackComponent();

// ────────────────────────────────────────── 配置 ──────────────────────────────────────────

public:

	// 近战基础伤害值。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee|Config", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float BaseDamage;

	// 近战攻击范围（射线最大距离，厘米）。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee|Config", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float AttackRange;

	// 近战扫描球体半径（厘米），越大越容易命中但手感越"粘"。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee|Config", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float AttackRadius;

	// 每次攻击消耗的耐久值。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee|Config", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DurabilityCostPerAttack;

	// 攻击窗口内是否允许命中多个目标。为 false 时单次窗口只命中第一个目标。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee|Config")
	bool bCanHitMultipleTargets;

	// 用于近战射线检测的碰撞通道。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee|Config")
	TEnumAsByte<ECollisionChannel> TraceChannel;

// ────────────────────────────────────────── 状态 ──────────────────────────────────────────

protected:

	// 当前是否处于攻击窗口内。
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Melee|State")
	bool bIsInAttackWindow;

	// 本次攻击窗口内已经命中的Actor集合，防止同一目标重复受伤。
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Melee|State")
	TSet<TObjectPtr<AActor>> HitActorsThisSwing;

// ────────────────────────────────────────── 事件 ──────────────────────────────────────────

public:

	// 近战命中时广播，便于音效、特效、UI 等外部系统响应。
	UPROPERTY(BlueprintAssignable, Category = "Melee|Event")
	FOnMeleeHitSignature OnMeleeHit;

// ────────────────────────────────────────── 方法 ──────────────────────────────────────────

public:

	/**
	 * 开启攻击窗口，允许命中检测。
	 * 通常由动画通知（AnimNotify）在攻击动画的伤害帧调用。
	 */
	UFUNCTION(BlueprintCallable, Category = "Melee")
	void BeginAttackWindow();

	/**
	 * 关闭攻击窗口，重置命中记录。
	 * 通常由动画通知在攻击动画结束时调用。
	 */
	UFUNCTION(BlueprintCallable, Category = "Melee")
	void EndAttackWindow();

	/**
	 * 主动执行一次近战攻击检测。
	 * 当攻击窗口开启时，会使用 HitActorsThisSwing 防止同一挥击内重复命中。
	 * 当攻击窗口关闭时，会将本次检测视为独立攻击，不做历史去重。
	 */
	UFUNCTION(BlueprintCallable, Category = "Melee")
	void ExecuteAttack();

	/**
	 * 获取当前是否处于攻击窗口内。
	 */
	UFUNCTION(BlueprintPure, Category = "Melee")
	bool IsInAttackWindow() const;

protected:

	/**
	 * 执行近战命中检测，在攻击窗口内扫描可命中目标。
	 * 检测到目标后调用 ApplyHitDamage() 结算伤害。
	 */
	void PerformHitDetection();

	/**
	 * 对命中的目标施加伤害。
	 * 优先查找目标上的 UHealthComponent 调用 ApplyDamage，
	 * 若目标无 HealthComponent 则使用 UE5 标准 TakeDamage。
	 * @param HitResult 命中结果。
	 * @return 实际生效的伤害值，未造成伤害时返回 0。
	 */
	float ApplyHitDamage(const FHitResult& HitResult);

	/**
	 * 获取命中检测的起始位置和方向。
	 * 优先从宿主 Actor 的根组件位置出发，方向为宿主朝向。
	 * @param OutOrigin 输出：射线起始位置。
	 * @param OutDirection 输出：射线方向（已归一化）。
	 */
	void GetTraceOrigin(FVector& OutOrigin, FVector& OutDirection) const;
};
