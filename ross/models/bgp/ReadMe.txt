1) one request is splitted to 16 packets, this is temporary
because we want to simulate IOR, where open is called once
but write is called 16 times and then close once.

We should be able to seperate the write/read request from
the data split and make it standard.

In ION LPs, the 16 request is gathered and then call close
once.


2) each write request may be splitted to several small ones
depending on the strip size, write request size and offset.
Unaligned requests have uneven offsets therefore talks to
more FS than aligned ones. This cause more meta confilicts.

The last piece is labeled "IsLastPacket" and then make sure the 
data ack event in FS only trigger one data ack event in ION.

At ION, the data ack is accumulated 16 times before send
out the write finish to CN.

3) Write and read models are seperate. 

