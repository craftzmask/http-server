#include <cstddef>
#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <vector>
#include <format>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#define BUFFER_SIZE 4096

std::vector<std::string> split(const std::string& s, const std::string& delimiter);
std::string trim(const std::string& s);

int main(int argc, char **argv) {
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;
  
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
   std::cerr << "Failed to create server socket\n";
   return EXIT_FAILURE;
  }
  
  // Since the tester restarts your program quite often, setting SO_REUSEADDR
  // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
    std::cerr << "setsockopt failed\n";
    return EXIT_FAILURE;
  }
  
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(4221);
  
  if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
    std::cerr << "Failed to bind to port 4221\n";
    return EXIT_FAILURE;
  }
  
  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
    std::cerr << "listen failed\n";
    return EXIT_FAILURE;
  }
  
  struct sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);
  
  std::cout << "Waiting for a client to connect...\n";
  
  const int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
  std::cout << "Client connected\n";

  std::vector<char> buffer(BUFFER_SIZE);
  const ssize_t bytes_read = recv(client_fd, buffer.data(), buffer.size(), 0);
  if (bytes_read < 0) {
    std::cerr << "Failed to receive the request.";
    return EXIT_FAILURE;
  }

  std::string request(buffer.data(), bytes_read);
  std::string request_line = split(request, "\r\n\r\n")[0];
  std::string path = split(request_line, " ")[1];

  std::string response;
  if (path == "/") {
    response = "HTTP/1.1 200 OK\r\n\r\n";
  } else if (path.starts_with("/echo/")) {
    const std::string endpoint = "/echo/";
    const std::string content = trim(path.substr(endpoint.size())); 
    response = std::format("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: {}\r\n\r\n{}", content.size(), content);
  } else {
    response = "HTTP/1.1 404 Not Found\r\n\r\n";
  }

  send(client_fd, response.c_str(), response.size(), 0);
  
  close(server_fd);

  return 0;
}

std::vector<std::string> split(const std::string& s, const std::string& delimiter) {
  std::vector<std::string> tokens;
  if (delimiter.empty()) {
    return tokens;  
  }

  size_t start = 0;
  size_t end = s.find(delimiter);

  while (end != std::string::npos) {
    if (start != end) {
      tokens.push_back(s.substr(start, end - start));
    }

    start = end + delimiter.size();
    end = s.find(delimiter, start);
  }

  if (start < s.size()) {
    tokens.push_back(s.substr(start));
  }

  return tokens;
}

std::string trim(const std::string& s) {
  const size_t start = s.find_first_not_of(" \t\n\r\f\v");
  const size_t end = s.find_last_not_of(" \t\n\r\f\v");
  if (start != std::string::npos && end != std::string::npos) {
    return s.substr(start, end - start + 1);
  }

  return "";
}