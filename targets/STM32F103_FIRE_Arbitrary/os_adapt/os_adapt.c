/* ----------------------------------------------------------------------------
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: LiteOS adaptor file.
 * Author: Huawei LiteOS Team
 * Create: 2020-12-10
 * --------------------------------------------------------------------------- */

#include "los_typedef.h"
#include "los_memory.h"
#include "los_task.h"
#include "los_tick.h"
#include "uart.h"
#include "stm32f1xx_hal.h"

extern char _ebss;
extern char _estack;

/* 默认堆内存起始与结束符号，按标准 GCC 链接脚本符号推导 */
__attribute__((weak)) UINT32 __LOS_HEAP_ADDR_START__ = (UINT32)&_ebss;
__attribute__((weak)) UINT32 __LOS_HEAP_ADDR_END__   = (UINT32)&_estack - 0x800;

__attribute__((weak)) VOID board_config(VOID)
{
    g_sys_mem_addr_end = __LOS_HEAP_ADDR_END__;
    SET_BIT(DBGMCU->CR, DBGMCU_CR_DBG_SLEEP | DBGMCU_CR_DBG_STOP | DBGMCU_CR_DBG_STANDBY);
}

__attribute__((weak)) UINT32 LOS_KernelInit(VOID)
{
    board_config();
    return OsMain();
}

__attribute__((weak)) VOID LOS_Start(VOID)
{
    OsStart();
}

__attribute__((weak)) VOID OsBackTrace(VOID)
{
}

/* 提供弱符号 __fast_end，避免在未做分散加载时，C++ 构造函数遍历出现未解析符号 */
__attribute__((weak)) CHAR __fast_end = 0;

#if defined(LOSCFG_KERNEL_CPPSUPPORT) && (LOSCFG_KERNEL_CPPSUPPORT == 1)
#include "los_cppsupport.h"

extern void (*__init_array_start[])(void);
extern void (*__init_array_end[])(void);

__attribute__((weak)) VOID LiteOS_CppSystemInit(VOID)
{
    static BOOL s_isInit = FALSE;
    if (s_isInit) {
        return;
    }
    s_isInit = TRUE;
    (VOID)LOS_CppSystemInit((UINTPTR)&__init_array_start, (UINTPTR)&__init_array_end, NO_SCATTER);
}
#endif

/* 默认应用任务入口（若用户未实现 app_init 则提供默认实现） */
__attribute__((weak)) VOID app_init(VOID)
{
#if defined(LOSCFG_KERNEL_CPPSUPPORT) && (LOSCFG_KERNEL_CPPSUPPORT == 1)
    LiteOS_CppSystemInit();
#endif
    (VOID)LOS_TaskDelete(LOS_CurTaskIDGet());
}

/* 代码段起止地址弱符号（用于异常回溯分析） */
__asm__(
    ".global __text_start\n"
    ".weak __text_start\n"
    ".set __text_start, 0x08000000\n"

    ".global __text_end\n"
    ".weak __text_end\n"
    ".set __text_end, 0x08080000\n"
);

extern __IO uint32_t uwTick;

/* Note: 由 SysTick 中断持续递增驱动，调度器启动前后无缝单调递增，无断层与死锁风险 */
__attribute__((weak)) uint32_t HAL_GetTick(void)
{
    return uwTick;
}

extern UART_HandleTypeDef huart1;

/* 适配 LiteOS PRINTK / printf 输出至串口 */
__attribute__((weak)) INT32 uart_write(const CHAR *buf, INT32 len, INT32 timeout)
{
    if (huart1.Instance != NULL && buf != NULL && len > 0) {
        (VOID)HAL_UART_Transmit(&huart1, (uint8_t *)buf, (uint16_t)len, (timeout > 0) ? (uint32_t)timeout : 0xFFFF);
    }
    return len;
}

__attribute__((weak)) VOID UartPuts(const CHAR *s, UINT32 len, BOOL isLock)
{
    (VOID)isLock;
    if (s != NULL && len > 0) {
        (VOID)uart_write(s, (INT32)len, 0xFFFF);
    }
}

__attribute__((weak)) VOID uart_init(VOID)
{
}
