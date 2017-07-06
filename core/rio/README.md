# RIO: A Checkpointing API for ROSS

RIO (originally ROSS-IO) is a checkpointing API for [Rensselaer's Optimistic Simulation System](http://github.com/carothersc/ROSS).
RIO is for checkpoint-restart operations, it cannot be used to created incremental checkpoints for fault tolerance.

## Documentation

The documentation for RIO can be found on the ROSS website (Look for the RIO section on the [archive page](http://carothersc.github.io/ROSS/archive.html)).
The documentation includes:

- [Overview](http://carothersc.github.io/ROSS/rio/rio-overview.html)
- [API Description](http://carothersc.github.io/ROSS/rio/rio-api.html)
- [Checkpoint Description](http://carothersc.github.io/ROSS/rio/rio-files.html)
- [Adding RIO to a Model](http://carothersc.github.io/ROSS/rio/rio-cmake.html)

## Example Usage

The full RIO API has been implemented in the [PHOLD-IO model](https://github.com/gonsie/pholdio).
