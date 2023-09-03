#ifndef _CLUASCRIPTS_H
#define _CLUASCRIPTS_H

#include <inttypes.h>

// These are defined in LuaScripts.S and are empty contiguous blocks of 24k flash that can be used 
// to put lua scripts in
extern uint8_t lua_script1_start;  
extern uint8_t lua_script2_start;
extern uint8_t lua_script3_start;
extern uint8_t lua_script4_start;
extern uint8_t lua_script5_start;
extern uint8_t lua_script5_end;

// Inbuilt Lua patterns that will always be present. Also defined in LuaScripts.S
extern uint8_t lua_script_waves_start;
extern uint8_t lua_script_waves_end;
extern uint8_t lua_script_orgasm_start;
extern uint8_t lua_script_orgasm_end;

struct lua_script_in_flash_t
{
  uint32_t start;
  uint32_t end;
  bool writeable; // Safe to rewrite in flash (multiple of 4096 bytes etc)
};

const lua_script_in_flash_t lua_scripts[] =
{
    {((uint32_t)&lua_script1_start)         , ((uint32_t)&lua_script2_start)        , true },
    {((uint32_t)&lua_script2_start)         , ((uint32_t)&lua_script3_start)        , true },
    {((uint32_t)&lua_script3_start)         , ((uint32_t)&lua_script4_start)        , true },
    {((uint32_t)&lua_script4_start)         , ((uint32_t)&lua_script5_start)        , true },
    {((uint32_t)&lua_script5_start)         , ((uint32_t)&lua_script5_end)          , true },

    {((uint32_t)&lua_script_waves_start)    , ((uint32_t)&lua_script_waves_end)     , false},
    {((uint32_t)&lua_script_orgasm_start)   , ((uint32_t)&lua_script_orgasm_end)    , false},
    { 0, 0, 0 }
};

uint8_t lua_script_count();
bool lua_script_is_writable(uint8_t index);

#endif
