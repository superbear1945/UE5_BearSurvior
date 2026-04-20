// Copyright Epic Games, Inc. All Rights Reserved.

#include "BearSurviorCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "Component/HealthComponent.h"
#include "InputActionValue.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "MainGameUserSetting.h"

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
	UpdateAimCamera(DeltaSeconds);
}


// 平滑过渡摄像机位置以适应瞄准状态
void ABearSurviorCharacter::UpdateAimCamera(float DeltaSeconds)
{
	if (!CameraBoom)
	{
		return;
	}

	const float TargetAlpha = bIsAiming ? 1.f : 0.f;
	if (AimTransitionTime <= KINDA_SMALL_NUMBER)
	{
		AimBlendAlpha = TargetAlpha;
	}
	else
	{
		AimBlendAlpha = FMath::FInterpConstantTo(AimBlendAlpha, TargetAlpha, DeltaSeconds, 1.f / AimTransitionTime);
	}

	CameraBoom->TargetArmLength = FMath::Lerp(DefaultTargetArmLength, AimTargetArmLength, AimBlendAlpha);
	CameraBoom->SocketOffset = FMath::Lerp(DefaultSocketOffset, AimSocketOffset, AimBlendAlpha);

	if (FollowCamera)
	{
		// 根据瞄准混合系数平滑调整 FOV：非瞄准使用默认值，瞄准使用 AimFov
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

		// Aiming
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &ABearSurviorCharacter::DoAimStart);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &ABearSurviorCharacter::DoAimEnd);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Canceled, this, &ABearSurviorCharacter::DoAimEnd);

		// Back action(example: open pause menu)
		// 需要在编辑器内勾选Back Action的bConsumeInput属性，以确保在UI交互时也能触发该输入事件。
		EnhancedInputComponent->BindAction(BackAction, ETriggerEvent::Started, this, &ABearSurviorCharacter::DoBackAction);
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

void ABearSurviorCharacter::DoAimStart()
{
	bIsAiming = true;
}

void ABearSurviorCharacter::DoAimEnd()
{
	bIsAiming = false;
}

void ABearSurviorCharacter::DoBackAction()
{
	// Back 输入在角色层直接触发暂停菜单切换，减少当前迭代复杂度。
	TogglePauseMenu();
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
}
