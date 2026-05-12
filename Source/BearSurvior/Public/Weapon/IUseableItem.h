// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "IUseableItem.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UUseableItem : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class BEARSURVIOR_API IUseableItem
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	// 主要的使用接口，对应鼠标左键，比如开火、近战攻击、使用道具等。后续可以根据需要添加更多接口来支持不同类型的使用行为。
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Default")
	void PrimaryUse(const FVector &AimLocation, const FVector &AimDirection);

	// 次要的使用接口，暂时对应鼠标右键，比如瞄准、近战重击等。后续可以根据需要添加更多接口来支持不同类型的使用行为。
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Default")
	void SecondaryUse();

	// 停止使用接口，供持续性使用行为（如持续开火、持续治疗等）在输入释放时调用，确保能够正确结束使用状态。
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Default")
	void StopPrimaryUse();
};
