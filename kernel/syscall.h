#pragma once

#include "types.h"

// System call numbers
#define SYS_fork     1
#define SYS_exit     2
#define SYS_wait     3
#define SYS_pipe     4
#define SYS_read     5
#define SYS_kill     6
#define SYS_exec     7
#define SYS_fstat    8
#define SYS_chdir    9
#define SYS_dup     10
#define SYS_getpid  11
#define SYS_sbrk    12
#define SYS_sleep   13
#define SYS_uptime  14
#define SYS_open    15
#define SYS_write   16
#define SYS_mknod   17
#define SYS_unlink  18
#define SYS_link    19
#define SYS_mkdir   20
#define SYS_close   21
#define SYS_connect 22

uint32 sys_chdir(void);
uint32 sys_close(void);
uint32 sys_dup(void);
uint32 sys_exec(void);
uint32 sys_exit(void);
uint32 sys_fork(void);
uint32 sys_fstat(void);
uint32 sys_getpid(void);
uint32 sys_kill(void);
uint32 sys_link(void);
uint32 sys_mkdir(void);
uint32 sys_mknod(void);
uint32 sys_open(void);
uint32 sys_pipe(void);
uint32 sys_read(void);
uint32 sys_sbrk(void);
uint32 sys_sleep(void);
uint32 sys_unlink(void);
uint32 sys_wait(void);
uint32 sys_write(void);
uint32 sys_uptime(void);
uint32 sys_connect(void);
