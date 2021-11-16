#ifndef _COPTIONSLIST_H
#define _COPTIONSLIST_H

#include <string>
#include <vector>

#include "CDisplay.h"

class COptionsList
{
    public:
        COptionsList(CDisplay* display, struct display_area area);
        void add_option(std::string options);
        void clear_options();
        void draw();
        void up();
        void down();
        void set_selected(uint8_t selection);
        uint8_t get_current_selection();

    private:
        std::vector<std::string> _options;
        CDisplay* _display;
        struct display_area _area;
        uint8_t _current_selection = 0;
};

#endif
