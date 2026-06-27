// 玩家相机反馈组件实现文件。
// 负责监听当前装备远程武器的开火事件，并将武器后坐力配置转换为本地玩家视角输入。
// 后续平滑回弹、累积后坐力、受击抖动等相机表现建议继续在本组件内集中扩展。

#include "Component/PlayerCameraFeedbackComponent.h"
#include "BearSurviorCharacter.h"
#include "Component/RangeAttackComponent.h"
#include "GameFramework/PlayerController.h"
#include "Weapon/WeaponBase.h"

/**
 * 初始化相机反馈组件默认属性。
 */
UPlayerCameraFeedbackComponent::UPlayerCameraFeedbackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

/**
 * 在游戏开始时缓存宿主角色，并尝试根据角色当前持有武器建立监听关系。
 */
void UPlayerCameraFeedbackComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ABearSurviorCharacter>(GetOwner());
	RefreshObservedWeaponFromOwner();
}

/**
 * 在组件结束播放时解绑委托，避免远程攻击组件仍持有本组件的动态回调引用。
 */
void UPlayerCameraFeedbackComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearObservedWeapon();
	Super::EndPlay(EndPlayReason);
}

/**
 * 使用宿主角色当前持有的武器刷新监听对象。
 * 仅当宿主是 BearSurviorCharacter 时才会继续处理。
 */
void UPlayerCameraFeedbackComponent::RefreshObservedWeaponFromOwner()
{
	if (!OwnerCharacter)
		OwnerCharacter = Cast<ABearSurviorCharacter>(GetOwner());

	if (!OwnerCharacter)
	{
		ClearObservedWeapon();
		return;
	}

	SetObservedWeapon(Cast<AWeaponBase>(OwnerCharacter->GetCurrentHeldItem()));
}

/**
 * 显式设置当前需要监听的武器。
 * 若新旧武器相同，则保持现有绑定不变，避免重复解绑和重复绑定。
 */
void UPlayerCameraFeedbackComponent::SetObservedWeapon(AWeaponBase* NewWeapon)
{
	if (ObservedWeapon == NewWeapon)
		return;

	UnbindObservedRangeAttackComponent();
	ObservedWeapon = NewWeapon;
	ObservedRangeAttackComponent = ResolveRangeAttackComponent(NewWeapon);

	if (!ObservedRangeAttackComponent)
		return;

	ObservedRangeAttackComponent->OnFire.AddDynamic(this, &UPlayerCameraFeedbackComponent::HandleObservedRangeWeaponFire);
}

/**
 * 清理当前监听武器与远程攻击组件缓存。
 */
void UPlayerCameraFeedbackComponent::ClearObservedWeapon()
{
	UnbindObservedRangeAttackComponent();
	ObservedWeapon = nullptr;
	ObservedRangeAttackComponent = nullptr;
}

/**
 * 处理远程攻击组件广播的开火事件。
 * 当前只把该事件作为相机表现触发信号，命中位置和命中对象保留给其它系统使用。
 */
void UPlayerCameraFeedbackComponent::HandleObservedRangeWeaponFire(FVector ImpactPoint, AActor* HitActor)
{
	ApplyObservedWeaponRecoil();
}

/**
 * 对当前本地玩家控制视角施加一次后坐力输入。
 * 仅本地玩家控制角色会执行该逻辑，AI 和远端代理不会产生多余计算。
 */
void UPlayerCameraFeedbackComponent::ApplyObservedWeaponRecoil()
{
	if (!OwnerCharacter || !ObservedRangeAttackComponent)
		return;

	APlayerController* PlayerController = Cast<APlayerController>(OwnerCharacter->GetController());
	if (!IsValid(PlayerController) || !PlayerController->IsLocalController())
		return;

	const FRotator RecoilRotation = ObservedRangeAttackComponent->GetRecoilRotation();
	if (RecoilRotation.IsNearlyZero())
		return;

	OwnerCharacter->AddControllerPitchInput(-RecoilRotation.Pitch);
	OwnerCharacter->AddControllerYawInput(RecoilRotation.Yaw);
}

/**
 * 从武器中解析远程攻击组件。
 * 组件只依赖 WeaponBase 对攻击组件的统一访问入口，不主动深入其它武器内部细节。
 */
URangeAttackComponent* UPlayerCameraFeedbackComponent::ResolveRangeAttackComponent(AWeaponBase* InWeapon) const
{
	if (!InWeapon)
		return nullptr;

	return InWeapon->GetRangeAttackComponent();
}

/**
 * 从当前监听的远程攻击组件上移除开火事件绑定。
 * 动态委托在移除前需要先判断组件是否仍然有效。
 */
void UPlayerCameraFeedbackComponent::UnbindObservedRangeAttackComponent()
{
	if (!ObservedRangeAttackComponent)
		return;

	ObservedRangeAttackComponent->OnFire.RemoveDynamic(this, &UPlayerCameraFeedbackComponent::HandleObservedRangeWeaponFire);
	ObservedRangeAttackComponent = nullptr;
}
