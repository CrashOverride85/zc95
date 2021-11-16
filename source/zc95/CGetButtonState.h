#ifndef _CGETBUTTONSTATE_H
#define _CGETBUTTONSTATE_H

#include <stdio.h>
#include "ECButtons.h"

// Read only access to current state of button (i.e. not has_button_state_changed which clears a flag after calling)
class CGetButtonState
{
    public:
        virtual bool button_state(enum Button button) = 0;
};


#endif
