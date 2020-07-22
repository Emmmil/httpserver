Assignment 3 HTTP Server: (cruzid: eingeman) Emil Ingemansson

My file builds using make. To run it, run the executable file with one 
required argument the sevrer address and the second being optional the 
port on the address. If the port is not specified, the default is 80. 
There can be two optional flags. -n [number] to specify the number of threads
and if this flag is not specified the server will defualt to 4. Second, 
the -l [filename] flag to enable sevrer logging and specify the name of the
file the sever will log to. Lastly, the -a [filename] flag MUST be specified.
If the name of the file does not exist the server will create the file and
if the file does exist, it will check it's magic number to see if the file is
compatible. 

EX: ./httpserver [address] [(optional) port] [(optional) -n number] [(optional) -l filename] [(REQUIRED) -a filename]

MY program seems to work as specified. On my last test, I passed all tests except for test3 from assignment 2 which I never implemented.  
