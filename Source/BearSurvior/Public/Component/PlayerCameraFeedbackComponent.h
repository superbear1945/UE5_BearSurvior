// 玩家相机反馈组件。挂载在玩家角色上，负责订阅当前装备远程武器的开火事件，
// 并在本地玩家开火后施加后坐力等相机表现效果。后续可在本组件内继续扩展
// 平滑回弹、受击抖动、奔跑晃动、开镜修正等相机反馈能力。

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "PlayerCameraFeedbackComponent.generated.h"

class AActor;
class AWeaponBase;
class URangeAttackComponent;
class ABearSurviorCharacter;

/**
 * 玩家相机反馈组件：统一管理玩家本地视角上的后坐力与其它相机表现。
 *
 * 设计目标：
 * 1. 组件只挂载在玩家角色身上，不直接参与武器攻击逻辑。
 * 2. 通过角色当前持有的 WeaponBase 作为中介，订阅远程武器的开火事件。
 * 3. 收到开火事件后，从远程攻击组件读取后坐力配置并施加到本地控制视角。
 * 4. 为后续加入平滑回弹、累积后坐力和其它相机效果预留集中扩展点。
 */
UCLASS(ClassGroup = (Camera), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class BEARSURVIOR_API UPlayerCameraFeedbackComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** 构造函数，初始化组件默认配置。 */
	UPlayerCameraFeedbackComponent();

	/**
	 * 使用当前角色持有的武器更新监听对象。
	 * 该函数适合在角色装备、切枪、卸下武器后调用，用于自动重建开火事件绑定关系。
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera|Feedback")
	void RefreshObservedWeaponFromOwner();

	/**
	 * 显式设置当前需要监听的武器。
	 * 传入空值时会自动解绑旧武器并清空当前监听状态。
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera|Feedback")
	void SetObservedWeapon(AWeaponBase* NewWeapon);

	/**
	 * 清理当前监听的武器与远程攻击组件引用。
	 * 通常在角色卸下武器、切枪或组件结束播放时调用。
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera|Feedback")
	void ClearObservedWeapon();

protected:
	/** 在游戏开始时缓存宿主角色引用，并按当前持武器状态建立初始监听关系。 */
	virtual void BeginPlay() override;

	/** 在组件结束播放时解绑事件，避免残留动态委托引用。 */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/**
	 * 处理监听到的远程武器开火事件。
	 * 该回调只负责把“开火发生”转换成“相机效果输入”，不关心命中结果本身。
	 */
	UFUNCTION()
	void HandleObservedRangeWeaponFire(FVector ImpactPoint, AActor* HitActor);

	/**
	 * 根据当前监听的远程攻击组件，对本地玩家控制视角施加一次后坐力输入。
	 * 该函数是后续扩展平滑回弹、累积后坐力的核心入口之一。
	 */
	void ApplyObservedWeaponRecoil();

	/**
	 * 从传入武器中解析远程攻击组件。
	 * 武器不支持远程攻击时返回空指针。
	 */
	URangeAttackComponent* ResolveRangeAttackComponent(AWeaponBase* InWeapon) const;

	/** 解绑当前缓存远程攻击组件上的开火事件。 */
	void UnbindObservedRangeAttackComponent();

protected:
	// 缓存宿主玩家角色，便于后续读取当前持有武器和向控制器施加视角输入。
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Camera|Feedback")
	TObjectPtr<ABearSurviorCharacter> OwnerCharacter = nullptr;

	// 当前正在监听的武器，仅作为观察目标缓存，不拥有该武器生命周期。
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Camera|Feedback")
	TObjectPtr<AWeaponBase> ObservedWeapon = nullptr;

	// 当前正在监听的远程攻击组件，用于绑定开火事件与读取后坐力配置。
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Camera|Feedback")
	TObjectPtr<URangeAttackComponent> ObservedRangeAttackComponent = nullptr;
};
