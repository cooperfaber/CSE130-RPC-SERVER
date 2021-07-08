RPCSERVER:

To run, call ./rpcserver -N threads -H buckets -a "address":port -I iterations -d directory, in any order. If the respective arguments are not given, the server will run with 4 threads, 16 buckets, on localhost:8912, with 50 iterations, and with a default directory of "data".

The server is capable of doing 64bit addition,subtraction, multiplication, division, and modulo operations, with a fully formed variable storage table. Every variable in the table is written to a file in the directory given, which the server will read upon launch.Variables can reference a value, or another variable, which can be evaluated by a recursive call to the above operations. Aditionally, file operations read,write,filesize, and create can be performed. Read and write have a 16bit data limit. Finally, the server can delete variables, load a properly formatted file into the var table, dump the var table into a text file, and can clear the entire table with the proper argument.

Currently,  the server will segfault if an invalid network argument (xxx:yyy) is given.

Additionally, a warning is thrown upon running make - as it was present in the skeleton of the code Jim provided us on piazza, I didn't touch it.  The issue is with the line of code Thread threads[numthreads] - C really doesn't enjoy variable length allocation. There is one other warning present - the compiler hates the literal 2^64-1, as it's too large to be stored - but it's neccessary to verify that write will not go out of bounds.

Other than that, if you overload the server (with a low amount of threads), sockets will time out. Adding more threads and buckets speeds the proccesses up a large amount.

