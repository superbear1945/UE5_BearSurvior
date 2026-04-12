// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

/// <summary>
/// 游戏目标配置类，定义 BearSurvior 在 UnrealBuildTool 下的构建规则。
/// </summary>
public class BearSurviorTarget : TargetRules
{
	public BearSurviorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		// 与 UE5.7 默认构建行为对齐，减少升级阶段的兼容性问题。
		DefaultBuildSettings = BuildSettingsVersion.V6;
		// 使用 UE5.7 头文件包含顺序，避免旧版本包含顺序告警。
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
		ExtraModuleNames.Add("BearSurvior");
	}
}
