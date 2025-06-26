#include <cstddef>
#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unordered_map>
#include <vector>
#include <thread>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "HttpRequest.h"
#include "HttpResponse.h"
#include "HttpParser.h"

#define PORT        4221
#define BUFFER_SIZE 4096

void handle_clent(int client_fd, const std::string& filename);

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
  server_addr.sin_port = htons(PORT);
  
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

  std::string filename;
  if (argc == 3 && std::string(argv[1]) == "--directory") {
    filename = std::string_view(argv[2]);
  }

  while (true) {
    const int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
    if (client_fd < 0) {
      std::cerr << "Failed to accept connection\n";
      continue;
    }

    std::cout << "Client connected\n";
    
    std::thread t(handle_clent, client_fd, filename);
    t.detach(); // Allow the thread to run independently
  }

  close(server_fd);

  return 0;
}

void handle_clent(int client_fd, const std::string& filename) {
  std::vector<char> buffer(BUFFER_SIZE);
  const ssize_t bytes_read = recv(client_fd, buffer.data(), buffer.size(), 0);
  if (bytes_read < 0) {
    std::cerr << "Failed to receive the request.";
    close(client_fd);
    return;
  }

  const std::string raw_request(buffer.data(), bytes_read);
  HttpRequest request = HttpParser::parse_http(raw_request);

  std::string response;
  if (!request.is_valid()) {
    response = HttpResponse::bad_request();
  } else if (request.path == "/") {
    response = HttpResponse::ok();
  } else if (request.path.starts_with("/echo")) {
    const std::string endpoint = "/echo/";
    const std::string content = Util::trim(request.path.substr(endpoint.size())); 
    response = HttpResponse::ok(content);
  } else if (request.path == "/user-agent") {
    const std::string endpoint = "/user-agent/";
    const std::string content = request.headers.contains("User-Agent") 
      ? Util::trim(request.headers["User-Agent"]) 
      : "";
    response = HttpResponse::ok(content);
  } else if (request.path.starts_with("/files/")) {
    const std::string endpoint = "/files/";
    const std::string content = Util::trim(request.path.substr(endpoint.size())); 
    std::filesystem::path file_path(filename + "/" + content);

    if (request.method == "POST") {
        std::ofstream out(file_path);
        if (out.is_open() && !request.body.empty()) {
          out << request.body;
          response = HttpResponse::created();  
        }
    } else {
      std::ifstream in(file_path);

      if (!in.is_open()) {
        response = HttpResponse::not_found();
      } else {
        std::ostringstream oss;
        oss << in.rdbuf();
        response = HttpResponse::file_ok(oss.str());
      }
    }
  } else {
    response = HttpResponse::not_found();
  }

  send(client_fd, response.c_str(), response.size(), 0);
  
  close(client_fd);
}