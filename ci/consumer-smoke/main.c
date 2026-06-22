/* ci/consumer-smoke — minimal external consumer of an installed ROSS.
 *
 * Exercises the CMake-config consumption path:
 *     find_package(ROSS REQUIRED) + target_link_libraries(... ROSS::ROSS)
 *
 * CODES consumes ROSS via pkg-config, so the CODES contract job does not
 * cover the find_package(ROSS) / ROSS::ROSS surface. This program does. It
 * includes the supported umbrella header and references a public symbol so
 * the link resolves against the installed library, not just its headers. It
 * does not run a simulation. */
#include <ross.h>
#include <stdio.h>

int main(void)
{
    /* Take the address of a public entry point so the linker must resolve it
       against libROSS — a header-only success would not catch a broken or
       mislocated installed library. */
    printf("ROSS consumer smoke ok: tw_init=%p\n", (void *)&tw_init);
    return 0;
}
