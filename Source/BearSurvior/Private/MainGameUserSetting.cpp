// 游戏用户设置类实现：负责统一管理并持久化玩家本地设置。

#include "MainGameUserSetting.h"

#include "BearSurviorCharacter.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerController.h"

UMainGameUserSetting::UMainGameUserSetting()
{
	ClampMouseSensitivity();
}

float UMainGameUserSetting::GetMouseSensitivity() const
{
	return MouseSensitivity;
}

void UMainGameUserSetting::SetMouseSensitivity(float InMouseSensitivity)
{
	MouseSensitivity = InMouseSensitivity;
	ClampMouseSensitivity();
	ApplyMouseSensitivityToLocalCharacter();
	SaveSettings();
}

UMainGameUserSetting* UMainGameUserSetting::GetMainGameUserSetting()
{
	if (!GEngine)
		return nullptr;

	return Cast<UMainGameUserSetting>(GEngine->GetGameUserSettings());
}

void UMainGameUserSetting::ApplySettings(bool bCheckForCommandLineOverrides)
{
	ClampMouseSensitivity();
	Super::ApplySettings(bCheckForCommandLineOverrides);
	ApplyMouseSensitivityToLocalCharacter();
}

void UMainGameUserSetting::ClampMouseSensitivity()
{
	MouseSensitivity = FMath::Clamp(MouseSensitivity, 0.f, 1.f);
}

void UMainGameUserSetting::ApplyMouseSensitivityToLocalCharacter() const
{
	if (!GEngine || !GEngine->GameViewport)
		return;

	UWorld* World = GEngine->GameViewport->GetWorld();
	if (!World)
		return;

	APlayerController* PlayerController = World->GetFirstPlayerController();
	if (!PlayerController)
		return;

	ABearSurviorCharacter* BearSurviorCharacter = Cast<ABearSurviorCharacter>(PlayerController->GetPawn());
	if (!BearSurviorCharacter)
		return;

	// 通过设置中心统一把最新灵敏度推送给当前本地角色，避免 UI 层直接依赖角色实现。
	BearSurviorCharacter->SetMouseSensitivity(MouseSensitivity);
}

