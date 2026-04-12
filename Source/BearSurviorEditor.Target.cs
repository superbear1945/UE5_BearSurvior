// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

/// <summary>
/// 编辑器目标配置类，定义 BearSurviorEditor 在 UnrealBuildTool 下的构建规则。
/// </summary>
public class BearSurviorEditorTarget : TargetRules
{
	public BearSurviorEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		// UE5.7 下使用 V6 默认构建设置，避免与 UnrealEditor 共享产物时出现规则冲突。
		DefaultBuildSettings = BuildSettingsVersion.V6;
		// 升级到 UE5.7 的头文件包含顺序，消除升级告警并保持行为一致。
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
		ExtraModuleNames.Add("BearSurvior");
	}
}
