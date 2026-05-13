// 物品基类实现文件。
// 负责解析 FItemData 公共物品数据，并将展示、背包和世界外观字段同步到 Actor。

#include "ItemBase.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DataTable.h"

// 空物品数据静态实例，用于 DataTable 未配置或解析失败时兜底。
static const FItemData EmptyItemData;

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
	CachedItemData = nullptr;
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

	ResolveItemData();
	SyncItemProperties();
}

/**
 * 解析 ItemDataRow 指向的公共物品数据。
 * 数据无效时保留构造函数中的默认值，避免影响未接入 DataTable 的测试物品。
 */
void AItemBase::ResolveItemData()
{
	if (!ItemDataRow.DataTable || ItemDataRow.RowName.IsNone())
		return;

	static const FString Context(TEXT("ItemDataResolve"));
	CachedItemData = ItemDataRow.DataTable->FindRow<FItemData>(ItemDataRow.RowName, Context);

	if (!CachedItemData)
	{
		UE_LOG(LogTemp, Error, TEXT("[%s] 解析 ItemDataRow 失败，表: %s，行: %s"),
			*GetNameSafe(this),
			*GetNameSafe(ItemDataRow.DataTable),
			*ItemDataRow.RowName.ToString());
	}
}

/**
 * 将公共物品数据同步到运行时字段。
 * 这些字段供背包 UI、拾取提示和世界物品网格直接读取。
 */
void AItemBase::SyncItemProperties()
{
	if (!CachedItemData)
		return;

	ItemDisplayName = CachedItemData->DisplayName;
	ItemDescription = CachedItemData->Description;
	ItemIcon = CachedItemData->Icon.LoadSynchronous();
	Weight = CachedItemData->Weight;
	Rarity = CachedItemData->Rarity;

	// 加载物品网格到 ItemMesh 组件上。
	if (!CachedItemData->WeaponMesh.IsNull() && ItemMesh)
	{
		UStaticMesh* LoadedMesh = CachedItemData->WeaponMesh.LoadSynchronous();
		if (LoadedMesh)
			ItemMesh->SetStaticMesh(LoadedMesh);
	}
}

/**
 * 返回缓存的物品公共数据引用。
 */
const FItemData& AItemBase::GetItemData() const
{
	return CachedItemData ? *CachedItemData : EmptyItemData;
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
