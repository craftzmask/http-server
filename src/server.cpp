#include <cstddef>
#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unordered_map>
#include <vector>
#include <format>
#include <thread>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#define BUFFER_SIZE 4096

struct HttpRequest {
  std::string method;
  std::string path;
  std::string version;
  std::unordered_map<std::string, std::string> headers;

  bool is_valid() const {
    return !method.empty() && !path.empty() && !version.empty();
  }
};

std::vector<std::string> split(const std::string& s, const std::string& delimiter);
std::string trim(const std::string& s);
HttpRequest parse_http(const std::string& raw_request);

std::string ok(const std::string_view content);
std::string ok();
std::string not_found();
std::string bad_request();

void handle_clent(int client_fd);

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

  while (true) {
    const int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
    if (client_fd < 0) {
      std::cerr << "Failed to accept connection\n";
      continue;
    }

    std::cout << "Client connected\n";
    
    std::thread t(handle_clent, client_fd);
    t.detach(); // Allow the thread to run independently
  }

  close(server_fd);

  return 0;
}

void handle_clent(int client_fd) {
  std::vector<char> buffer(BUFFER_SIZE);
  const ssize_t bytes_read = recv(client_fd, buffer.data(), buffer.size(), 0);
  if (bytes_read < 0) {
    std::cerr << "Failed to receive the request.";
    close(client_fd);
    return;
  }

  const std::string raw_request(buffer.data(), bytes_read);
  HttpRequest request = parse_http(raw_request);

  std::string response;
  if (!request.is_valid()) {
    response = bad_request();
  } else if (request.path == "/") {
    response = ok();
  } else if (request.path.starts_with("/echo")) {
    const std::string endpoint = "/echo/";
    const std::string content = trim(request.path.substr(endpoint.size())); 
    response = ok(content);
  } else if (request.path == "/user-agent") {
    const std::string endpoint = "/user-agent/";
    const std::string content = request.headers.contains("User-Agent") 
      ? trim(request.headers["User-Agent"]) 
      : "";
    response = ok(content);
  } else {
    response = not_found();
  }

  send(client_fd, response.c_str(), response.size(), 0);
  
  close(client_fd);
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

HttpRequest parse_http(const std::string& raw_request) {
  HttpRequest request;

  const auto lines = split(raw_request, "\r\n");
  if (lines.empty()) {
    return request;
  }
  
  const auto request_line =  split(lines[0], " ");
  if (request_line.size() != 3) {
    std::cerr << "Malformed request line: " << lines[0] << '\n';
    return request;
  }

  request.method = request_line[0];
  request.path = request_line[1];
  request.version = request_line[2];

  for (int i = 1; i < lines.size(); i++) {
    if (lines[i].find(':') != std::string::npos) {
      const auto header = split(lines[i], ":");
      if (header.size() == 2) {
        const auto key= trim(header[0]);
        const auto value = trim(header[1]);
        request.headers[key] = trim(value);
      }
    }
  }

  return request;
}

std::string ok(const std::string_view content) {
  return std::format(
    "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: {}\r\n\r\n{}", 
    content.size(), 
    content
  );
}

std::string ok() {
  return "HTTP/1.1 200 OK\r\n\r\n";
}

std::string not_found() {
  return "HTTP/1.1 404 Not Found\r\n\r\n";
}

std::string bad_request() {
  return "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
}