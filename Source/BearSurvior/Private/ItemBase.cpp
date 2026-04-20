// 物品基类实现文件。

#include "ItemBase.h"
#include "Components/StaticMeshComponent.h"

/**
 * 初始化物品默认属性和组件。
 */
AItemBase::AItemBase()
{
	// 默认开启Tick，子类可根据需要关闭。
	PrimaryActorTick.bCanEverTick = false;

	// 创建物品可视化网格组件，作为根组件。
	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
	SetRootComponent(ItemMesh);
	ItemMesh->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	ItemMesh->SetSimulatePhysics(true);

	// 初始化默认物品数据。
	ItemDisplayName = FText::FromString(TEXT("未命名物品"));
	ItemDescription = FText::GetEmpty();
	ItemIcon = nullptr;
	Weight = 1.0f;
	Rarity = EItemRarity::Common;
	MaxStackCount = 1;
	StackCount = 1;
}

/**
 * 在游戏开始或生成时调用。
 */
void AItemBase::BeginPlay()
{
	Super::BeginPlay();
}

/**
 * 拾取物品。当角色拾取此物品时调用。
 * @param NewOwner 拾取此物品的Actor（通常是角色）。
 */
void AItemBase::PickUp(AActor* NewOwner)
{
	// 设置新的拥有者，但不自动执行任何视觉或物理操作。
	SetOwner(NewOwner);

	// 关闭物理模拟和碰撞，物品被拾取后不再存在于世界中。
	ItemMesh->SetSimulatePhysics(false);
	ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 隐藏物品在世界中的显示。
	SetActorHiddenInGame(true);
}

/**
 * 装备物品。当角色装备此物品时调用。
 * @param CharacterOwner 装备此物品的角色。
 */
void AItemBase::Equip(AActor* CharacterOwner)
{
	// 基类默认无操作，子类可覆盖实现装备效果。
}

/**
 * 卸下物品。当角色卸下此物品时调用。
 * @param CharacterOwner 卸下此物品的角色。
 */
void AItemBase::UnEquip(AActor* CharacterOwner)
{
	// 基类默认无操作，子类可覆盖实现卸下效果。
}

/**
 * 丢弃物品。将物品丢弃到世界中时调用。
 */
void AItemBase::Drop()
{
	// 清除拥有者。
	SetOwner(nullptr);

	// 重新启用物理模拟和碰撞，物品重新回到世界中。
	SetActorHiddenInGame(false);
	ItemMesh->SetSimulatePhysics(true);
	ItemMesh->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
}
