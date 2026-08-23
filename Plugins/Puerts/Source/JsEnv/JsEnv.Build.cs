/*
* Tencent is pleased to support the open source community by making Puerts available.
* Copyright (C) 2020 Tencent.  All rights reserved.
* Puerts is licensed under the BSD 3-Clause License, except for the third-party components listed in the file 'LICENSE' which may be subject to their corresponding license terms.
* This file is subject to the terms and conditions defined in file 'LICENSE', which is part of this source code package.
*/

using System.Collections.Generic;
using UnrealBuildTool;
using System.IO;
using System.Reflection;

public class JsEnv : ModuleRules
{    
    /// <summary>
    /// 可用的 JS 引擎版本枚举（V8 内核）。
    /// - VDeprecated：仅用于 UE 4.24 及更早版本的旧版 V8（对应文档 "V8 for ue 4.24 or below"）。
    /// - V8_4_371_19：V8 8.4 版本（较老，兼容老平台）。
    /// - V9_4_146_24：V8 9.4 版本（稳定推荐）。
    /// - V11_8_172：V8 11.8 版本（本仓库 ThirdParty 当前使用的版本，UE4.x 下会自动强制 C++17 编译）。
    /// 官方文档（源码安装方式）列出的可下载版本为 8.4.371.19 / 9.4.146.24 / 10.6.194（文档滞后于 releases），
    /// 实际以 backend-v8 releases 为准；下载后解压到 Plugins/Puerts/ThirdParty，
    /// 并把 UseV8Version 设为与所下载版本一致的枚举值。
    /// </summary>
    enum SupportedV8Versions
    {
        VDeprecated, // 仅适用于 4.24 或更早版本
        V8_4_371_19,
        V9_4_146_24,
        V11_8_172
    }

    /// <summary>
    /// 选择使用的 V8 引擎版本。
    /// UE 4.25 及以上默认使用 V8 11.8（与本仓库 ThirdParty/v8_11.8.172 目录一致）；
    /// 更老的引擎回退到 VDeprecated（旧版）。
    /// 若下载了其他版本，请手动改为对应的枚举值。
    /// </summary>
    private SupportedV8Versions UseV8Version = 
#if UE_4_25_OR_LATER
        SupportedV8Versions.V11_8_172;
#else
        SupportedV8Versions.VDeprecated;
#endif

    /// <summary>
    /// 是否使用 Node.js 作为 JS 引擎（而非纯 V8）。
    /// true：启用 Node.js 后端，可使用更多的 npm 模块（fs、net、Buffer 等 Node API），
    ///       需要从 backend-nodejs releases 下载库并解压到 Plugins/Puerts/ThirdParty，
    ///       代价是包体比 V8 更大；
    /// false：使用纯 V8（或 QuickJS），更轻量。
    /// 优先级：UseNodejs > UseQuickjs > UseV8Version（V8），与官方文档"虚拟机切换"说明一致。
    /// </summary>
    private bool UseNodejs = false;

    /// <summary>
    /// 在使用 Node.js 的前提下，选择 Node 大版本。
    /// true：使用 nodejs_16（Node 16，对应 libnode.93）；
    /// false：使用 nodejs（老版本，对应 libnode.83）。
    /// 需要与 ThirdParty 目录下实际存在的库目录名对应。
    /// </summary>
    private bool Node16 = true;

    /// <summary>
    /// 是否使用 QuickJS 作为 JS 引擎（替代 V8）。
    /// true：使用 QuickJS 后端，体积小，适合对包大小苛刻的场景，
    ///       需要从 backend-quickjs releases 下载库并解压到 Plugins/Puerts/ThirdParty；
    ///       同时会强制开启 ForceStaticLibInEditor、关闭 Inspector（WITHOUT_INSPECTOR）；
    /// false：使用 V8（默认）。
    /// 注意：该开关仅在 UseNodejs 为 false 时生效，与官方文档"虚拟机切换"说明一致。
    /// </summary>
    private bool UseQuickjs = false;

    /// <summary>
    /// 是否给 QuickJS 的符号加命名空间后缀（避免与 V8 符号冲突）。
    /// true：定义 WITH_QJS_NAMESPACE_SUFFIX=1 和 QJSV8NAMESPACE=v8_qjs，
    ///       一般用于 QuickJS 与 V8 同时链接的场景；
    /// false：不加后缀（默认，单独使用 QuickJS 时保持）。
    /// </summary>
    private bool QjsNamespaceSuffix = false;

    /// <summary>
    /// 是否启用 FFI（Foreign Function Interface）能力，允许 JS 直接调用原生 C 函数。
    /// true：链接 libffi 库并定义 WITH_FFI，可做高性能原生调用；
    /// false：不启用 FFI（默认），更轻量、更安全。
    /// </summary>
    private bool WithFFI = false;
    
    /// <summary>
    /// 是否在编辑器目标中强制使用 V8/QuickJS 静态库而非 DLL。
    /// true：编辑器构建也链接静态库（避免 DLL 依赖、便于调试符号内联，但增量编译更慢）；
    /// false：编辑器构建使用 DLL，非编辑器（打包/游戏）目标用静态库（默认）。
    /// 使用 QuickJS 时该开关会被强制置为 true。
    /// </summary>
    private bool ForceStaticLibInEditor = false;

    /// <summary>
    /// 是否开启线程安全模式（Puerts 支持多线程环境）。
    /// true：定义 THREAD_SAFE，允许 JS 运行时被多线程访问（需自行保证并发安全）；
    /// false：定义 NOT_THREAD_SAFE（默认），单线程模式性能更好。
    /// </summary>
    private bool ThreadSafe = false;

    /// <summary>
    /// FText 类型在 JS 中的映射方式。
    /// true：FText 转为 JS 字符串（默认，符合直觉，多语言本地化按当前语言输出）；
    /// false：定义 PUERTS_FTEXT_AS_OBJECT，FText 保留为对象，
    ///        需要显式调用 ToString 等接口获取文本（适合需要保留本地化 key 的场景）。
    /// </summary>
    private bool FTextAsString = true;
    
    /// <summary>
    /// 是否为编辑器构建添加后缀宏（PUERTS_WITH_EDITOR_SUFFIX）。
    /// true：定义宏，用于区分编辑器/运行时构建行为（默认，编辑器正常使用）；
    /// false：不定义，部分编辑器相关的兼容逻辑会跳过。
    /// </summary>
    private bool bEditorSuffix = true;

    /// <summary>
    /// 是否使用单线程平台（仅对 V8 9.4+ 生效）。
    /// true：定义 USING_SINGLE_THREAD_PLATFORM，强制 V8 按单线程平台编译/运行，
    ///       可减小体积，但会禁用 V8 的部分多线程优化；
    /// false：使用默认多线程 V8（推荐）。
    /// </summary>
    private bool SingleThreaded = false;
    
    /// <summary>
    /// 是否启用源码管理（Source Control）相关扩展（供其它模块读取）。
    /// true：Puerts 会启用源码控制相关的胶水代码（如编辑器内 TS 源码版本管理）；
    /// false：不启用（默认）。
    /// </summary>
    public static bool WithSourceControl = false;

    /// <summary>
    /// 是否支持 V8 字节码（ByteCode）。
    /// true：定义 WITH_V8_BYTECODE，允许在 JS 中编译/加载 V8 字节码（可用于代码加密/防篡改）；
    /// false：不支持字节码（默认），仅支持明文脚本。
    /// 仅在使用纯 V8（非 Node.js）时生效。
    /// </summary>
    public bool WithByteCode = false;

    /// <summary>
    /// 是否启用 WebSocket 支持。
    /// true：定义 WITH_WEBSOCKET / WITH_WEBSOCKET_SSL，并额外依赖 OpenSSL 模块，
    ///       允许 JS 侧使用 WebSocket（含 TLS）；
    /// false：不启用（默认），体积更小。
    /// </summary>
    private bool WithWebsocket = false;
    
    public JsEnv(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicDefinitions.Add("USING_IN_UNREAL_ENGINE");
        //PublicDefinitions.Add("WITH_V8_FAST_CALL");
        
        PublicDefinitions.Add("TS_BLUEPRINT_PATH=\"/Blueprints/TypeScript/\"");
        
        // 线程安全开关：true -> THREAD_SAFE（多线程模式）；false -> NOT_THREAD_SAFE（单线程模式）
        PublicDefinitions.Add(ThreadSafe ? "THREAD_SAFE" : "NOT_THREAD_SAFE");

        // 编辑器后缀开关：true -> 定义 PUERTS_WITH_EDITOR_SUFFIX，区分编辑器/运行时构建
        if (bEditorSuffix)
        {
            PublicDefinitions.Add("PUERTS_WITH_EDITOR_SUFFIX");
        }

        // WebSocket 开关：true -> 启用 WebSocket（含 SSL）并额外依赖 OpenSSL
        if (WithWebsocket)
        {
            PublicDefinitions.Add("WITH_WEBSOCKET");
            PublicDefinitions.Add("WITH_WEBSOCKET_SSL");
            PublicDependencyModuleNames.Add("OpenSSL");
        }


#if UE_5_6_OR_LATER
        CppCompileWarningSettings.ShadowVariableWarningLevel = WarningLevel.Warning;
#else
        ShadowVariableWarningLevel = WarningLevel.Warning;
#endif

        // FText 映射开关：false -> 定义 PUERTS_FTEXT_AS_OBJECT，FText 以对象形式暴露给 JS
        if (!FTextAsString)
        {
            PublicDefinitions.Add("PUERTS_FTEXT_AS_OBJECT");
        }

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core", "CoreUObject", "Engine", "ParamDefaultValueMetas", "UMG"
        });

        if (Target.bBuildEditor)
        {
            PublicDependencyModuleNames.AddRange(new string[] { "DirectoryWatcher", });
        }

        bEnableExceptions = true;
        var ContextField = GetType().GetField("Context", BindingFlags.Instance | BindingFlags.NonPublic);
        if (ContextField != null)
        {
            var bCanHotReloadField = ContextField.FieldType.GetField("bCanHotReload", BindingFlags.Instance | BindingFlags.Public);
            if (bCanHotReloadField != null)
            {
                bCanHotReloadField.SetValue(ContextField.GetValue(this), false);
            }
        }

        // 是否强制所有 UFunction 都生成纯 C++ 绑定（不走反射动态生成）
        // true：PUERTS_FORCE_CPP_UFUNCTION=1，全部用 C++ 静态绑定，性能更高但需要每次 rebuild 胶水代码；
        // false：PUERTS_FORCE_CPP_UFUNCTION=0（默认），混合模式，可动态生成绑定。
        bool bForceAllUFunctionInCPP = false;
        if (bForceAllUFunctionInCPP)
        {
            PublicDefinitions.Add("PUERTS_FORCE_CPP_UFUNCTION=1");
        }
        else
        {
            PublicDefinitions.Add("PUERTS_FORCE_CPP_UFUNCTION=0");
        }

        // 是否保持 UObject 强引用（避免 JS 持有对象被 GC 回收）
        // true：PUERTS_KEEP_UOBJECT_REFERENCE=1（默认），JS 侧持有 UObject 引用更安全；
        // false：PUERTS_KEEP_UOBJECT_REFERENCE=0，引用更弱、GC 更激进，需谨慎处理悬垂引用。
        bool bKeepUObjectReference = true;
        if(bKeepUObjectReference)
        {
            PublicDefinitions.Add("PUERTS_KEEP_UOBJECT_REFERENCE=1");
        }
        else
        {
            PublicDefinitions.Add("PUERTS_KEEP_UOBJECT_REFERENCE=0");
        }

        // 是否启用 Wasm3 解释器（用于在 JS 内跑 WebAssembly）
        // true：USE_WASM3=1，启用 Wasm 支持；false：USE_WASM3=0（默认），不启用。
        bool UseWasm = false;
        if (UseWasm)
        {
            PublicDefinitions.Add("USE_WASM3=1");
        }
        else
        {
            PublicDefinitions.Add("USE_WASM3=0");
        }
        // 是否用 Wasm3 覆盖/替代引擎自带的 WebAssembly 实现
        // true：WASM3_OVERRIDE_WEBASSEMBLY=1；false：WASM3_OVERRIDE_WEBASSEMBLY=0（默认）。
        // 仅在 UseWasm=true 时才有意义。
        bool OverrideWebAssembly = false;
        if (OverrideWebAssembly)
        {
            PublicDefinitions.Add("WASM3_OVERRIDE_WEBASSEMBLY=1");
        }
        else
        {
            PublicDefinitions.Add("WASM3_OVERRIDE_WEBASSEMBLY=0");
        }
        PublicDependencyModuleNames.AddRange(new string[]
            {
                "WasmCore", "Json"
            });

        // JS 引擎选择逻辑（优先级从高到低）：
        // 1. UseNodejs=true  -> 使用 Node.js（ThirdPartyNodejs）
        // 2. UseQuickjs=true -> 使用 QuickJS（ThirdPartyQJS，会强制静态库）
        // 3. UseV8Version 为有效版本 -> 使用对应版本 V8（ThirdParty）
        // 4. 以上都不满足 -> 使用旧版 V8（OldThirdParty，仅老引擎）
        if (UseNodejs)
        {
            ThirdPartyNodejs(Target);
        }
        else if (UseQuickjs)
        {
            ForceStaticLibInEditor = true;
            ThirdPartyQJS(Target);
        }
        else if (UseV8Version > SupportedV8Versions.VDeprecated)
        {
            ThirdParty(Target);
        }
        else
        {
            OldThirdParty(Target);
        }
        
        // FFI 开关：true -> 链接 libffi，JS 可调用原生 C 函数
        if (WithFFI) AddFFI(Target);

        string coreJSPath = Path.GetFullPath(Path.Combine(ModuleDirectory, "..", "..", "Content"));
        string destDirName = Path.GetFullPath(Path.Combine(ModuleDirectory, "..", "..", "..", "..", "Content"));
        DirectoryCopy(coreJSPath, destDirName, true);

        // 每次build时拷贝一些手写的.d.ts到Typing目录以同步更新
        string srcDtsDirName  = Path.GetFullPath(Path.Combine(ModuleDirectory, "..", "..", "Typing"));
        string dstDtsDirName = Path.GetFullPath(Path.Combine(ModuleDirectory, "..", "..", "..", "..", "Typing"));
        DirectoryCopy(srcDtsDirName, dstDtsDirName, true);

    }

    void OldThirdParty(ReadOnlyTargetRules Target)
    {
        string LibraryPath = Path.GetFullPath(Path.Combine(ModuleDirectory, "..", "..", "ThirdParty", "v8_for_ue424_or_below", "Lib"));
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            //if (Target.bBuildEditor)
            //{
            //    WinDll(Path.Combine(LibraryPath, "V8"));
            //}
            //else
            {
                string V8LibraryPath = Path.Combine(LibraryPath, "Win64");

                PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "encoding.lib"));
                PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "inspector.lib"));
                PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "inspector_string_conversions.lib"));
                PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "v8_base_without_compiler_0.lib"));
                PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "v8_base_without_compiler_1.lib"));
                PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "v8_compiler.lib"));
                PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "v8_external_snapshot.lib"));
                PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "v8_libbase.lib"));
                PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "v8_libplatform.lib"));
                PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "v8_libsampler.lib"));
            }
        }
        else if (Target.Platform == UnrealTargetPlatform.Android)
        {
            if (Target.Version.MajorVersion == 4 && Target.Version.MinorVersion >= 25)
            {
                // for arm7
                string V8LibraryPath = Path.Combine(LibraryPath, "Android", "armeabi-v7a", "8.4.371.19");
                PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libwee8.a"));
                // for arm64
                V8LibraryPath = Path.Combine(LibraryPath, "V8", "Android", "arm64-v8a", "8.4.371.19");
                PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libwee8.a"));
            }
            else if (Target.Version.MajorVersion == 4 && Target.Version.MinorVersion < 25 && Target.Version.MinorVersion >= 22)
            {
                // for arm7
                string V8LibraryPath = Path.Combine(LibraryPath, "Android", "armeabi-v7a", "7.4.288");
                PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libinspector.a"));
                PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libv8_base.a"));
                PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libv8_external_snapshot.a"));
                PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libv8_libbase.a"));
                PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libv8_libplatform.a"));
                PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libv8_libsampler.a"));
                // for arm64
                V8LibraryPath = Path.Combine(LibraryPath, "Android", "arm64-v8a", "7.4.288");
                PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libinspector.a"));
                PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libv8_base.a"));
                PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libv8_external_snapshot.a"));
                PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libv8_libbase.a"));
                PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libv8_libplatform.a"));
                PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libv8_libsampler.a"));
            } 
#if !UE_4_22_OR_LATER
            else if (Target.Version.MajorVersion == 4 && Target.Version.MinorVersion < 22) 
            {
                string V8LibraryPath = Path.Combine(LibraryPath, "Android", "armeabi-v7a", "7.4.288");
                PublicLibraryPaths.Add(V8LibraryPath);
                V8LibraryPath = Path.Combine(LibraryPath, "Android", "arm64-v8a", "7.4.288");
                PublicLibraryPaths.Add(V8LibraryPath);
                PublicAdditionalLibraries.Add("inspector");
                PublicAdditionalLibraries.Add("v8_base");
                PublicAdditionalLibraries.Add("v8_external_snapshot");
                PublicAdditionalLibraries.Add("v8_libbase");
                PublicAdditionalLibraries.Add("v8_libplatform");
                PublicAdditionalLibraries.Add("v8_libsampler");
            }
#endif
        }
        else if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            // PublicFrameworks.AddRange(new string[] { "WebKit",  "JavaScriptCore" });
            PublicFrameworks.AddRange(new string[] { "WebKit" });
            string V8LibraryPath = Path.Combine(LibraryPath, "macOS");
            PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libbindings.a"));
            PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libencoding.a"));
            PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libinspector.a"));
            PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libinspector_string_conversions.a"));
            PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libtorque_base.a"));
            PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libtorque_generated_definitions.a"));
            PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libtorque_generated_initializers.a"));
            PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libv8_base_without_compiler.a"));
            PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libv8_compiler.a"));
            PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libv8_external_snapshot.a"));
            PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libv8_init.a"));
            PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libv8_initializers.a"));
            PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libv8_libbase.a"));
            PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libv8_libplatform.a"));
            PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libv8_libsampler.a"));
            PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libv8_nosnapshot.a"));
        }
        else if (Target.Platform == UnrealTargetPlatform.IOS)
        {
            PublicFrameworks.AddRange(new string[] { "WebKit" });
            string V8LibraryPath = Path.Combine(LibraryPath, "iOS", "arm64");
            PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libbindings.a"));
            PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libencoding.a"));
            PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libinspector.a"));
            PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libinspector_string_conversions.a"));
            PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libtorque_generated_definitions.a"));
            PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libv8_base_without_compiler.a"));
            PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libv8_compiler.a"));
            PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libv8_external_snapshot.a"));
            PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libv8_libbase.a"));
            PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libv8_libplatform.a"));
            PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libv8_libsampler.a"));

            //PublicAdditionalLibraries.Add(Path.Combine(Path.Combine(LibraryPath, "ffi", "iOS"), "libffi.a"));
        }
        else if (Target.Platform == UnrealTargetPlatform.Linux)
        {
            string V8LibraryPath = Path.Combine(LibraryPath, "Linux");
            PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libwee8.a"));
        }

        string V8HeaderPath = Path.GetFullPath(Path.Combine(ModuleDirectory, "..", "..", "ThirdParty", "v8_for_ue424_or_below", "Inc"));
        // External headers
        if (Target.Platform == UnrealTargetPlatform.Android)
        {
            if (Target.Version.MajorVersion == 4 && Target.Version.MinorVersion >= 25)
            {
                PublicIncludePaths.AddRange(new string[] { Path.Combine(V8HeaderPath, "8.4.371.19") });
            }
            else if (Target.Version.MajorVersion == 4 && Target.Version.MinorVersion < 25)
            {
                PublicIncludePaths.AddRange(new string[] { Path.Combine(V8HeaderPath, "7.4.288") });
            }
        }
        //else if (Target.bBuildEditor && Target.Platform == UnrealTargetPlatform.Win64)
        //{
        //    PublicIncludePaths.AddRange(new string[] { Path.Combine(HeaderPath, "8.4.371.19") });
        //}
        else if (Target.Platform == UnrealTargetPlatform.Win64 ||
            Target.Platform == UnrealTargetPlatform.IOS ||
            Target.Platform == UnrealTargetPlatform.Mac ||
            Target.Platform == UnrealTargetPlatform.Linux)
        {
            PublicIncludePaths.AddRange(new string[] { Path.Combine(V8HeaderPath, "7.7.299") });
        }
        string HeaderPath = Path.GetFullPath(Path.Combine(ModuleDirectory, "..", "..", "ThirdParty", "Include"));
        PublicIncludePaths.AddRange(new string[] { Path.Combine(HeaderPath, "websocketpp") });
        PublicIncludePaths.AddRange(new string[] { Path.Combine(HeaderPath, "asio") });
    }

    void AddFFI(ReadOnlyTargetRules Target)
    {
        string HeaderPath = Path.GetFullPath(Path.Combine(ModuleDirectory, "..", "..", "ThirdParty", "Include"));
        string LibraryPath = Path.GetFullPath(Path.Combine(ModuleDirectory, "..", "..", "ThirdParty", "Library"));
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PublicIncludePaths.AddRange(new string[] {Path.Combine(HeaderPath, "ffi", "Win64")});
            PublicAdditionalLibraries.Add(Path.Combine(LibraryPath, "ffi", "Win64", "ffi.lib"));
        }
        else if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            PublicIncludePaths.AddRange(new string[] {Path.Combine(HeaderPath, "ffi", "macOS")});
            PublicAdditionalLibraries.Add(Path.Combine(LibraryPath, "ffi", "macOS", "libffi.a"));
        }
        else if (Target.Platform == UnrealTargetPlatform.IOS)
        {
            PublicIncludePaths.AddRange(new string[] {Path.Combine(HeaderPath, "ffi", "iOS")});
            PublicAdditionalLibraries.Add(Path.Combine(LibraryPath, "ffi", "iOS", "libffi.a"));
        }
        else if (Target.Platform == UnrealTargetPlatform.Android)
        {
            PublicIncludePaths.AddRange(new string[] {Path.Combine(HeaderPath, "ffi", "Android")});
            PublicAdditionalLibraries.Add(Path.Combine(LibraryPath, "ffi", "Android", "armeabi-v7a", "libffi.a"));
            PublicAdditionalLibraries.Add(Path.Combine(LibraryPath, "ffi", "Android", "arm64-v8a", "libffi.a"));
        }

        PrivateDefinitions.Add("WITH_FFI");
    }

    void AddRuntimeDependencies(string[] DllNames, string LibraryPath, bool Delay)
    {
        foreach (var DllName in DllNames)
        {
            if(Delay) PublicDelayLoadDLLs.Add(DllName);
            var DllPath = Path.Combine(LibraryPath, DllName);
            var DestDllPath = Path.Combine("$(BinaryOutputDir)", DllName);
            RuntimeDependencies.Add(DestDllPath, DllPath, StagedFileType.NonUFS);
        }
    }

    void WinDll(string LibraryPath)
    {
        string V8LibraryPath = Path.Combine(LibraryPath, "Win64DLL");
        PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "v8.dll.lib"));
        PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "v8_libplatform.dll.lib"));

        List<string> deps = new List<string> {
            "v8.dll",
            "v8_libplatform.dll",
            "v8_libbase.dll"
        };
        deps.Add(UseV8Version == SupportedV8Versions.V11_8_172 ? "third_party_zlib.dll" : "zlib.dll");

        AddRuntimeDependencies(deps.ToArray(), V8LibraryPath, false);
    }
    
    void MacDylib(string LibraryPath)
    {
        PublicAdditionalLibraries.Add(Path.Combine(LibraryPath, "libv8.dylib"));
        PublicAdditionalLibraries.Add(Path.Combine(LibraryPath, "libv8_libplatform.dylib"));
        PublicAdditionalLibraries.Add(Path.Combine(LibraryPath, "libv8_libbase.dylib"));
        PublicAdditionalLibraries.Add(Path.Combine(LibraryPath, "libchrome_zlib.dylib"));
        if (UseV8Version == SupportedV8Versions.V11_8_172)
        {
            PublicAdditionalLibraries.Add(Path.Combine(LibraryPath, "libthird_party_abseil-cpp_absl.dylib"));
        }
    }

    void ThirdParty(ReadOnlyTargetRules Target)
    {
        // 单线程开关：true -> 强制 V8 单线程平台编译（体积更小，牺牲多线程优化）
        if (SingleThreaded)
        {
            PrivateDefinitions.Add("USING_SINGLE_THREAD_PLATFORM");
        }

        // 字节码开关：true -> 允许 JS 侧编译/加载 V8 字节码（代码加密）
        if (WithByteCode)
        {
            PrivateDefinitions.Add("WITH_V8_BYTECODE");
        }

        string v8LibSuffix = "";
        
        if (UseV8Version == SupportedV8Versions.V8_4_371_19)
        {
            if(Directory.Exists(Path.Combine(ModuleDirectory, "..", "..", "ThirdParty", "v8_8.4.371.19")))
            {
                v8LibSuffix = "_8.4.371.19";
            }
        }
        else if (UseV8Version == SupportedV8Versions.V9_4_146_24)
        {
            v8LibSuffix = "_9.4.146.24";
        }
        else if (UseV8Version == SupportedV8Versions.V11_8_172)
        {
#if !UE_5_0_OR_LATER
            CppStandard = CppStandardVersion.Cpp17;
#endif
            v8LibSuffix = "_11.8.172";
        }
        //Add header
        string HeaderPath = Path.GetFullPath(Path.Combine(ModuleDirectory, "..", "..", "ThirdParty", "Include"));
        PublicIncludePaths.AddRange(new string[] { Path.Combine(HeaderPath, "websocketpp") });
        PublicIncludePaths.AddRange(new string[] { Path.Combine(HeaderPath, "asio") });
        PublicIncludePaths.AddRange(new string[] { Path.Combine(ModuleDirectory, "..", "..", "ThirdParty", "v8" + v8LibSuffix, "Inc") });

        string LibraryPath = Path.GetFullPath(Path.Combine(ModuleDirectory, "..", "..", "ThirdParty", "v8" + v8LibSuffix, "Lib"));
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            // 编辑器且未强制静态库时使用 DLL；否则（游戏目标或 ForceStaticLibInEditor=true）链接静态库 wee8.lib
            if (!Target.bBuildEditor || ForceStaticLibInEditor)
            {
                string V8LibraryPath = Path.Combine(LibraryPath, "Win64MD");
                PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "wee8.lib"));
            }
            else 
            {
                WinDll(LibraryPath);
            }
        }
        else if (Target.Platform == UnrealTargetPlatform.Android)
        {
            string V8LibraryPath = Path.Combine(LibraryPath, "Android", "armeabi-v7a");
            PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libwee8.a"));
            V8LibraryPath = Path.Combine(LibraryPath, "Android", "arm64-v8a");
            PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libwee8.a"));
        }
        else if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            // 编辑器且未强制静态库时用 dylib；否则用静态库 libwee8.a（arm64 时自动切 _arm64 目录）
            if (!Target.bBuildEditor || ForceStaticLibInEditor)
            {
                LibraryPath = Path.Combine(LibraryPath, "macOS");
#if UE_5_2_OR_LATER
                if (Target.Architecture == UnrealArch.Arm64)
                {
                    LibraryPath += "_arm64";
                }
#endif
                PublicAdditionalLibraries.Add(Path.Combine(LibraryPath, "libwee8.a"));
            }
            else
            {
                LibraryPath = Path.Combine(LibraryPath, "macOSdylib");
#if UE_5_2_OR_LATER
                if (Target.Architecture == UnrealArch.Arm64)
                {
                    LibraryPath += "_arm64";
                }
#endif
                MacDylib(LibraryPath);
            }
        }
        else if (Target.Platform == UnrealTargetPlatform.IOS)
        {
            PublicFrameworks.AddRange(new string[] { "WebKit" });
            string V8LibraryPath = Path.Combine(LibraryPath, "iOS", "arm64");
            PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libwee8.a"));
        } 
        else if (Target.Platform == UnrealTargetPlatform.Linux) 
        {
            string V8LibraryPath = Path.Combine(LibraryPath, "Linux");
            PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libwee8.a"));
        }

        // 强制静态库开关：true -> 定义 FORCE_USE_STATIC_V8_LIB（编辑器也使用静态 V8）
        if (ForceStaticLibInEditor)
        {
            PrivateDefinitions.Add("FORCE_USE_STATIC_V8_LIB");
        }
    }
    
    void ThirdPartyNodejs(ReadOnlyTargetRules Target)
    {
        PrivateDefinitions.Add("WITH_NODEJS");
        string WsHeaderPath = Path.GetFullPath(Path.Combine(ModuleDirectory, "..", "..", "ThirdParty", "Include"));
        PublicIncludePaths.AddRange(new string[] { Path.Combine(WsHeaderPath, "websocketpp") });
        PublicIncludePaths.AddRange(new string[] { Path.Combine(WsHeaderPath, "asio") });

        // Node 版本开关：true -> 使用 nodejs_16 目录（Node 16 / libnode.93）；false -> 使用 nodejs 目录（旧版 / libnode.83）
        string NodeRoot = Node16 ? "nodejs_16" : "nodejs";
        string HeaderPath = Path.GetFullPath(Path.Combine(ModuleDirectory, "..", "..", "ThirdParty", NodeRoot));
        PublicIncludePaths.AddRange(new string[] { Path.Combine(HeaderPath, "include") });
        PublicIncludePaths.AddRange(new string[] { Path.Combine(HeaderPath, "deps", "v8", "include") });
        PublicIncludePaths.AddRange(new string[] { Path.Combine(HeaderPath, "deps", "uv", "include") });

        string LibraryPath = Path.GetFullPath(Path.Combine(ModuleDirectory, "..", "..", "ThirdParty", NodeRoot, "lib"));
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            string V8LibraryPath = Path.Combine(LibraryPath, "Win64");
            PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libnode.lib"));

            RuntimeDependencies.Add("$(TargetOutputDir)/libnode.dll", Path.Combine(V8LibraryPath, "libnode.dll"));
            AddRuntimeDependencies(new string[] { "libnode.dll" }, V8LibraryPath, false);
        }
        else if (Target.Platform == UnrealTargetPlatform.Android)
        {
            /*
#if UE_4_19_OR_LATER
                        AdditionalPropertiesForReceipt.Add("AndroidPlugin", Path.Combine(ModuleDirectory, "..", "..", "ThirdParty", "Libnode_APL.xml"));
#else
                        AdditionalPropertiesForReceipt.Add(new ReceiptProperty("AndroidPlugin", Path.Combine(ModuleDirectory, "..", "..", "ThirdParty", "Libnode_APL.xml")));
#endif
#if UE_4_24_OR_LATER
                        PublicSystemLibraryPaths.Add(Path.Combine(LibraryPath, "Android", "armeabi-v7a"));
                        PublicSystemLibraryPaths.Add(Path.Combine(LibraryPath, "Android", "arm64-v8a"));
                        PublicSystemLibraries.Add("node");
#else
                        PublicLibraryPaths.Add(Path.Combine(LibraryPath, "Android", "armeabi-v7a"));
                        PublicLibraryPaths.Add(Path.Combine(LibraryPath, "Android", "arm64-v8a"));
                        PublicAdditionalLibraries.Add("node");
#endif  //UE_4_24_OR_LATER
            */
            
            string[] Archs = new string[] { "armeabi-v7a", "arm64-v8a" };
            foreach (var Arch in Archs)
            {
                string V8LibraryPath = Path.Combine(LibraryPath, "Android", Arch);
                PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libhistogram.a"));
                PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libuvwasi.a"));
                PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libnode.a"));
                PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libnode_stub.a"));
                PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libv8_snapshot.a"));
                PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libv8_libplatform.a"));
                PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libzlib.a"));
                PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libllhttp.a"));
                PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libcares.a"));
                PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libuv.a"));
                PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libnghttp2.a"));
                PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libbrotli.a"));
                PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libv8_base_without_compiler.a"));
                PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libv8_libbase.a"));
                PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libv8_zlib.a"));
                PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libv8_compiler.a"));
                PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libv8_initializers.a"));
                if (!Node16)
                {
                    PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libv8_libsampler.a"));
                }
            }
            
        }
        else if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            string V8LibraryPath = Path.Combine(LibraryPath, "macOS");
#if UE_5_2_OR_LATER
            if (Target.Architecture == UnrealArch.Arm64)
            {
                V8LibraryPath = Path.Combine(LibraryPath, "macOS_arm64");
            }
#endif
            if (Node16)
            {
                PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libnode.93.dylib"));
            }
            else
            {
                PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libnode.83.dylib"));
            }
        }
        else if (Target.Platform == UnrealTargetPlatform.IOS)
        {
            string V8LibraryPath = Path.Combine(LibraryPath, "iOS");
            PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libhistogram.a"));
            PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libuvwasi.a"));
            PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libnode.a"));
            PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libnode_stub.a"));
            PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libv8_snapshot.a"));
            PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libv8_libplatform.a"));
            PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libzlib.a"));
            PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libllhttp.a"));
            PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libcares.a"));
            PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libuv.a"));
            PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libnghttp2.a"));
            PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libbrotli.a"));
            PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libopenssl.a"));
            PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libv8_base_without_compiler.a"));
            PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libv8_libbase.a"));
            PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libv8_zlib.a"));
            PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libv8_compiler.a"));
            PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libv8_initializers.a"));
            if (!Node16)
            {
                PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libv8_libsampler.a"));
            }
        } 
        else if (Target.Platform == UnrealTargetPlatform.Linux) 
        {
            string V8LibraryPath = Path.Combine(LibraryPath, "Linux");
            PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libnode.so"));
            RuntimeDependencies.Add("$(TargetOutputDir)/libnode.so.93", Path.Combine(V8LibraryPath, "libnode.so.93"));
        }
    }

    void ThirdPartyQJS(ReadOnlyTargetRules Target)
    {
        PrivateDefinitions.Add("WITHOUT_INSPECTOR");
        PrivateDefinitions.Add("WITH_QUICKJS");
        if (QjsNamespaceSuffix)
        {
            PublicDefinitions.Add("WITH_QJS_NAMESPACE_SUFFIX=1");
            PublicDefinitions.Add("QJSV8NAMESPACE=v8_qjs");
        }
        
        string ThirdPartyPath = Path.GetFullPath(Path.Combine(ModuleDirectory, "..", "..", "ThirdParty"));
        string HeaderPath = Path.GetFullPath(Path.Combine(ThirdPartyPath, "Include"));
        PublicIncludePaths.AddRange(new string[] { Path.Combine(HeaderPath, "websocketpp") });
        PublicIncludePaths.AddRange(new string[] { Path.Combine(HeaderPath, "asio") });
        PublicIncludePaths.AddRange(new string[] { Path.Combine(ThirdPartyPath, "quickjs", "Inc") });

        string LibraryPath = Path.GetFullPath(Path.Combine(ThirdPartyPath, "quickjs", "Lib"));
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            string V8LibraryPath = Path.Combine(LibraryPath, "Win64MD");

            // 是否使用 QuickJS 源码编译（而非预编译库）
            // true：链接 libquickjs.dll.a 并定义 BUILDING_V8_SHARED（需要源码参与构建）；
            // false：使用预编译的 quickjs.dll.lib（默认，推荐）。
            bool UsingSource = false;
            if (UsingSource)
            {
                PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libquickjs.dll.a"));
                PrivateDefinitions.Add("BUILDING_V8_SHARED");
            }
            else
            {
                if (Target.bBuildEditor && !ForceStaticLibInEditor)
                {
                    V8LibraryPath = Path.Combine(LibraryPath, "Win64DLL");
                    AddRuntimeDependencies(new string[] { "v8qjs.dll" }, V8LibraryPath, false);
                }

                PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "quickjs.dll.lib"));
            }

            AddRuntimeDependencies(new string[] { "msys-quickjs.dll" }, V8LibraryPath, false);
            AddRuntimeDependencies(new string[]
            {
                "libgcc_s_seh-1.dll",
                "libwinpthread-1.dll"
            }, V8LibraryPath, true);
        }
        else if (Target.Platform == UnrealTargetPlatform.Android)
        {
            string V8LibraryPath = Path.Combine(LibraryPath, "Android", "armeabi-v7a");
            PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libquickjs.a"));
            V8LibraryPath = Path.Combine(LibraryPath, "Android", "arm64-v8a");
            PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libquickjs.a"));
        }
        else if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            // PublicFrameworks.AddRange(new string[] { "WebKit",  "JavaScriptCore" });
            //PublicFrameworks.AddRange(new string[] { "WebKit" });
            if (!Target.bBuildEditor || ForceStaticLibInEditor)
            {
                string V8LibraryPath = Path.Combine(LibraryPath, "macOS");
#if UE_5_2_OR_LATER
                if (Target.Architecture == UnrealArch.Arm64)
                {
                    V8LibraryPath = Path.Combine(LibraryPath, "macOS_arm64");
                }
#endif
                PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libquickjs.a"));
            }
            else
            {
               string V8LibraryPath = Path.Combine(LibraryPath, "macOSdylib");
               string QJSDylibName = "libquickjs.dylib";
#if UE_5_2_OR_LATER
                if (Target.Architecture == UnrealArch.Arm64)
                {
                    V8LibraryPath = Path.Combine(LibraryPath, "macOS_arm64");
                    QJSDylibName = "libquickjs.a";
                }
#endif
                PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, QJSDylibName));
            }
        }
        else if (Target.Platform == UnrealTargetPlatform.IOS)
        {
            PublicFrameworks.AddRange(new string[] { "WebKit" });
            string V8LibraryPath = Path.Combine(LibraryPath, "iOS", "arm64");
            PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libquickjs.a"));
        }
        else if (Target.Platform == UnrealTargetPlatform.Linux)
        {
            string V8LibraryPath = Path.Combine(LibraryPath, "Linux");
            PublicAdditionalLibraries.Add(Path.Combine(V8LibraryPath, "libquickjs.a"));
        }
    }

    private static void DirectoryCopy(string sourceDirName, string destDirName, bool copySubDirs)
    {
        DirectoryInfo dir = new DirectoryInfo(sourceDirName);

        if (!dir.Exists)
        {
            throw new DirectoryNotFoundException(
            "Source directory does not exist or could not be found: "
            + sourceDirName);
        }

        if (!Directory.Exists(destDirName))
        {
            Directory.CreateDirectory(destDirName);
        }

        // Get the files in the directory and copy them to the new location.
        FileInfo[] files = dir.GetFiles();
        foreach (FileInfo file in files)
        {
            string temppath = Path.Combine(destDirName, file.Name);
            file.CopyTo(temppath, true);
        }

        if (copySubDirs)
        {
            DirectoryInfo[] dirs = dir.GetDirectories();
            foreach (DirectoryInfo subdir in dirs)
            {
                string temppath = Path.Combine(destDirName, subdir.Name);
                DirectoryCopy(subdir.FullName, temppath, copySubDirs);
            }
        }
    }

}
