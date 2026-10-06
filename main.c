#include <stdint.h>
#include <stdbool.h>

#include <ti/devices/msp/m0p/mspm0g350x.h>
#include <ti/driverlib/dl_gpio.h>
#include <ti/driverlib/dl_timer.h>
#include <ti/driverlib/m0p/dl_core.h>

// LED 端口与引脚（对应 GPIOA_0）
#define LED_PORT     GPIOA
#define LED_PIN      DL_GPIO_PIN_0

// 按键端口与引脚（对应 GPIOA_4）
#define BUTTON_PORT  GPIOA
#define BUTTON_PIN   DL_GPIO_PIN_4

// 使用 TIMER0（通用定时器）
#include "ti/devices/msp/peripherals/hw_gptimer.h"
#ifndef TIMER0
#define TIMER0      ((GPTimer_Regs *) GPTIMER0_BASE)
#endif

void PORTA_IRQHandler(void);
void TIMG0_IRQHandler(void);

void configureGPIO(void);
void configureTimer(void);

int main(void) {
    // 使用内部 8MHz RC 振荡器作为系统时钟
    SysCtlClockSet(SYSCTL_OSC_INT_RC | SYSCTL_USE_OSC | SYSCTL_MAIN_Osc_FREQ_8MHZ);

    configureGPIO();
    configureTimer();

    __enable_irq();  // 使能全局中断

    while (1) {
        // 主循环空闲，功能全部由中断驱动
    }
}

void configureGPIO(void) {
    // 配置 LED（输出）
    DL_GPIO_setPins(LED_PORT, LED_PIN);         // 初始熄灭
    DL_GPIO_enableOutput(LED_PORT, LED_PIN);    // 使能为输出

    // 配置按键（输入 + 上拉）
    DL_GPIO_clearPins(BUTTON_PORT, BUTTON_PIN);
    DL_GPIO_setDirectionIn(BUTTON_PORT, BUTTON_PIN);
    DL_GPIO_setInternalPull(BUTTON_PORT, BUTTON_PIN, DL_GPIO_INTERNAL_PULL_UP);

    // 按键下降沿触发中断
    DL_GPIO_setPinFunctionAsGPIOPinWithInterrupt(BUTTON_PORT, BUTTON_PIN, DL_GPIO_IRQ_TYPE_FALLING_EDGE);
    DL_GPIO_enableInterrupt(BUTTON_PORT, BUTTON_PIN);

    // 使能 PORTA 中断
    NVIC_EnableIRQ(GPIOA_INT_IRQn);
}

void configureTimer(void) {
    // 定时器时钟源用总线时钟 BUSCLK(8MHz)
    DL_Timer_initTimerMode(TIMER0, DL_TIMER_CLOCK_SOURCE_BUSCLK, DL_TIMER_CLOCK_DIVIDE_1);
    DL_Timer_setLoadValue(TIMER0, 8000000 - 1);  // 1 秒定时
    DL_Timer_enableInterrupt(TIMER0);            // 使能定时器中断
    NVIC_EnableIRQ(TIMG0_INT_IRQn);              // 使能 TIMG0 中断
}

// 按键中断服务函数
void PORTA_IRQHandler(void) {
    uint32_t status = DL_GPIO_getPendingInterrupt(BUTTON_PORT);

    if (status & BUTTON_PIN) {
        DL_GPIO_clearInterruptStatus(BUTTON_PORT, BUTTON_PIN);

        // 软件消抖
        for (volatile int i = 0; i < 10000; i++);

        // 二次确认按键仍处于按下状态
        if (!(DL_GPIO_readPins(BUTTON_PORT, BUTTON_PIN) & BUTTON_PIN)) {
            DL_GPIO_setPins(LED_PORT, LED_PIN);          // 点亮 LED
            DL_Timer_startCounter(TIMER0);               // 启动定时器
        }
    }
}

// 定时器超时中断服务函数
void TIMG0_IRQHandler(void) {
    DL_Timer_stopCounter(TIMER0);                       // 停止定时器
    DL_GPIO_clearPins(LED_PORT, LED_PIN);               // 熄灭 LED
    TIMER0->INT_CLR = GPTIMER_INT_CLR_TOUT;             // 清除中断标志
}
