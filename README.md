# ListeningServerC
#### A file server in C++ utilizing TCP sockets for linux computers.

The client must send a request in the form `GET <FILEPATH ON SERVER>`

This server works as follows:
  1. It sets up its memory and configures the server to use TCP sockets.
  2. It pulls a linked list of structures that contains the host's address information
  3. The program then loops through the address structures, trying to find one that it can use. It first checks if it can set up a TCP socket using the address, then if it can, it configures the socket to be able to reuse the local address(removed by default due to a security issue). Lastly it binds the socket to one of the computer's network ports that allow it to communicate with the outside world.
  4. It gets rid of the list of address information because is of no use since it has found the working one
  5. It then configures the socket to 'listen' for client connections on the port it is using.
  6. It then continues to try and accept connecctions and if one is present, it creates a separate connection with the client on another socket.
  7. The program then receives a message from the client in the form `GET <FILEPATH ON SERVER>` and translates that into a request to send the contents of the file to the user.
  8. It takes the file contents and sends them to the user.
  9. The connection is closed and the program returns to step 6.
  
 #### What is a socket?
 Socket programming is a way of connecting two nodes on a network to communicate with each other. One socket(node) listens on a particular port at an IP, while other socket reaches out to the other to form a connection. Server forms the listener socket while client reaches out to the server.
 
 #### What is a TCP Socket?
 A TCP socket is a socket that streams data over the connection. This is used for sending information in an orderly fashion in which data that is send first must show up first. This is useful for sending file contents because you want the information to be in order.
