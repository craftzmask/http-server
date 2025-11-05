#include <algorithm>
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

struct HttpRequest {
  std::string version;
  std::string method;
  std::string path;
  std::string body;
  std::unordered_map<std::string, std::string> headers;
};

std::vector<std::string> split(const std::string& s, const std::string& deli);
HttpRequest parseRequest(const std::string& requestString);
std::string respond(
  const std::string& body,
  const std::string& status = "200 OK",
  const std::string& contentType = "text/plain"
);
std::string lower(std::string s);

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
  
  int socket_fd = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
  std::cout << "Client connected\n";
  
  char buffer[2048];
  read(socket_fd, buffer, 2048);

  std::string requestString(buffer);

  HttpRequest request = parseRequest(requestString);

  std::string msg = respond("");
  if (request.path.find("/echo/") != std::string::npos) {
    std::string path = "/echo/";
    std::string str = request.path.substr(path.size());
    msg = respond(str);
  } else if (request.path.find("/user-agent") != std::string::npos) {
    const std::string& userAgent = request.headers["user-agent"];
    msg = respond(userAgent);
  } else if (request.path != "/") {
    msg = respond("", "404 Not Found");
  }

  write(socket_fd, msg.c_str(), msg.size());

  close(socket_fd);
  close(server_fd);

  return 0;
}

std::vector<std::string> split(const std::string& s, const std::string& deli) {
  std::vector<std::string> tokens;
  size_t start = 0;
  size_t end = s.find(deli);

  if (deli.empty()) {
    tokens.push_back(s);
    return tokens;
  }

  while (end != std::string::npos) {
    tokens.push_back(s.substr(start, end - start));
    start = end + deli.size();
    end = s.find(deli, start); 
  }

  tokens.push_back(s.substr(start));
  
  return tokens;
}

HttpRequest parseRequest(const std::string& requestString) {
  std::vector<std::string> parts = split(requestString, "\r\n");
  std::vector<std::string> requestLineParts = split(parts[0], " ");

  HttpRequest request;
  request.method = requestLineParts[0];
  request.path = requestLineParts[1];
  request.version = requestLineParts[2];
  
  for (int i = 1; i < parts.size(); i++) {
    const std::string& headerLine = parts[i];
    if (headerLine.empty()) {
      break;
    }

    const std::vector<std::string> headerParts = split(headerLine, ": ");
    const std::string& headerName = lower(headerParts[0]);
    const std::string& headerValue = headerParts[1];
    request.headers[headerName] = headerValue;
  }

  return request;
}

std::string respond(
  const std::string& body,
  const std::string& status,
  const std::string& contentType
) {
  return "HTTP/1.1 " + status + "\r\n" +
         "Content-Type: " + contentType + "\r\n" + 
         "Content-Length: " + std::to_string(body.size()) + "\r\n\r\n" +
          body;
}

std::string lower(std::string s) {
  std::transform(
    s.begin(), 
    s.end(), 
    s.begin(), 
    [](unsigned char c) { return std::tolower(c); }
  );
  return s;
}