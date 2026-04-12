// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MyAIController.h"
#include "UObject/PropertyWrapper.h"
#include "EnemyBase.generated.h"

class AMyAIController;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "AI", meta = (AllowPrivateAccess = "true"))	
	TObjectPtr<AMyAIController> EnemyAIController;

public:
	/** 每帧调用，用于更新敌人基础角色逻辑。 */
	virtual void Tick(float DeltaTime) override;

	/** 绑定输入组件。 */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
