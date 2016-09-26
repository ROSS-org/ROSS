# Welcome to Simplified ROSS!

Welcome to a leaner, meaner, *faster* version of ROSS.
While the entire history of ROSS has been preserved in this repository, a major change in the directory structure has made getting the full history of a file somewhat of a pain.
You may find the now-deprecated version at the [ROSS-Legacy tag](https://github.com/carothersc/ROSS/releases/tag/Legacy) in this repository.
Using this repository you can compare files from the new `ROSS/core` to `ROSS/ross`.
For a detailed list of changes between old ROSS and SR please visit [the wiki](https://github.com/carothersc/ROSS/wiki/Differences-between-Simplified-ROSS-and-ROSS-Legacy).

[![Build Status](https://travis-ci.org/carothersc/ROSS.svg?branch=master)](https://travis-ci.org/carothersc/ROSS)
[![codecov.io](http://codecov.io/github/carothersc/ROSS/coverage.svg?branch=master)](http://codecov.io/github/carothersc/ROSS?branch=master)
[![Doxygen](https://img.shields.io/badge/doxygen-reference-blue.svg)](http://carothersc.github.io/ROSS/ROSS-docs/docs/html)

## History

ROSS's history starts with a one-week re-implementation of [Georgia Tech Time Warp (GTW)](http://www.cc.gatech.edu/computing/pads/tech-parallel-gtw.html) by Shawn Pearce and Dave Bauer in 1999.
After 10 years of in-house development, version 5.0 of [Rensselaer's Optimistic Simulation System](http://sourceforge.net/projects/pdes/) went live at SourceForge.net.
Thus the official version history began!

Through the years ROSS has migrated from CVS, to SVN, to Git and GitHub.com.
The code was maintained by Chris Carothers and his graduate students at RPI ([publications](http://cs.rpi.edu//~chrisc/#publications)).
Over the years, several features (including a shared-memory version) were implemented within ROSS.
Some of these features have since been optimized out, leaving behind cruft.

In early 2015 a sleeker version of ROSS was released.
Developed as Simplified ROSS ([gonsie/SR](http://github.com/gonsie/SR)), this version removed many files, functions, and variables that had become deprecated over time.

## Requirements

1. ROSS is written in C standard and thus requires a C compiler (C11 is prefered, but not required).
1. The build system is [CMake](http://cmake.org), and we require version 2.8 or higher.
2. ROSS relies on MPI.
   We recommend the [MPICH](http://www.mpich.org) implementation.

## Startup Instructions

1. Clone the repository to your local machine:
  ```
  git clone git@github.com:carothersc/ROSS.git
  cd ROSS
  ```

2. *Optional* Install the submodules:
  ```
  git submodule init
  git submodule update
  ```
  Currently, ROSS includes three submodules:
  - [ROSS-Models](http://github.com/carothersc/ROSS-Models) is a set of existing models
  - [template-model](http://github.com/nmcglohon/template-model) is a starting place for new models
  - [RIO](http://github.com/gonsie/RIO) is a work-in-progress checkpointing framework
  

3. *Optional* Symlink your model to ROSS.
Please [this wiki page](https://github.com/carothersc/ROSS/wiki/Constructing-the-Model) for details about creating and integrating a model with ROSS.
  ```
  ln -s ~/path-to/your-existing-model models/your-model-name
  ```

3. Create a build directory.
ROSS developers typically do out-of-tree builds.  See the [Installation page](https://github.com/carothersc/ROSS/wiki/Installation) for more details.
  ```
  cd ~/directory-of-builds/
  mkdir ROSS-build
  cd ROSS-build
  export ARCH=x86_64
  export CC=mpicc
  ccmake ~/path-to/ROSS
  ```

4. Make your model(s) with one of the following commands
  ```
  make -k         // ignore errors from other models
  make -j 12      // parallel build
  make model-name // build only one model
  ```

5. Run your model.
See [this wiki page](https://github.com/carothersc/ROSS/wiki/Running-the-Simulator) for details about the ROSS command line options.
  ```
  cd ~/directory-of-builds/ROSS-build/models/your-model
  ./your-model --synch=1               // sequential mode
  mpirun -np 2 ./your-model --synch=2  // conservative mode
  mpirun -np 2 ./your-model --synch=3  // optimistic mode
  ./your-model --synch=4               // optimistic debug mode (note: not a parallel execution!)
  ```
