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
extern uint8_t lua_script_climb_start;
extern uint8_t lua_script_climb_end;
extern uint8_t lua_script_intense_start;
extern uint8_t lua_script_intense_end;
extern uint8_t lua_script_phasing2_start;
extern uint8_t lua_script_phasing2_end;
extern uint8_t lua_script_stroke_start;
extern uint8_t lua_script_stroke_end;
extern uint8_t lua_script_rhythm_start;
extern uint8_t lua_script_rhythm_end;
extern uint8_t lua_script_combo_start;
extern uint8_t lua_script_combo_end;
extern uint8_t lua_script_torment_start;
extern uint8_t lua_script_torment_end;

// Lua libraries
extern uint8_t lua_lib_script_ettot_start;
extern uint8_t lua_lib_script_ettot_end;

struct lua_script_in_flash_t
{
  uint32_t start;
  uint32_t end;
  bool writeable; // Safe to rewrite in flash (multiple of 4096 bytes etc)
};

struct lua_lib_in_flash_t
{
  uint32_t start;
  uint32_t end;
  const char *name;
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
    {((uint32_t)&lua_script_climb_start)    , ((uint32_t)&lua_script_climb_end)     , false},
    {((uint32_t)&lua_script_intense_start)  , ((uint32_t)&lua_script_intense_end)   , false},
    {((uint32_t)&lua_script_phasing2_start) , ((uint32_t)&lua_script_phasing2_end)  , false},
    {((uint32_t)&lua_script_stroke_start)   , ((uint32_t)&lua_script_stroke_end)    , false},    
    {((uint32_t)&lua_script_rhythm_start)   , ((uint32_t)&lua_script_rhythm_end)    , false},
    {((uint32_t)&lua_script_torment_start)  , ((uint32_t)&lua_script_torment_end)   , false},
    {((uint32_t)&lua_script_combo_start)    , ((uint32_t)&lua_script_combo_end)     , false},
    { 0, 0, 0 }
};

const lua_lib_in_flash_t lua_libs[] =
{
    {((uint32_t)&lua_lib_script_ettot_start), ((uint32_t)&lua_lib_script_ettot_end), "./ettot.lua"},
    { 0, 0, 0 }
};

uint8_t lua_script_count();
bool lua_script_is_writable(uint8_t index);
int get_lib_index_by_name(const char *name);
const char *get_lib_by_index(uint8_t index);
int get_lib_length_by_index(uint8_t index);

#endif
