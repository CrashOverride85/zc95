#ifndef _CROUTINES_H
#define _CROUTINES_H

#include "CRoutine.h"
#include "CWaves.h"
#include "CToggle.h"
#include "CRoundRobin.h"
#include "CTens.h"
#include "CClimb.h"
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
#include "../../LuaScripts/LuaScripts.h"

class CRoutines
{
    public:
        struct Routine
        {
            routine_creator routine_maker;
            int param;
        };

        static void get_routines(std::vector<Routine> *routines)
        {
            // Loop through and add all valid lua scripts
            for (uint8_t index = 0; index < lua_script_count(); index++)
            {
                if (is_lua_script_valid(index))
                    routines->push_back({&(CLuaRoutine::create), index});
            }
            
         // routines->push_back({&(CWaves::create)         , 0}); now a lua script
            routines->push_back({&(CToggle::create)        , 0});
            routines->push_back({&(CRoundRobin::create)    , 0});
            routines->push_back({&(CTens::create)          , 0});
        //  routines->push_back({&(CClimb::create)         , 0}); now a lua script
            routines->push_back({&(CTriggeredClimb::create), 0});
            routines->push_back({&(CFire::create)          , 0});
            routines->push_back({&(CAudioThreshold::create), 0});
            routines->push_back({&(CAudioWave::create)     , 0});
            routines->push_back({&(CAudioIntensity::create), 0});
            routines->push_back({&(CAudioVirtual3::create) , 0});
            routines->push_back({&(CClimbPulse::create)    , 0});
            routines->push_back({&(CPredicament::create)   , 0});
            routines->push_back({&(CShockChoice::create)   , 0});
            routines->push_back({&(CCamTrigger::create)    , 0});
            routines->push_back({&(CBuzz::create)          , 0});
        }

    private:
        static bool is_lua_script_valid(uint8_t index)
        {
            CLuaRoutine *lua = new CLuaRoutine(index);
            bool is_valid = lua->is_script_valid();
            delete lua;
            return is_valid;
        }
};

#endif
