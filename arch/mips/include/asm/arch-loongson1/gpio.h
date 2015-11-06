#ifndef __ASM_MACH_GPIO_H
#define __ASM_MACH_GPIO_H

/* GPIO Controller */
#if defined(CONFIG_CPU_LOONGSON1A)
#define LS1X_GPIO_COUNT	96
#elif defined(CONFIG_CPU_LOONGSON1B)
#define LS1X_GPIO_COUNT 64
#elif defined(CONFIG_CPU_LOONGSON1C)
#define LS1X_GPIO_COUNT 128
#endif

#define LOONGSON1_MAX_GPIO LS1X_GPIO_COUNT

#endif /* __ASM_MACH_GPIO_H */
