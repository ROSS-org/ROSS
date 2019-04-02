# RIO: A Checkpoint/Restart API for ROSS

RIO (ROSS Restart IO) is a checkpointing API for [Rensselaer's Optimistic Simulation System](https://github.com/ROSS-org/ROSS).
RIO is for checkpoint-restart operations and in its current state it cannot be used to created incremental checkpoints for fault tolerance.

## Limitations

As ROSS is developed, full RIO functionality may be lacking and certain LP and event information may not be saved in a checkpoint.
At this time, the following features are not compatible with RIO:
- delta encoding
- LP suspend
- instrumentation

## Documentation

The documentation for RIO can be found on the ROSS website (Look for the RIO section on the [archive page](https://ROSS-org.github.io/archive.html)).
The documentation includes:

- [Overview](https://ROSS-org.github.io/rio/rio-overview.html)
- [API Description](https://ROSS-org.github.io/rio/rio-api.html)
- [Checkpoint Description](https://ROSS-org.github.io/rio/rio-files.html)
- [Adding RIO to a Model](https://ROSS-org.github.io/rio/rio-cmake.html)

## Example Usage

The full RIO API has been implemented in the [PHOLD-IO model](https://github.com/ROSS-org/pholdio).

## Coding Conventions

RIO API functionality is prefixed with `io`.
