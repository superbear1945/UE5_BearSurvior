// 远程攻击组件实现文件。
// 设计期数据由宿主武器的 DataTable 提供，通过 InitializeFromWeaponData 设置。

#include "Component/RangeAttackComponent.h"
#include "Component/HealthComponent.h"
#include "Weapon/WeaponDataTypes.h"
#include "Engine/HitResult.h"
#include "Engine.h"
#include "Engine/World.h"
#include "Kismet/KismetMathLibrary.h"

/**
 * 初始化组件默认配置。
 * 设计期数值由 InitializeFromWeaponData 在 BeginPlay 时设置。
 */
URangeAttackComponent::URangeAttackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// 配置默认值（防止 DataTable 未加载时引用未初始化数据）。
	CachedBaseDamage = 20.0f;
	CachedFireRate = 600.0f;
	CachedMagazineCapacity = 30;
	CachedReloadTime = 2.0f;
	CachedSpreadAngle = 1.0f;
	bCachedAutomaticFire = true;
	CachedMaxRange = 10000.0f;
	CachedDurabilityCostPerShot = 0.5f;
	CachedReserveAmmo = 90;
	TraceChannel = ECC_Visibility;

	// 运行时状态默认值。
	bIsFiring = false;
	bIsReloading = false;
	CurrentAmmoInMagazine = 0;
	AimTarget = nullptr;
}

/**
 * 从远程武器 DataTable 行数据初始化组件配置。
 * 由宿主 AWeaponBase::InitializeAttackComponents 在 BeginPlay 中调用。
 */
void URangeAttackComponent::InitializeFromWeaponData(const FRangedWeaponData& Data)
{
	CachedBaseDamage = Data.BaseDamage;
	CachedFireRate = Data.FireRate;
	CachedMagazineCapacity = Data.MagazineCapacity;
	CachedReloadTime = Data.ReloadTime;
	CachedSpreadAngle = Data.SpreadAngle;
	bCachedAutomaticFire = Data.bAutomaticFire;
	CachedMaxRange = Data.MaxRange;
	CachedDurabilityCostPerShot = Data.DurabilityCostPerShot;
	CachedReserveAmmo = Data.ReserveAmmo;

	// 初始化弹匣弹药数。
	CurrentAmmoInMagazine = CachedMagazineCapacity;
}

/**
 * 开始射击。按配置的射击模式处理开火节奏。
 */
void URangeAttackComponent::StartFire()
{
	if (bIsReloading)
		return;

	if (CurrentAmmoInMagazine <= 0)
	{
		Reload();
		return;
	}

	bIsFiring = true;
	FireOnce();

	if (bCachedAutomaticFire && CachedFireRate > 0.0f)
	{
		const float FireInterval = 1.0f / CachedFireRate;
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
		GetWorld()->GetTimerManager().ClearTimer(FireTimerHandle);
}

/**
 * 触发装弹。弹匣已满或正在装弹时不会重复触发。
 */
void URangeAttackComponent::Reload()
{
	if (CurrentAmmoInMagazine >= CachedMagazineCapacity)
		return;

	if (CachedReserveAmmo == 0)
		return;

	if (bIsReloading)
		return;

	StopFire();

	bIsReloading = true;
	OnReloadStarted.Broadcast();

	GetWorld()->GetTimerManager().SetTimer(
		ReloadTimerHandle,
		this,
		&URangeAttackComponent::ReloadTimerCallback,
		CachedReloadTime,
		false
	);
}

/**
 * 取消装弹。
 */
void URangeAttackComponent::CancelReload()
{
	if (!bIsReloading)
		return;

	bIsReloading = false;

	if (ReloadTimerHandle.IsValid())
		GetWorld()->GetTimerManager().ClearTimer(ReloadTimerHandle);
}

/**
 * 返回武器当前是否可以射击。
 */
bool URangeAttackComponent::CanFire() const
{
	if (bIsReloading)
		return false;

	if (CurrentAmmoInMagazine <= 0)
		return false;

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
	return CurrentAmmoInMagazine >= CachedMagazineCapacity;
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
	return CachedMagazineCapacity;
}

/**
 * 返回弹药百分比（0~1）。
 */
float URangeAttackComponent::GetAmmoPercent() const
{
	if (CachedMagazineCapacity <= 0)
		return 0.0f;
	return static_cast<float>(CurrentAmmoInMagazine) / static_cast<float>(CachedMagazineCapacity);
}

/**
 * 添加弹药到储备中。
 */
void URangeAttackComponent::AddReserveAmmo(int32 Amount)
{
	if (CachedReserveAmmo < 0)
		return;

	CachedReserveAmmo = FMath::Max(0, CachedReserveAmmo + Amount);
}

/**
 * 设置当前瞄准目标。
 */
void URangeAttackComponent::SetAimTarget(AActor* NewAimTarget)
{
	AimTarget = NewAimTarget;
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

	ConsumeAmmo();

	FHitResult HitResult = PerformLineTrace();

	FVector ImpactPoint = HitResult.ImpactPoint;
	if (!HitResult.bBlockingHit)
	{
		// 未命中时，计算射程末端位置作为 ImpactPoint。
		FVector Start, Direction;
		GetTraceOriginAndDirection(Start, Direction);
		ImpactPoint = Start + Direction * CachedMaxRange;
	}

	float FinalDamage = 0.0f;
	AActor* HitActor = nullptr;
	if (HitResult.bBlockingHit && HitResult.GetActor())
	{
		HitActor = HitResult.GetActor();
		FinalDamage = ApplyHitDamage(HitResult);
	}

	OnFire.Broadcast(ImpactPoint, HitActor);

	if (CurrentAmmoInMagazine <= 0)
	{
		StopFire();

		if (CachedReserveAmmo != 0)
			Reload();
	}
}

/**
 * 射速定时器回调。
 */
void URangeAttackComponent::FireTimerCallback()
{
	if (!bIsFiring)
		return;

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
 * 获取射击的起始位置和方向。
 * 优先通过持有者（角色）的 GetActorEyesViewPoint 获取视角信息。
 * 当 AimTarget 有效时方向指向目标。无持有者时回退到武器位置和朝向。
 */
void URangeAttackComponent::GetTraceOriginAndDirection(FVector& OutStart, FVector& OutDirection) const
{
	AActor* Owner = GetOwner();
	if (!Owner)
		return;

	// 尝试从持有者（角色）获取眼睛位置和朝向。
	// 玩家角色返回摄像头位置和朝向，NPC 默认返回 Actor位置+眼睛高度+控制器旋转。
	APawn* WeaponOwner = Owner->GetInstigator();

	UE_LOG(LogTemp, Warning, TEXT("WeaponOwner: %s"), WeaponOwner ? *WeaponOwner->GetName() : TEXT("None"));

	if (WeaponOwner)
	{
		FRotator EyeRotation;
		WeaponOwner->GetActorEyesViewPoint(OutStart, EyeRotation);
		OutDirection = EyeRotation.Vector();

		// 有瞄准目标时覆盖方向，指向目标位置。
		if (AimTarget && AimTarget != WeaponOwner)
		{
			const FVector ToTarget = (AimTarget->GetActorLocation() - OutStart).GetSafeNormal();
			if (!ToTarget.IsNearlyZero())
				OutDirection = ToTarget;
		}
		return;
	}

	// 回退：使用武器位置和朝向（无持有者场景，如固定炮台）。
	OutStart = Owner->GetActorLocation();
	OutDirection = Owner->GetActorForwardVector();
}

/**
 * 执行射线扫描检测命中目标。
 * 射线起点和方向从持有者视角获取（玩家=摄像头，NPC=眼睛位置），
 * 当 AimTarget 有效时优先朝目标方向发射。
 */
FHitResult URangeAttackComponent::PerformLineTrace()
{
	FHitResult HitResult;

	AActor* Owner = GetOwner();
	if (!Owner)
		return HitResult;

	// 获取射线起点和方向（从持有者眼睛位置）。
	FVector Start, Direction;
	GetTraceOriginAndDirection(Start, Direction);

	// 应用散布角度。
	if (CachedSpreadAngle > 0.0f)
	{
		const float HalfAngleRad = FMath::DegreesToRadians(CachedSpreadAngle * 0.5f);
		const float RandomYaw = FMath::FRandRange(-HalfAngleRad, HalfAngleRad);
		const float RandomPitch = FMath::FRandRange(-HalfAngleRad, HalfAngleRad);
		const FRotator SpreadRotator(FMath::RadiansToDegrees(RandomPitch), FMath::RadiansToDegrees(RandomYaw), 0.0f);
		Direction = SpreadRotator.RotateVector(Direction);
	}

	const FVector End = Start + Direction * CachedMaxRange;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Owner);

	// 同时忽略持有者（角色自身），避免射线打到自己。
	APawn* WeaponOwner = Owner->GetInstigator();
	if (WeaponOwner)
		QueryParams.AddIgnoredActor(WeaponOwner);

	GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, TraceChannel, QueryParams);

	return HitResult;
}

/**
 * 对命中的目标施加伤害。
 * 伤害值从组件的缓存数据获取。
 */
float URangeAttackComponent::ApplyHitDamage(const FHitResult& HitResult)
{
	AActor* HitActor = HitResult.GetActor();
	if (!HitActor)
		return 0.0f;

	AActor* Owner = GetOwner();
	if (!Owner)
		return 0.0f;

	UHealthComponent* HealthComp = HitActor->FindComponentByClass<UHealthComponent>();
	if (HealthComp)
		return HealthComp->ApplyDamage(CachedBaseDamage, Owner);

	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Blue, FString::Printf(TEXT("目标 %s 无 HealthComponent，未施加伤害"), *HitActor->GetName()));
	return 0.0f;
}

/**
 * 消耗一发弹药。
 */
void URangeAttackComponent::ConsumeAmmo()
{
	if (CurrentAmmoInMagazine > 0)
		CurrentAmmoInMagazine--;

	OnAmmoChanged.Broadcast(CurrentAmmoInMagazine, CachedMagazineCapacity);
}

/**
 * 补充弹药到弹匣。
 */
void URangeAttackComponent::RefillMagazine()
{
	const int32 AmmoNeeded = CachedMagazineCapacity - CurrentAmmoInMagazine;

	if (AmmoNeeded <= 0)
		return;

	if (CachedReserveAmmo < 0)
	{
		CurrentAmmoInMagazine = CachedMagazineCapacity;
	}
	else
	{
		const int32 AmmoToTake = FMath::Min(AmmoNeeded, CachedReserveAmmo);
		CurrentAmmoInMagazine += AmmoToTake;
		CachedReserveAmmo -= AmmoToTake;
	}

	OnAmmoChanged.Broadcast(CurrentAmmoInMagazine, CachedMagazineCapacity);
}
