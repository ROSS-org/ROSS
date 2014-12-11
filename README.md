# Señior: Simplified ROSS

This is a simplified version of Rensselaer's Optimistic Simulation System.

Visit the main branch of ROSS [here](https://github.com/carothersc/ROSS).

Please visit [the wiki](http://github.com/gonsie/SR/wiki) for details on how SR differs from regular ROSS.


## Installation

Be sure you initialize the submodules before you try to build, otherwise you'll get a GetGitRevision error in CMake.
(TODO: make this not a requirement)

```
git submodule init
git submodule update
```

## Usage

There has been little change in how SR is used.
The most obvious is that models and ROSS core functionality code are now in their own directories on the same level (SR/models/ and SR/core/ respectively).
Other than the top level directory name, everything retains the name "ROSS". 
