# the name of the target operating system
SET(CMAKE_SYSTEM_NAME BlueGeneL)

#TODO: CHANGE THIS FOR MPIXLC
set(CMAKE_C_COMPILER  /opt/ibmcmp/vac/bg/8.0/bin/blrts_xlc)
set(CMAKE_CXX_COMPILER  /opt/ibmcmp/vacpp/bg/8.0/bin/blrts_xlC)

# set the search path for the environment coming with the compiler
# and a directory where you can install your own compiled software
# TODO: CHANGE THIS FOR OUR BG/L
set(CMAKE_FIND_ROOT_PATH
    /bgl/BlueLight/ppcfloor/
    /bgl/BlueLight/V1R3M2_140_2007-070424/ppc/bglsys
    /home/alex/bgl-install
)

# adjust the default behaviour of the FIND_XXX() commands:
# search headers and libraries in the target environment, search 
# programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

## TODO: Add BG ROSS Specific Variable Settings
