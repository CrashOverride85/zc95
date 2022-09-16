#include "CUtil.h"
#include "globals.h"

CInteruptableSection::CInteruptableSection()
{
    
}

void CInteruptableSection::start()
{
    _inital_state = gInteruptable;
    gInteruptable = true;
}

void CInteruptableSection::end()
{
    if (!_inital_state)
        gInteruptable = false;
}
