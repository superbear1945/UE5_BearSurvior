// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyBase.h"
#include "MyAIController.h"

/**
 * 初始化敌人基础角色的默认属性。
 */
AEnemyBase::AEnemyBase()
{
	// 开启 Tick，便于后续在基础敌人类中扩展逐帧逻辑。
	PrimaryActorTick.bCanEverTick = true;
}

/**
 * 在角色生成完成后执行初始化逻辑。
 */
void AEnemyBase::BeginPlay()
{
	Super::BeginPlay();
}

/**
 * 当控制器接管此角色时调用，用于赋值 EnemyAIController。
 */
void AEnemyBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	EnemyAIController = Cast<AMyAIController>(NewController);
}

/**
 * 敌人基础角色的逐帧更新函数。
 */
void AEnemyBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

/**
 * 绑定敌人基础角色可能使用的输入组件。
 */
void AEnemyBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}
