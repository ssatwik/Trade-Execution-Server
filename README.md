# TCP Trade Execution Server

## Overview

This project implements a trading system where a central server (server.cpp) handles trade orders from multiple clients (client.cpp). The system allows clients to place, modify, or cancel orders for financial instruments, as well as view existing orders. The server processes all incoming requests parallely.

## Usage

1) Download the repository or the required files (server.cpp and client.cpp).

2) Start the server:
  Open a terminal in the repository folder, compile the server code, and run it. By default, it listens on port 5555, but this can be changed in the code.
```bat
$ g++ server.cpp -o server
$ ./server
```

3) Run the client:
   Open a separate terminal, compile the client, and start it. The client will connect to the server and allow users to send trade-related commands.
```bat
$ g++ client.cpp -o client
$ ./client
```
4) Multiple clients can connect:
   The system supports multiple traders. Just open additional terminals and run more instances of the client executable.

5) Alternatively, you could download all the files in the repository and run the bash script `execute.sh`, followed by the file name (`server.cpp` or `client.cpp`) and it runs the executable of it.
Using the script for easier execution
   If you have all the project files, you can use execute.sh to compile and run either the server or client without manually typing out the commands.
 ```bat
 $ ./execute.sh server.cpp
 ```
 ```bat
 $ ./execute.sh client.cpp
 ```

