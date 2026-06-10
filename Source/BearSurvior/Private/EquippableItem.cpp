// 版权所有 (c) 2026 BearSurvior。保留所有权利。

/**
 * @file EquippableItem.cpp
 * @brief 可装备物品接口实现。
 * 
 * 接口提供 BlueprintNativeEvent 的默认空实现，具体装备逻辑由派生类（如 WeaponBase 等）覆盖。
 */

#include "EquippableItem.h"

/**
 * 装备物品的默认接口实现。
 * 基类接口不假设具体物品形态，因此默认不执行任何附着或状态变更，派生类按需覆盖。
 * @param CharacterOwner 装备此物品的角色指针。
 * @param AttachSocketName 装备时需要附着到角色骨骼上的插槽名称。
 */
void IEquippableItem::Equip_Implementation(ACharacter *CharacterOwner, FName AttachSocketName)
{
	// 默认没有装备行为，具体可装备物品需要在自身类中覆盖实现。
}
