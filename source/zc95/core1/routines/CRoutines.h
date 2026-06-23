#ifndef _CROUTINES_H
#define _CROUTINES_H

#include "CRoutine.h"
#include "CWaves.h"
#include "CToggle.h"
#include "CRoundRobin.h"
#include "CTriggeredClimb.h"
#include "CFire.h"
#include "CClimbPulse.h"
#include "CPredicament.h"
#include "CShockChoice.h"
#include "CCamTrigger.h"
#include "CBuzz.h"
#include "CAudioThreshold.h"
#include "CAudioWave.h"
#include "CAudioIntensity.h"
#include "CAudioVirtual3.h"
#include "CLuaRoutine.h"
#include "CDirectPulse.h"
#include "../../LuaScripts/LuaScripts.h"

class CRoutines
{
    public:
        struct Routine
        {
            routine_creator routine_maker;
            std::string script_name;
            bool hidden;
            audio_mode_t audio_mode;
            int param;
        };

        static void get_routines(std::vector<Routine> &routines)
        {
            // Loop through and add all valid lua scripts
            for (uint8_t index = 0; index < lua_script_count(); index++)
            {
                add_routine_if_lua_script_valid(routines, index);
            }
            
            add_routine(CToggle::create         , routines);
            add_routine(CRoundRobin::create     , routines);
            add_routine(CTriggeredClimb::create , routines);
            add_routine(CFire::create           , routines);
            add_routine(CAudioThreshold::create , routines);
            add_routine(CAudioWave::create      , routines);
            add_routine(CAudioIntensity::create , routines);
            add_routine(CAudioVirtual3::create  , routines);
            add_routine(CClimbPulse::create     , routines);
            add_routine(CPredicament::create    , routines);
            add_routine(CShockChoice::create    , routines);
            add_routine(CCamTrigger::create     , routines);
            add_routine(CBuzz::create           , routines);
            add_routine(CDirectPulse::create    , routines);  // special for BLE. hidden from menu.         
        }

    private:
        static void add_routine(routine_creator routine, std::vector<Routine> &routines, uint8_t param_index = 0)
        { 
            struct routine_conf conf;
            CRoutine* routine_ptr = routine(param_index);
            routine_ptr->get_config(&conf);
            delete routine_ptr;

            // Add a warning for routines that are able to disable channel isolation
            if (!conf.force_channel_isolation)
                conf.name = "(!)" + conf.name;

            routines.push_back({routine, conf.name, conf.hidden_from_menu, conf.audio_processing_mode, 0});
        }

        static void add_routine_if_lua_script_valid(std::vector<Routine> &routines, uint8_t index)
        {
            CLuaRoutine lua = CLuaRoutine(index);
            bool is_valid = lua.is_script_valid();
            if (is_valid)
            {
                struct routine_conf conf;
                lua.get_config(&conf);

                if (!conf.force_channel_isolation)
                    conf.name = "(!)" + conf.name;

                routines.push_back({&(CLuaRoutine::create), conf.name, conf.hidden_from_menu, conf.audio_processing_mode, index});
            }
        }
};

#endif
