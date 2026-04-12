// 暂停菜单基类实现文件：负责绑定按钮与滑条事件，并将返回按钮逻辑转发给角色暂停系统。


#include "PauseMenu.h"
#include "BearSurvior.h"
#include "MainGameUserSetting.h"
#include "Components/Button.h"
#include "Components/Slider.h"
#include "BearSurviorCharacter.h"

void UPauseMenu::NativeConstruct()
{
	Super::NativeConstruct();

	ValidateWidgetReferences();
	BindWidgetEvents();

	if (!MouseSensitiveSlider)
		return;

	UMainGameUserSetting* MainGameUserSetting = UMainGameUserSetting::GetMainGameUserSetting();
	if (!MainGameUserSetting)
	{
		UE_LOG(LogTemp, Warning, TEXT("无法获取 UMainGameUserSetting，暂停菜单将使用滑条当前默认值。"));
		return;
	}

	// 菜单打开时从持久化设置同步灵敏度，避免 UI 直接依赖角色实例数据。
	MouseSensitiveSlider->SetValue(MainGameUserSetting->GetMouseSensitivity());
}

void UPauseMenu::NativeDestruct()
{
	if (ReturnButton)
	{
		ReturnButton->OnClicked.RemoveDynamic(this, &UPauseMenu::HandleReturnButtonClicked);
	}

	if (MouseSensitiveSlider)
	{
		MouseSensitiveSlider->OnValueChanged.RemoveDynamic(this, &UPauseMenu::HandleMouseSensitiveSliderValueChanged);
	}

	Super::NativeDestruct();
}

void UPauseMenu::ValidateWidgetReferences() const
{
	if (!ReturnButton)
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("PauseMenu is missing a bound Button named 'returnButton'."));
	}

	if (!MouseSensitiveSlider)
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("PauseMenu is missing a bound Slider named 'mouseSensitiveSlider'."));
	}
}

void UPauseMenu::BindWidgetEvents()
{
	if (!ReturnButton)
	{
		UE_LOG(LogTemp, Warning, TEXT("未找到 ReturnButton，无法绑定点击事件。请确保蓝图中存在名为 'returnButton' 的按钮并正确绑定。"));
		return;
	}

	ReturnButton->OnClicked.RemoveDynamic(this, &UPauseMenu::HandleReturnButtonClicked);
	ReturnButton->OnClicked.AddDynamic(this, &UPauseMenu::HandleReturnButtonClicked);

	if (!MouseSensitiveSlider)
	{
		UE_LOG(LogTemp, Warning, TEXT("未找到 MouseSensitiveSlider，无法绑定滑条事件。请确保蓝图中存在名为 'mouseSensitiveSlider' 的滑条并正确绑定。"));
		return;
	}

	MouseSensitiveSlider->OnValueChanged.RemoveDynamic(this, &UPauseMenu::HandleMouseSensitiveSliderValueChanged);
	MouseSensitiveSlider->OnValueChanged.AddDynamic(this, &UPauseMenu::HandleMouseSensitiveSliderValueChanged);
}

void UPauseMenu::HandleReturnButtonClicked()
{
	ABearSurviorCharacter* BearSurviorCharacter = Cast<ABearSurviorCharacter>(GetOwningPlayerPawn());
	if (!BearSurviorCharacter)
	{
		return;
	}

	BearSurviorCharacter->TogglePauseMenu();
}

void UPauseMenu::HandleMouseSensitiveSliderValueChanged(float InValue)
{
	if (!MouseSensitiveSlider)
		return;

	const float ClampedValue = FMath::Clamp(InValue, 0.f, 1.f);

	if (!FMath::IsNearlyEqual(MouseSensitiveSlider->GetValue(), ClampedValue))
		MouseSensitiveSlider->SetValue(ClampedValue);

	UMainGameUserSetting* MainGameUserSetting = UMainGameUserSetting::GetMainGameUserSetting();
	if (!MainGameUserSetting)
	{
		UE_LOG(LogTemp, Warning, TEXT("无法保存鼠标灵敏度：未获取到 UMainGameUserSetting对象。"));
		return;
	}

	// 将滑条结果写入统一设置中心，由设置类负责持久化与角色同步来源。
	MainGameUserSetting->SetMouseSensitivity(ClampedValue);
}
