#ifdef ROSS_CLOCK_i386
#  include "clock/i386.h"
#endif
#ifdef ROSS_CLOCK_amd64
#  include "clock/amd64.h"
#endif
#ifdef ROSS_CLOCK_ia64
#  include "clock/ia64.h"
#endif
#ifdef ROSS_CLOCK_ppc
#  include "clock/ppc.h"
#endif
#ifdef ROSS_CLOCK_ppc64le
#  include "clock/ppc64le.h"
#endif
#ifdef ROSS_CLOCK_bgl
#  include "clock/bgl.h"
#endif
#ifdef ROSS_CLOCK_bgq
#  include "clock/bgq.h"
#endif
#ifdef ROSS_CLOCK_aarch64
#  include "clock/aarch64.h"
#endif
#ifdef ROSS_CLOCK_armv7l
#  include "clock/armv7l.h"
#endif
#ifdef ROSS_CLOCK_gtod
#  include "clock/gtod.h"
#endif
