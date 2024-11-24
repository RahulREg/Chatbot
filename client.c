#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main()
{

  char *ip = "127.0.0.1";
  int port = 5566;

  int sock;
  struct sockaddr_in addr;
  socklen_t addr_size;
  char buffer[1024];
  int n;

  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
  {
    perror("[-]Socket error");
    exit(1);
  }
  printf("[+]TCP server socket created.\n");

  memset(&addr, '\0', sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = port;
  addr.sin_addr.s_addr = inet_addr(ip);

  connect(sock, (struct sockaddr *)&addr, sizeof(addr));
  printf("Connected to the server.\n");

  // Set up file descriptor sets for select
  fd_set readfds;
  // FD_ZERO(&readfds);
  // FD_SET(STDIN_FILENO, &readfds);  // Standard input
  // FD_SET(sock, &readfds); // Client socket

  // Buffer for user input
  // char input_buffer[1024];
  int max_fd = sock + 1;
  int chat = 0;

  while (1)
  {
    // fd_set tmpfds = readfds; // Create a copy of readfds since select modifies it
    fd_set tmpfds;
    FD_ZERO(&tmpfds);
    FD_SET(STDIN_FILENO, &tmpfds); // Standard input
    FD_SET(sock, &tmpfds);         // Client socket

    // Use select to wait for input from standard input or the server
    int activity = select(max_fd, &tmpfds, NULL, NULL, NULL);
    if (activity == -1)
    {
      perror("Select error");
      exit(EXIT_FAILURE);
    }

    // Check if standard input has data to read
    if (FD_ISSET(STDIN_FILENO, &tmpfds))
    {
      fgets(buffer, sizeof(buffer), stdin); // Read input from user
      if (strncmp(buffer, "/logout", strlen("/logout")) == 0)
      {
        close(sock);
        printf("Disconnected from the server.\n");
        return 0;
      }
      else if (strncmp(buffer, "/chatbot login", strlen("/chatbot login")) == 0 ||
               strncmp(buffer, "/chatbot_v2 login", strlen("/chatbot_v2 login")) == 0)
      {
        chat = 1;
      }
      else if (strncmp(buffer, "/chatbot logout", strlen("/chatbot logout")) == 0 ||
               strncmp(buffer, "/chatbot_v2 logout", strlen("/chatbot_v2 logout")) == 0)
      {
        chat = 0;
      }
      send(sock, buffer, strlen(buffer), 0); // Send message to server
    }

    // Check if the client socket has data to read
    if (FD_ISSET(sock, &tmpfds))
    {
      memset(buffer, 0, sizeof(buffer));                          // Clear the buffer
      int bytes_received = recv(sock, buffer, sizeof(buffer), 0); // Receive data from server
      if (bytes_received > 0)
      {
        printf("%s", buffer); // Print received data
      }
      else if (bytes_received == 0)
      {
        printf("Server disconnected\n");
        break;
      }
      else
      {
        perror("Receive error");
        exit(EXIT_FAILURE);
      }
      if (chat == 1)
      {
        printf("user> ");
        fflush(stdout);
      }
    }
  }

  close(sock);
  printf("Disconnected from the server.\n");

  return 0;
}