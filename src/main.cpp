#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <thread>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "HttpRequest.h"
#include "HttpResponse.h"

int create_server();
void handle_client(int client_fd);
const std::string readline(int socket_fd);

int main(int argc, char **argv) {
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;
  
  int server_fd = create_server();
  if (server_fd < 0) {
    std::cerr << "Failed to create server socket\n";
    return 1;
  }
  
  struct sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);
  
  while (true) {
    std::cout << "Waiting for a client to connect...\n";
    int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
    std::cout << "Client connected\n";
    std::thread(handle_client, client_fd).detach();
  }

  close(server_fd);

  return 0;
}

int create_server() {
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
   std::cerr << "Failed to create server socket\n";
   return -1;
  }
  
  // Since the tester restarts your program quite often, setting SO_REUSEADDR
  // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
    std::cerr << "setsockopt failed\n";
    return -2;
  }
  
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(4221);
  
  if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
    std::cerr << "Failed to bind to port 4221\n";
    return -3;
  }
  
  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
    std::cerr << "listen failed\n";
    return -4;
  }

  return server_fd;
}

void handle_client(int client_fd) {
  const std::string requestString = readline(client_fd);

  HttpRequest request(requestString);
  HttpResponse response(client_fd);

  if (request.path().find("/echo/") != std::string::npos) {
    response.ok(request.echo_str());
  } else if (request.path().find("/user-agent") != std::string::npos) {
    response.ok(request.header("user-agent"));
  } else if (request.path() != "/") {
    response.not_found();
  } else {
    response.ok();
  }
}

const std::string readline(int socket_fd) {
  char buffer[2048];
  read(socket_fd, buffer, 2048);
  std::string requestString(buffer);
  return requestString;
}