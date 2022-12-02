#ifndef _CROUTINES_H
#define _CROUTINES_H

#include "CRoutineMaker.h"
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

class CRoutines
{
    public:
        struct Routine
        {
            CRoutineMaker* routine_maker;
            int param;
        };

        static void get_routines(std::vector<Routine> *routines)
        {

            // TODO: loop through and add all lua scripts
            routines->push_back({make<CLuaRoutine>, 0});

            routines->push_back({make<CWaves>, 0});
            routines->push_back({make<CToggle>, 0});
            routines->push_back({make<CRoundRobin>, 0});
            routines->push_back({make<CTens>, 0});
            routines->push_back({make<CClimb>, 0});
            routines->push_back({make<CTriggeredClimb>, 0});
            routines->push_back({make<CFire>, 0});
            routines->push_back({make<CAudioThreshold>, 0});
            routines->push_back({make<CAudioWave>, 0});
            routines->push_back({make<CAudioIntensity>, 0});
            routines->push_back({make<CAudioVirtual3>, 0});
            routines->push_back({make<CClimbPulse>, 0});
            routines->push_back({make<CPredicament>, 0});
            routines->push_back({make<CShockChoice>, 0});
            routines->push_back({make<CCamTrigger>, 0});
            routines->push_back({make<CBuzz>, 0});


        }
};

#endif
