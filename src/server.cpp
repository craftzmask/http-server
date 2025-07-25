#include <format>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unordered_map>
#include <vector>
#include <thread>
#include <filesystem>
#include <sstream>
#include <fstream>

struct HttpRequest {
  std::string method;
  std::string path;
  std::string version;
  std::unordered_map<std::string, std::string> headers;
  std::string body;
};

std::vector<std::string> split_str(const std::string& s, const std::string& delimiter);
HttpRequest parse_request(const std::string& raw_request);
void handle_client(int client_fd, const std::filesystem::path& dir_path);

int main(int argc, char **argv) {
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;
  
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
   std::cerr << "Failed to create server socket\n";
   return 1;
  }
  
  // Since the tester restarts your program quite often, setting SO_REUSEADDR
  // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
    std::cerr << "setsockopt failed\n";
    return 1;
  }
  
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(4221);
  
  if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
    std::cerr << "Failed to bind to port 4221\n";
    return 1;
  }
  
  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
    std::cerr << "listen failed\n";
    return 1;
  }
  
  struct sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);
  
  std::cout << "Waiting for a client to connect...\n";

  while (true) {
    int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
    std::cout << "Client connected\n";

    std::filesystem::path dir_path;
    if (argc == 3 && std::string(argv[1]) == "--directory") {
      dir_path = std::filesystem::path(argv[2]);
    }

    std::thread t(handle_client, client_fd, dir_path);
    t.detach();
  }
  
  close(server_fd);

  return 0;
}

HttpRequest parse_request(const std::string& raw_request) {
  HttpRequest request;
  const std::vector<std::string> lines = split_str(raw_request, "\r\n");
  
  const std::vector<std::string> request_line = split_str(lines[0], " ");
  request.method = request_line[0];
  request.path = request_line[1];
  request.version = request_line[2];

  const int n = request.method == "POST" ? lines.size() - 1 : lines.size();

  for (int i = 1; i < n; i++) {
    if (lines[i].empty()) continue;
    const std::vector<std::string> parts = split_str(lines[i], ": ");
    request.headers[parts[0]] = parts[1];
  }

  request.body = lines[lines.size() - 1];

  return request;
}

std::vector<std::string> split_str(const std::string& s, const std::string& delimiter) {
  std::vector<std::string> tokens;
  size_t start = 0;
  size_t end = s.find(delimiter);

  while (end != std::string::npos) {
    tokens.push_back(s.substr(start, end - start));
    start = end + delimiter.size();
    end = s.find(delimiter, start);
  }

  tokens.push_back(s.substr(start));

  return tokens;
}

void handle_client(int client_fd, const std::filesystem::path& dir_path) {
  char buf[4096];
  ssize_t byte_read = recv(client_fd, buf, sizeof(buf), 0);
  const std::string raw_request(buf, byte_read);

  HttpRequest request = parse_request(raw_request);

  std::string response;
  if (request.path == "/") {
    response = "HTTP/1.1 200 OK\r\n\r\n";
  } else if (request.path.starts_with("/echo/")) {
    const std::string path = "/echo/";
    const std::string content = request.path.substr(path.size());
    response = std::format(
      "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: {}\r\n\r\n{}",
      content.size(),
      content
    );
  } else if (request.path == "/user-agent") {
    const std::string content = request.headers["User-Agent"];
    response = std::format(
      "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: {}\r\n\r\n{}",
      content.size(),
      content
    );
  } else if (request.path.starts_with("/files/")) {
    const std::string path = "/files/";
    const std::string filename = request.path.substr(path.size());
    const std::filesystem::path full_path = dir_path / filename;
    if (request.method == "POST") {
      std::ofstream file(full_path, std::ios::binary);
      file << request.body;
      file.close();
      response = "HTTP/1.1 201 Created\r\n\r\n";
    } else {
      if (std::filesystem::exists(full_path)) {
        std::ifstream file(full_path);
        std::ostringstream ss;
        ss << file.rdbuf();
        
        std::string content = ss.str();
        response = std::format(
          "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: {}\r\n\r\n{}",
          content.size(),
          content
        );
      } else {
        response = "HTTP/1.1 404 Not Found\r\n\r\n";
      }
    }
  } else {
    response = "HTTP/1.1 404 Not Found\r\n\r\n";
  }

  send(client_fd, response.c_str(), response.size(), 0);
}