
#ifndef _CHORZBARGRAPH_H
#define _CHORZBARGRAPH_H

#include <stdint.h>
#include <string>

#include "CDisplay.h"

class CHorzBarGraph
{
    public:
        CHorzBarGraph(CDisplay *display);
        void draw_horz_bar_graph(display_area area, int16_t min_val, int16_t max_val, int16_t current_val, std::string UoM, color_t bar_colour);

    private:
        CDisplay *_display;


};


#endif
