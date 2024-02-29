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

.global lua_lib_script_ettot_start
.global lua_lib_script_ettot_end

.global lua_script_waves_start
.global lua_script_waves_end
.global lua_script_orgasm_start
.global lua_script_orgasm_end
.global lua_script_climb_start
.global lua_script_climb_end

lua_script1_start:
.balign 4096
.fill 49152, 1, 0x00 /* 48k */

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


/* Inbuilt lua scripts that can't be changed, so don't need to care about size/alignment etc */

/* Lua scripts for in built libraries */
lua_lib_script_ettot_start:
.incbin "LuaScripts/lib/ettot.lua"
/* null termination not needed for libs (and breaks it) */
lua_lib_script_ettot_end:


/* Lua scripts for in built patterns. These can't be changed, so don't need to care about size/alignment etc */
lua_script_waves_start:
.incbin "LuaScripts/waves.lua" 
.fill 2, 1, 0x00 /* null terminate */
lua_script_waves_end:

lua_script_orgasm_start:
.incbin "LuaScripts/orgasm.lua"
.fill 2, 1, 0x00 /* null terminate */
lua_script_orgasm_end:

lua_script_climb_start:
.incbin "LuaScripts/climb.lua"
.fill 2, 1, 0x00 /* null terminate */
lua_script_climb_end:

