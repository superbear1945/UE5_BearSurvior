// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyBase.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "MyAIController.h"
#include "Component/HealthComponent.h"
#include "Engine.h"
#include "Components/WidgetComponent.h"
#include "Kismet/GameplayStatics.h"

/**
 * 初始化敌人基础角色的默认属性。
 */
AEnemyBase::AEnemyBase()
{
	// 开启 Tick，便于后续在基础敌人类中扩展逐帧逻辑。
	PrimaryActorTick.bCanEverTick = true;

	// 创建UI组件，用于显示敌人血条等信息
	UIWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("UIWidgetComponent"));
	UIWidgetComponent->SetupAttachment(RootComponent);

	// 创建血条组件
	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
}

/**
 * 在角色生成完成后执行初始化逻辑。
 */
void AEnemyBase::BeginPlay()
{
	Super::BeginPlay();
}

void AEnemyBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 这里可以添加一些基础的逐帧逻辑，例如根据当前状态执行不同的行为等。
	KeepUIFacingTarget(UGameplayStatics::GetPlayerPawn(this, 0));
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
 * 绑定敌人基础角色可能使用的输入组件。
 */
void AEnemyBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void AEnemyBase::SwitchEnemyState(EEnemyState NewState)
{
	CurrentState = NewState;
	// 这里可以添加状态切换时的公共逻辑，例如播放动画、修改属性等。
	// 具体的状态切换行为可以在子类中实现。
	switch (NewState)
	{
		case EEnemyState::Idle:
			// 切换到待机状态的逻辑
			break;
		case EEnemyState::Patrol:
			// 切换到游荡状态的逻辑
			GetCharacterMovement()->MaxWalkSpeed = PatrolSpeed;
			break;
		case EEnemyState::Chase:
			// 切换到追逐状态的逻辑
			GetCharacterMovement()->MaxWalkSpeed = ChaseSpeed;
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Blue, FString::Printf(TEXT("开始追逐")));
			break;
		case EEnemyState::Attack:
			// 切换到攻击状态的逻辑
			break;
		case EEnemyState::Dead:
			// 切换到死亡状态的逻辑
			break;
		default:
			break;
	}
}

void AEnemyBase::KeepUIFacingTarget(AActor* TargetActor)
{
	if (!UIWidgetComponent) 
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: UIWidgetComponent is not valid"), *GetName());
		return;
	}

	FVector TargetLocation = TargetActor ? TargetActor->GetActorLocation() : GetActorLocation() + GetActorForwardVector() * 100.0f;
	FVector DirectionToTarget = (TargetLocation - GetActorLocation()).GetSafeNormal();
	FRotator LookAtRotation = DirectionToTarget.Rotation();
	LookAtRotation.Roll = 0.0f;
	UIWidgetComponent->SetWorldRotation(LookAtRotation);
}