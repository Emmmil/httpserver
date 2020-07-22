Assignment 2 HTTP Server: eingeman Emil Ingemansson

My file builds using make. To run it, run the executable file with one 
required argument the sevrer address and the second being optional the 
port on the address. If the port is not specified, the default is 80. 
There can be two optional flags. -n [number] to specify the number of threads
and if this flag is not specified the server will defualt to 4. Second, 
the -l [filename] flag to enable sevrer logging and specify the name of the
file the sever will log to. 

EX: ./httpserver [address] [(optional) port] [(optional) -n number] [(optional) -l filename]

MY program seems to work as specified for simple multithreading problems but
runs into errors on more complex things such as when there are more connections than
threads. My parse header function works as specified being able to handle multiple requests from one connection
but in my design I read in one byte at a time so it can be slow. Besides being slow my server
should work as intended for all cases except then more connections than threads. My server
passed all but test 11 when running on the test machine. 
