/**
 * 首个 PuerTS TypeScript 启动脚本（对应官方 demo 的 QuickStart 惯例）。
 *
 * 保存本文件后，编辑器内置 TS 服务（Content/JavaScript/PuertsEditor）会自动
 * 编译出 Content/JavaScript/QuickStart.js。
 *
 * 入口方式（二选一）：
 *  1. C++ 侧：puerts::FJsEnv 实例执行 JsEnv->Start("QuickStart")；
 *  2. 或通过 puer.ts 的“继承引擎类”功能让 UE 识别 TS 类（更进阶，暂未启用）。
 */
import { argv } from 'puerts';

// 最基本的验证：控制台/编辑器 Output Log 中应能看到这条日志
console.log('Hello PuerTS from TypeScript!');

// 演示读取 C++ 侧通过 JsEnv->Start("QuickStart", Arguments) 传入的参数
const gameInstance = argv.getByName('GameInstance');
if (gameInstance)
{
    console.log('GameInstance:', gameInstance.GetName());
}
