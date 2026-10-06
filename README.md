# MSPM0G3507 按键中断 + 定时器 LED 控制

基于 **TI MSPM0G3507 LaunchPad (LP-MSPM0G3507)** 的裸机例程：用按键外部中断触发 LED 点亮，再用硬件定时器实现 1 秒后自动熄灭。全程使用 TI DriverLib，不依赖 RTOS。

## 功能

- **按键中断**：GPIOA_4 下降沿触发，带 10 ms 软件消抖（延时后二次确认电平）
- **LED 控制**：GPIOA_0 输出，按键按下后点亮
- **定时器超时**：TIMG0 定时 1 秒，超时中断里熄灭 LED，实现「按一下亮 1 秒」的脉冲效果
- **时钟配置**：内部 8 MHz RC 振荡器（`SYSCTL_OSC_INT_RC`）

## 硬件

| 功能 | 端口 | 引脚 | 说明 |
|---|---|---|---|
| LED | GPIOA | PA0 | 推挽输出 |
| 按键 | GPIOA | PA4 | 上拉输入，下降沿中断 |

> `untitled.syscfg` 中另配了板载三色灯：LED_GREEN = PORTA.7、LED_RED = PORTB.3、LED_BLUE = PORTB.2（可自行启用）。

## 文件说明

```
.
├── main.c                       # 主程序：GPIO/定时器/中断配置
├── ti_msp_dl_config.c/h         # SysConfig 自动生成的初始化代码
├── untitled.syscfg              # SysConfig 图形化配置文件（引脚/外设）
├── device_linker.cmd / .lds     # 链接脚本（IAR / GCC）
├── device.cmd.genlibs / .lds.genlibs
├── empty.syscfg                 # 空白参考配置
└── keil/
    ├── LED.uvprojx              # Keil MDK 工程
    ├── LED.uvoptx               # Keil 工程选项
    ├── startup_mspm0g350x_uvision.s  # 启动文件
    ├── mspm0g3507.sct           # Keil 分散加载文件
    └── EventRecorderStub.scvd
```

## 依赖

- **MSPM0 SDK** `2.05.00.05`（`mspm0_sdk@2.05.00.05`）
- **SysConfig** `1.21.0+3721`
- **Keil MDK**（含 MSPM0 器件支持包）或 CCS / IAR

## 构建

### 方式一：Keil MDK

1. 安装 MSPM0 SDK 2.05.00.05，路径建议 `C:\ti\mspm0_sdk_2_05_00_05`
2. 用 Keil 打开 `keil/LED.uvprojx`
3. 在工程选项 → C/C++ → Include Paths 中，把 SDK 路径指向你本机的实际位置
4. 编译下载（板载 XDS110 调试器）

### 方式二：CCS / SysConfig

1. 用 SysConfig 打开 `untitled.syscfg`，确认引脚分配后重新生成代码
2. 参考 `ti_msp_dl_config.c/h` 的初始化内容接入你的工程

> **注意**：仓库内 `ti_msp_dl_config.c/h` 由 SysConfig 生成，修改引脚请改 `.syscfg` 而不是直接改生成文件。

## 实现要点

```c
// 按键中断：消抖后点亮 LED 并启动定时器
void PORTA_IRQHandler(void) {
    uint32_t status = DL_GPIO_getPendingInterrupt(BUTTON_PORT);
    if (status & BUTTON_PIN) {
        DL_GPIO_clearInterruptStatus(BUTTON_PORT, BUTTON_PIN);
        for (volatile int i = 0; i < 10000; i++);   // 软件消抖
        if (!(DL_GPIO_readPins(BUTTON_PORT, BUTTON_PIN) & BUTTON_PIN)) {
            DL_GPIO_setPins(LED_PORT, LED_PIN);
            DL_Timer_startCounter(TIMER0);
        }
    }
}

// 定时器超时：熄灭 LED
void TIMG0_IRQHandler(void) {
    DL_Timer_stopCounter(TIMER0);
    DL_GPIO_clearPins(LED_PORT, LED_PIN);
    TIMER0->INT_CLR = GPTIMER_INT_CLR_TOUT;
}
```

## License

仅含本人编写的应用代码与工程配置；TI SDK 部分请遵循 TI 官方许可协议，使用时需自行从 TI 官网获取 SDK。
