Optimized Link State Routing (OLSR) Protocol
============================================

This piece of code is actually a model for the [ROSS](http://odin.cs.rpi.edu)
simulator.  ROSS is an optimistic time warp simulator.  The OLSR model,
however, is currently conservative only.  The implementation currently has
a few restrictions, namely each node only has one interface and all links
are symmetric.