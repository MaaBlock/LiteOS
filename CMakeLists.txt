cmake_minimum_required(VERSION 3.22)
project(liteos C CXX ASM)

# ==========================================
# 目标平台与架构基础配置
# ==========================================
set(LITEOS_PLATFORM "STM32F103_FIRE_Arbitrary" CACHE STRING "LiteOS 目标板级平台名称")
set(LITEOS_ARCH "arm/cortex_m" CACHE STRING "LiteOS 体系架构目录相对路径")
set(LITEOS_LIB_TYPE "OBJECT" CACHE STRING "LiteOS 库构建类型 (OBJECT 或 STATIC)")

# ==========================================
# LiteOS 内核特性裁剪选项 (父工程可直接开启或覆盖)
# ==========================================
option(LITEOS_ENABLE_CPPSUPPORT "启用 LiteOS C++ 支持（全局构造函数与 C++ 源码混编）" ON)
option(LITEOS_ENABLE_CMSIS      "启用 CMSIS-RTOS 兼容层 (cmsis_os.h)" ON)
set(LITEOS_CMSIS_VERSION "1.0" CACHE STRING "CMSIS-RTOS 兼容层版本 (1.0 或 2.0)")
option(LITEOS_ENABLE_CPUP       "启用 CPU 占用率监控统计 (kernel/extended/cpup)" OFF)
option(LITEOS_ENABLE_SHELL      "启用 LiteOS 命令行调试终端 (shell/full)" OFF)

# ==========================================
# LiteOS 功能模块子目录清单 (集中统一扫描收集)
# ==========================================
set(LITEOS_MODULE_DIRS
    # 内核基础调度、任务、IPC 与内存
    kernel/base
    kernel/base/sched/sched_sq
    kernel/base/mem/bestfit_little
    kernel/base/mem/membox
    kernel/base/mem/common/memstat

    # 内核初始化入口
    kernel/init

    # Cortex-M 架构上下文切换与异常
    arch/${LITEOS_ARCH}/src

    # BSP 硬件抽象：NVIC 与 SysTick
    targets/bsp/hw/arm/interrupt/nvic
    targets/bsp/hw/arm/timer/arm_cortex_m
    targets/bsp/common

    # 目标板级适配层
    targets/${LITEOS_PLATFORM}/os_adapt

    # 安全 C 库实现
    lib/libsec/src
)

# 动态纳入可选功能模块
if(LITEOS_ENABLE_CPPSUPPORT)
    list(APPEND LITEOS_MODULE_DIRS kernel/extended/cppsupport)
endif()

if(LITEOS_ENABLE_CMSIS)
    list(APPEND LITEOS_MODULE_DIRS compat/cmsis)
endif()

if(LITEOS_ENABLE_CPUP)
    list(APPEND LITEOS_MODULE_DIRS kernel/extended/cpup)
endif()

if(LITEOS_ENABLE_SHELL)
    list(APPEND LITEOS_MODULE_DIRS
        shell/full/src/base
        shell/full/src/cmds
    )
endif()

# ==========================================
# 自动扫描并收集各模块源码文件 (C、C++ 与汇编)
# ==========================================
set(LITEOS_SOURCES)
foreach(mod_dir ${LITEOS_MODULE_DIRS})
    file(GLOB _dir_srcs
        LIST_DIRECTORIES false
        "${CMAKE_CURRENT_SOURCE_DIR}/${mod_dir}/*.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/${mod_dir}/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/${mod_dir}/*.S"
        "${CMAKE_CURRENT_SOURCE_DIR}/${mod_dir}/*.s"
    )
    list(APPEND LITEOS_SOURCES ${_dir_srcs})
endforeach()

# 排除未开启的硬件特性与示例文件
list(FILTER LITEOS_SOURCES EXCLUDE REGEX ".*/fpb\\.c$")
list(FILTER LITEOS_SOURCES EXCLUDE REGEX ".*/mpu\\.c$")
list(FILTER LITEOS_SOURCES EXCLUDE REGEX ".*/demo_entry\\.c$")
list(FILTER LITEOS_SOURCES EXCLUDE REGEX ".*/los_membox_dyn\\.c$")
if(NOT LITEOS_ENABLE_SHELL)
    list(FILTER LITEOS_SOURCES EXCLUDE REGEX ".*/cpup_shellcmd\\.c$")
endif()

# 创建 LiteOS 目标库
add_library(liteos ${LITEOS_LIB_TYPE} ${LITEOS_SOURCES})

# ==========================================
# 公共头文件目录导出
# ==========================================
target_include_directories(liteos PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/kernel/include
    ${CMAKE_CURRENT_SOURCE_DIR}/kernel/base/include
    ${CMAKE_CURRENT_SOURCE_DIR}/kernel/extended/include
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/${LITEOS_ARCH}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/targets/bsp/include
    ${CMAKE_CURRENT_SOURCE_DIR}/targets/bsp/hw/include
    ${CMAKE_CURRENT_SOURCE_DIR}/targets/bsp/common
    ${CMAKE_CURRENT_SOURCE_DIR}/targets/${LITEOS_PLATFORM}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/libsec/include
)

if(LITEOS_ENABLE_CPPSUPPORT)
    target_include_directories(liteos PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/kernel/extended/cppsupport
    )
endif()

if(LITEOS_ENABLE_CMSIS)
    target_include_directories(liteos PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/compat/cmsis
        ${CMAKE_CURRENT_SOURCE_DIR}/compat/cmsis/${LITEOS_CMSIS_VERSION}
    )
endif()

if(LITEOS_ENABLE_SHELL)
    target_include_directories(liteos PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/shell/full/include
    )
endif()

# 内部头文件目录
target_include_directories(liteos PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/kernel/base/mem
    ${CMAKE_CURRENT_SOURCE_DIR}/kernel/base/mem/bestfit_little
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/libsec/src
)

# ==========================================
# 核心宏定义
# ==========================================
target_compile_definitions(liteos PUBLIC
    __LITEOS__
    SECUREC_IN_KERNEL=0
)

if(LITEOS_ENABLE_CPPSUPPORT)
    target_compile_definitions(liteos PUBLIC
        LOSCFG_KERNEL_CPPSUPPORT=1
        LOSCFG_KERNEL_EXTKERNEL=1
    )
endif()

if(LITEOS_ENABLE_CMSIS)
    if(LITEOS_CMSIS_VERSION STREQUAL "1.0")
        target_compile_definitions(liteos PUBLIC
            LOSCFG_COMPAT_CMSIS=1
            CMSIS_OS_VER=1
        )
    else()
        target_compile_definitions(liteos PUBLIC
            LOSCFG_COMPAT_CMSIS=1
            CMSIS_OS_VER=2
        )
    endif()
endif()

if(LITEOS_ENABLE_CPUP)
    target_compile_definitions(liteos PUBLIC
        LOSCFG_KERNEL_CPUP=1
    )
endif()

if(LITEOS_ENABLE_SHELL)
    target_compile_definitions(liteos PUBLIC
        LOSCFG_SHELL=1
    )
endif()

# 自动强制注入板级 menuconfig.h 配置 (C、C++ 与汇编统一生效)
set(LITEOS_MENUCONFIG_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/targets/${LITEOS_PLATFORM}/include/menuconfig.h")
if(EXISTS "${LITEOS_MENUCONFIG_HEADER}")
    target_compile_options(liteos PUBLIC
        $<$<COMPILE_LANGUAGE:C>:-include${LITEOS_MENUCONFIG_HEADER}>
        $<$<COMPILE_LANGUAGE:CXX>:-include${LITEOS_MENUCONFIG_HEADER}>
        $<$<COMPILE_LANGUAGE:ASM>:-include${LITEOS_MENUCONFIG_HEADER}>
    )
endif()

# 若工程中已存在 stm32cubemx 目标，则自动继承其 HAL 与 CMSIS 依赖
if(TARGET stm32cubemx)
    target_link_libraries(liteos PUBLIC stm32cubemx)
endif()
