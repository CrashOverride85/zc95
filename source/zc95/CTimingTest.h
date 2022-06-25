#define __int64_t_defined 1
#include <inttypes.h>
#include <string>

class CTimingTest
{
    public:
        CTimingTest();
        void print_time(); 
        void print_time(std::string message);

    private:
        uint64_t _start_time;

};
