// 版权所有 (c) 2026 BearSurvior。保留所有权利。

/**
 * @file EquippableItem.h
 * @brief 可装备物品接口定义。
 * 
 * 本文件声明了 IEquippableItem 接口，该接口定义了物品装备与卸下的生命周期行为。
 * 凡是能够被角色装备的物品（例如武器、防具等）都应当实现此接口。
 */

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "EquippableItem.generated.h"

class ACharacter;

/**
 * @class UEquippableItem
 * @brief 系统反射类，供反射系统及蓝图识别使用。
 */
UINTERFACE(MinimalAPI, Blueprintable)
class UEquippableItem : public UInterface
{
	GENERATED_BODY()
};

/**
 * @class IEquippableItem
 * @brief 可装备物品接口。
 * 
 * 定义装备与卸下的生命周期入口，供武器、防具等可装备物品实现。
 * 装备/卸下仅负责附加到角色骨骼、隐藏/显示世界网格等设置，使用阶段由 IUseableItem 独立管理。
 */
class BEARSURVIOR_API IEquippableItem
{
	GENERATED_BODY()

public:
	/**
	 * @brief 装备物品：将此物品附加到角色指定插槽并激活装备效果。
	 * @param CharacterOwner 装备此物品的角色指针。
	 * @param AttachSocketName 装备时需要附着到角色骨骼上的插槽名称。
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Equipment")
	void Equip(ACharacter* CharacterOwner, FName AttachSocketName);

	/**
	 * @brief 卸下物品：从角色骨骼分离并停用装备效果。
	 * @param CharacterOwner 卸下此物品的角色指针。
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Equipment")
	void UnEquip(ACharacter* CharacterOwner);
};
