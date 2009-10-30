#SET(QUEUE      calendar) #Calendar Queue
#SET(QUEUE      heap) #Push/Down Heap
SET(QUEUE       splay) #Splay Tree

#SET(MAPPING      none)
SET(MAPPING       linear)
#SET(MAPPING      rrobin)

SET(RAND           clcg4)

#MEMORY is either on or off depending on whether or not we desire
#memory buffers.  If it's not set to YES, it defaults to NO
SET(MEMORY        YES)

SET(NETWORK       mpi)
#SET(NETWORK      tcp)
#SET(NETWORK      none)

#SET(THREAD       pthread)
SET(THREAD        none)

#SET(CLOCK         amd64)
#SET(CLOCK        i386)
#SET(CLOCK        ppc)

SET(GVT           mpi_allreduce)
#SET(GVT                  7oclock)
#SET(GVT                  none)

SET(ARCH bgl)
#SET(ARCH bgp)
#SET(ARCH x86_64)
#SET(ARCH ppc64)

ADD_SUBDIRECTORY(ross)
ADD_SUBDIRECTORY(rnf)
ADD_SUBDIRECTORY(models)
