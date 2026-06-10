/**
 * UE MCP Bridge - Pi 扩展
 *
 * 连接 UE5 编辑器（通过 UE_MCP_Bridge 插件在 ws://localhost:9877 提供 WebSocket 服务），
 * 注册 185+ 个 MCP 工具供 LLM 调用，实现对 UE5 编辑器的全面控制。
 *
 * 使用方式：
 *   1. 启动 UE5 Editor（自动加载 UE_MCP_Bridge 插件）
 *   2. 启动 pi，扩展会自动连接
 *   3. 直接使用工具操作 UE5 编辑器
 */

import type { ExtensionAPI, ToolResult } from "@earendil-works/pi-coding-agent";
import { Type, type TSchema } from "typebox";

// ─── 类型定义 ─────────────────────────────────────────────────────────────────

/** JSON-RPC 请求 */
interface JsonRpcRequest {
  jsonrpc: "2.0";
  id: number;
  method: string;
  params: Record<string, unknown>;
}

/** JSON-RPC 成功响应 */
interface JsonRpcSuccess {
  jsonrpc: "2.0";
  id: number;
  result: Record<string, unknown>;
}

/** JSON-RPC 错误响应 */
interface JsonRpcError {
  jsonrpc: "2.0";
  id: number | null;
  error: {
    code: number;
    message: string;
  };
}

type JsonRpcResponse = JsonRpcSuccess | JsonRpcError;

// ─── WebSocket 连接管理器 ─────────────────────────────────────────────────────

class UEMCPConnection {
  private ws: WebSocket | null = null;
  private requestId = 0;
  private pending = new Map<number, { resolve: (v: unknown) => void; reject: (e: Error) => void }>();
  private connected = false;
  private connecting = false;
  private reconnectTimer: ReturnType<typeof setTimeout> | null = null;
  private url: string;
  private onStatusChange: ((connected: boolean) => void) | null = null;

  constructor(url = "ws://localhost:9877") {
    this.url = url;
  }

  /** 注册连接状态变更回调 */
  onConnectionChange(cb: (connected: boolean) => void): void {
    this.onStatusChange = cb;
  }

  /** 连接（或重连）到 UE MCP Bridge */
  async connect(): Promise<void> {
    if (this.connected || this.connecting) return;
    this.connecting = true;

    return new Promise((resolve, reject) => {
      try {
        this.ws = new WebSocket(this.url);

        this.ws.onopen = () => {
          console.log("[UE-MCP] 已连接到 UE5 编辑器 MCP Bridge");
          this.connected = true;
          this.connecting = false;
          this.onStatusChange?.(true);
          resolve();
        };

        this.ws.onmessage = (event: MessageEvent) => {
          try {
            const data = typeof event.data === "string" ? event.data : new TextDecoder().decode(event.data as ArrayBuffer);
            const response = JSON.parse(data) as JsonRpcResponse;
            const id = typeof response.id === "number" ? response.id : -1;
            const pending = this.pending.get(id);
            if (pending) {
              this.pending.delete(id);
              if ("error" in response) {
                pending.reject(new Error(`[UE-MCP] ${response.error.message}`));
              } else {
                pending.resolve(response.result);
              }
            }
          } catch (e) {
            console.error("[UE-MCP] 解析响应失败:", e);
          }
        };

        this.ws.onerror = (_event: Event) => {
          this.connected = false;
          this.connecting = false;
          this.onStatusChange?.(false);
          if (!this.connected) {
            reject(new Error("无法连接到 UE MCP Bridge（ws://localhost:9877）。请确保 UE5 编辑器已启动。"));
          }
        };

        this.ws.onclose = () => {
          this.connected = false;
          this.connecting = false;
          this.onStatusChange?.(false);
          this.scheduleReconnect();
        };
      } catch (e) {
        this.connecting = false;
        reject(e);
      }
    });
  }

  /** 调度自动重连（10 秒后） */
  private scheduleReconnect(): void {
    if (this.reconnectTimer) return;
    this.reconnectTimer = setTimeout(() => {
      this.reconnectTimer = null;
      this.connect().catch(() => {});
    }, 10000);
  }

  /** 发送 JSON-RPC 请求并等待响应 */
  async call(method: string, params: Record<string, unknown> = {}): Promise<unknown> {
    if (!this.connected) {
      throw new Error("未连接到 UE MCP Bridge。请确保 UE5 编辑器已启动。");
    }

    const id = ++this.requestId;
    const request: JsonRpcRequest = {
      jsonrpc: "2.0",
      id,
      method,
      params,
    };

    return new Promise((resolve, reject) => {
      this.pending.set(id, { resolve, reject });

      const timeout = setTimeout(() => {
        this.pending.delete(id);
        reject(new Error(`[UE-MCP] 请求超时: ${method} (30s)`));
      }, 30000);

      try {
        this.ws?.send(JSON.stringify(request));
      } catch (e) {
        clearTimeout(timeout);
        this.pending.delete(id);
        reject(e);
      }

      // 清除超时
      const origReject = reject;
      this.pending.set(id, {
        resolve,
        reject: (e: Error) => {
          clearTimeout(timeout);
          origReject(e);
        },
      });
    });
  }

  /** 断开连接 */
  disconnect(): void {
    if (this.reconnectTimer) {
      clearTimeout(this.reconnectTimer);
      this.reconnectTimer = null;
    }
    this.connected = false;
    this.connecting = false;
    this.ws?.close();
    this.ws = null;
    this.onStatusChange?.(false);
  }

  /** 是否已连接 */
  isConnected(): boolean {
    return this.connected;
  }
}

// ─── 工具定义 ─────────────────────────────────────────────────────────────────

/**
 * 工具描述符 - 用于定义每个 MCP 方法对应的 pi 工具
 */
interface ToolDescriptor {
  /** MCP 方法名（API 名称） */
  method: string;
  /** 工具显示名 */
  name?: string;
  /** 工具描述 */
  description: string;
  /** 参数 schema（可选，留空则接受任意对象） */
  parameters?: Record<string, TSchema>;
  /** 必需的参数名列表 */
  required?: string[];
}

/**
 * 工具渲染器 - 在 pi 终端中显示 UE-MCP 工具调用
 */
function renderToolCall(method: string, args: Record<string, unknown>): string {
  let text = `🎮 ${method}`;
  const keys = Object.keys(args);
  if (keys.length > 0) {
    text += " " + keys.map(k => `${k}=${JSON.stringify(args[k])}`).join(" ");
  }
  return text;
}

/**
 * 提取结果中的关键信息用于渲染
 */
function renderResultSummary(result: unknown): string {
  if (!result || typeof result !== "object") return String(result ?? "");

  const obj = result as Record<string, unknown>;

  // 检查 error 字段
  if (obj.error && typeof obj.error === "string") {
    return `❌ ${obj.error}`;
  }

  // 检查 success 字段
  if (obj.success === false && obj.error) {
    return `❌ ${obj.error}`;
  }

  // 提取数量信息
  const countFields = ["count", "propertyCount", "functionCount", "fieldCount", "valueCount", "numEnums"];
  for (const field of countFields) {
    if (typeof obj[field] === "number") {
      return `✅ ${obj[field]} 项`;
    }
  }

  // 提取简单的 name/className 字段
  if (typeof obj.name === "string") {
    return `✅ ${obj.name}`;
  }
  if (typeof obj.className === "string") {
    return `✅ ${obj.className}`;
  }
  if (typeof obj.path === "string") {
    return `✅ ${obj.path}`;
  }

  return `✅ 成功`;
}

/**
 * 创建 pi 工具注册
 */
function createPiTool(
  connection: UEMCPConnection,
  desc: ToolDescriptor,
) {
  const toolName = desc.name ?? desc.method;
  const paramSchema: Record<string, TSchema> = desc.parameters ?? {};
  const required = desc.required ?? [];

  // 构建 TypeBox schema
  const schemaProps: Record<string, TSchema> = {};
  for (const [key, schema] of Object.entries(paramSchema)) {
    schemaProps[key] = schema;
  }

  // 如果没有预定义参数，使用通用 Record<string, any>
  let parameters: TSchema;
  if (Object.keys(paramSchema).length > 0) {
    parameters = Type.Object(schemaProps, {
      additionalProperties: false,
    });
    if (required.length > 0) {
      parameters = Type.Object(schemaProps, { minProperties: required.length });
    }
  } else {
    parameters = Type.Record(Type.String(), Type.Unknown());
  }

  return {
    name: toolName,
    label: `UE5: ${toolName}`,
    description: `${desc.description}\nMCP 方法: ${desc.method}`,
    promptSnippet: `Perform UE5 editor operation: ${desc.description}`,
    promptGuidelines: [
      `Use ${toolName} when you need to ${desc.description.toLowerCase()}`,
      `Parameters are passed as JSON key-value pairs matching the UE5 MCP API.`,
    ],
    parameters,
    async execute(
      _toolCallId: string,
      params: Record<string, unknown>,
      _signal: AbortSignal | undefined,
      _onUpdate: ((update: ToolResult) => void) | undefined,
    ): Promise<ToolResult> {
      try {
        const result = await connection.call(desc.method, params);
        const summary = renderResultSummary(result);
        return {
          content: [
            { type: "text", text: summary },
            { type: "text", text: `\n\n完整结果:\n\`\`\`json\n${JSON.stringify(result, null, 2)}\n\`\`\`` },
          ],
          details: { method: desc.method, result },
        };
      } catch (e) {
        return {
          content: [{ type: "text", text: `❌ ${(e as Error).message}` }],
          isError: true,
        };
      }
    },
  };
}

// ─── 全部工具定义 ──────────────────────────────────────────────────────────────

/**
 * 所有 MCP 工具的定义
 *
 * 来源：UE_MCP_Bridge 插件 Handler 头文件
 *       Plugins/UE_MCP_Bridge/Source/UE_MCP_Bridge/Private/Handlers/*.h
 */
function getAllToolDescriptors(): ToolDescriptor[] {
  return [
    // ════════════════════════════════════════════════════════════════
    // Editor（编辑器核心操作）
    // ════════════════════════════════════════════════════════════════
    {
      method: "execute_command",
      description: "执行 UE5 控制台命令（如 'stat fps'、'r.SetRes 1920x1080'）",
      parameters: { command: Type.String({ description: "控制台命令字符串" }) },
      required: ["command"],
    },
    {
      method: "execute_python",
      description: "在 UE5 编辑器中执行 Python 脚本",
      parameters: { script: Type.String({ description: "Python 脚本内容" }) },
      required: ["script"],
    },
    {
      method: "set_property",
      description: "设置编辑器中某个对象的属性值",
      parameters: {
        objectPath: Type.String({ description: "对象路径" }),
        propertyName: Type.String({ description: "属性名" }),
        propertyValue: Type.Unknown({ description: "属性值" }),
      },
    },
    {
      method: "set_config",
      description: "设置 UE5 配置文件（.ini）中的值",
      parameters: {
        section: Type.String({ description: "配置节" }),
        key: Type.String({ description: "配置键" }),
        value: Type.String({ description: "配置值" }),
        configFile: Type.Optional(Type.String({ description: "配置文件路径" })),
      },
      required: ["section", "key", "value"],
    },
    {
      method: "read_config",
      description: "读取 UE5 配置文件中的值",
      parameters: {
        section: Type.String({ description: "配置节" }),
        key: Type.String({ description: "配置键" }),
        configFile: Type.Optional(Type.String({ description: "配置文件路径" })),
      },
      required: ["section", "key"],
    },
    {
      method: "get_viewport_info",
      description: "获取编辑器视口信息（分辨率、相机位置等）",
    },
    {
      method: "get_editor_performance_stats",
      description: "获取编辑器性能统计",
    },
    {
      method: "get_output_log",
      description: "获取编辑器输出日志内容",
      parameters: {
        lines: Type.Optional(Type.Number({ description: "获取最近的行数" })),
      },
    },
    {
      method: "search_log",
      description: "搜索编辑器日志",
      parameters: {
        query: Type.String({ description: "搜索关键词" }),
        maxResults: Type.Optional(Type.Number({ description: "最大结果数" })),
      },
      required: ["query"],
    },
    {
      method: "get_message_log",
      description: "获取 UE 消息日志",
    },
    {
      method: "get_build_status",
      description: "获取当前构建状态",
    },
    {
      method: "pie_control",
      description: "控制 Play In Editor（启动/停止/暂停）",
      parameters: {
        action: Type.String({ description: "操作：play/stop/pause" }),
      },
      required: ["action"],
    },
    {
      method: "capture_screenshot",
      description: "截取编辑器视口截图",
      parameters: {
        filename: Type.Optional(Type.String({ description: "保存文件名" })),
      },
    },
    {
      method: "set_viewport_camera",
      description: "设置编辑器视口相机位置",
      parameters: {
        location: Type.Optional(Type.Object({
          x: Type.Number(),
          y: Type.Number(),
          z: Type.Number(),
        })),
        rotation: Type.Optional(Type.Object({
          pitch: Type.Number(),
          yaw: Type.Number(),
          roll: Type.Number(),
        })),
      },
    },
    {
      method: "undo",
      description: "撤销上一步操作",
    },
    {
      method: "redo",
      description: "重做被撤销的操作",
    },
    {
      method: "reload_handlers",
      description: "重新加载所有 MCP 处理器",
    },
    {
      method: "save_asset",
      description: "保存指定资源",
      parameters: {
        assetPath: Type.String({ description: "资源路径，如 /Game/MyFolder/MyAsset" }),
      },
      required: ["assetPath"],
    },
    {
      method: "save_all",
      description: "保存所有未保存的资源",
    },
    {
      method: "get_crash_reports",
      description: "获取崩溃报告列表",
    },
    {
      method: "pie_get_runtime_value",
      description: "在 PIE 运行时获取指定对象属性的值",
      parameters: {
        objectPath: Type.String({ description: "运行时对象路径" }),
        propertyName: Type.String({ description: "属性名" }),
      },
    },
    {
      method: "build_lighting",
      description: "构建场景光照",
    },
    {
      method: "build_all",
      description: "构建所有（光照+导航等）",
    },
    {
      method: "validate_assets",
      description: "验证所有资源是否有效",
    },
    {
      method: "cook_content",
      description: "打包/烹饪游戏内容",
      parameters: {
        target: Type.Optional(Type.String({ description: "目标平台" })),
      },
    },
    {
      method: "focus_viewport_on_actor",
      description: "聚焦视口到指定的 Actor",
      parameters: {
        actorName: Type.String({ description: "Actor 名称或标签" }),
      },
      required: ["actorName"],
    },
    {
      method: "hot_reload",
      description: "热重载 C++ 代码",
    },
    {
      method: "create_new_level",
      description: "创建新关卡",
      parameters: {
        levelPath: Type.String({ description: "关卡保存路径" }),
        template: Type.Optional(Type.String({ description: "关卡模板" })),
      },
      required: ["levelPath"],
    },
    {
      method: "save_current_level",
      description: "保存当前关卡",
    },
    {
      method: "open_asset",
      description: "在编辑器中打开指定资源",
      parameters: {
        assetPath: Type.String({ description: "资源路径" }),
      },
      required: ["assetPath"],
    },
    {
      method: "run_stat_command",
      description: "运行 stat 命令（如 stat fps, stat game）",
      parameters: {
        command: Type.String({ description: "stat 命令" }),
      },
      required: ["command"],
    },
    {
      method: "set_scalability",
      description: "设置可扩展性级别",
      parameters: {
        level: Type.String({ description: "级别：low/medium/high/epic/cine" }),
      },
      required: ["level"],
    },
    {
      method: "build_project",
      description: "构建项目（UBT 编译）",
    },
    {
      method: "generate_project_files",
      description: "重新生成 Visual Studio 项目文件",
    },
    {
      method: "build_geometry",
      description: "构建几何体",
    },
    {
      method: "build_hlod",
      description: "构建 HLOD（分层 LOD）",
    },
    {
      method: "list_crashes",
      description: "列出崩溃报告列表",
    },
    {
      method: "get_crash_info",
      description: "获取崩溃报告的详细信息",
      parameters: {
        crashId: Type.String({ description: "崩溃报告 ID" }),
      },
    },
    {
      method: "check_for_crashes",
      description: "检查是否有新的崩溃报告",
    },

    // ════════════════════════════════════════════════════════════════
    // Asset（资源管理）
    // ════════════════════════════════════════════════════════════════
    {
      method: "list_assets",
      description: "列出指定路径下的资源",
      parameters: {
        path: Type.Optional(Type.String({ description: "资源路径，如 /Game" })),
        filter: Type.Optional(Type.String({ description: "过滤条件" })),
      },
    },
    {
      method: "search_assets",
      description: "搜索资源",
      parameters: {
        query: Type.String({ description: "搜索关键词" }),
        maxResults: Type.Optional(Type.Number({ description: "最大结果数" })),
      },
      required: ["query"],
    },
    {
      method: "read_asset",
      description: "读取资源的详细信息",
      parameters: {
        assetPath: Type.String({ description: "资源路径" }),
      },
      required: ["assetPath"],
    },
    {
      method: "read_asset_properties",
      description: "读取资源的属性列表",
      parameters: {
        assetPath: Type.String({ description: "资源路径" }),
      },
      required: ["assetPath"],
    },
    {
      method: "duplicate_asset",
      description: "复制资源",
      parameters: {
        sourcePath: Type.String({ description: "源资源路径" }),
        destinationPath: Type.String({ description: "目标路径" }),
      },
      required: ["sourcePath", "destinationPath"],
    },
    {
      method: "rename_asset",
      description: "重命名资源",
      parameters: {
        oldPath: Type.String({ description: "原路径" }),
        newPath: Type.String({ description: "新路径" }),
      },
      required: ["oldPath", "newPath"],
    },
    {
      method: "move_asset",
      description: "移动资源到新目录",
      parameters: {
        sourcePath: Type.String({ description: "源路径" }),
        destinationPath: Type.String({ description: "目标路径" }),
      },
      required: ["sourcePath", "destinationPath"],
    },
    {
      method: "delete_asset",
      description: "删除指定资源",
      parameters: {
        assetPath: Type.String({ description: "要删除的资源路径" }),
      },
      required: ["assetPath"],
    },
    {
      method: "import_data_table_json",
      description: "从 JSON 文件导入 DataTable 数据",
      parameters: {
        dataTablePath: Type.String({ description: "DataTable 资源路径" }),
        jsonFilePath: Type.String({ description: "JSON 文件路径" }),
      },
    },
    {
      method: "export_data_table_json",
      description: "将 DataTable 导出为 JSON",
      parameters: {
        dataTablePath: Type.String({ description: "DataTable 资源路径" }),
      },
    },
    {
      method: "create_data_table",
      description: "创建新的 DataTable 资源",
      parameters: {
        path: Type.String({ description: "创建路径" }),
        name: Type.String({ description: "资源名称" }),
        rowStruct: Type.String({ description: "行结构体类名" }),
      },
    },
    {
      method: "read_data_table",
      description: "读取 DataTable 中的所有行",
      parameters: {
        dataTablePath: Type.String({ description: "DataTable 资源路径" }),
      },
    },
    {
      method: "import_static_mesh",
      description: "导入静态网格体（FBX）",
      parameters: {
        filePath: Type.String({ description: "FBX 文件路径" }),
        destinationPath: Type.String({ description: "导入目标路径" }),
      },
    },
    {
      method: "import_skeletal_mesh",
      description: "导入骨骼网格体（FBX）",
      parameters: {
        filePath: Type.String({ description: "FBX 文件路径" }),
        destinationPath: Type.String({ description: "导入目标路径" }),
        skeletonPath: Type.Optional(Type.String({ description: "骨骼资源路径" })),
      },
    },
    {
      method: "import_animation",
      description: "导入动画（FBX）",
      parameters: {
        filePath: Type.String({ description: "FBX 文件路径" }),
        destinationPath: Type.String({ description: "导入目标路径" }),
        skeletonPath: Type.String({ description: "骨骼资源路径" }),
      },
    },
    {
      method: "set_mesh_material",
      description: "设置网格体的材质",
      parameters: {
        meshPath: Type.String({ description: "网格体资源路径" }),
        materialPath: Type.String({ description: "材质资源路径" }),
        slotIndex: Type.Optional(Type.Number({ description: "材质槽索引" })),
      },
    },
    {
      method: "import_texture",
      description: "导入纹理",
      parameters: {
        filePath: Type.String({ description: "纹理文件路径" }),
        destinationPath: Type.String({ description: "导入目标路径" }),
      },
    },
    {
      method: "export_asset",
      description: "导出资源到文件",
      parameters: {
        assetPath: Type.String({ description: "资源路径" }),
        exportPath: Type.String({ description: "导出目标路径" }),
      },
    },
    {
      method: "add_socket",
      description: "为网格体添加插槽",
      parameters: {
        meshPath: Type.String({ description: "网格体路径" }),
        socketName: Type.String({ description: "插槽名" }),
        location: Type.Optional(Type.Object({
          x: Type.Number(), y: Type.Number(), z: Type.Number(),
        })),
      },
    },
    {
      method: "list_sockets",
      description: "列出网格体的插槽",
      parameters: {
        meshPath: Type.String({ description: "网格体路径" }),
      },
    },

    // ════════════════════════════════════════════════════════════════
    // Blueprint（蓝图操作）
    // ════════════════════════════════════════════════════════════════
    {
      method: "create_blueprint",
      description: "创建新的蓝图类",
      parameters: {
        name: Type.String({ description: "蓝图名称" }),
        packagePath: Type.String({ description: "包路径" }),
        parentClass: Type.String({ description: "父类名，如 Actor、Character" }),
      },
    },
    {
      method: "read_blueprint",
      description: "读取蓝图的信息",
      parameters: {
        assetPath: Type.String({ description: "蓝图资源路径" }),
      },
    },
    {
      method: "compile_blueprint",
      description: "编译蓝图",
      parameters: {
        assetPath: Type.String({ description: "蓝图资源路径" }),
      },
    },
    {
      method: "add_variable",
      description: "为蓝图添加变量",
      parameters: {
        assetPath: Type.String({ description: "蓝图路径" }),
        variableName: Type.String({ description: "变量名" }),
        variableType: Type.String({ description: "变量类型，如 int、float、bool、Name" }),
        category: Type.Optional(Type.String({ description: "分类" })),
        defaultValue: Type.Optional(Type.Unknown({ description: "默认值" })),
      },
    },
    {
      method: "add_component",
      description: "为蓝图 Actor 添加组件",
      parameters: {
        assetPath: Type.String({ description: "蓝图路径" }),
        componentName: Type.String({ description: "组件名" }),
        componentClass: Type.String({ description: "组件类，如 StaticMeshComponent" }),
      },
    },
    {
      method: "add_blueprint_interface",
      description: "为蓝图添加接口",
      parameters: {
        assetPath: Type.String({ description: "蓝图路径" }),
        interfacePath: Type.String({ description: "接口路径" }),
      },
    },
    {
      method: "search_node_types",
      description: "搜索蓝图节点类型",
      parameters: {
        query: Type.String({ description: "搜索关键词" }),
      },
    },
    {
      method: "list_node_types",
      description: "列出可用的蓝图节点类型",
    },
    {
      method: "list_blueprint_variables",
      description: "列出蓝图中的所有变量",
      parameters: {
        assetPath: Type.String({ description: "蓝图路径" }),
      },
    },
    {
      method: "create_function",
      description: "在蓝图中创建函数",
      parameters: {
        assetPath: Type.String({ description: "蓝图路径" }),
        functionName: Type.String({ description: "函数名" }),
      },
    },
    {
      method: "add_node",
      description: "在蓝图蓝图中添加节点",
      parameters: {
        assetPath: Type.String({ description: "蓝图路径" }),
        graphName: Type.String({ description: "图表名，如 EventGraph" }),
        nodeType: Type.String({ description: "节点类型" }),
        position: Type.Optional(Type.Object({
          x: Type.Number(), y: Type.Number(),
        })),
      },
    },
    {
      method: "read_blueprint_graph",
      description: "读取蓝图图表内容",
      parameters: {
        assetPath: Type.String({ description: "蓝图路径" }),
        graphName: Type.Optional(Type.String({ description: "图表名" })),
      },
    },
    {
      method: "connect_pins",
      description: "连接两个蓝图的引脚",
      parameters: {
        assetPath: Type.String({ description: "蓝图路径" }),
        fromNodeId: Type.String({ description: "源节点 ID" }),
        fromPin: Type.String({ description: "源引脚名" }),
        toNodeId: Type.String({ description: "目标节点 ID" }),
        toPin: Type.String({ description: "目标引脚名" }),
      },
    },
    {
      method: "delete_node",
      description: "删除蓝图中的节点",
      parameters: {
        assetPath: Type.String({ description: "蓝图路径" }),
        nodeId: Type.String({ description: "节点 ID" }),
      },
    },
    {
      method: "set_node_property",
      description: "设置蓝图节点的属性",
      parameters: {
        assetPath: Type.String({ description: "蓝图路径" }),
        nodeId: Type.String({ description: "节点 ID" }),
        propertyName: Type.String({ description: "属性名" }),
        propertyValue: Type.Unknown({ description: "属性值" }),
      },
    },
    {
      method: "list_graphs",
      description: "列出蓝图中的图表",
      parameters: {
        assetPath: Type.String({ description: "蓝图路径" }),
      },
    },
    {
      method: "set_class_default",
      description: "设置蓝图类默认值",
      parameters: {
        assetPath: Type.String({ description: "蓝图路径" }),
        propertyName: Type.String({ description: "属性名" }),
        propertyValue: Type.Unknown({ description: "属性值" }),
      },
    },
    {
      method: "search_callable_functions",
      description: "搜索可调用的函数",
      parameters: {
        query: Type.String({ description: "搜索关键词" }),
      },
    },
    {
      method: "add_event_dispatcher",
      description: "为蓝图添加事件分发器",
      parameters: {
        assetPath: Type.String({ description: "蓝图路径" }),
        dispatcherName: Type.String({ description: "事件分发器名" }),
      },
    },
    {
      method: "add_function_parameter",
      description: "为蓝图函数添加参数",
      parameters: {
        assetPath: Type.String({ description: "蓝图路径" }),
        functionName: Type.String({ description: "函数名" }),
        parameterName: Type.String({ description: "参数名" }),
        parameterType: Type.Optional(Type.String({ description: "参数类型" })),
      },
    },
    {
      method: "set_variable_properties",
      description: "设置蓝图中变量的属性",
      parameters: {
        assetPath: Type.String({ description: "蓝图路径" }),
        variableName: Type.String({ description: "变量名" }),
        category: Type.Optional(Type.String({ description: "分类" })),
        isExposedToCinematics: Type.Optional(Type.Boolean()),
      },
    },
    {
      method: "set_variable_default",
      description: "设置蓝图中变量的默认值",
      parameters: {
        assetPath: Type.String({ description: "蓝图路径" }),
        variableName: Type.String({ description: "变量名" }),
        defaultValue: Type.Unknown({ description: "默认值" }),
      },
    },
    {
      method: "delete_variable",
      description: "删除蓝图中的变量",
      parameters: {
        assetPath: Type.String({ description: "蓝图路径" }),
        variableName: Type.String({ description: "变量名" }),
      },
    },
    {
      method: "remove_component",
      description: "从蓝图或 Actor 中移除组件",
      parameters: {
        assetPath: Type.String({ description: "蓝图路径" }),
        componentName: Type.String({ description: "组件名" }),
      },
    },
    {
      method: "delete_function",
      description: "删除蓝图中的函数",
      parameters: {
        assetPath: Type.String({ description: "蓝图路径" }),
        functionName: Type.String({ description: "函数名" }),
      },
    },
    {
      method: "rename_function",
      description: "重命名蓝图中的函数",
      parameters: {
        assetPath: Type.String({ description: "蓝图路径" }),
        oldName: Type.String({ description: "原函数名" }),
        newName: Type.String({ description: "新函数名" }),
      },
    },
    {
      method: "create_blueprint_interface",
      description: "创建蓝图接口资源",
      parameters: {
        path: Type.String({ description: "创建路径" }),
        name: Type.String({ description: "接口名" }),
      },
    },

    // ════════════════════════════════════════════════════════════════
    // Level（关卡/场景操作）
    // ════════════════════════════════════════════════════════════════
    {
      method: "get_outliner",
      description: "获取关卡大纲中的所有 Actor 列表",
    },
    {
      method: "place_actor",
      description: "在关卡中放置 Actor",
      parameters: {
        className: Type.String({ description: "Actor 类名，如 StaticMeshActor、PointLight" }),
        location: Type.Optional(Type.Object({
          x: Type.Number({ description: "X 坐标" }),
          y: Type.Number({ description: "Y 坐标" }),
          z: Type.Number({ description: "Z 坐标" }),
        })),
        rotation: Type.Optional(Type.Object({
          pitch: Type.Number(), yaw: Type.Number(), roll: Type.Number(),
        })),
        scale: Type.Optional(Type.Object({
          x: Type.Number(), y: Type.Number(), z: Type.Number(),
        })),
        assetPath: Type.Optional(Type.String({ description: "如果使用 StaticMeshActor，需要指定网格体资源路径" })),
      },
      required: ["className"],
    },
    {
      method: "delete_actor",
      description: "删除关卡中的 Actor",
      parameters: {
        actorName: Type.String({ description: "Actor 名称" }),
      },
      required: ["actorName"],
    },
    {
      method: "get_actor_details",
      description: "获取 Actor 的详细信息",
      parameters: {
        actorName: Type.String({ description: "Actor 名称" }),
      },
      required: ["actorName"],
    },
    {
      method: "get_current_level",
      description: "获取当前关卡信息",
    },
    {
      method: "list_levels",
      description: "列出项目中的所有关卡",
    },
    {
      method: "get_selected_actors",
      description: "获取当前选中的 Actor 列表",
    },
    {
      method: "list_volumes",
      description: "列出关卡中的所有体积 Actor",
    },
    {
      method: "move_actor",
      description: "移动 Actor 到新位置",
      parameters: {
        actorName: Type.String({ description: "Actor 名称" }),
        location: Type.Object({
          x: Type.Number(), y: Type.Number(), z: Type.Number(),
        }),
        rotation: Type.Optional(Type.Object({
          pitch: Type.Number(), yaw: Type.Number(), roll: Type.Number(),
        })),
      },
      required: ["actorName", "location"],
    },
    {
      method: "select_actors",
      description: "选择关卡中的 Actor",
      parameters: {
        actorNames: Type.Array(Type.String(), { description: "Actor 名称数组" }),
      },
    },
    {
      method: "spawn_light",
      description: "在关卡中创建光源",
      parameters: {
        lightType: Type.String({ description: "光源类型：DirectionalLight/PointLight/SpotLight/RectLight" }),
        location: Type.Optional(Type.Object({
          x: Type.Number(), y: Type.Number(), z: Type.Number(),
        })),
        intensity: Type.Optional(Type.Number({ description: "光照强度" })),
        color: Type.Optional(Type.Object({
          r: Type.Number(), g: Type.Number(), b: Type.Number(),
        })),
      },
    },
    {
      method: "set_light_properties",
      description: "设置光源属性",
      parameters: {
        actorName: Type.String({ description: "光源 Actor 名称" }),
        intensity: Type.Optional(Type.Number()),
        color: Type.Optional(Type.Object({
          r: Type.Number(), g: Type.Number(), b: Type.Number(),
        })),
        castShadows: Type.Optional(Type.Boolean()),
        temperature: Type.Optional(Type.Number()),
      },
    },
    {
      method: "spawn_volume",
      description: "创建体积 Actor（如 TriggerVolume、NavModifierVolume）",
      parameters: {
        volumeType: Type.String({ description: "体积类型" }),
        location: Type.Optional(Type.Object({
          x: Type.Number(), y: Type.Number(), z: Type.Number(),
        })),
        extent: Type.Optional(Type.Object({
          x: Type.Number(), y: Type.Number(), z: Type.Number(),
        })),
      },
    },
    {
      method: "add_component_to_actor",
      description: "为关卡中的 Actor 添加组件",
      parameters: {
        actorName: Type.String({ description: "Actor 名称" }),
        componentClass: Type.String({ description: "组件类名" }),
        componentName: Type.Optional(Type.String({ description: "组件名" })),
      },
    },
    {
      method: "load_level",
      description: "加载指定关卡",
      parameters: {
        levelPath: Type.String({ description: "关卡路径" }),
      },
    },
    {
      method: "save_level",
      description: "保存关卡",
      parameters: {
        levelPath: Type.Optional(Type.String({ description: "关卡路径" })),
      },
    },
    {
      method: "list_sublevels",
      description: "列出当前关卡的所有子关卡",
    },
    {
      method: "set_component_property",
      description: "设置组件属性",
      parameters: {
        actorName: Type.String({ description: "Actor 名称" }),
        componentName: Type.String({ description: "组件名称" }),
        propertyName: Type.String({ description: "属性名" }),
        propertyValue: Type.Unknown({ description: "属性值" }),
      },
    },
    {
      method: "get_world_settings",
      description: "获取世界设置",
    },
    {
      method: "set_world_settings",
      description: "设置世界设置",
      parameters: {
        propertyName: Type.String({ description: "属性名" }),
        propertyValue: Type.Unknown({ description: "属性值" }),
      },
    },

    // ════════════════════════════════════════════════════════════════
    // Reflection（反射/类信息查询）
    // ════════════════════════════════════════════════════════════════
    {
      method: "reflect_class",
      description: "反射查询类的详细信息（属性、函数、父类链）",
      parameters: {
        className: Type.String({ description: "类名，如 Character、BearSurviorCharacter" }),
        includeInherited: Type.Optional(Type.Boolean({ description: "是否包含继承成员" })),
      },
      required: ["className"],
    },
    {
      method: "reflect_struct",
      description: "反射查询结构体的详细信息",
      parameters: {
        structName: Type.String({ description: "结构体名，如 Vector、Transform" }),
      },
      required: ["structName"],
    },
    {
      method: "reflect_enum",
      description: "反射查询枚举的详细信息",
      parameters: {
        enumName: Type.String({ description: "枚举名" }),
      },
      required: ["enumName"],
    },
    {
      method: "list_classes",
      description: "列出 UE5 类（可指定父类过滤）",
      parameters: {
        parentFilter: Type.Optional(Type.String({ description: "父类过滤，如 Character、Actor" })),
        limit: Type.Optional(Type.Number({ description: "返回数量限制" })),
      },
    },
    {
      method: "list_gameplay_tags",
      description: "列出所有 Gameplay Tag",
      parameters: {
        filter: Type.Optional(Type.String({ description: "前缀过滤" })),
      },
    },
    {
      method: "create_gameplay_tag",
      description: "创建新的 Gameplay Tag（写入 DefaultGameplayTags.ini，需重启编辑器生效）",
      parameters: {
        tag: Type.String({ description: "Tag 名，如 Status.Buff.Slow" }),
        comment: Type.Optional(Type.String({ description: "注释说明" })),
      },
      required: ["tag"],
    },

    // ════════════════════════════════════════════════════════════════
    // Gameplay（AI 与玩法系统）
    // ════════════════════════════════════════════════════════════════
    {
      method: "create_smart_object_definition",
      description: "创建 Smart Object 定义资源",
      parameters: {
        path: Type.String({ description: "创建路径" }),
        name: Type.String({ description: "资源名" }),
      },
    },
    {
      method: "get_navmesh_info",
      description: "获取导航网格信息",
    },
    {
      method: "get_game_framework_info",
      description: "获取当前游戏框架信息（GameMode、Pawn、Controller 等）",
    },
    {
      method: "list_input_assets",
      description: "列出项目中的输入资源（Input Actions、Input Mapping Contexts）",
    },
    {
      method: "list_behavior_trees",
      description: "列出项目中的行为树资源",
    },
    {
      method: "list_eqs_queries",
      description: "列出项目中的 EQS 查询资源",
    },
    {
      method: "list_state_trees",
      description: "列出项目中的 StateTree 资源",
    },
    {
      method: "project_point_to_navigation",
      description: "将点投影到导航网格上",
      parameters: {
        x: Type.Number({ description: "X 坐标" }),
        y: Type.Number({ description: "Y 坐标" }),
        z: Type.Number({ description: "Z 坐标" }),
      },
    },
    {
      method: "create_input_action",
      description: "创建增强输入动作资源",
      parameters: {
        path: Type.String({ description: "创建路径" }),
        name: Type.String({ description: "动作名" }),
        valueType: Type.Optional(Type.String({ description: "值类型：Digital/Axis2D/Axis3D" })),
      },
    },
    {
      method: "create_input_mapping_context",
      description: "创建增强输入映射上下文资源",
      parameters: {
        path: Type.String({ description: "创建路径" }),
        name: Type.String({ description: "映射上下文名" }),
      },
    },
    {
      method: "create_blackboard",
      description: "创建黑板资源",
      parameters: {
        path: Type.String({ description: "创建路径" }),
        name: Type.String({ description: "黑板名" }),
      },
    },
    {
      method: "create_behavior_tree",
      description: "创建行为树资源",
      parameters: {
        path: Type.String({ description: "创建路径" }),
        name: Type.String({ description: "行为树名" }),
        blackboardPath: Type.Optional(Type.String({ description: "关联的黑板路径" })),
      },
    },
    {
      method: "create_eqs_query",
      description: "创建 EQS 查询资源",
      parameters: {
        path: Type.String({ description: "创建路径" }),
        name: Type.String({ description: "查询名" }),
      },
    },
    {
      method: "create_state_tree",
      description: "创建 StateTree 资源",
      parameters: {
        path: Type.String({ description: "创建路径" }),
        name: Type.String({ description: "StateTree 名" }),
      },
    },
    {
      method: "create_game_mode",
      description: "创建 GameMode 蓝图",
      parameters: {
        path: Type.String({ description: "创建路径" }),
        name: Type.String({ description: "GameMode 名" }),
      },
    },
    {
      method: "create_game_state",
      description: "创建 GameState 蓝图",
      parameters: {
        path: Type.String({ description: "创建路径" }),
        name: Type.String({ description: "GameState 名" }),
      },
    },
    {
      method: "create_player_controller",
      description: "创建 PlayerController 蓝图",
      parameters: {
        path: Type.String({ description: "创建路径" }),
        name: Type.String({ description: "PlayerController 名" }),
      },
    },
    {
      method: "create_player_state",
      description: "创建 PlayerState 蓝图",
      parameters: {
        path: Type.String({ description: "创建路径" }),
        name: Type.String({ description: "PlayerState 名" }),
      },
    },
    {
      method: "create_hud",
      description: "创建 HUD 蓝图",
      parameters: {
        path: Type.String({ description: "创建路径" }),
        name: Type.String({ description: "HUD 名" }),
      },
    },
    {
      method: "set_world_game_mode",
      description: "设置关卡的 GameMode 覆盖",
      parameters: {
        gameModePath: Type.String({ description: "GameMode 蓝图路径" }),
      },
    },
    {
      method: "create_ai_perception_config",
      description: "创建 AI 感知配置",
      parameters: {
        path: Type.String({ description: "创建路径" }),
        name: Type.String({ description: "配置名" }),
      },
    },
    {
      method: "add_blackboard_key",
      description: "为黑板添加键",
      parameters: {
        blackboardPath: Type.String({ description: "黑板资源路径" }),
        keyName: Type.String({ description: "键名" }),
        keyType: Type.String({ description: "键类型：int/float/bool/string/Name/Object/Vector/Rotator" }),
      },
    },
    {
      method: "setup_enhanced_input",
      description: "为 Actor 或 Pawn 设置增强输入绑定",
      parameters: {
        targetPath: Type.String({ description: "目标蓝图路径" }),
        mappingContextPath: Type.String({ description: "输入映射上下文路径" }),
      },
    },
    {
      method: "add_perception_component",
      description: "为蓝图添加 AI 感知组件",
      parameters: {
        targetPath: Type.String({ description: "目标蓝图路径" }),
      },
    },
    {
      method: "add_state_tree_component",
      description: "为蓝图添加 StateTree 组件",
      parameters: {
        targetPath: Type.String({ description: "目标蓝图路径" }),
        stateTreePath: Type.Optional(Type.String({ description: "StateTree 资源路径" })),
      },
    },
    {
      method: "add_smart_object_component",
      description: "为蓝图添加 SmartObject 组件",
      parameters: {
        targetPath: Type.String({ description: "目标蓝图路径" }),
      },
    },
    {
      method: "get_behavior_tree_info",
      description: "获取行为树详细信息",
      parameters: {
        assetPath: Type.String({ description: "行为树路径" }),
      },
    },
    {
      method: "configure_behavior_tree",
      description: "配置行为树参数",
      parameters: {
        assetPath: Type.String({ description: "行为树路径" }),
        blackboardPath: Type.Optional(Type.String()),
      },
    },
    {
      method: "configure_ai_perception_sense",
      description: "配置 AI 感知感官",
      parameters: {
        targetPath: Type.String({ description: "目标蓝图路径" }),
        senseName: Type.String({ description: "感官名：Sight/Hearing/Damage/etc" }),
      },
    },
    {
      method: "setup_path_following",
      description: "设置路径跟随组件",
      parameters: {
        targetPath: Type.String({ description: "目标蓝图路径" }),
        acceptanceRadius: Type.Optional(Type.Number({ description: "接受半径" })),
      },
    },
    {
      method: "run_eqs_query",
      description: "运行 EQS 查询",
      parameters: {
        queryPath: Type.String({ description: "EQS 查询资源路径" }),
        querierLocation: Type.Optional(Type.Object({
          x: Type.Number(), y: Type.Number(), z: Type.Number(),
        })),
      },
    },
    {
      method: "get_cdo_defaults",
      description: "获取类的 CDO 默认值",
      parameters: {
        className: Type.String({ description: "类名" }),
      },
    },
    {
      method: "set_volume_properties",
      description: "设置体积 Actor 的属性",
      parameters: {
        actorName: Type.String({ description: "体积 Actor 名" }),
        extent: Type.Optional(Type.Object({
          x: Type.Number(), y: Type.Number(), z: Type.Number(),
        })),
        color: Type.Optional(Type.Object({
          r: Type.Number(), g: Type.Number(), b: Type.Number(),
        })),
      },
    },
    {
      method: "read_imc",
      description: "读取输入映射上下文（IMC）的配置",
      parameters: {
        assetPath: Type.String({ description: "IMC 资源路径" }),
      },
    },
    {
      method: "add_imc_mapping",
      description: "为输入映射上下文添加动作映射",
      parameters: {
        assetPath: Type.String({ description: "IMC 资源路径" }),
        actionPath: Type.String({ description: "Input Action 资源路径" }),
        key: Type.String({ description: "按键名，如 SpaceBar、W" }),
      },
    },
    {
      method: "inspect_pie",
      description: "在 PIE 运行时检查游戏对象状态",
      parameters: {
        objectPath: Type.String({ description: "运行时对象路径" }),
      },
    },
    {
      method: "get_pie_anim_state",
      description: "获取 PIE 中角色的动画状态",
      parameters: {
        actorName: Type.String({ description: "角色名称" }),
      },
    },

    // ════════════════════════════════════════════════════════════════
    // Material（材质）
    // ════════════════════════════════════════════════════════════════
    {
      method: "create_material",
      description: "创建新的材质资源",
      parameters: {
        path: Type.String({ description: "创建路径" }),
        name: Type.String({ description: "材质名" }),
      },
    },
    {
      method: "read_material",
      description: "读取材质的详细信息",
      parameters: {
        assetPath: Type.String({ description: "材质资源路径" }),
      },
    },
    {
      method: "create_material_instance",
      description: "创建材质实例",
      parameters: {
        path: Type.String({ description: "创建路径" }),
        name: Type.String({ description: "材质实例名" }),
        parentPath: Type.String({ description: "父材质路径" }),
      },
    },
    {
      method: "set_material_shading_model",
      description: "设置材质的着色模型",
      parameters: {
        assetPath: Type.String({ description: "材质路径" }),
        shadingModel: Type.String({ description: "着色模型：DefaultLit/Subsurface/ThinTranslucent/etc" }),
      },
    },
    {
      method: "set_material_blend_mode",
      description: "设置材质的混合模式",
      parameters: {
        assetPath: Type.String({ description: "材质路径" }),
        blendMode: Type.String({ description: "混合模式：Opaque/Masked/Translucent/Additive/etc" }),
      },
    },
    {
      method: "set_material_base_color",
      description: "设置材质的 Base Color",
      parameters: {
        assetPath: Type.String({ description: "材质路径" }),
        color: Type.Object({
          r: Type.Number(), g: Type.Number(), b: Type.Number(),
        }),
      },
    },
    {
      method: "add_material_expression",
      description: "在材质蓝图中添加表达式节点",
      parameters: {
        assetPath: Type.String({ description: "材质路径" }),
        expressionType: Type.String({ description: "表达式类型" }),
        position: Type.Optional(Type.Object({
          x: Type.Number(), y: Type.Number(),
        })),
      },
    },
    {
      method: "list_material_parameters",
      description: "列出材质的参数",
      parameters: {
        assetPath: Type.String({ description: "材质路径" }),
      },
    },
    {
      method: "recompile_material",
      description: "重新编译材质",
      parameters: {
        assetPath: Type.String({ description: "材质路径" }),
      },
    },
    {
      method: "set_material_parameter",
      description: "设置材质实例的参数值",
      parameters: {
        assetPath: Type.String({ description: "材质实例路径" }),
        parameterName: Type.String({ description: "参数名" }),
        parameterValue: Type.Unknown({ description: "参数值" }),
        parameterType: Type.Optional(Type.String({ description: "参数类型：Scalar/Vector/Texture" })),
      },
    },
    {
      method: "connect_expression",
      description: "连接材质表达式节点",
      parameters: {
        assetPath: Type.String({ description: "材质路径" }),
        fromExpression: Type.String({ description: "源表达式名" }),
        fromOutput: Type.String({ description: "源输出引脚" }),
        toExpression: Type.String({ description: "目标表达式名" }),
        toInput: Type.String({ description: "目标输入引脚" }),
      },
    },
    {
      method: "connect_material_property",
      description: "连接表达式到材质属性（如 Base Color、Metallic 等）",
      parameters: {
        assetPath: Type.String({ description: "材质路径" }),
        expressionName: Type.String({ description: "表达式名" }),
        outputIndex: Type.Optional(Type.Number({ description: "输出索引" })),
        propertyName: Type.String({ description: "材质属性名" }),
      },
    },
    {
      method: "list_expression_types",
      description: "列出可用的材质表达式类型",
    },
    {
      method: "list_material_expressions",
      description: "列出材质图中的所有表达式",
      parameters: {
        assetPath: Type.String({ description: "材质路径" }),
      },
    },
    {
      method: "delete_expression",
      description: "删除材质图中的表达式节点",
      parameters: {
        assetPath: Type.String({ description: "材质路径" }),
        expressionName: Type.String({ description: "表达式名" }),
      },
    },
    {
      method: "set_expression_value",
      description: "设置材质表达式节点的值",
      parameters: {
        assetPath: Type.String({ description: "材质路径" }),
        expressionName: Type.String({ description: "表达式名" }),
        propertyName: Type.String({ description: "属性名" }),
        propertyValue: Type.Unknown({ description: "属性值" }),
      },
    },
    {
      method: "create_material_from_texture",
      description: "从纹理创建材质",
      parameters: {
        path: Type.String({ description: "创建路径" }),
        name: Type.String({ description: "材质名" }),
        texturePath: Type.String({ description: "纹理资源路径" }),
      },
    },
    {
      method: "read_material_instance",
      description: "读取材质实例信息",
      parameters: {
        assetPath: Type.String({ description: "材质实例路径" }),
      },
    },

    // ════════════════════════════════════════════════════════════════
    // Animation（动画）
    // ════════════════════════════════════════════════════════════════
    {
      method: "list_anim_assets",
      description: "列出项目中的动画资源",
    },
    {
      method: "list_skeletal_meshes",
      description: "列出项目中的骨骼网格体资源",
    },
    {
      method: "get_skeleton_info",
      description: "获取骨骼信息",
      parameters: {
        skeletonPath: Type.String({ description: "骨骼资源路径" }),
      },
    },
    {
      method: "read_anim_blueprint",
      description: "读取动画蓝图信息",
      parameters: {
        assetPath: Type.String({ description: "动画蓝图路径" }),
      },
    },
    {
      method: "read_anim_montage",
      description: "读取动画蒙太奇信息",
      parameters: {
        assetPath: Type.String({ description: "蒙太奇路径" }),
      },
    },
    {
      method: "read_anim_sequence",
      description: "读取动画序列信息",
      parameters: {
        assetPath: Type.String({ description: "动画序列路径" }),
      },
    },
    {
      method: "create_anim_blueprint",
      description: "创建动画蓝图",
      parameters: {
        path: Type.String({ description: "创建路径" }),
        name: Type.String({ description: "动画蓝图名" }),
        skeletonPath: Type.String({ description: "骨骼资源路径" }),
      },
    },
    {
      method: "create_montage",
      description: "创建动画蒙太奇",
      parameters: {
        path: Type.String({ description: "创建路径" }),
        name: Type.String({ description: "蒙太奇名" }),
        skeletonPath: Type.Optional(Type.String({ description: "骨骼路径" })),
      },
    },
    {
      method: "create_blendspace",
      description: "创建 Blendspace 资源",
      parameters: {
        path: Type.String({ description: "创建路径" }),
        name: Type.String({ description: "Blendspace 名" }),
      },
    },
    {
      method: "read_blendspace",
      description: "读取 Blendspace 信息",
      parameters: {
        assetPath: Type.String({ description: "Blendspace 路径" }),
      },
    },
    {
      method: "set_montage_sequence",
      description: "设置蒙太奇中某个片段的动画序列",
      parameters: {
        assetPath: Type.String({ description: "蒙太奇路径" }),
        sectionName: Type.String({ description: "片段名" }),
        sequencePath: Type.String({ description: "动画序列路径" }),
      },
    },
    {
      method: "set_montage_properties",
      description: "设置蒙太奇属性（混合时间等）",
      parameters: {
        assetPath: Type.String({ description: "蒙太奇路径" }),
        blendInTime: Type.Optional(Type.Number({ description: "淡入时间" })),
        blendOutTime: Type.Optional(Type.Number({ description: "淡出时间" })),
      },
    },
    {
      method: "set_montage_slot",
      description: "设置蒙太奇的插槽",
      parameters: {
        assetPath: Type.String({ description: "蒙太奇路径" }),
        slotName: Type.String({ description: "插槽名" }),
      },
    },
    {
      method: "add_montage_section",
      description: "为蒙太奇添加片段",
      parameters: {
        assetPath: Type.String({ description: "蒙太奇路径" }),
        sectionName: Type.String({ description: "片段名" }),
      },
    },
    {
      method: "add_curve",
      description: "在动画序列中添加曲线",
      parameters: {
        assetPath: Type.String({ description: "动画序列路径" }),
        curveName: Type.String({ description: "曲线名" }),
      },
    },
    {
      method: "set_bone_keyframes",
      description: "设置骨骼关键帧",
      parameters: {
        assetPath: Type.String({ description: "动画序列路径" }),
        boneName: Type.String({ description: "骨骼名" }),
        keyframes: Type.Array(Type.Object({
          time: Type.Number({ description: "时间（秒）" }),
          rotation: Type.Optional(Type.Object({ x: Type.Number(), y: Type.Number(), z: Type.Number(), w: Type.Number() })),
        })),
      },
    },
    {
      method: "get_bone_transforms",
      description: "获取骨骼变换",
      parameters: {
        assetPath: Type.String({ description: "动画序列路径" }),
        time: Type.Number({ description: "时间（秒）" }),
        boneName: Type.Optional(Type.String({ description: "骨骼名" })),
      },
    },
    {
      method: "create_sequence",
      description: "创建动画序列",
      parameters: {
        path: Type.String({ description: "创建路径" }),
        name: Type.String({ description: "动画序列名" }),
        skeletonPath: Type.String({ description: "骨骼路径" }),
      },
    },
    {
      method: "add_anim_notify",
      description: "在动画通知轨道添加通知",
      parameters: {
        assetPath: Type.String({ description: "蒙太奇或序列路径" }),
        time: Type.Number({ description: "通知时间（秒）" }),
        notifyName: Type.String({ description: "通知名" }),
      },
    },
    {
      method: "create_state_machine",
      description: "在动画蓝图中创建状态机",
      parameters: {
        assetPath: Type.String({ description: "动画蓝图路径" }),
        name: Type.String({ description: "状态机名" }),
      },
    },
    {
      method: "add_state",
      description: "在状态机中添加状态",
      parameters: {
        assetPath: Type.String({ description: "动画蓝图路径" }),
        stateMachineName: Type.String({ description: "状态机名" }),
        stateName: Type.String({ description: "状态名" }),
      },
    },
    {
      method: "add_transition",
      description: "在状态机中添加状态转换规则",
      parameters: {
        assetPath: Type.String({ description: "动画蓝图路径" }),
        stateMachineName: Type.String({ description: "状态机名" }),
        fromState: Type.String({ description: "源状态" }),
        toState: Type.String({ description: "目标状态" }),
      },
    },
    {
      method: "set_state_animation",
      description: "设置状态的动画序列",
      parameters: {
        assetPath: Type.String({ description: "动画蓝图路径" }),
        stateMachineName: Type.String({ description: "状态机名" }),
        stateName: Type.String({ description: "状态名" }),
        animationPath: Type.String({ description: "动画序列路径" }),
      },
    },
    {
      method: "set_transition_blend",
      description: "设置状态转换的混合参数",
      parameters: {
        assetPath: Type.String({ description: "动画蓝图路径" }),
        stateMachineName: Type.String({ description: "状态机名" }),
        transitionIndex: Type.Number({ description: "转换索引" }),
        blendTime: Type.Number({ description: "混合时间" }),
      },
    },
    {
      method: "read_state_machine",
      description: "读取状态机结构",
      parameters: {
        assetPath: Type.String({ description: "动画蓝图路径" }),
        stateMachineName: Type.String({ description: "状态机名" }),
      },
    },
    {
      method: "read_anim_graph",
      description: "读取动画蓝图图表",
      parameters: {
        assetPath: Type.String({ description: "动画蓝图路径" }),
      },
    },
    {
      method: "create_ik_rig",
      description: "创建 IK Rig 资源",
      parameters: {
        path: Type.String({ description: "创建路径" }),
        name: Type.String({ description: "IK Rig 名" }),
        skeletonPath: Type.String({ description: "骨骼路径" }),
      },
    },
    {
      method: "read_ik_rig",
      description: "读取 IK Rig 信息",
      parameters: {
        assetPath: Type.String({ description: "IK Rig 路径" }),
      },
    },
    {
      method: "list_control_rig_variables",
      description: "列出 Control Rig 变量",
      parameters: {
        assetPath: Type.String({ description: "Control Rig 路径" }),
      },
    },

    // ════════════════════════════════════════════════════════════════
    // Widget（UI 编辑器）
    // ════════════════════════════════════════════════════════════════
    {
      method: "list_widget_blueprints",
      description: "列出项目中的 Widget Blueprint",
    },
    {
      method: "create_widget_blueprint",
      description: "创建 Widget Blueprint",
      parameters: {
        path: Type.String({ description: "创建路径" }),
        name: Type.String({ description: "Widget 名" }),
      },
    },
    {
      method: "read_widget_tree",
      description: "读取 Widget 的控件树",
      parameters: {
        assetPath: Type.String({ description: "Widget 蓝图路径" }),
      },
    },
    {
      method: "add_widget",
      description: "在 Widget 蓝图中添加子控件",
      parameters: {
        assetPath: Type.String({ description: "Widget 路径" }),
        parentName: Type.Optional(Type.String({ description: "父控件名" })),
        widgetClass: Type.String({ description: "控件类名，如 Button、TextBlock、Image" }),
        widgetName: Type.String({ description: "新控件名" }),
      },
    },
    {
      method: "remove_widget",
      description: "从 Widget 蓝图中移除控件",
      parameters: {
        assetPath: Type.String({ description: "Widget 路径" }),
        widgetName: Type.String({ description: "要移除的控件名" }),
      },
    },
    {
      method: "set_widget_property",
      description: "设置 Widget 控件属性",
      parameters: {
        assetPath: Type.String({ description: "Widget 路径" }),
        widgetName: Type.String({ description: "控件名" }),
        propertyName: Type.String({ description: "属性名" }),
        propertyValue: Type.Unknown({ description: "属性值" }),
      },
    },
    {
      method: "read_widget_animations",
      description: "读取 Widget 的动画信息",
      parameters: {
        assetPath: Type.String({ description: "Widget 路径" }),
      },
    },
    {
      method: "create_editor_utility_widget",
      description: "创建 Editor Utility Widget",
      parameters: {
        path: Type.String({ description: "创建路径" }),
        name: Type.String({ description: "EUW 名" }),
      },
    },
    {
      method: "run_editor_utility_widget",
      description: "运行 Editor Utility Widget",
      parameters: {
        assetPath: Type.String({ description: "EUW 路径" }),
      },
    },
    {
      method: "list_widget_classes",
      description: "列出可用的控件类",
    },

    // ════════════════════════════════════════════════════════════════
    // PCG（程序化内容生成）
    // ════════════════════════════════════════════════════════════════
    {
      method: "list_pcg_graphs",
      description: "列出项目中的 PCG 图表资源",
    },
    {
      method: "get_pcg_components",
      description: "获取关卡中所有 PCG 体积和组件",
    },
    {
      method: "create_pcg_graph",
      description: "创建 PCG 图表资源",
      parameters: {
        path: Type.String({ description: "创建路径" }),
        name: Type.String({ description: "PCG 图表名" }),
      },
    },
    {
      method: "read_pcg_graph",
      description: "读取 PCG 图表内容",
      parameters: {
        assetPath: Type.String({ description: "PCG 图表路径" }),
      },
    },
    {
      method: "add_pcg_node",
      description: "在 PCG 图表中添加节点",
      parameters: {
        assetPath: Type.String({ description: "PCG 图表路径" }),
        nodeType: Type.String({ description: "节点类型" }),
        position: Type.Optional(Type.Object({
          x: Type.Number(), y: Type.Number(),
        })),
      },
    },
    {
      method: "connect_pcg_nodes",
      description: "连接 PCG 图表中的节点",
      parameters: {
        assetPath: Type.String({ description: "PCG 图表路径" }),
        fromNode: Type.String({ description: "源节点" }),
        fromPin: Type.String({ description: "源引脚" }),
        toNode: Type.String({ description: "目标节点" }),
        toPin: Type.String({ description: "目标引脚" }),
      },
    },
    {
      method: "remove_pcg_node",
      description: "删除 PCG 图表中的节点",
      parameters: {
        assetPath: Type.String({ description: "PCG 图表路径" }),
        nodeName: Type.String({ description: "节点名" }),
      },
    },
    {
      method: "set_pcg_node_settings",
      description: "设置 PCG 节点的参数",
      parameters: {
        assetPath: Type.String({ description: "PCG 图表路径" }),
        nodeName: Type.String({ description: "节点名" }),
        settings: Type.Record(Type.String(), Type.Unknown(), { description: "设置键值对" }),
      },
    },
    {
      method: "execute_pcg_graph",
      description: "执行 PCG 图表生成",
      parameters: {
        pcgComponentPath: Type.Optional(Type.String({ description: "PCG 组件路径" })),
        assetPath: Type.Optional(Type.String({ description: "PCG 图表路径" })),
      },
    },
    {
      method: "spawn_pcg_volume",
      description: "在关卡中创建 PCG 体积",
      parameters: {
        location: Type.Optional(Type.Object({
          x: Type.Number(), y: Type.Number(), z: Type.Number(),
        })),
        extent: Type.Optional(Type.Object({
          x: Type.Number(), y: Type.Number(), z: Type.Number(),
        })),
      },
    },

    // ════════════════════════════════════════════════════════════════
    // Sequencer（过场动画）
    // ════════════════════════════════════════════════════════════════
    {
      method: "create_level_sequence",
      description: "创建关卡序列资源",
      parameters: {
        path: Type.String({ description: "创建路径" }),
        name: Type.String({ description: "序列名" }),
      },
    },
    {
      method: "read_sequence_info",
      description: "读取关卡序列信息",
      parameters: {
        assetPath: Type.String({ description: "关卡序列路径" }),
      },
    },
    {
      method: "add_track",
      description: "为关卡序列添加轨道",
      parameters: {
        assetPath: Type.String({ description: "关卡序列路径" }),
        trackType: Type.String({ description: "轨道类型" }),
        bindingName: Type.Optional(Type.String({ description: "绑定的 Actor 名" })),
      },
    },
    {
      method: "sequence_control",
      description: "控制序列播放（播放/暂停/跳转）",
      parameters: {
        action: Type.String({ description: "操作：play/pause/stop/scrub" }),
        time: Type.Optional(Type.Number({ description: "跳转时间（秒）" })),
        assetPath: Type.Optional(Type.String({ description: "序列路径" })),
      },
    },

    // ════════════════════════════════════════════════════════════════
    // Audio / Foliage / Landscape / Niagara / Spline
    // ════════════════════════════════════════════════════════════════
    {
      method: "create_spline",
      description: "创建样条线 Actor",
      parameters: {
        location: Type.Optional(Type.Object({
          x: Type.Number(), y: Type.Number(), z: Type.Number(),
        })),
        points: Type.Optional(Type.Array(Type.Object({
          x: Type.Number(), y: Type.Number(), z: Type.Number(),
        }), { description: "样条点数组" })),
      },
    },
    {
      method: "rebuild_navmesh",
      description: "重建导航网格",
    },
    {
      method: "set_collision_profile",
      description: "设置碰撞预设",
      parameters: {
        targetPath: Type.String({ description: "目标路径" }),
        profileName: Type.String({ description: "碰撞预设名" }),
      },
    },
    {
      method: "set_physics_enabled",
      description: "启用/禁用物理模拟",
      parameters: {
        targetPath: Type.String({ description: "目标路径" }),
        enabled: Type.Boolean({ description: "是否启用" }),
      },
    },

    // ════════════════════════════════════════════════════════════════
    // GAS（Gameplay Ability System）
    // ════════════════════════════════════════════════════════════════
    {
      method: "create_attribute_set",
      description: "创建 Gameplay AttributeSet",
      parameters: {
        path: Type.String({ description: "创建路径" }),
        name: Type.String({ description: "AttributeSet 名" }),
      },
    },
    {
      method: "create_gameplay_ability",
      description: "创建 Gameplay Ability",
      parameters: {
        path: Type.String({ description: "创建路径" }),
        name: Type.String({ description: "Ability 名" }),
      },
    },
    {
      method: "create_gameplay_effect",
      description: "创建 Gameplay Effect",
      parameters: {
        path: Type.String({ description: "创建路径" }),
        name: Type.String({ description: "Effect 名" }),
      },
    },
    {
      method: "create_gameplay_cue",
      description: "创建 Gameplay Cue",
      parameters: {
        path: Type.String({ description: "创建路径" }),
        name: Type.String({ description: "Cue 名" }),
      },
    },
    {
      method: "create_gameplay_cue_notify",
      description: "创建 Gameplay Cue Notify",
      parameters: {
        path: Type.String({ description: "创建路径" }),
        name: Type.String({ description: "Cue Notify 名" }),
      },
    },
    {
      method: "get_gas_info",
      description: "获取 GAS 系统信息",
    },
    {
      method: "add_ability_system_component",
      description: "为蓝图添加 AbilitySystemComponent",
      parameters: {
        targetPath: Type.String({ description: "目标蓝图路径" }),
      },
    },
    {
      method: "add_attribute",
      description: "为 AttributeSet 添加属性",
      parameters: {
        targetPath: Type.String({ description: "目标蓝图路径" }),
        attributeName: Type.String({ description: "属性名" }),
      },
    },
    {
      method: "add_ability_tag",
      description: "为 Gameplay Ability 添加标签",
      parameters: {
        abilityPath: Type.String({ description: "Ability 蓝图路径" }),
        tag: Type.String({ description: "Gameplay Tag" }),
      },
    },
    {
      method: "set_ability_tags",
      description: "设置 Gameplay Ability 的标签",
      parameters: {
        abilityPath: Type.String({ description: "Ability 蓝图路径" }),
        tags: Type.Array(Type.String(), { description: "Gameplay Tag 列表" }),
      },
    },
    {
      method: "set_effect_modifier",
      description: "设置 Gameplay Effect 的修饰器",
      parameters: {
        effectPath: Type.String({ description: "Effect 蓝图路径" }),
        attributeName: Type.String({ description: "属性名" }),
        modifierOp: Type.Optional(Type.String({ description: "运算：Add/Multiply/Divide/Override" })),
        magnitude: Type.Optional(Type.Number({ description: "数值" })),
      },
    },

    // ════════════════════════════════════════════════════════════════
    // Foliage（植被系统）
    // ════════════════════════════════════════════════════════════════
    {
      method: "list_foliage_types",
      description: "列出项目中的植被类型",
    },
    {
      method: "get_foliage_settings",
      description: "获取植被类型设置",
      parameters: {
        foliageTypePath: Type.String({ description: "植被类型路径" }),
      },
    },
    {
      method: "paint_foliage",
      description: "在关卡中绘制植被实例",
      parameters: {
        foliageTypePath: Type.String({ description: "植被类型路径" }),
        location: Type.Object({
          x: Type.Number(), y: Type.Number(), z: Type.Number(),
        }, { description: "绘制位置" }),
        radius: Type.Optional(Type.Number({ description: "绘制半径" })),
        density: Type.Optional(Type.Number({ description: "密度" })),
      },
    },
    {
      method: "erase_foliage",
      description: "擦除关卡中的植被实例",
      parameters: {
        foliageTypePath: Type.Optional(Type.String({ description: "植被类型路径" })),
        location: Type.Optional(Type.Object({
          x: Type.Number(), y: Type.Number(), z: Type.Number(),
        })),
        radius: Type.Optional(Type.Number({ description: "擦除半径" })),
      },
    },
    {
      method: "create_foliage_type",
      description: "创建新的植被类型",
      parameters: {
        path: Type.String({ description: "创建路径" }),
        name: Type.String({ description: "植被类型名" }),
        meshPath: Type.String({ description: "静态网格体路径" }),
      },
    },
    {
      method: "set_foliage_type_settings",
      description: "设置植被类型参数",
      parameters: {
        foliageTypePath: Type.String({ description: "植被类型路径" }),
        settings: Type.Record(Type.String(), Type.Unknown(), { description: "设置键值对" }),
      },
    },
    {
      method: "create_foliage_layer",
      description: "创建植被层",
      parameters: {
        name: Type.String({ description: "层名" }),
      },
    },
    {
      method: "sample_foliage",
      description: "采样指定位置附近的植被实例",
      parameters: {
        location: Type.Object({
          x: Type.Number(), y: Type.Number(), z: Type.Number(),
        }),
        radius: Type.Optional(Type.Number({ description: "采样半径" })),
      },
    },

    // ════════════════════════════════════════════════════════════════
    // Landscape（地形系统）
    // ════════════════════════════════════════════════════════════════
    {
      method: "get_landscape_info",
      description: "获取地形信息",
    },
    {
      method: "list_landscape_layers",
      description: "列出地形的所有层",
    },
    {
      method: "sample_landscape",
      description: "采样地形在某位置的高度",
      parameters: {
        location: Type.Object({
          x: Type.Number(), y: Type.Number(), z: Type.Number(),
        }),
      },
    },
    {
      method: "list_landscape_splines",
      description: "列出地形的样条线",
    },
    {
      method: "get_landscape_bounds",
      description: "获取地形边界",
    },
    {
      method: "sculpt_landscape",
      description: "雕刻地形高度",
      parameters: {
        location: Type.Object({
          x: Type.Number(), y: Type.Number(), z: Type.Number(),
        }),
        radius: Type.Number({ description: "雕刻半径" }),
        height: Type.Number({ description: "高度变化" }),
        tool: Type.Optional(Type.String({ description: "工具类型：raise/lower/smooth/flatten" })),
      },
    },
    {
      method: "paint_landscape_layer",
      description: "在地形上绘制层（材质混合）",
      parameters: {
        layerName: Type.String({ description: "层名" }),
        location: Type.Object({
          x: Type.Number(), y: Type.Number(), z: Type.Number(),
        }),
        radius: Type.Optional(Type.Number({ description: "绘制半径" })),
        strength: Type.Optional(Type.Number({ description: "绘制强度" })),
      },
    },
    {
      method: "import_heightmap",
      description: "导入高度图创建地形",
      parameters: {
        heightmapPath: Type.String({ description: "高度图文件路径" }),
        location: Type.Optional(Type.Object({
          x: Type.Number(), y: Type.Number(), z: Type.Number(),
        })),
        size: Type.Optional(Type.Object({
          x: Type.Number({ description: "X 尺寸" }),
          y: Type.Number({ description: "Y 尺寸" }),
        })),
      },
    },
    {
      method: "set_landscape_material",
      description: "设置地形的材质",
      parameters: {
        materialPath: Type.String({ description: "材质路径" }),
      },
    },
    {
      method: "add_landscape_layer_info",
      description: "添加地形层信息",
      parameters: {
        layerName: Type.String({ description: "层名" }),
        layerInfoPath: Type.Optional(Type.String({ description: "层信息资源路径" })),
      },
    },
    {
      method: "get_landscape_component",
      description: "获取地形的某个组件信息",
      parameters: {
        sectionX: Type.Number({ description: "组件 X 索引" }),
        sectionY: Type.Number({ description: "组件 Y 索引" }),
      },
    },

    // ════════════════════════════════════════════════════════════════
    // Niagara（粒子/视觉特效）
    // ════════════════════════════════════════════════════════════════
    {
      method: "list_niagara_systems",
      description: "列出项目中的 Niagara 系统资源",
    },
    {
      method: "list_niagara_modules",
      description: "列出可用的 Niagara 模块",
    },
    {
      method: "create_niagara_system",
      description: "创建 Niagara 系统资源",
      parameters: {
        path: Type.String({ description: "创建路径" }),
        name: Type.String({ description: "Niagara 系统名" }),
      },
    },
    {
      method: "get_niagara_info",
      description: "获取 Niagara 系统信息",
      parameters: {
        assetPath: Type.String({ description: "Niagara 系统路径" }),
      },
    },
    {
      method: "list_emitters_in_system",
      description: "列出 Niagara 系统中的所有发射器",
      parameters: {
        assetPath: Type.String({ description: "Niagara 系统路径" }),
      },
    },
    {
      method: "create_niagara_emitter",
      description: "创建 Niagara 发射器资源",
      parameters: {
        path: Type.String({ description: "创建路径" }),
        name: Type.String({ description: "发射器名" }),
      },
    },
    {
      method: "spawn_niagara_at_location",
      description: "在关卡中生成 Niagara 粒子特效",
      parameters: {
        systemPath: Type.String({ description: "Niagara 系统路径" }),
        location: Type.Object({
          x: Type.Number(), y: Type.Number(), z: Type.Number(),
        }),
        autoDestroy: Type.Optional(Type.Boolean({ description: "播放完后自动销毁" })),
      },
    },
    {
      method: "set_niagara_parameter",
      description: "设置 Niagara 系统的参数",
      parameters: {
        assetPath: Type.String({ description: "Niagara 系统路径" }),
        parameterName: Type.String({ description: "参数名" }),
        parameterValue: Type.Unknown({ description: "参数值" }),
        parameterType: Type.Optional(Type.String({ description: "参数类型：float/vec2/vec3/color" })),
      },
    },
    {
      method: "add_emitter_to_system",
      description: "向 Niagara 系统添加发射器",
      parameters: {
        assetPath: Type.String({ description: "Niagara 系统路径" }),
        emitterPath: Type.String({ description: "发射器资源路径" }),
      },
    },
    {
      method: "set_emitter_property",
      description: "设置 Niagara 发射器的属性",
      parameters: {
        assetPath: Type.String({ description: "Niagara 系统路径" }),
        emitterName: Type.String({ description: "发射器名" }),
        propertyName: Type.String({ description: "属性名" }),
        propertyValue: Type.Unknown({ description: "属性值" }),
      },
    },
    {
      method: "get_emitter_info",
      description: "获取发射器的详细信息",
      parameters: {
        assetPath: Type.String({ description: "Niagara 系统路径" }),
        emitterName: Type.String({ description: "发射器名" }),
      },
    },

    // ════════════════════════════════════════════════════════════════
    // Audio（音频）
    // ════════════════════════════════════════════════════════════════
    {
      method: "list_sound_assets",
      description: "列出项目中的音频资源",
    },
    {
      method: "create_sound_cue",
      description: "创建 Sound Cue 资源",
      parameters: {
        path: Type.String({ description: "创建路径" }),
        name: Type.String({ description: "Sound Cue 名" }),
      },
    },
    {
      method: "create_meta_sound_source",
      description: "创建 MetaSound Source 资源",
      parameters: {
        path: Type.String({ description: "创建路径" }),
        name: Type.String({ description: "MetaSound 名" }),
      },
    },
    {
      method: "play_sound_at_location",
      description: "在关卡中的指定位置播放音效",
      parameters: {
        soundPath: Type.String({ description: "音效资源路径" }),
        location: Type.Object({
          x: Type.Number(), y: Type.Number(), z: Type.Number(),
        }),
        volumeMultiplier: Type.Optional(Type.Number({ description: "音量倍数" })),
      },
    },
    {
      method: "spawn_ambient_sound",
      description: "在关卡中创建环境音效 Actor",
      parameters: {
        soundPath: Type.String({ description: "音效资源路径" }),
        location: Type.Object({
          x: Type.Number(), y: Type.Number(), z: Type.Number(),
        }),
        autoPlay: Type.Optional(Type.Boolean({ description: "自动播放" })),
      },
    },

    // ════════════════════════════════════════════════════════════════
    // Spline（样条线）
    // ════════════════════════════════════════════════════════════════
    {
      method: "read_spline",
      description: "读取样条线 Actor 的信息",
      parameters: {
        actorName: Type.String({ description: "样条线 Actor 名" }),
      },
    },
    {
      method: "set_spline_points",
      description: "设置样条线的控制点",
      parameters: {
        actorName: Type.String({ description: "样条线 Actor 名" }),
        points: Type.Array(Type.Object({
          x: Type.Number(), y: Type.Number(), z: Type.Number(),
        }), { description: "控制点数组" }),
      },
    },

    // ════════════════════════════════════════════════════════════════
    // Networking（网络/多人）
    // ════════════════════════════════════════════════════════════════
    {
      method: "get_networking_info",
      description: "获取网络信息",
    },
    {
      method: "set_replicates",
      description: "设置 Actor 是否复制到客户端",
      parameters: {
        targetPath: Type.String({ description: "目标蓝图路径" }),
        replicates: Type.Boolean({ description: "是否复制" }),
      },
    },
    {
      method: "set_replicate_movement",
      description: "设置是否复制移动",
      parameters: {
        targetPath: Type.String({ description: "目标蓝图路径" }),
        replicateMovement: Type.Boolean({ description: "是否复制移动" }),
      },
    },
    {
      method: "configure_net_update_frequency",
      description: "配置网络更新频率",
      parameters: {
        targetPath: Type.String({ description: "目标蓝图路径" }),
        frequency: Type.Number({ description: "更新频率（Hz）" }),
      },
    },
    {
      method: "set_net_dormancy",
      description: "设置网络休眠模式",
      parameters: {
        targetPath: Type.String({ description: "目标蓝图路径" }),
        dormancy: Type.String({ description: "休眠模式：Never/Awake/DormantAll" }),
      },
    },
    {
      method: "set_always_relevant",
      description: "设置 Actor 是否始终相关",
      parameters: {
        targetPath: Type.String({ description: "目标蓝图路径" }),
        alwaysRelevant: Type.Boolean({ description: "是否始终相关" }),
      },
    },
    {
      method: "set_net_priority",
      description: "设置网络优先级",
      parameters: {
        targetPath: Type.String({ description: "目标蓝图路径" }),
        priority: Type.Number({ description: "优先级" }),
      },
    },
    {
      method: "set_variable_replication",
      description: "设置变量复制条件",
      parameters: {
        targetPath: Type.String({ description: "目标蓝图路径" }),
        variableName: Type.String({ description: "变量名" }),
        replicationCondition: Type.String({ description: "复制条件：None/Always/OwnerOnly/etc" }),
      },
    },
    {
      method: "get_replication_info",
      description: "获取复制信息",
      parameters: {
        targetPath: Type.String({ description: "目标蓝图路径" }),
      },
    },
    {
      method: "set_owner_only_relevant",
      description: "设置是否仅所有者相关",
      parameters: {
        targetPath: Type.String({ description: "目标蓝图路径" }),
        ownerOnly: Type.Boolean({ description: "是否仅所有者" }),
      },
    },
    {
      method: "set_net_load_on_client",
      description: "设置客户端是否加载此 Actor",
      parameters: {
        targetPath: Type.String({ description: "目标蓝图路径" }),
        loadOnClient: Type.Boolean({ description: "客户端加载" }),
      },
    },
    {
      method: "configure_net_cull_distance",
      description: "配置网络裁剪距离",
      parameters: {
        targetPath: Type.String({ description: "目标蓝图路径" }),
        cullDistance: Type.Number({ description: "裁剪距离" }),
      },
    },

    // ════════════════════════════════════════════════════════════════
    // Demo（调试与演示工具）
    // ════════════════════════════════════════════════════════════════
    {
      method: "demo_list_all_tools",
      description: "列出所有可用的 MCP 工具和方法名",
    },
    {
      method: "demo_step",
      description: "执行 Neon Shrine 演示场景的构建步骤（按索引）",
      parameters: {
        step: Type.Optional(Type.Number({ description: "步骤索引（不传则返回步骤列表）" })),
      },
    },
    {
      method: "demo_get_steps",
      description: "获取 Neon Shrine 演示场景的所有构建步骤列表",
    },
    {
      method: "demo_cleanup",
      description: "清除所有演示场景的 Actor 和资源",
    },
  ];
}

// ─── 扩展入口 ─────────────────────────────────────────────────────────────────

export default function (pi: ExtensionAPI) {
  const connection = new UEMCPConnection();

  // 连接状态指示器
  let connected = false;
  connection.onConnectionChange((status) => {
    connected = status;
    if (status) {
      pi.sendMessage({
        customType: "ue-mcp-status",
        content: "✅ UE5 MCP Bridge 已连接",
        display: true,
      });
    }
  });

  // Session 启动时连接
  pi.on("session_start", async () => {
    try {
      await connection.connect();
    } catch (e) {
      console.warn(`[UE-MCP] ${(e as Error).message}`);
      // 开始时显示提示但不阻塞
      pi.sendMessage({
        customType: "ue-mcp-status",
        content: `⚠️ UE5 MCP Bridge 未连接 — ${(e as Error).message}`,
        display: true,
      });
    }
  });

  // Session 关闭时断开
  pi.on("session_shutdown", () => {
    connection.disconnect();
  });

  // ─── 注册全部工具 ─────────────────────────────────────────────────
  const descriptors = getAllToolDescriptors();

  for (const desc of descriptors) {
    pi.registerTool(createPiTool(connection, desc));
  }

  // 额外注册一个调试工具：ue_mcp_raw_call
  pi.registerTool({
    name: "ue_mcp_raw_call",
    label: "UE5: MCP Raw Call",
    description: "直接调用任意 UE5 MCP 方法。用于工具列表中未包含的 MCP 方法。方法名参考 demo_list_all_tools",
    promptSnippet: "Make arbitrary UE5 editor MCP calls",
    promptGuidelines: ["Use ue_mcp_raw_call when there is no specific tool for the MCP method you need."],
    parameters: Type.Object({
      method: Type.String({ description: "MCP 方法名" }),
      params: Type.Optional(Type.Record(Type.String(), Type.Unknown(), { description: "方法参数" })),
    }),
    async execute(
      _toolCallId: string,
      params: { method: string; params?: Record<string, unknown> },
    ): Promise<ToolResult> {
      try {
        const result = await connection.call(params.method, params.params ?? {});
        return {
          content: [
            { type: "text", text: `✅ ${params.method}` },
            { type: "text", text: `\n\`\`\`json\n${JSON.stringify(result, null, 2)}\n\`\`\`` },
          ],
          details: { method: params.method, result },
        };
      } catch (e) {
        return {
          content: [{ type: "text", text: `❌ ${(e as Error).message}` }],
          isError: true,
        };
      }
    },
  });

  // 注册连接状态命令
  pi.registerCommand("ue-status", {
    description: "显示 UE5 MCP Bridge 连接状态",
    handler: async (_args, ctx) => {
      if (connection.isConnected()) {
        ctx.ui.notify("✅ UE5 MCP Bridge 已连接 (ws://localhost:9877)", "info");
      } else {
        ctx.ui.notify("❌ UE5 MCP Bridge 未连接", "warning");
      }
    },
  });

  // 注册重连命令
  pi.registerCommand("ue-reconnect", {
    description: "重新连接到 UE5 MCP Bridge",
    handler: async (_args, ctx) => {
      ctx.ui.notify("正在重新连接...", "info");
      try {
        connection.disconnect();
        await connection.connect();
        ctx.ui.notify("✅ 已重新连接到 UE5 MCP Bridge", "info");
      } catch (e) {
        ctx.ui.notify(`❌ 连接失败: ${(e as Error).message}`, "error");
      }
    },
  });

  // 注册工具计数命令
  pi.registerCommand("ue-tools", {
    description: "显示已注册的 UE5 MCP 工具数量",
    handler: async (_args, ctx) => {
      const allTools = pi.getAllTools();
      const ueTools = allTools.filter((t) =>
        t.name.startsWith("ue_") ||
        t.sourceInfo.path?.includes("ue-mcp-bridge"),
      );
      ctx.ui.notify(`已注册 ${ueTools.length} 个 UE5 MCP 工具`, "info");
    },
  });

  console.log(`[UE-MCP] 已注册 ${descriptors.length + 1} 个 MCP 工具`);
}
