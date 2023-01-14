#include "CLuaTest.h"

#include "../external/lua/lua-5.1.5/include/lstate.h"

CLuaTest::CLuaTest()
{
    printf("CLuaTest::CLuaTest()\n");

}

CLuaTest::~CLuaTest()
{
    printf("~CLuaTest()\n");
}

bool CLuaTest::check_script(const char *lua_script, std::string &error)
{
    bool retval;
    
    
    _lua_state = luaL_newstate_ud(this);
    luaL_openlibs(_lua_state);    
    
    int lua_retval = luaL_dostring(_lua_state, lua_script);
    if (lua_retval == 0)
    {
        retval = true;
    }
    else
    {
        const char *lua_error = lua_tostring(_lua_state, -1);
        printf("LuaTest::check_script(): Bad script! error:\n %s\n", lua_error);
        error = lua_error;
        retval = false;
    }
    
    lua_close(_lua_state);
    _lua_state = NULL;

    return retval;
}
