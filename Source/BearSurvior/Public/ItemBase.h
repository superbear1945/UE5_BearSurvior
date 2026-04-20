// 物品基类。作为项目中所有可拾取/可装备物品的公共父类。
// 提供物品通用属性（名称、描述、重量、品质等）和基础行为虚函数（拾取、装备、卸下、丢弃）。

#pragma once

// #include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemBase.generated.h"

// 物品品质/稀有度枚举，用于背包分类、UI显示、掉落权重等。
UENUM(BlueprintType)
enum class EItemRarity : uint8
{
	Common       UMETA(DisplayName = "普通"),
	Uncommon     UMETA(DisplayName = "优秀"),
	Rare         UMETA(DisplayName = "稀有"),
	Epic         UMETA(DisplayName = "史诗"),
	Legendary    UMETA(DisplayName = "传说")
};

/**
 * 物品基类：所有可拾取物品的公共父类。
 * 封装物品数据、可视化组件，以及物品生命周期中的虚函数入口。
 * 物品拾取通过射线检测实现，物品的 StaticMesh 本身作为射线目标。
 */
UCLASS(Abstract)
class BEARSURVIOR_API AItemBase : public AActor
{
	GENERATED_BODY()

public:
	/** 构造函数，初始化物品默认属性和组件。 */
	AItemBase();

// ────────────────────────────────────────── 组件 ──────────────────────────────────────────

protected:

	// 物品的可视化网格组件，用于在世界中显示物品外观。
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item|Component")
	TObjectPtr<UStaticMeshComponent> ItemMesh;

// ────────────────────────────────────────── 数据 ──────────────────────────────────────────

public:

	// 物品显示名称，用于背包UI、拾取提示等。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FText ItemDisplayName;

	// 物品描述文本，用于Tooltip等详细信息展示。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FText ItemDescription;

	// 物品图标，用于背包格子、快捷栏等UI。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	TObjectPtr<UTexture2D> ItemIcon;

	// 物品重量，单位自定义。可用于背包容量计算等。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float Weight;

	// 物品品质/稀有度。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	EItemRarity Rarity;

	// 物品最大堆叠数量，为1表示不可堆叠。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta = (ClampMin = "1", UIMin = "1"))
	int32 MaxStackCount;

	// 当前堆叠数量。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta = (ClampMin = "1", UIMin = "1"))
	int32 StackCount;

// ────────────────────────────────────────── 行为 ──────────────────────────────────────────

public:

	// 拾取物品。当角色拾取此物品时调用，子类可覆盖实现拾取逻辑。
	UFUNCTION(BlueprintCallable, Category = "Item")
	virtual void PickUp(AActor* NewOwner);

	// 装备物品。当角色装备此物品时调用，子类可覆盖实现装备效果。
	UFUNCTION(BlueprintCallable, Category = "Item")
	virtual void Equip(AActor* CharacterOwner);

	// 卸下物品。当角色卸下此物品时调用，子类可覆盖实现卸下效果。
	UFUNCTION(BlueprintCallable, Category = "Item")
	virtual void UnEquip(AActor* CharacterOwner);

	// 丢弃物品。将物品丢弃到世界中时调用，子类可覆盖实现丢弃逻辑。
	UFUNCTION(BlueprintCallable, Category = "Item")
	virtual void Drop();

protected:

	/** 在游戏开始或生成时调用。 */
	virtual void BeginPlay() override;
};
