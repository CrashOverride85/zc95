#ifndef _EUTIL_H
#define _EUTIL_H

class CInteruptableSection
{
    public:
        CInteruptableSection();
        void start();
        void end();

    private:
        bool _inital_state;
};


#endif