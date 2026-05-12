// 可使用物品接口文件。
// 统一定义主要/次要使用的按下与松开入口，供武器、道具和可交互物品响应持续输入。

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "IUseableItem.generated.h"

// UE 反射接口声明：让实现类可以被蓝图和 C++ 统一识别为可使用物品。
UINTERFACE(MinimalAPI)
class UUseableItem : public UInterface
{
	GENERATED_BODY()
};

/**
 * 可使用物品接口。
 * 输入事件拆分为 Start/End，便于全自动武器、蓄力道具和按住右键互动等持续行为管理生命周期。
 */
class BEARSURVIOR_API IUseableItem
{
	GENERATED_BODY()

public:
	// 主要使用开始接口，对应鼠标左键按下，比如开始开火、近战攻击、使用道具等。
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Default")
	void PrimaryUseStart(const FVector& AimLocation, const FVector& AimDirection);

	// 主要使用结束接口，对应鼠标左键松开，用于结束持续开火、蓄力、治疗等行为。
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Default")
	void PrimaryUseEnd();

	// 次要使用开始接口，对应鼠标右键按下，比如开始瞄准、格挡或按住互动。
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Default")
	void SecondaryUseStart();

	// 次要使用结束接口，对应鼠标右键松开，用于结束瞄准、格挡或按住互动。
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Default")
	void SecondaryUseEnd();
};
