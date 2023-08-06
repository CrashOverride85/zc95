#ifndef _COPTIONSLIST_H
#define _COPTIONSLIST_H

#include <string>
#include <vector>

#include "CDisplay.h"

class COptionsList
{
    public:
        COptionsList(CDisplay* display, struct display_area area);
        void add_option(std::string option_text);
        void add_option(std::string option_text, int id);
        void clear_options();
        void draw();
        void up();
        void down();
        void set_selected(uint8_t selection);
        uint8_t get_current_selection();
        int get_current_selection_id();
        uint8_t count();

    private:
        class option_t
        {
            public:
                option_t(std::string display_text, int id) 
                {
                    _display_text = display_text;
                    _id = id;
                }

                std::string DisplayText() {return _display_text;}
                int Id() {return _id;}

            private:
                std::string _display_text;
                int _id;
        };

        std::vector<option_t> _options;
        CDisplay* _display;
        struct display_area _area;
        uint8_t _current_selection = 0;
};

#endif
