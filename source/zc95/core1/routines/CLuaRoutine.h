#include "CRoutine.h"

#include "../../external/lua/lua-5.1.5/include/lua.h"
#include "../../external/lua/lua-5.1.5/include/lauxlib.h"
#include "../../external/lua/lua-5.1.5/include/lualib.h"


class CLuaRoutine: public CRoutine
{
    public:
        CLuaRoutine();
        ~CLuaRoutine();
        void get_config(struct routine_conf *conf);
        void menu_min_max_change(uint8_t menu_id, int16_t new_value);
        void menu_multi_choice_change(uint8_t menu_id, uint8_t choice_id);
        void soft_button_pushed (soft_button button, bool pushed);
        void trigger(trigger_socket socket, trigger_part part, bool active);
        void start();
        void loop(uint64_t time_us);
        void stop();


        

    private:
        bool CheckLua(int r);
        bool GetMenuEntryTypeFromString(const char* type, menu_entry_type *menu_type_out);
        void get_multi_choice_entry(struct menu_entry *entry);
        void get_min_max_entry(struct menu_entry *entry);
        int get_int_field(const char *field_name);
        std::string get_string_field(const char *field_name);
        bool is_channel_number_valid(int channel_number);

        // called form lua
        int lua_channel_on(lua_State *L);
        int lua_channel_off(lua_State *L);
        int lua_set_power(lua_State *L);
        int lua_set_freq(lua_State *L);
        int lua_set_pulse_width(lua_State *L);
              
        lua_State *_lua_state;
        bool _script_valid = false;
};
