# ASCII Wave

一个使用纯 Win32 控制台 API 的 ASCII 抽象海面模拟。

## 特性

- 由 6 组叠加正弦波生成连续变化的抽象海面。
- 使用可打印 ASCII 字符 `32-126` 映射海面亮度。
- 通过 `WriteConsoleOutputA` 一次性写入整帧，减少闪烁。
- 轮询控制台窗口位置，拖动窗口时产生对应方向的反向惯性偏移，并自然衰减。
- 控制台窗口大小变化后会自动适配。

## 操作

- `+` / `-`：增大 / 减小波幅
- `[` / `]`：减慢 / 加快动画
- `R`：清除拖动惯性
- `Q` 或 `Esc`：退出

## 使用 CLion 构建

使用 MinGW 工具链打开本目录，CMake 会自动读取 `CMakeLists.txt`。构建目标为 `ascii_wave`，运行后请直接拖动控制台窗口标题栏观察海面的惯性偏移。

也可以使用命令行：

```powershell
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build --parallel
```
