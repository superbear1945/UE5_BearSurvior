// Copyright Epic Games, Inc. All Rights Reserved.
// 目前作为玩家角色基类，封装移动、视角、跳跃和瞄准等输入能力。后续会根据需要添加更多功能。

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ItemBase.h"
#include "Logging/LogMacros.h"
#include "Math/MathFwd.h"
#include "UObject/ObjectPtr.h"
#include "BearSurviorCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UMainGameUserSetting;
class UUserWidget;
class UHealthComponent;
class UWidgetComponent;
class AItemBase;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  A simple player-controllable third person character
 *  Implements a controllable orbiting camera
 */
// 玩家角色基类：封装移动、视角、跳跃、瞄准与暂停菜单相关的输入能力。
UCLASS(abstract)
class ABearSurviorCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;
	
protected:

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	UInputAction* MouseLookAction;

	/** Secondary Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	UInputAction* SecondaryAction;

	// 回退按钮，比如菜单中回到上一级或回到游戏，或者游戏中打开暂停菜单等。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* BackAction;

	// 执行Action，比如开火，近战攻击，使用道具等
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* ExecuteAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Component|Health")
	TObjectPtr<UHealthComponent> HealthComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Component|3DUI")
	TObjectPtr<UWidgetComponent> WidgetComponent;

	// 当前角色手中持有的物品
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Item")
	TObjectPtr<AItemBase> CurrentHeldItem = nullptr;

public:

	/** Constructor */
	ABearSurviorCharacter();	

	virtual void Tick(float DeltaSeconds) override;

	virtual void BeginPlay() override;

	virtual UHealthComponent* GetHealthComponent() const { return HealthComponent; }

	virtual void InitComponents();
protected:

	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	/** 根据当前瞄准状态切换越肩瞄准相机参数 */
	void UpdateAimCamera();

public:

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles look inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoLook(float Yaw, float Pitch);

	/** 设置鼠标视角灵敏度缓存值，供设置系统在运行时同步角色输入参数 */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void SetMouseSensitivity(float InSensitivity);

	/** 获取当前鼠标视角灵敏度缓存值，供需要实时读取角色输入状态的逻辑使用 */
	UFUNCTION(BlueprintCallable, Category="Input")
	float GetMouseSensitivity() const;

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

	/** Handles Secondary pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoSecondaryUseStart();

	/** Handles Secondary released inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoSecondaryUseEnd();

	/** Handles back pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoBackAction();

	// 玩家和手中物品互动时调用的接口，后续会根据需要添加更多参数来支持不同类型的互动。
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoPrimaryUseStart();

	/** 玩家松开左键时调用的函数 */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoPrimaryUseEnd();

	/** 切换暂停菜单状态，同时处理游戏暂停与输入模式切换 */
	UFUNCTION(BlueprintCallable, Category="UI|Pause")
	virtual void TogglePauseMenu();

	/** 从游戏用户设置同步鼠标灵敏度到角色，确保运行时输入参数与持久化设置一致。 */
	void ApplyMouseSensitivityFromSettings();

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera|Aim")
	float AimTargetArmLength = 220.f;

	// 在瞄准状态时，摄像头偏移量相对于默认位置的偏移
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera|Aim")
	FVector AimSocketOffset = FVector(0.f, 45.f, 20.f);

	// 在瞄准状态时，相机目标视野角（FOV）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera|Aim", meta=(ClampMin="1.0", ClampMax="179.0", UIMin="30.0", UIMax="120.0"))
	float AimFov = 70.f;

	// 进入瞄准状态时，摄像机过渡到目标位置的时间，单位为秒
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera|Aim", meta=(ClampMin="0.0", UIMin="0.0"))
	float AimTransitionTime = 0.12f;

	float DefaultTargetArmLength = 0.f;
	FVector DefaultSocketOffset = FVector::ZeroVector;
	float DefaultCameraFov = 90.f;

	// 瞄准相机插值的起始时间戳，用于在短时定时器回调里按时长推进过渡。
	float AimTransitionStartTime = 0.f;

	// 瞄准相机插值开始时的混合系数，用于支持中途打断后重新插值。
	float AimTransitionStartAlpha = 0.f;

	// 瞄准相机当前过渡到的目标混合系数，用于配合线性瞄准使用：0 表示默认视角，1 表示瞄准视角。
	float AimTargetBlendAlpha = 0.f;

	// 仅在瞄准进入或退出的短暂时间内运行的定时器句柄，避免占用角色常驻 Tick。
	FTimerHandle AimCameraTimerHandle;

	float AimBlendAlpha = 0.f;
	bool bIsAiming = false;

	// 鼠标视角灵敏度倍率，统一作用于增强输入传入的视角值。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input", meta=(ClampMin="0.0", ClampMax="1.0", UIMin="0.0", UIMax="1.0"))
	float MouseSensitivity = 0.5f;

	// 暂停菜单Widget类，在蓝图中指定具体UMG资产。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="UI|Pause")
	TSubclassOf<UUserWidget> PauseMenuWidgetClass;

	// 暂停菜单实例，首次打开时创建并复用。
	UPROPERTY(Transient, BlueprintReadOnly, Category="UI|Pause")
	TObjectPtr<UUserWidget> PauseMenuWidgetInstance;

	// 暂停菜单当前是否处于打开状态。
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="UI|Pause")
	bool bIsPauseMenuOpen = false;

public:

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	/**
	 * 获取角色视角原点和朝向。
	 * 玩家角色返回 FollowCamera 的世界位置和旋转，使远程武器射线从屏幕中心发出。
	 * NPC 角色使用默认实现（Actor位置 + 眼睛高度 + 控制器旋转）。
	 */
	virtual void GetActorEyesViewPoint(FVector& OutLocation, FRotator& OutRotation) const override;
};

