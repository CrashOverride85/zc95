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

class CRoutines
{
    public:
        static void get_routines(std::vector<CRoutineMaker*> *routines)
        {   
            routines->push_back(make<CWaves>);
            routines->push_back(make<CToggle>);
            routines->push_back(make<CRoundRobin>);
            routines->push_back(make<CTens>);
            routines->push_back(make<CClimb>);
            routines->push_back(make<CTriggeredClimb>);
            routines->push_back(make<CFire>);
            routines->push_back(make<CAudioThreshold>);
            routines->push_back(make<CAudioWave>);
            routines->push_back(make<CAudioIntensity>);
            routines->push_back(make<CAudioVirtual3>);
            routines->push_back(make<CClimbPulse>);
            routines->push_back(make<CPredicament>);
            routines->push_back(make<CShockChoice>);
            routines->push_back(make<CCamTrigger>);
            routines->push_back(make<CBuzz>);   
        }
};
