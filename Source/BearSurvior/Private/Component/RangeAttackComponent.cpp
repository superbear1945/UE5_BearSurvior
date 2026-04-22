// 远程攻击组件实现文件。

#include "Component/RangeAttackComponent.h"
#include "Component/HealthComponent.h"
#include "Engine.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetMathLibrary.h"

/**
 * 初始化组件默认配置。
 */
URangeAttackComponent::URangeAttackComponent()
{
	// 远程组件通过定时器驱动开火节奏，不需要逐帧 Tick。
	PrimaryComponentTick.bCanEverTick = false;

	// 配置默认值。
	BaseDamage = 20.0f;
	FireRate = 600.0f;
	MagazineCapacity = 30;
	ReloadTime = 2.0f;
	SpreadAngle = 1.0f;
	bAutomaticFire = true;
	MaxRange = 10000.0f;
	DurabilityCostPerShot = 0.5f;
	ReserveAmmo = 90;
	TraceChannel = ECC_Visibility;

	// 状态默认值。
	bIsFiring = false;
	bIsReloading = false;
	CurrentAmmoInMagazine = 0;
}

/**
 * 开始射击。按配置的射击模式处理开火节奏。
 */
void URangeAttackComponent::StartFire()
{
	// 正在装弹时不允许射击。
	if (bIsReloading)
	{
		return;
	}

	// 弹匣为空时尝试自动装弹。
	if (CurrentAmmoInMagazine <= 0)
	{
		Reload();
		return;
	}

	bIsFiring = true;

	// 立即射击第一发。
	FireOnce();

	// 全自动模式下启动定时器持续射击。
	if (bAutomaticFire && FireRate > 0.0f)
	{
		const float FireInterval = 1.0f / FireRate;
		GetWorld()->GetTimerManager().SetTimer(
			FireTimerHandle,
			this,
			&URangeAttackComponent::FireTimerCallback,
			FireInterval,
			true
		);
	}
}

/**
 * 停止射击，取消开火定时器。
 */
void URangeAttackComponent::StopFire()
{
	bIsFiring = false;

	if (FireTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(FireTimerHandle);
	}
}

/**
 * 触发装弹。弹匣已满或正在装弹时不会重复触发。
 */
void URangeAttackComponent::Reload()
{
	// 弹匣已满，无需装弹。
	if (CurrentAmmoInMagazine >= MagazineCapacity)
	{
		return;
	}

	// 无储备弹药且非无限弹药模式，无法装弹。
	if (ReserveAmmo == 0)
	{
		return;
	}

	// 正在装弹中，不重复触发。
	if (bIsReloading)
	{
		return;
	}

	// 停止射击。
	StopFire();

	bIsReloading = true;
	OnReloadStarted.Broadcast();

	// 启动装弹定时器。
	GetWorld()->GetTimerManager().SetTimer(
		ReloadTimerHandle,
		this,
		&URangeAttackComponent::ReloadTimerCallback,
		ReloadTime,
		false
	);
}

/**
 * 取消装弹。
 */
void URangeAttackComponent::CancelReload()
{
	if (!bIsReloading)
	{
		return;
	}

	bIsReloading = false;

	if (ReloadTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(ReloadTimerHandle);
	}
}

/**
 * 返回武器当前是否可以射击。
 */
bool URangeAttackComponent::CanFire() const
{
	if (bIsReloading)
	{
		return false;
	}

	if (CurrentAmmoInMagazine <= 0)
	{
		return false;
	}

	return true;
}

/**
 * 返回当前是否正在射击。
 */
bool URangeAttackComponent::IsFiring() const
{
	return bIsFiring;
}

/**
 * 返回当前是否正在装弹。
 */
bool URangeAttackComponent::IsReloading() const
{
	return bIsReloading;
}

/**
 * 返回弹匣是否为空。
 */
bool URangeAttackComponent::IsMagazineEmpty() const
{
	return CurrentAmmoInMagazine <= 0;
}

/**
 * 返回弹匣是否已满。
 */
bool URangeAttackComponent::IsMagazineFull() const
{
	return CurrentAmmoInMagazine >= MagazineCapacity;
}

/**
 * 返回当前弹匣内弹药数。
 */
int32 URangeAttackComponent::GetCurrentAmmo() const
{
	return CurrentAmmoInMagazine;
}

/**
 * 返回弹匣容量。
 */
int32 URangeAttackComponent::GetMagazineCapacity() const
{
	return MagazineCapacity;
}

/**
 * 返回弹药百分比（0~1）。
 */
float URangeAttackComponent::GetAmmoPercent() const
{
	if (MagazineCapacity <= 0)
	{
		return 0.0f;
	}
	return static_cast<float>(CurrentAmmoInMagazine) / static_cast<float>(MagazineCapacity);
}

/**
 * 添加弹药到储备中。
 */
void URangeAttackComponent::AddReserveAmmo(int32 Amount)
{
	// 无限弹药模式下不需要添加。
	if (ReserveAmmo < 0)
	{
		return;
	}

	ReserveAmmo = FMath::Max(0, ReserveAmmo + Amount);
}

/**
 * 执行一次射击：消耗弹药、执行射线检测、施加伤害、广播事件。
 */
void URangeAttackComponent::FireOnce()
{
	if (!CanFire())
	{
		StopFire();
		return;
	}

	// 消耗弹药。
	ConsumeAmmo();

	// 执行射线检测。
	FHitResult HitResult = PerformLineTrace();

	// 计算命中点（若未命中任何物体，取射线终点作为ImpactPoint）。
	FVector ImpactPoint = HitResult.ImpactPoint;
	if (!HitResult.bBlockingHit)
	{
		// 未命中任何物体时，取射线最大距离处作为 ImpactPoint。
		AActor* Owner = GetOwner();
		if (Owner)
		{
			const FVector Start = Owner->GetActorLocation();
			const FVector Direction = Owner->GetActorForwardVector();
			ImpactPoint = Start + Direction * MaxRange;
		}
	}

	// 施加伤害并获取最终伤害值。
	float FinalDamage = 0.0f;
	AActor* HitActor = nullptr;
	if (HitResult.bBlockingHit && HitResult.GetActor())
	{
		HitActor = HitResult.GetActor();
		FinalDamage = ApplyHitDamage(HitResult);
	}

	// 广播开火事件，供外部处理枪口特效、弹壳、音效等。
	OnFire.Broadcast(ImpactPoint, HitActor);

	// 弹药耗尽时自动装弹。
	if (CurrentAmmoInMagazine <= 0)
	{
		StopFire();

		// 有储备弹药时自动装弹。
		if (ReserveAmmo != 0)
		{
			Reload();
		}
	}
}

/**
 * 射速定时器回调。
 */
void URangeAttackComponent::FireTimerCallback()
{
	if (!bIsFiring)
	{
		return;
	}

	FireOnce();
}

/**
 * 装弹定时器回调。
 */
void URangeAttackComponent::ReloadTimerCallback()
{
	RefillMagazine();
	bIsReloading = false;

	OnReloadFinished.Broadcast(CurrentAmmoInMagazine);
}

/**
 * 执行射线扫描检测命中目标。
 * 从宿主 Actor 位置出发，沿宿主朝向执行射线检测。
 * 支持散布角度，射线方向会在此角度内随机偏移。
 */
FHitResult URangeAttackComponent::PerformLineTrace()
{
	FHitResult HitResult;

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return HitResult;
	}

	const FVector Start = Owner->GetActorLocation();
	FVector Direction = Owner->GetActorForwardVector();

	// 应用散布偏移：在锥形角度内随机偏移射线方向。
	if (SpreadAngle > 0.0f)
	{
		const float HalfAngleRad = FMath::DegreesToRadians(SpreadAngle * 0.5f);

		// 在散布锥形内随机偏移。
		const float RandomYaw = FMath::FRandRange(-HalfAngleRad, HalfAngleRad);
		const float RandomPitch = FMath::FRandRange(-HalfAngleRad, HalfAngleRad);

		// 使用旋转矩阵将散布偏移应用到朝向上。
		const FRotator SpreadRotator(FMath::RadiansToDegrees(RandomPitch), FMath::RadiansToDegrees(RandomYaw), 0.0f);
		Direction = SpreadRotator.RotateVector(Direction);
	}

	const FVector End = Start + Direction * MaxRange;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Owner);

	GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, TraceChannel, QueryParams);

	return HitResult;
}

/**
 * 对命中的目标施加伤害。
 * 优先查找目标上的 UHealthComponent。
 */
float URangeAttackComponent::ApplyHitDamage(const FHitResult& HitResult)
{
	AActor* HitActor = HitResult.GetActor();
	if (!HitActor)
	{
		return 0.0f;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return 0.0f;
	}

	// 优先查找目标上的 HealthComponent。
	UHealthComponent* HealthComp = HitActor->FindComponentByClass<UHealthComponent>();
	if (HealthComp)
	{
		return HealthComp->ApplyDamage(BaseDamage, Owner);
	}

	// 若目标无 HealthComponent，则不施加伤害并输出警告。
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Blue, FString::Printf(TEXT("目标 %s 无 HealthComponent，未施加伤害"), *HitActor->GetName()));
	return 0.0f;
}

/**
 * 消耗一发弹药。
 */
void URangeAttackComponent::ConsumeAmmo()
{
	if (CurrentAmmoInMagazine > 0)
	{
		CurrentAmmoInMagazine--;
	}

	OnAmmoChanged.Broadcast(CurrentAmmoInMagazine, MagazineCapacity);
}

/**
 * 补充弹药到弹匣。
 */
void URangeAttackComponent::RefillMagazine()
{
	// 计算需要补充的弹药数。
	const int32 AmmoNeeded = MagazineCapacity - CurrentAmmoInMagazine;

	if (AmmoNeeded <= 0)
	{
		return;
	}

	// 无限弹药模式（ReserveAmmo < 0），直接填满。
	if (ReserveAmmo < 0)
	{
		CurrentAmmoInMagazine = MagazineCapacity;
	}
	else
	{
		// 从储备中取弹药，补充到弹匣中。
		const int32 AmmoToTake = FMath::Min(AmmoNeeded, ReserveAmmo);
		CurrentAmmoInMagazine += AmmoToTake;
		ReserveAmmo -= AmmoToTake;
	}

	OnAmmoChanged.Broadcast(CurrentAmmoInMagazine, MagazineCapacity);
}
