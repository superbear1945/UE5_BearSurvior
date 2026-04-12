// 游戏用户设置类：统一管理可持久化的本地设置项，例如鼠标灵敏度。

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "MainGameUserSetting.generated.h"

class ABearSurviorCharacter;

/**
 * 游戏用户设置类：负责保存与读取玩家本地设置，并为外部系统提供统一访问入口。
 */
UCLASS(Config=GameUserSettings)
class BEARSURVIOR_API UMainGameUserSetting : public UGameUserSettings
{
	GENERATED_BODY()

public:
	/** 构造函数：初始化可持久化设置的默认值。 */
	UMainGameUserSetting();

	/** 获取当前鼠标灵敏度，供角色与 UI 初始化时读取。 */
	UFUNCTION(BlueprintPure, Category = "Settings|Input")
	float GetMouseSensitivity() const;

	/** 设置鼠标灵敏度并立即保存配置，避免重启游戏后丢失。 */
	UFUNCTION(BlueprintCallable, Category = "Settings|Input")
	void SetMouseSensitivity(float InMouseSensitivity);

	/** 获取主游戏用户设置单例，失败时返回空指针。 */
	UFUNCTION(BlueprintPure, Category = "Settings")
	static UMainGameUserSetting* GetMainGameUserSetting();

	/** 重写应用设置逻辑，确保自定义设置在系统应用阶段同样被约束到合法范围。 */
	virtual void ApplySettings(bool bCheckForCommandLineOverrides) override;

protected:
	/** 将当前鼠标灵敏度同步到本地玩家角色，保证设置修改后立即生效。 */
	void ApplyMouseSensitivityToLocalCharacter() const;

	// 鼠标视角灵敏度，使用 Config 持久化到本地用户配置文件中。
	UPROPERTY(Config, VisibleAnywhere, BlueprintReadOnly, Category = "Settings|Input", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float MouseSensitivity = 0.5f;

private:
	/** 将鼠标灵敏度限制在合法范围内，避免外部写入异常值。 */
	void ClampMouseSensitivity();
};
