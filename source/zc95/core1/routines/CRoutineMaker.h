#ifndef _CROUTINEMAKER_H
#define _CROUTINEMAKER_H

#include "CRoutine.h"

// Stolen from:
// https://stackoverflow.com/a/10723311

typedef CRoutine* CRoutineMaker();

template <class X> CRoutine* make()
{
  return new X;
}

#endif
