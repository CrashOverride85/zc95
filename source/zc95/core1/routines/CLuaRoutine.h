#include "CRoutine.h"
#include "../../CLuaStorage.h"

#include "../../external/lua/lua-5.1.5/include/lua.h"
#include "../../external/lua/lua-5.1.5/include/lauxlib.h"
#include "../../external/lua/lua-5.1.5/include/lualib.h"

#define CHANNEL_COUNT 4

class CLuaRoutine: public CRoutine
{
    public:
        CLuaRoutine(uint8_t script_index);
        CLuaRoutine(const char *script);
        ~CLuaRoutine();
        static CRoutine* create(uint8_t param) { return new CLuaRoutine(param); };
        void get_config(struct routine_conf *conf);
        bool get_and_validate_config(struct routine_conf *conf);
        void menu_min_max_change(uint8_t menu_id, int16_t new_value);
        void menu_multi_choice_change(uint8_t menu_id, uint8_t choice_id);
        void soft_button_pushed (soft_button button, bool pushed);
        void trigger(trigger_socket socket, trigger_part part, bool active);
        void bluetooth_remote_keypress(CBluetoothRemote::keypress_t key);
        void start();
        void loop(uint64_t time_us);
        void stop();
        bool is_script_valid();
        std::string get_last_lua_error();
        lua_script_state_t lua_script_state();
        void lua_hook(lua_Debug *ar);

    private:
        enum class ScriptValid 
        {
            VALID, 
            INVALID, 
            UNKNOWN   // inital state before CheckLua ran
        };

        void CLuaRoutine_common();
        void load_lua_script_if_required();
        bool CheckLua(int r);
        bool GetMenuEntryTypeFromString(const char* type, menu_entry_type *menu_type_out);
        void get_multi_choice_entry(struct menu_entry *entry);
        void get_min_max_entry(struct menu_entry *entry);
        int get_int_field(const char *field_name);
        std::string get_string_field(const char *field_name);
        bool get_bool_field(const char *field_name);
        bool is_channel_number_valid(int channel_number);
        bool runnable();
        int pcall (int nargs, int nresults, int errfunc);
        void channel_pulse_processing();

        static void s_lua_hook(lua_State *L, lua_Debug *ar);

        // called form lua
        int lua_print(lua_State *L);

        int lua_channel_on(lua_State *L);
        int lua_channel_off(lua_State *L);
        int lua_channel_pulse_ms(lua_State *L);
        int lua_set_power(lua_State *L);
        int lua_set_freq(lua_State *L);
        int lua_set_pulse_width(lua_State *L);
        int lua_acc_io_write(lua_State *L);
        uint16_t _loop_freq_hz = 0;
        uint32_t _last_loop = 0;

        lua_State *_lua_state;
        ScriptValid _script_valid = ScriptValid::UNKNOWN;
        uint64_t _channel_switch_off_at_us[CHANNEL_COUNT] = {0};
        uint8_t _script_index;
        const char *_script;
        std::string _last_lua_error;
        int _instruction_count = 0;
        const int _hook_call_frequency = 25;
};
