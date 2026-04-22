// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementReplication.h"
#include "MyAIController.h"
#include "UObject/PropertyWrapper.h"
#include "EnemyBase.generated.h"

class AMyAIController;
class UHealthComponent;
class UWidgetComponent;

// 敌人当前状态
UENUM(BlueprintType)
enum class EEnemyState : uint8
{
	Idle UMETA(DisplayName = "待机"),
	Patrol UMETA(DisplayName = "游荡"),
	Chase UMETA(DisplayName = "追逐"),
	Attack UMETA(DisplayName = "攻击"),
	Dead UMETA(DisplayName = "死亡")
};

/**
 * 敌人基础角色类。
 * 当前作为项目中敌人角色的公共父类使用。
 */
UCLASS()
class BEARSURVIOR_API AEnemyBase : public ACharacter
{
	GENERATED_BODY()

public:
	/** 构造函数，用于初始化敌人基础角色默认属性。 */
	AEnemyBase();

protected:
	/** 在游戏开始或生成时调用。 */
	virtual void BeginPlay() override;

	/** 当控制器接管此角色时调用，用于赋值 EnemyAIController。 */
	virtual void PossessedBy(AController* NewController) override;

	// 缓存敌人控制器，便于后续调用
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "AI", meta = (AllowPrivateAccess = "true"))	
	TObjectPtr<AMyAIController> EnemyAIController;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "AI", meta = (AllowPrivateAccess = "true"))	
	float PatrolSpeed = 27.5f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "AI", meta = (AllowPrivateAccess = "true"))	
	float ChaseSpeed = 64.2f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "AI", meta = (AllowPrivateAccess = "true"))	
	EEnemyState CurrentState = EEnemyState::Idle;

	// 生命值组件，管理血量等
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Component|Health", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UHealthComponent> HealthComponent;

	// UI组件，用于显示敌人血条等信息
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Component|UI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWidgetComponent> UIWidgetComponent;
public:
	/** 每帧调用，用于更新敌人基础角色逻辑。 */
	virtual void Tick(float DeltaTime) override;

	/** 绑定输入组件。 */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintCallable, Category = "AI|StateTree")
	virtual void SwitchEnemyState(EEnemyState NewState);

	UFUNCTION(BlueprintCallable, Category = "UI")
	virtual void KeepUIFacingTarget(AActor* TargetActor);
};
