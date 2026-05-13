// Copyright Epic Games, Inc. All Rights Reserved.

#include "BearSurviorCharacter.h"
#include "Engine/EngineTypes.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "Math/MathFwd.h"
#include "Weapon/IUseableItem.h"
#include "Component/HealthComponent.h"
#include "InputActionValue.h"
#include "Blueprint/UserWidget.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/PlayerController.h"
#include "MainGameUserSetting.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

ABearSurviorCharacter::ABearSurviorCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
	
	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)

	// 初始化组件
	InitComponents();
}

void ABearSurviorCharacter::BeginPlay()
{
	Super::BeginPlay();
	ApplyMouseSensitivityFromSettings();

	if (CameraBoom)
	{
		DefaultTargetArmLength = CameraBoom->TargetArmLength;
		DefaultSocketOffset = CameraBoom->SocketOffset;
		AimBlendAlpha = bIsAiming ? 1.f : 0.f;
	}

	if (FollowCamera)
	{
		// 缓存角色默认相机 FOV，用于退出瞄准时平滑恢复
		DefaultCameraFov = FollowCamera->FieldOfView;
	}

	AimTargetBlendAlpha = bIsAiming ? 1.f : 0.f;

	// 按初始瞄准状态同步一次相机参数，避免角色出生时相机状态和布尔状态不一致。
	UpdateAimCamera();
}

void ABearSurviorCharacter::ApplyMouseSensitivityFromSettings()
{
	UMainGameUserSetting* MainGameUserSetting = UMainGameUserSetting::GetMainGameUserSetting();
	if (!MainGameUserSetting)
	{
		// 设置系统不可用时退回角色默认值，保证输入逻辑仍可正常工作。
		SetMouseSensitivity(MouseSensitivity);
		return;
	}

	// 使用持久化用户设置初始化角色输入参数，确保每次进入游戏都恢复上次配置。
	SetMouseSensitivity(MainGameUserSetting->GetMouseSensitivity());
}

void ABearSurviorCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}


// 由短时定时器驱动瞄准相机插值，平滑过渡臂长、偏移和 FOV。
void ABearSurviorCharacter::UpdateAimCamera()
{
	if (!CameraBoom && !FollowCamera)
		return;

	if (AimTransitionTime <= KINDA_SMALL_NUMBER)
	{
		AimBlendAlpha = AimTargetBlendAlpha;
	}
	else
	{
		const float CurrentTime = GetWorld()->GetTimeSeconds();
		const float ElapsedTime = CurrentTime - AimTransitionStartTime;
		const float Progress = FMath::Clamp(ElapsedTime / AimTransitionTime, 0.f, 1.f);
		AimBlendAlpha = FMath::Lerp(AimTransitionStartAlpha, AimTargetBlendAlpha, Progress);

		if (Progress >= 1.f)
		{
			AimBlendAlpha = AimTargetBlendAlpha;
			GetWorldTimerManager().ClearTimer(AimCameraTimerHandle);
		}
	}

	if (CameraBoom)
	{
		CameraBoom->TargetArmLength = FMath::Lerp(DefaultTargetArmLength, AimTargetArmLength, AimBlendAlpha);
		CameraBoom->SocketOffset = FMath::Lerp(DefaultSocketOffset, AimSocketOffset, AimBlendAlpha);
	}

	if (FollowCamera)
	{
		// 按同一混合系数同步 FOV，保证视野和越肩机位过渡节奏一致。
		FollowCamera->SetFieldOfView(FMath::Lerp(DefaultCameraFov, AimFov, AimBlendAlpha));
	}
}

void ABearSurviorCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ABearSurviorCharacter::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ABearSurviorCharacter::Look);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ABearSurviorCharacter::Look);

		// Secondary action
		EnhancedInputComponent->BindAction(SecondaryAction, ETriggerEvent::Started, this, &ABearSurviorCharacter::DoSecondaryUseStart);
		EnhancedInputComponent->BindAction(SecondaryAction, ETriggerEvent::Completed, this, &ABearSurviorCharacter::DoSecondaryUseEnd);
		EnhancedInputComponent->BindAction(SecondaryAction, ETriggerEvent::Canceled, this, &ABearSurviorCharacter::DoSecondaryUseEnd);

		// Back action(example: open pause menu)
		// 需要在编辑器内勾选Back Action的bConsumeInput属性，以确保在UI交互时也能触发该输入事件。
		EnhancedInputComponent->BindAction(BackAction, ETriggerEvent::Started, this, &ABearSurviorCharacter::DoBackAction);

		// Execute Action(示例：开火)
		EnhancedInputComponent->BindAction(ExecuteAction, ETriggerEvent::Started, this, &ABearSurviorCharacter::DoPrimaryUseStart);
		EnhancedInputComponent->BindAction(ExecuteAction, ETriggerEvent::Completed, this, &ABearSurviorCharacter::DoPrimaryUseEnd);
		EnhancedInputComponent->BindAction(ExecuteAction, ETriggerEvent::Canceled, this, &ABearSurviorCharacter::DoPrimaryUseEnd);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ABearSurviorCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);
}

void ABearSurviorCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route the input
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void ABearSurviorCharacter::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void ABearSurviorCharacter::DoLook(float Yaw, float Pitch)
{
	// 统一对鼠标/摇杆传入的视角值应用灵敏度倍率，避免在多个输入绑定点重复处理。
	const float ScaledYaw = Yaw * MouseSensitivity;
	const float ScaledPitch = Pitch * MouseSensitivity;

	if (GetController() != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(ScaledYaw);
		AddControllerPitchInput(ScaledPitch);
	}
}

void ABearSurviorCharacter::SetMouseSensitivity(float InSensitivity)
{
	// 将外部 UI 传入的滑条值限制在合法范围，避免极端值导致视角完全失控。
	MouseSensitivity = FMath::Clamp(InSensitivity, 0.f, 1.f);

	UMainGameUserSetting* MainGameUserSetting = UMainGameUserSetting::GetMainGameUserSetting();
	if (!MainGameUserSetting)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to get main game user setting."));
		return;
	}

	// 仅在设置值发生变化时回写持久化配置，避免重复触发配置保存。
	if (FMath::IsNearlyEqual(MainGameUserSetting->GetMouseSensitivity(), MouseSensitivity))
		return;

	MainGameUserSetting->SetMouseSensitivity(MouseSensitivity);
}

float ABearSurviorCharacter::GetMouseSensitivity() const
{
	return MouseSensitivity;
}

void ABearSurviorCharacter::DoJumpStart()
{
	// signal the character to jump
	Jump();
}

void ABearSurviorCharacter::DoJumpEnd()
{
	// signal the character to stop jumping
	StopJumping();
}

void ABearSurviorCharacter::DoSecondaryUseStart()
{
	bIsAiming = true;
	AimTransitionStartTime = GetWorld()->GetTimeSeconds();
	AimTransitionStartAlpha = AimBlendAlpha;
	AimTargetBlendAlpha = 1.f;
	if (AimTransitionTime <= KINDA_SMALL_NUMBER)
		GetWorldTimerManager().ClearTimer(AimCameraTimerHandle);
	else
		GetWorldTimerManager().SetTimer(AimCameraTimerHandle, this, &ABearSurviorCharacter::UpdateAimCamera, 1.f / 60.f, true);
	UpdateAimCamera();

	if (CurrentHeldItem == nullptr)
	{
		// 没有持有物品时仍保留角色本地瞄准相机效果，方便后续空手互动扩展。
		UE_LOG(LogTemplateCharacter, Warning, TEXT("DoSecondaryUseStart called but CurrentHeldItem is null."));
		return;
	}

	if (CurrentHeldItem->Implements<UUseableItem>())
	{
		// 将右键按下事件传递给物品，支持按住瞄准、格挡或持续互动等次要使用逻辑。
		IUseableItem::Execute_SecondaryUseStart(CurrentHeldItem);

		return;
	}

	UE_LOG(LogTemplateCharacter, Warning, TEXT("CurrentHeldItem can't start secondary use."));
}

void ABearSurviorCharacter::DoSecondaryUseEnd()
{
	bIsAiming = false;
	AimTransitionStartTime = GetWorld()->GetTimeSeconds();
	AimTransitionStartAlpha = AimBlendAlpha;
	AimTargetBlendAlpha = 0.f;
	if (AimTransitionTime <= KINDA_SMALL_NUMBER)
		GetWorldTimerManager().ClearTimer(AimCameraTimerHandle);
	else
		GetWorldTimerManager().SetTimer(AimCameraTimerHandle, this, &ABearSurviorCharacter::UpdateAimCamera, 1.f / 60.f, true);
	UpdateAimCamera();

	if (CurrentHeldItem == nullptr)
	{
		// 没有持有物品时只结束角色本地瞄准状态。
		return;
	}

	if (CurrentHeldItem->Implements<UUseableItem>())
	{
		// 将右键松开事件传递给物品，确保持续次要行为能正确收尾。
		IUseableItem::Execute_SecondaryUseEnd(CurrentHeldItem);
		return;
	}

	UE_LOG(LogTemplateCharacter, Warning, TEXT("CurrentHeldItem can't end secondary use."));
}

void ABearSurviorCharacter::DoBackAction()
{
	// Back 输入在角色层直接触发暂停菜单切换，减少当前迭代复杂度。
	TogglePauseMenu();
}

void ABearSurviorCharacter::DoPrimaryUseStart()
{
	if (CurrentHeldItem == nullptr)
	{
		// 后续可以添加空手交互逻辑，比如近战攻击、环境互动等，目前先输出日志以便调试。
		UE_LOG(LogTemplateCharacter, Warning, TEXT("DoPrimaryUseStart called but CurrentHeldItem is null."));
		return;
	}

	// 调用当前持有物品的 PrimaryUseStart 接口，触发按下阶段的使用行为（如开始开火、蓄力等）。
	if (CurrentHeldItem->Implements<UUseableItem>())
	{
		FVector AimLocation = FVector::Zero();
		FRotator AimRotator = FRotator::ZeroRotator;
		GetActorEyesViewPoint(AimLocation, AimRotator);
		IUseableItem::Execute_PrimaryUseStart(CurrentHeldItem, AimLocation, AimRotator.Vector());
		return;
	}

	// 无法使用的物品，后续可以添加提示反馈，目前先输出日志以便调试。
	UE_LOG(LogTemp, Warning, TEXT("CurrentHeldItem can't be used"));
	return;
}

void ABearSurviorCharacter::DoPrimaryUseEnd()
{
	if (CurrentHeldItem == nullptr)
	{
		// 没有持有物品时无需结束物品使用逻辑，后续可扩展空手持续交互。
		return;
	}

	if (CurrentHeldItem->Implements<UUseableItem>())
	{
		// 调用当前持有物品的 PrimaryUseEnd 接口，确保全自动武器或持续道具停止使用。
		IUseableItem::Execute_PrimaryUseEnd(CurrentHeldItem);
		return;
	}

	UE_LOG(LogTemplateCharacter, Warning, TEXT("CurrentHeldItem can't end primary use."));
}

void ABearSurviorCharacter::TogglePauseMenu()
{
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("TogglePauseMenu ignored: invalid local PlayerController."));
		return;
	}

	if (bIsPauseMenuOpen)
	{
		if (PauseMenuWidgetInstance && PauseMenuWidgetInstance->IsInViewport())
		{
			PauseMenuWidgetInstance->RemoveFromParent();
		}

		PlayerController->SetPause(false);

		// 关闭菜单后恢复纯游戏输入。
		FInputModeGameOnly InputMode;
		PlayerController->SetInputMode(InputMode);
		PlayerController->bShowMouseCursor = false;
		bIsPauseMenuOpen = false;
		return;
	}

	if (!PauseMenuWidgetClass)
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("PauseMenuWidgetClass is not set on %s."), *GetNameSafe(this));
		return;
	}

	// 首次打开时创建菜单实例，后续复用避免重复创建。
	if (!PauseMenuWidgetInstance)
	{
		PauseMenuWidgetInstance = CreateWidget<UUserWidget>(PlayerController, PauseMenuWidgetClass);
		if (!PauseMenuWidgetInstance)
		{
			UE_LOG(LogTemplateCharacter, Error, TEXT("Failed to create pause menu widget."));
			return;
		}
	}

	if (!PauseMenuWidgetInstance->IsInViewport())
	{
		PauseMenuWidgetInstance->AddToViewport(100);
	}

	PlayerController->SetPause(true);

	// 打开菜单时切换为 GameAndUI，允许鼠标交互并保持游戏输入上下文。
	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetWidgetToFocus(PauseMenuWidgetInstance->TakeWidget());
	PlayerController->SetInputMode(InputMode);
	PlayerController->bShowMouseCursor = true;
	bIsPauseMenuOpen = true;
}

void ABearSurviorCharacter::InitComponents()
{
	// 创建生命组件，作为角色的核心状态组件之一。
	// UActorComponent 不需要 Attach 操作，创建后自动归属于宿主 Actor。
	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));

	// 创建 Widget 组件，用于显示角色的 3D UI。
	WidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("WidgetComponent"));
}

/**
 * 获取角色视角原点和朝向。
 * 玩家角色返回 FollowCamera 的世界位置和旋转，使远程武器射线从屏幕中心发出。
 * 当 FollowCamera 不可用时回退到默认实现。
 */
void ABearSurviorCharacter::GetActorEyesViewPoint(FVector& OutLocation, FRotator& OutRotation) const
{
	if (FollowCamera)
	{
		OutLocation = FollowCamera->GetComponentLocation();
		OutRotation = FollowCamera->GetComponentRotation();
		return;
	}

	// 回退到默认实现，使用 Actor 位置加上眼睛高度，并结合控制器旋转来确定视角。
	Super::GetActorEyesViewPoint(OutLocation, OutRotation);
}
