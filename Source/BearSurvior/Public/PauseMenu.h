// 暂停菜单基类：负责承载暂停菜单的 C++ 逻辑与按钮事件绑定入口。

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PauseMenu.generated.h"

class UButton;
class USlider;

/**
 * 暂停菜单界面基类，后续可在此集中处理继续游戏、返回主菜单、退出游戏等按钮逻辑。
 */
UCLASS()
class BEARSURVIOR_API UPauseMenu : public UUserWidget
{
	GENERATED_BODY()

protected:
	/**
	 * 初始化暂停菜单控件引用并绑定按钮、滑条事件。
	 */
	virtual void NativeConstruct() override;

	/**
	 * 解除暂停菜单中动态绑定的事件，避免重复绑定导致的多次触发。
	 */
	virtual void NativeDestruct() override;

	/**
	 * 校验关键控件是否已由 UMG 自动绑定，并在缺失时输出警告日志。
	 */
	void ValidateWidgetReferences() const;

	/**
	 * 绑定暂停菜单中需要由 C++ 驱动的控件事件。
	 */
	void BindWidgetEvents();

	/**
	 * 处理返回按钮点击事件，通知角色切换暂停菜单状态。
	 */
	UFUNCTION()
	void HandleReturnButtonClicked();

	/**
	 * 处理鼠标灵敏度滑条变化事件，当前先记录调节结果，后续可接入正式设置系统。
	 */
	UFUNCTION()
	void HandleMouseSensitiveSliderValueChanged(float InValue);

	// 暂停菜单中的返回按钮，要求蓝图子类中存在同名按钮变量 returnButton。
	UPROPERTY(BlueprintReadOnly, Category = "PauseMenu", meta = (BindWidget ,AllowPrivateAccess = "true"))
	TObjectPtr<UButton> ReturnButton = nullptr;

	// 暂停菜单中的鼠标灵敏度滑条，要求蓝图子类中存在同名滑条变量 mouseSensitiveSlider。
	UPROPERTY(BlueprintReadOnly, Category = "PauseMenu", meta = (BindWidget ,AllowPrivateAccess = "true"))
	TObjectPtr<USlider> MouseSensitiveSlider = nullptr;
};
