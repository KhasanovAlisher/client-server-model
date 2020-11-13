# Http-like Client-Server Model in Linux-C

## Description
This is a high level API for Linux written in C for the typical http-like one-way client-server model. Sockets have been used to implement the networking functionality which has been simplified
into an easy-to-use interface.

## Compilation
The project can be compiled either by using Make or CMake.   
  
To compile with Make  
```$ make```  
  
To clean up object files  
```$ make clean```

## Documentation
Extensive description of each function can be found in the header files of the library.   
Configurable variables are in the 
[z_net_common.h](z_net_lib/z_net_common.h) and [z_net_common_pvt.h](z_net_lib/z_net_common_pvt.h) files, implemented as defines.

## Author
Doniyorbek "zDonik" Tokhirov
