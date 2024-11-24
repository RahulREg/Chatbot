#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/wait.h>

#define MAX_CLIENTS 10

typedef struct
{
  int socket;
  char id[37]; // UUID length is 36 characters + '\0'
  int chat;
  int gpt;
} Client;

Client clients[MAX_CLIENTS];
fd_set readfds;
int server_fd;

// Function to generate a random number between min and max (inclusive)
int random_number(int min, int max)
{
  return rand() % (max - min + 1) + min;
}

// Function to generate a random UUID
void generate_uuid(char *uuid)
{
  // Seed the random number generator
  srand(time(NULL));

  // Generate each part of the UUID
  sprintf(uuid, "%04x%04x-%04x-%04x-%04x-%04x%04x%04x",
          random_number(0, 0xffff), random_number(0, 0xffff),
          random_number(0, 0xffff),
          random_number(0, 0x0fff) | 0x4000,
          random_number(0, 0x3fff) | 0x8000,
          random_number(0, 0xffff), random_number(0, 0xffff), random_number(0, 0xffff));
}

#define LOG_FILE "chat_history.log"
void log_message(const char *sender_uuid, const char *receiver_uuid, const char *message)
{
  FILE *log_file;

  // Open log file in append mode
  log_file = fopen(LOG_FILE, "a");
  if (log_file == NULL)
  {
    perror("Error opening log file");
    return;
  }

  // Write message to log file
  if (fprintf(log_file, "%s|||%s|||%s", sender_uuid, receiver_uuid, message) < 0)
  {
    perror("Error writing to log file");
  }

  // Close log file
  fclose(log_file);
}

void retrieve_history(int client_socket, const char *sender_uuid, const char *recipient_uuid)
{
  FILE *log_file;
  char line[1024];
  char sender[1024], recipient[1024], message[1024];
  int found = 0;

  // Open log file for reading
  log_file = fopen(LOG_FILE, "r");
  if (log_file == NULL)
  {
    perror("Error opening log file");
    return;
  }

  // Read each line from the log file
  while (fgets(line, sizeof(line), log_file) != NULL)
  {
    printf("%s\n", line);
    // Parse sender UUID, recipient UUID, and message from the line
    if (sscanf(line, "%[^|||]|||%[^|||]|||%[^\n]", sender, recipient, message) == 3)
    {
      printf("%s %s %s\n", sender, recipient, message);
      // Check if the line corresponds to the requested conversation
      if ((strcmp(sender, sender_uuid) == 0 && strcmp(recipient, recipient_uuid) == 0) ||
          (strcmp(sender, recipient_uuid) == 0 && strcmp(recipient, sender_uuid) == 0))
      {
        // Send the message to the client
        strcat(message, "\n");
        send(client_socket, message, strlen(message), 0);
        found = 1;
      }
    }
  }

  // Close the log file
  fclose(log_file);
  printf("%d\n", client_socket);

  // If no history found, send a message to the client
  if (found == 0)
  {
    send(client_socket, "No history found\n", strlen("No history found\n"), 0);
  }
}

void delete_history(const char *sender_uuid, const char *recipient_uuid)
{
  FILE *log_file;
  FILE *temp_file;
  char line[1024];

  // Open log file for reading
  log_file = fopen(LOG_FILE, "r");
  if (log_file == NULL)
  {
    perror("Error opening log file");
    return;
  }

  // Create a temporary file for writing
  temp_file = fopen("temp.log", "w");
  if (temp_file == NULL)
  {
    perror("Error creating temporary file");
    fclose(log_file);
    return;
  }

  // Read each line from the log file
  while (fgets(line, sizeof(line), log_file) != NULL)
  {
    // Parse sender UUID, recipient UUID, and message from the line
    char sender[1024], recipient[1024];
    if (sscanf(line, "%[^|||]|||%[^|||]", sender, recipient) == 2)
    {
      // Check if the line corresponds to the specified recipient
      if ((strcmp(sender, sender_uuid) == 0 && strcmp(recipient, recipient_uuid) == 0) ||
          (strcmp(sender, recipient_uuid) == 0 && strcmp(recipient, sender_uuid) == 0))
      {
        // printf("1\n");
        continue;
      }
      else
      {
        fputs(line, temp_file);
      }
    }
  }

  // Close files
  fclose(log_file);
  fclose(temp_file);

  // Remove the original log file
  remove(LOG_FILE);

  // Rename the temporary file to the original log file
  rename("temp.log", LOG_FILE);
}

void delete_all_history(const char *sender_uuid)
{
  FILE *log_file;
  FILE *temp_file;
  char line[1024];

  // Open log file for reading
  log_file = fopen(LOG_FILE, "r");
  if (log_file == NULL)
  {
    perror("Error opening log file");
    return;
  }

  // Create a temporary file for writing
  temp_file = fopen("temp.log", "w");
  if (temp_file == NULL)
  {
    perror("Error creating temporary file");
    fclose(log_file);
    return;
  }

  // Read each line from the log file
  while (fgets(line, sizeof(line), log_file) != NULL)
  {
    // Parse sender UUID from the line
    char sender[1024];
    char recipient[1024];
    if (sscanf(line, "%[^|||]|||%[^|||]", sender, recipient) == 2)
    {
      // Check if the line corresponds to the requesting client
      if (strcmp(sender, sender_uuid) != 0 && strcmp(recipient, sender_uuid) != 0)
      {
        // Write the line to the temporary file if it does not match the requesting client
        fputs(line, temp_file);
      }
    }
  }

  // Close files
  fclose(log_file);
  fclose(temp_file);

  // Remove the original log file
  remove(LOG_FILE);

  // Rename the temporary file to the original log file
  rename("temp.log", LOG_FILE);
}

void call_python_script()
{
  pid_t pid;
  int status;

  // Fork a child process
  pid = fork();

  if (pid == 0)
  {
    // Child process
    execlp("python3", "python3", "GPT_inference.py", NULL);
    exit(EXIT_SUCCESS);
  }
  else if (pid < 0)
  {
    // Error forking
    perror("Fork failed");
    exit(EXIT_FAILURE);
  }
  else
  {
    // Parent process
    // Wait for the child process to finish
    waitpid(pid, &status, 0);
    if (WIFEXITED(status))
    {
      // Child process exited normally
      // printf("Python script exited with status %d\n", WEXITSTATUS(status));
    }
    else
    {
      // Child process exited abnormally
      printf("Python script exited abnormally\n");
    }
  }
}

int main()
{

  char *ip = "127.0.0.1";
  int port = 5566;

  int client_sock;
  struct sockaddr_in server_addr, client_addr;
  socklen_t addr_size;
  char buffer[1024];
  int n;

  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0)
  {
    perror("[-]Socket error");
    exit(1);
  }
  printf("[+]TCP server socket created.\n");

  memset(&server_addr, '\0', sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = port;
  server_addr.sin_addr.s_addr = inet_addr(ip);

  n = bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
  if (n < 0)
  {
    perror("[-]Bind error");
    exit(1);
  }
  printf("[+]Bind to the port number: %d\n", port);

  listen(server_fd, 5);
  printf("Listening...\n");

  // fd_set master;
  int max_sd, activity, new_socket;

  while (1)
  {
    FD_ZERO(&readfds);
    FD_SET(server_fd, &readfds);
    max_sd = server_fd;

    // Add client sockets to set
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
      int client_socket = clients[i].socket;
      if (client_socket != 0)
      {
        // printf("1\n");
        FD_SET(client_socket, &readfds);
      }
      if (client_socket > max_sd)
      {
        max_sd = client_socket;
      }
    }

    // printf("File descriptors in the set:\n");
    // for (int fd = 0; fd <= max_sd; fd++)
    // {
    //   if (FD_ISSET(fd, &readfds))
    //   {
    //     printf("%d ", fd);
    //   }
    // }
    // printf("\n");

    // printf("Socket values for each client:\n");
    // for (int i = 0; i < MAX_CLIENTS; i++)
    // {
    //   printf("Client[%d].socket: %d\n", i, clients[i].socket);
    //   printf("Client[%d].id: %s\n", i, clients[i].id);
    // }

    // Wait for an activity on one of the sockets
    activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
    // printf("%d %d\n", activity, max_sd);

    if ((activity < 0))
    {
      printf("Select error\n");
    }

    // If something happened on the master socket, then its an incoming connection
    if (FD_ISSET(server_fd, &readfds))
    {
      if ((new_socket = accept(server_fd, (struct sockaddr *)&client_addr, &addr_size)) < 0)
      {
        perror("accept");
        exit(EXIT_FAILURE);
      }

      printf("New connection, no. is %d, ip is: %s, port: %d\n", new_socket, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

      // Add new connection to clients array
      for (int i = 0; i < MAX_CLIENTS; i++)
      {
        if (clients[i].socket == 0)
        {
          clients[i].socket = new_socket;
          char uuid[37]; // UUID length is 36 characters + '\0'

          generate_uuid(uuid);
          strcpy(clients[i].id, uuid);
          bzero(buffer, 1024);
          strcpy(buffer, uuid);
          strcat(buffer, "\n");
          // sleep(1);
          send(clients[i].socket, buffer, strlen(buffer), 0);
          printf("Adding to list of sockets as %s\n", clients[i].id);
          break;
        }
      }
    }

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
      if (FD_ISSET(clients[i].socket, &readfds))
      {
        bzero(buffer, 1024);
        n = recv(clients[i].socket, buffer, sizeof(buffer), 0);

        if (n == 0)
        {
          // Client disconnected
          printf("[+] Client %s disconnected.\n", clients[i].id);
          close(clients[i].socket);
          clients[i].socket = 0;
          strcpy(clients[i].id, "");
          clients[i].chat = 0;
          clients[i].gpt = 0;
        }
        else if (n < 0)
        {
          perror("[-]recv error");
          close(clients[i].socket);
          clients[i].socket = -1;
        }
        else if (clients[i].gpt == 1)
        {
          if (strncmp(buffer, "/chatbot_v2 logout", strlen("/chatbot_v2 logout")) == 0)
          {
            clients[i].gpt = 0;
            bzero(buffer, 1024);
            strcpy(buffer, "gpt2bot> Bye! Have a nice day and hope you do not have any complaints about me\n");
            send(clients[i].socket, buffer, strlen(buffer), 0);
            continue;
          }
          FILE *file = fopen("gpt.txt", "w");
          if (file == NULL)
          {
            perror("Error opening file");
            return 1;
          }

          fprintf(file, "%s", buffer);
          fclose(file);

          call_python_script();

          FILE *fp;
          bzero(buffer, 1024);

          fp = fopen("gpt.txt", "r");
          if (fp == NULL)
          {
            perror("Error opening file");
            exit(EXIT_FAILURE);
          }

          char temp[1024];
          strcpy(temp, "gpt2bot> ");

          // Read file content into buffer
          fgets(buffer, sizeof(buffer), fp);
          fgets(buffer, sizeof(buffer), fp);
          while (fgets(buffer, sizeof(buffer), fp) != NULL)
          {
            strcat(temp, buffer); // Append buffer to file_content
          }

          fclose(fp);

          strcat(temp, "\n");

          send(clients[i].socket, temp, strlen(temp), 0);
        }
        else if (clients[i].chat == 1)
        {
          if (strncmp(buffer, "/chatbot logout", strlen("/chatbot logout")) == 0)
          {
            clients[i].chat = 0;
            bzero(buffer, 1024);
            strcpy(buffer, "stupidbot> Bye! Have a nice day and do not complain about me\n");
            send(clients[i].socket, buffer, strlen(buffer), 0);
            continue;
          }
          FILE *file = fopen("FAQs.txt", "r");
          if (file == NULL)
          {
            perror("Error opening file");
            return 1;
          }
          char line[1024];
          int f = 0;
          while (fgets(line, sizeof(line), file))
          {
            // printf("%s\n", line);
            int s = 0;
            char t1[1024];
            char t2[1024];
            for (int j = 0; j < strlen(line) - 2; j++)
            {
              if (line[j] == '|' && line[j + 1] == '|' && line[j + 2] == '|')
              {
                s = j;
                break;
              }
            }
            // printf("1\n");
            strncpy(t1, line, s);
            t1[s - 1] = '\0';
            if (strncmp(t1, buffer, strlen(buffer) - 1) == 0)
            {
              // printf("2\n");
              // printf("%ld %ld\n", strlen(t1), strlen(buffer));
              if (strlen(t1) == (strlen(buffer) - 1))
              {
                f = 1;
                strcpy(t2, line + (s + 4));
                bzero(buffer, 1024);
                strcpy(buffer, "stupidbot> ");
                strcat(buffer, t2);
                send(clients[i].socket, buffer, strlen(buffer), 0);
                // send(clients[i].socket, t2, strlen(t2), 0);
                break;
              }
            }
          }
          if (f == 0)
          {
            bzero(buffer, 1024);
            strcpy(buffer, "stupidbot> System Malfunction, I couldn't understand your query.\n");
            send(clients[i].socket, buffer, strlen(buffer), 0);
          }
          fclose(file);
        }
        else
        {
          // Handle client commands
          if (buffer[0] == '/')
          {
            // Process commands
            if (strncmp(buffer, "/active", strlen("/active")) == 0)
            {
              // Send list of active clients
              // char active_list[1024];
              // int list_count = 0;
              bzero(buffer, 1024);
              // snprintf(active_list, sizeof(active_list), "[*] Active clients:\n");
              for (int j = 0; j < MAX_CLIENTS; j++)
              {
                if (clients[j].socket != 0)
                {
                  // list_count++;
                  // strcpy(buffer, clients[j].socket);
                  char temp[100];
                  sprintf(temp, "%d", clients[j].socket);
                  strcat(buffer, temp);
                  strcat(buffer, ", ");
                  strcat(buffer, clients[j].id);
                  strcat(buffer, "\n");
                  // send(clients[i].socket, buffer, strlen(buffer), 0);
                  // printf("%s\n", buffer);
                }
              }
              // if (list_count == 0)
              // {
              //   strcat(buffer, "  - No clients currently active.\n");
              //   // send(clients[i].socket, buffer, strlen(buffer), 0);
              // }
              send(clients[i].socket, buffer, strlen(buffer), 0);
            }
            else if (strncmp(buffer, "/send", 5) == 0)
            {
              int s = 0;
              char temp[1024];
              char t1[37];
              for (int j = 6; j < strlen(buffer); j++)
              {
                if (buffer[j] == ' ')
                {
                  s = j;
                  break;
                }
              }
              s = s - 6;
              strncpy(t1, buffer + 6, s);
              t1[s] = '\0';
              // int n = atoi(temp);
              int l = strlen(buffer) - (6 + s + 1);
              strncpy(temp, buffer + (6 + s + 1), l);
              temp[l] = '\0';
              // printf("%d %s\n", n, temp);
              for (int j = 0; j < MAX_CLIENTS; j++)
              {
                if (strncmp(clients[j].id, t1, strlen(t1)) == 0)
                {
                  send(clients[j].socket, temp, strlen(temp), 0);
                  log_message(clients[i].id, t1, temp);
                }
              }
            }
            else if (strncmp(buffer, "/history_delete", strlen("/history_delete")) == 0)
            {
              // int s = 0;
              char t1[100];
              // printf("%d\n", s);
              strcpy(t1, buffer + 16);
              t1[36] = '\0';
              // printf("%s\n", t1);
              delete_history(clients[i].id, t1);
            }
            else if (strncmp(buffer, "/history", strlen("/history")) == 0)
            {
              // int s = 0;
              char t1[100];
              // printf("%d\n", s);
              strcpy(t1, buffer + 9);
              t1[36] = '\0';
              // printf("%s\n", t1);
              retrieve_history(clients[i].socket, clients[i].id, t1);
            }
            else if (strncmp(buffer, "/delete_all", strlen("/delete_all")) == 0)
            {
              delete_all_history(clients[i].id);
            }
            else if (strncmp(buffer, "/chatbot login", strlen("/chatbot login")) == 0)
            {
              clients[i].chat = 1;
              bzero(buffer, 1024);
              strcpy(buffer, "stupidbot> Hi, I am stupid bot, I am able to answer a limited set of your questions\n");
              send(clients[i].socket, buffer, strlen(buffer), 0);
            }
            else if (strncmp(buffer, "/chatbot_v2 login", strlen("/chatbot_v2 login")) == 0)
            {
              clients[i].gpt = 1;
              bzero(buffer, 1024);
              strcpy(buffer, "gpt2bot> Hi, I am updated bot, I am able to answer any question be it correct or incorrect\n");
              send(clients[i].socket, buffer, strlen(buffer), 0);
            }
          }
        }
      }
    }

    // addr_size = sizeof(client_addr);
    // client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_size);
    // printf("[+]Client connected.\n");

    // bzero(buffer, 1024);
    // recv(client_sock, buffer, sizeof(buffer), 0);
    // printf("Client: %s\n", buffer);

    // bzero(buffer, 1024);
    // strcpy(buffer, "HI, THIS IS SERVER. HAVE A NICE DAY!!!");
    // printf("Server: %s\n", buffer);
    // send(client_sock, buffer, strlen(buffer), 0);

    // close(client_sock);
    // printf("[+]Client disconnected.\n\n");
  }
  return 0;
}