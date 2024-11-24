Chat Server and Client
Description
This project implements a simple chat server and client application in C. The server can handle multiple client connections and facilitate communication between them. The client allows users to send messages to each other through the server.

Dependencies
This project requires a C compiler, such as GCC.
It uses the socket programming library in C.
No external libraries are required.

Compiling and Running

Server
To compile the server, use the following command:
gcc -o server.out server.c

To run the server, use the following command:
./server.out
By default, the server listens on port 5566. You can change this port by modifying the PORT variable in the server.c file.

Client
To compile the client, use the following command:
gcc -o client.out client.c

To run the client, use the following command:
./client.out
ports and ip are already hardcoded

Features
Chat History: The server maintains a chat history log file, allowing clients to retrieve conversation history with specific recipients and delete their own chat history.

Chatbot: The server can toggle a chatbot feature using commands "/chatbot login" and "/chatbot logout". When enabled, the chatbot responds to client messages with predefined responses.

Chatbot_v2: uses gpt2 to answer query.

Peer to Peer chat: Use uuid to send messages to other clients