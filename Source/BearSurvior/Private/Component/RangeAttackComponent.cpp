// 远程攻击组件实现文件。
// 设计期数据由独立 FRangedWeaponData 提供，通过 InitializeFromWeaponData 设置。

#include "Component/RangeAttackComponent.h"
#include "Component/HealthComponent.h"
#include "Weapon/WeaponDataTypes.h"
#include "GameFramework/Character.h"
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
	CachedAttackInterval = 0.1f;
	CachedFireRate = 600.0f;
	CachedMagazineCapacity = 30;
	CachedReloadTime = 2.0f;
	CachedSpreadAngle = 1.0f;
	bCachedAutomaticFire = true;
	CachedMaxRange = 10000.0f;
	CachedDurabilityCostPerShot = 0.5f;
	CachedReserveAmmo = 90;
	CachedGunshotSound = nullptr;
	LoadedGunshotSound = nullptr;
	TraceChannel = ECC_Visibility;

	// 数据缓存指针初始化。
	CachedRangedWeaponData = nullptr;

	// 运行时状态默认值。
	bIsFiring = false;
	bIsReloading = false;
	CurrentAmmoInMagazine = 0;
	AimTarget = nullptr;
	CachedAimLocation = FVector::ZeroVector;
	CachedAimDirection = FVector::ZeroVector;

	// 创建弹匣网格组件（可选），并设置默认碰撞配置。
	if (MagazineMeshComponent == nullptr)
	{
		MagazineMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MagazineMesh"));
		MagazineMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void URangeAttackComponent::BeginPlay()
{
	// 父类调用了ResolveWeaponData来解析数据
	Super::BeginPlay();

	// 组件在 BeginPlay 时不直接解析数据，等待宿主 AWeaponBase 调用 InitializeFromWeaponData。
}

/**
 * 从独立远程武器 DataTable 行数据初始化组件配置。
 * 由宿主 AWeaponBase::InitializeAttackComponents 在 BeginPlay 中调用。
 */
void URangeAttackComponent::InitializeFromWeaponData(const FRangedWeaponData& Data)
{
	CachedBaseDamage = Data.BaseDamage;
	CachedAttackInterval = Data.AttackInterval;
	CachedFireRate = Data.FireRate;
	CachedMagazineCapacity = Data.MagazineCapacity;
	CachedReloadTime = Data.ReloadTime;
	CachedSpreadAngle = Data.SpreadAngle;
	bCachedAutomaticFire = Data.bAutomaticFire;
	CachedMaxRange = Data.MaxRange;
	CachedDurabilityCostPerShot = Data.DurabilityCostPerShot;
	CachedReserveAmmo = Data.ReserveAmmo;
	CachedGunshotSound = Data.GunshotSound;

	// 初始化弹匣弹药数。
	CurrentAmmoInMagazine = CachedMagazineCapacity;

	// 初始化弹匣mesh
	if (MagazineMeshComponent && Data.MagazineMesh)
	{
		MagazineMeshComponent->SetStaticMesh(Data.MagazineMesh.LoadSynchronous());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] 远程武器数据缺少 MagazineMesh，弹匣外观将不可用"), *GetNameSafe(this));
	}
}

/**
 * 解析 RangedWeaponDataRow 指向的远程武器 DataTable 行，并缓存到 CachedRangedWeaponData。
 * 数据无效时保留构造函数中的默认值，避免影响未接入 DataTable 的测试武器。
 */
void URangeAttackComponent::ResolveWeaponData()
{
	if (!RangedWeaponDataRow.DataTable || RangedWeaponDataRow.RowName.IsNone())
		return;

	static const FString Context(TEXT("RangedWeaponDataResolve"));
	CachedRangedWeaponData = RangedWeaponDataRow.DataTable->FindRow<FRangedWeaponData>(RangedWeaponDataRow.RowName, Context);

	if (!CachedRangedWeaponData)
	{
		UE_LOG(LogTemp, Error, TEXT("[%s] 解析 RangedWeaponDataRow 失败，表: %s，行: %s"),
			*GetNameSafe(this),
			*GetNameSafe(RangedWeaponDataRow.DataTable),
			*RangedWeaponDataRow.RowName.ToString());
		return;
	}

	// 将解析出的数据同步到组件缓存字段。
	InitializeFromWeaponData(*CachedRangedWeaponData);
}

// 空远程武器数据静态实例，用于空指针兜底。
static const FRangedWeaponData EmptyRangedWeaponData;

/**
 * 返回缓存的远程武器数据引用。
 */
const FRangedWeaponData& URangeAttackComponent::GetRangedWeaponData() const
{
	return CachedRangedWeaponData ? *CachedRangedWeaponData : EmptyRangedWeaponData;
}

/**
 * 开始远程攻击。
 * 半自动武器仅在 StartFire() 内射击一次；全自动武器会由 StartFire() 根据射速开启循环定时器。
 */
bool URangeAttackComponent::StartAttack(const FVector& AimLocation, const FVector& AimDirection)
{
	if (!CanAttack())
		return false;

	// 缓存角色输入链路传入的瞄准数据，如果传入的数据接近0，则说明无瞄准输入
	CachedAimLocation = AimLocation;
	CachedAimDirection = AimDirection.GetSafeNormal();

	MarkFirstAttackTime();
	StartFire();
	return true;
}

/**
 * 结束远程攻击。
 * 松开输入时统一停止射击，确保全自动武器清理开火定时器，半自动武器重置输入按住状态。
 */
void URangeAttackComponent::StopAttack()
{
	StopFire();
	CachedAimDirection = FVector::ZeroVector;
}

/**
 * 判断远程组件当前是否可以发起攻击。
 */
bool URangeAttackComponent::CanAttack() const
{
	if (!Super::CanAttack())
		return false;

	if (bIsFiring)
		return false;

	return CanFire();
}

/**
 * 返回当前远程组件管理的攻击间隔。
 */
float URangeAttackComponent::GetAttackInterval() const
{
	return CachedAttackInterval;
}

/**
 * 返回当前远程组件管理的基础伤害。
 */
float URangeAttackComponent::GetBaseDamage() const
{
	return CachedBaseDamage;
}

/**
 * 返回当前远程组件管理的默认耐久消耗。
 */
float URangeAttackComponent::GetDefaultDurabilityCost() const
{
	return CachedDurabilityCostPerShot;
}

/**
 * 返回当前远程组件是否已经成功加载 DataTable 数据。
 */
bool URangeAttackComponent::IsDataLoaded() const
{
	return CachedRangedWeaponData != nullptr;
}

/**
 * 返回当前缓存的枪声资源引用。
 */
TSoftObjectPtr<USoundBase> URangeAttackComponent::GetGunshotSound() const
{
	return CachedGunshotSound;
}

/**
 * 返回当前已解析并缓存的枪声资源硬引用。
 */
USoundBase* URangeAttackComponent::GetLoadedGunshotSound() const
{
	return LoadedGunshotSound;
}

/**
 * 开始射击。按配置的射击模式处理开火节奏。
 */
void URangeAttackComponent::StartFire()
{
	if (bIsFiring)
		return;

	if (bIsReloading)
		return;

	if (CurrentAmmoInMagazine <= 0)
	{
		Reload();
		return;
	}

	FireOnce();
	bIsFiring = true;

	if (bCachedAutomaticFire && CachedFireRate > 0.0f)
	{
		// FireRate 使用每分钟射击次数（RPM）配置，因此这里需要先换算成每发之间的秒数。
		const float FireInterval = 60.0f / CachedFireRate;
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

	// 播放枪声（如果已加载）。
	if (LoadedGunshotSound)
		UGameplayStatics::PlaySoundAtLocation(this, LoadedGunshotSound, GetOwner()->GetActorLocation());
	else
		UE_LOG(LogTemp, Warning, TEXT("[%s] 射击时枪声资源未加载，无法播放枪声"), *GetNameSafe(this));

	FHitResult HitResult = PerformLineTrace();

	FVector ImpactPoint = HitResult.ImpactPoint;
	if (!HitResult.bBlockingHit)
	{
		const FVector Start = CachedAimLocation;
		const FVector Direction = CachedAimDirection;
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
 * 执行射线扫描检测命中目标。
 * 射线起点和方向优先使用由外部动作/输入链路注入并缓存的瞄准起点和方向。
 */
FHitResult URangeAttackComponent::PerformLineTrace()
{
	FHitResult HitResult;

	AActor* Owner = GetOwner();
	if (!Owner)
		return HitResult;

	// 获取射线起点和方向（优先使用已注入并缓存的瞄准起点和方向，否则使用主人或武器方向兜底）
	const bool bHasValidAim = !CachedAimDirection.IsNearlyZero();
	const FVector Start = bHasValidAim ? CachedAimLocation : (GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector);
	FVector Direction = bHasValidAim ? CachedAimDirection : (GetOwner() ? GetOwner()->GetActorForwardVector() : FVector::ForwardVector);

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

/**
 * 武器装备时同步解析枪声软引用，并缓存硬引用供后续播放逻辑直接使用。
 */
void URangeAttackComponent::OnEquip(ACharacter *CharacterOwner)
{
	Super::OnEquip(CharacterOwner);

	LoadedGunshotSound = nullptr;

	if (CachedGunshotSound.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] 远程武器数据未配置枪声资源"), *GetNameSafe(this));	
		return;
	}

	LoadedGunshotSound = CachedGunshotSound.LoadSynchronous();
	if (!LoadedGunshotSound)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] 枪声资源同步加载失败: %s"),
			*GetNameSafe(this),
			*CachedGunshotSound.ToSoftObjectPath().ToString());
	}
}

/**
 * 武器卸下时释放枪声资源的硬引用缓存。
 */
void URangeAttackComponent::OnUnEquip(ACharacter *CharacterOwner)
{
	Super::OnUnEquip(CharacterOwner);

	LoadedGunshotSound = nullptr;
}
