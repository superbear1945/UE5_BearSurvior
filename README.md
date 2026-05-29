# 通过UE5制作的学习UE C++用的小游戏

## Git LFS 拉取说明

因为github的lfs仓库较小，选择使用腾讯工蜂的500GB超大lfs仓库来作为lfs仓库，代码等文本文件仍然用github托管

不知道我的腾讯工蜂为什么不能开public仓库，只能private仓库独享lfs大文件了😢

腾讯工蜂的 Git LFS 服务端 TLS 配置较旧，Windows 上新版 Git LFS 可能在拉取 LFS 文件时出现 `tls: handshake failure`。
若出现该问题，按下述方式操作

在 Windows PowerShell 中拉取前，可以先为当前终端临时设置 Go TLS 兼容选项：

```powershell
$env:GODEBUG='tlsrsakex=1,tls3des=1'
git pull
```

该设置只对当前 PowerShell 窗口生效，关闭窗口后不会保留。
