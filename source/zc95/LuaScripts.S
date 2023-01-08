.section .rodata

/* Allocate 5 x 24k empty blocks in flash to store 5 lua scripts in.
 * As these sections are re-written during operation, things are much easier if the size is
 * a multiple of 4096 (flash page size)
 */

.global lua_script1_start
.global lua_script2_start
.global lua_script3_start
.global lua_script4_start
.global lua_script5_start
.global lua_script5_end

lua_script1_start:
.balign 4096
.fill 24576, 1, 0x00 /* 24k */

lua_script2_start:
.balign 4096
.fill 24576, 1, 0x00 /* 24k */

lua_script3_start:
.balign 4096
.fill 24576, 1, 0x00 /* 24k */

lua_script4_start:
.balign 4096
.fill 24576, 1, 0x00 /* 24k */

lua_script5_start:
.balign 4096
.fill 24576, 1, 0x00 /* 24k */

.global lua_script5_end
lua_script5_end:
