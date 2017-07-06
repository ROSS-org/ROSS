# RIO: A Checkpoint/Restart API for ROSS

RIO (ROSS Restart IO) is a checkpointing API for [Rensselaer's Optimistic Simulation System](https://github.com/carothersc/ROSS).
RIO is for checkpoint-restart operations and in its current state it cannot be used to created incremental checkpoints for fault tolerance.

## Documentation

The documentation for RIO can be found on the ROSS website (Look for the RIO section on the [archive page](https://carothersc.github.io/ROSS/archive.html)).
The documentation includes:

- [Overview](https://carothersc.github.io/ROSS/rio/rio-overview.html)
- [API Description](https://carothersc.github.io/ROSS/rio/rio-api.html)
- [Checkpoint Description](https://carothersc.github.io/ROSS/rio/rio-files.html)
- [Adding RIO to a Model](https://carothersc.github.io/ROSS/rio/rio-cmake.html)

## Example Usage

The full RIO API has been implemented in the [PHOLD-IO model](https://github.com/gonsie/pholdio).

## Coding Conventions

RIO API functionality is prefixed with `io`.
