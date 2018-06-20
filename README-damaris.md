## Using Damaris with ROSS
[Damaris](https://project.inria.fr/damaris) is an I/O and data management software.
Support for Damaris is currently being added to ROSS to help manage extensions for in situ data analysis and visualization. 
This readme contains some basic info for how to set-up ROSS to use Damaris.

## Building ROSS with Damaris
ROSS is currently up to date with [Damaris version 1.2.0](https://project.inria.fr/damaris/download).
For Damaris installation instructions, see the [Damaris User Guide](https://project.inria.fr/damaris/documentation).

Once that is done, make sure you've cloned caitlinross/ROSS-Vis and checkout the `damaris` branch.  
First you'll need to do some setup with your model to make sure it will correctly link with Damaris and its dependencies.

### Setting up your model
So far this is being tested with PHOLD, so that model is set.  The basic instructions for setting up with another ROSS model should be the following:

Add the following commands to your `CMakeLists.txt` file in your model directory.
```
IF(USE_DAMARIS)
  INCLUDE_DIRECTORIES(${DAMARIS_DIR}/include)
ENDIF(USE_DAMARIS)
```

After you've created the executable for your model in `CMakeLists.txt` (e.g., phold uses `ADD_EXECUTABLE`), you'll need to add the following lines (replacing `phold` with the name of your model executable):
```
IF(USE_DAMARIS)
  SET(DAMARIS_LINKER_FLAGS "-rdynamic -L/${DAMARIS_DIR}/lib -Wl,--whole-archive,-ldamaris,--no-whole-archive \
    -lxerces-c -lboost_log -lboost_log_setup -lboost_filesystem -lboost_system -lboost_date_time -lboost_thread -lstdc++ -lmpi_cxx -lrt -ldl")
  TARGET_LINK_LIBRARIES(phold ROSS ${DAMARIS_LINKER_FLAGS} ${DAMARIS_LIB} m)
ENDIF(USE_DAMARIS)
```
### Build ROSS
Set ARCH and CC as usual with a ROSS build.  For Damaris, you will also need to set `LD_LIBRARY_PATH`. 
I installed Damaris and all of the dependencies (Xerces-C, etc) to `$HOME/local` as suggested in the Damaris User Guide, so I used `export LD_LIBRARY_PATH=$HOME/local/lib`. 

Now run ccmake (or set options with cmake command).  You should set `CMAKE_BUILD_TYPE` and `CMAKE_INSTALL_PREFIX` as you wish.  Now set `USE_DAMARIS` to `ON` and set `DAMARIS_DIR` appropriately.  It assumes `$HOME/local` as the default.  
We assume that Damaris and its dependencies are all in this same directory for simplicty.  
Now configure and generate the files as usual with ccmake.

Now just `make` and `make install` and you should be set to run.

## Running your model with Damaris
As of right now, there are no extra commands added to ROSS for Damaris.  This will probably change as development continues, but for now you run as normal.  
```
mpirun -np 4 ./phold --synch=3 
```
Damaris is configured to run in dedicated-core mode, so for this example run on a single node system, 1 rank will be dedicated to Damaris and 3 ranks dedicated to run ROSS.  If you run on more than one node, 1 rank/node will be dedicated to Damaris and the remaining ranks will be dedicated to ROSS.
ROSS ranks make use of the `MPI_COMM_ROSS` sub communicator.  

### Setting up ROSS/Damaris with VisIt
My current setup is running simulations on a remote system, so I had to install VisIt on that system as well as my local system (need to make sure they are the same version).  
We're using VisIt version 2.12.1. [VisIt can be downloaded here](https://wci.llnl.gov/simulation/computer-codes/visit/downloads)
For the local system, I just downloaded the VisIt executable available for my OS.  
For the remote system, it's best to download the source so we can change some of the configuration.  You should be able to just download the `build_visit` script for the version you want, which will actually download the VisIt source for you.
This is the configuration I used for building VisIt:
```
./build_visit2_12_1 --parallel --mesa --server-components-only --makeflags '-j 16'
```
If you're setting up on your local system, you probably want to remove the --server-components-only arg.  The build can take a couple of hours because the script downloads and installs dependencies.  

Now that VisIt is set up, you need to rebuild Damaris.  See the Damaris User Guide for building with VisIt support.  
Now you can build and link your model, but you'll need to add libsimV2 to `DAMARIS_LINKER_FLAGS`:
```
SET(DAMARIS_LINKER_FLAGS "-rdynamic -L/${DAMARIS_DIR}/lib -Wl,--whole-archive,-ldamaris,--no-whole-archive \
  -lxerces-c -lboost_log -lboost_log_setup -lboost_filesystem -lboost_system -lboost_date_time -lboost_thread -lstdc++ -lpthread -lmpi_cxx -lrt -ldl \ 
  -L/path/to/visit2.12.1/src/lib -lsimV2")
```

## Running ROSS with Damaris and VisIt
In the `ROSS-Vis/damaris` directory, there is a test.xml file.  This describes the data to Damaris.  The only thing you need to check here is that the path listed for VisIt is correct.  It should look like:
```
<visit>
    <path>/path/to/visit2.12.1/src</path>
</visit>
```
On your local system, you need to set up a host profile so you can connect to VisIt on the remote system.
Go to Options and then Host Profiles.
Fill out Remote Host name, Path to Visit, Username, and check tunnel data connections through SSH.
Then click launch profiles tab.
Set a profile name and then on the Parallel tab, I checked Launch parallel engine along with setting Parallel launch method to mpirun.
You can then set number of desired processors/nodes.  Click Apply and then Dismiss.

On the VisIt window, click Open under Sources.
Change Host to the Host you just created a profile for.
Enter your password when prompted and the path should change to your home directory on that system.
It will also start the VisIt processes running on the remote system.
Go into the .visit/simulations Directory.
The file list will be empty at this point.
Now you can run your simulation as described above for running with Damaris, but we need to turn on instrumentation (see next paragraph).
Click the refresh button on the File open window and there should be a file that starts with some number followed by `ROSS_test.sim2`.
Click on that and then OK.
Now you can try out different visualizations and choose the variables you want to examine.  

Right now the command line arguments for turning on instrumentation are the same as before (if you have ROSS built to use Damaris, it just exposes the data to Damaris instead of writing out any data to file).
You can use this to run with both GVT-based and real time instrumentation (together or independently).
See the [ROSS Instrumentation Documentation](http://carothersc.github.io/ROSS/instrumentation/instrumentation.html) for instructions on using the instrumenation.
*Note*: Damaris is not yet setup to use the event tracing data or virtual time sampling data, yet.
Those features should be avaliable soon!
Another side note, `--granularity=n` has no effect here as when Damaris is turned on, the instrumentation collects everything at the finest granularity.  
Setting `--num-gvt` between 100 and 500 has worked well for the PHOLD model. (This has not yet been tested with CODES.)



