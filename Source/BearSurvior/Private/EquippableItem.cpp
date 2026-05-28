// 版权所有 (c) 2026 BearSurvior。保留所有权利。

/**
 * @file EquippableItem.cpp
 * @brief 可装备物品接口实现。
 * 
 * 接口本身提供纯虚/虚函数的编译基准，所有的具体实现移交给派生类（如 WeaponBase 等）。
 */

#include "EquippableItem.h"

// 此处无需要主动实现的虚函数，BlueprintNativeEvent 在 C++ 中的默认行为由 UHT 内部生成，
// 或者在接口类中仅作静态断言/不实现。
