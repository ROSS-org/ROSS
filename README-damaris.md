## Using Damaris with ROSS
[Damaris](http://damaris.gforge.inria.fr/doku.php) is a I/O and data management software.
Support for Damaris is currently being added to ROSS to help manage extensions for in situ data analysis and visualization. 
This readme contains some basic info for how to set-up ROSS to use Damaris.

## Building ROSS with Damaris
For Damaris installation instructions, see the [Damaris User Guide](http://damaris.gforge.inria.fr/doc/DamarisUserManual-1.0.1.pdf).

Once that is done, make sure you've cloned caitlinross/ROSS-Vis and checkout the `damaris` branch.  
First you'll need to do some setup with your model to make sure it will correctly link with Damaris and its dependencies.

### Setting up your model
So far this is being tested with Phold, so that model is set.  The basic instructions for setting up with another ROSS model should be the following:

Add the following commands to your `CMakeLists.txt` file in your model directory.
```
IF(USE_DAMARIS)
  INCLUDE_DIRECTORIES(${DAMARIS_DIR}/include ${DAMARIS_DIR}/lib)
ENDIF(USE_DAMARIS)
```

After you've created the executable for your model in `CMakeLists.txt` (e.g., phold uses `ADD_EXECUTABLE`), you'll need to add the following lines (replacing `phold` with the name of your model executable):
```
IF(USE_DAMARIS)
  SET(DAMARIS_LINKER_FLAGS "-rdynamic -L/${DAMARIS_DIR}/lib -Wl,--whole-archive,-ldamaris,--no-whole-archive -lxerces-c -lboost_filesystem -lboost_system -lboost_date_time -lstdc++ -lmpi_cxx -lrt -ldl")
  TARGET_LINK_LIBRARIES(phold ROSS ${DAMARIS_LINKER_FLAGS} ${DAMARIS_LIB} m)
ENDIF(USE_DAMARIS)
```
### Build ROSS
Set ARCH and CC as usual with a ROSS build.  For Damaris, you will also need to set `LD_LIBRARY_PATH`. 
I installed Damaris and all of the dependencies (Xerces-C, etc) to `$HOME/local` as suggested in the Damaris User Guide, so I used `export LD_LIBRARY_PATH=$HOME/local/lib`. 

Now run ccmake (or set options with cmake command).  You should set `CMAKE_BUILD_TYPE` and `CMAKE_INSTALL_PREFIX` as you wish.  Now set `USE_DAMARIS` to `ON` and set `DAMARIS_DIR` appropriately.  It assumes `$HOME/local` as the default.  
Right now I assume that Damaris and its dependencies are all in this same directory for simplicty, but I'll improve it in the future.  
Now configure and generate the files as usual with ccmake.

Now just `make` and `make install` and you should be set to run.

## Running your model with Damaris
As of right now, there are no extra commands added to ROSS for Damaris.  This will probably change as development continues, but for now you run as normal.  
```
mpirun -np 4 ./phold --synch=3 
```
Damaris is configured to run in dedicated-core mode, so for this example run on a single node system, 1 rank will be dedicated to Damaris and 3 ranks dedicated to run ROSS.  If you run on more than one node, 1 rank/node will be dedicated to Damaris and the remaining ranks will be dedicated to ROSS.
ROSS ranks make use of the `MPI_COMM_ROSS` sub communicator.  


