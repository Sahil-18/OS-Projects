# OS-Projects

These are the OS assignments I have completed during the OS course in my college.
These assignments include developing a basic bash shell and a concurrent web server. 

## Bash Shell
This shell program is written in C++ language.\
Below are the features or functionalities suppoted by the shell program are:\
* Execute a single command.
* Executing multiple commands sequentially.
* Executing multiple commands parallely.
* Executing commands for redirection, eg. cd, cd.. etc.

One of the main task which is required to perform above operations is parsing the commands.
This program parses commands in a 2D arrays of strings.\
Commands are provided by user like command line inputs.

## Concurrent Web Server
This is an assignment which requires to process multiple http request. Such servers require a multi-threaded approach.\
We were provided with the template of web server. We were asked to write code in 'request.c' file.
The part of code which we had to complete were:
* request_handle function
* functions to implement scheduling policies
* buffer to store requests
* multi-threading using mutex locks and condition variables

We had to write code a multi-threaded program which will serve the request based on below two scheduling policies:
* First in first out (FIFO)
* Shortest file first (SFF)
