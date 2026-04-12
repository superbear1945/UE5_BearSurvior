#pragma once

#include "CoreMinimal.h"
#include "Components/StateTreeComponent.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "Perception/AIPerceptionComponent.h"
#include "UObject/Object.h"
#include "UObject/ObjectPtr.h"
#include "MyAIController.generated.h"

class UStateTree;

/**
 * 基于 AAIController 的通用 AI 控制器类。
 * 当前负责挂载 StateTreeAI 组件，并在接管 Pawn 时自动启动 AI 逻辑。
 */
UCLASS()
class BEARSURVIOR_API AMyAIController : public AAIController
{
	GENERATED_BODY()

public:
	/** 构造函数，用于初始化 AI 控制器相关组件与默认行为。 */
	AMyAIController();

	UFUNCTION(BlueprintCallable, Category = "AI|StateTree")
	void SetIsSeeingPlayer(bool bSeeingPlayer){ bIsSeeingPlayer = bSeeingPlayer; };

	UFUNCTION(BlueprintCallable, Category = "AI|StateTree")
	bool GetIsSeeingPlayer() const{ return bIsSeeingPlayer; };

	// 看到玩家时调用的函数
	UFUNCTION(BlueprintCallable, Category = "AI|StateTree")
	void OnSeeingPlayer(AActor* UpdatedActor, FAIStimulus Stimulus);


	// 忘记玩家时调用的函数
	UFUNCTION(BlueprintCallable, Category = "AI|StateTree")
	void OnForgetPlayer(AActor* UpdatedActor);
protected:
	/** 控制器开始运行时调用，用于注册 AI 感知等初始化逻辑。 */
	virtual void BeginPlay() override;

	/** StateTree AI 组件，用于承载和驱动 State Tree 行为。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStateTreeComponent> StateTree;

	/** AI 感知组件，用于接收视觉、听觉等感知结果并分发事件。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "AI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAIPerceptionComponent> AIPerceptionComponent;

	/** StateTree 资源引用，用于指定该控制器驱动的状态树资产。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "AI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStateTree> StateTreeAsset;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "AI|StateTree", meta = (AllowPrivateAccess = "true"))
	bool bIsSeeingPlayer;

	
	// 供状态树使用的目标 Character 引用，示例中用于存储玩家角色。
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "AI|StateTree", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ACharacter> TargetCharacter;
};

