#ifndef _CLUATEST_H
#define _CLUATEST_H

#include "../external/lua/lua-5.1.5/include/lua.h"
#include "../external/lua/lua-5.1.5/include/lauxlib.h"
#include "../external/lua/lua-5.1.5/include/lualib.h"

#include <inttypes.h>
#include <stdio.h>
#include <string>

class CLuaTest
{
    public:
        CLuaTest();
        ~CLuaTest();
        bool check_script(const char *data, std::string &error);


    private:
        lua_State *_lua_state;
};

#endif
