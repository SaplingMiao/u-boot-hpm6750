// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2016, Bin Meng <bmeng.cn@gmail.com>
 */

#include <init.h>
#include <asm/post.h>
#include <asm/processor.h>
#include <asm/u-boot-x86.h>

int arch_cpu_init(void)
{
	post_code(POST_CPU_INIT);

	return x86_cpu_init_f();
}
