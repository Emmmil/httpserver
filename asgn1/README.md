Assignment 1 HTTP Server: eingeman Emil Ingemansson

My file builds using make. To run it, run the executable file with one 
required argument the sevrer address and the second being optional the 
port on the address. If the port is not specified, the default is 80. 

EX: ./httpserver [address] [(optional) port]

My program works as specified but does not take in multiple files.
If I needed to take in multiple files, I would need to read the content 
from the first header and then check if there is more bytes to be read. 
If there are more bytes to be read, then I can restart the process of handling
a single request which I did in my program. 
