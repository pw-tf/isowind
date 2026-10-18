#ifndef OPENMW_MP_RAKNETINCLUDES_HPP
#define OPENMW_MP_RAKNETINCLUDES_HPP

// RakNet's types. On Windows they bring in windows.h, whose macros clash with names OpenMW uses (DrawState becomes
// DrawStateW, breaking MWMechanics::DrawState; near and far become nothing), so those are undefined again here.
#include <RakNetTypes.h>

#ifdef DrawState
#undef DrawState
#endif
#ifdef near
#undef near
#endif
#ifdef far
#undef far
#endif

#endif //OPENMW_MP_RAKNETINCLUDES_HPP
