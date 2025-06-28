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
#include <zlib.h>

#include "HttpRequest.h"
#include "HttpResponse.h"
#include "HttpParser.h"

#define PORT        4221
#define BUFFER_SIZE 4096

void handle_clent(int client_fd, const std::string& filename);
std::string router(HttpRequest& request, const std::string& filename);
std::vector<char> compress(const std::string& data);
void send_all(int sock, const void* data, size_t size);

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
  while (true) {
    std::vector<char> buffer(BUFFER_SIZE);
    const ssize_t bytes_read = recv(client_fd, buffer.data(), buffer.size(), 0);
    if (bytes_read < 0) {
      std::cerr << "Failed to receive the request.";
      close(client_fd);
      return;
    }

    if (bytes_read == 0) {
      std::cerr << "Client closed the connection.\n";
      close(client_fd);
      return;
    }

    const std::string raw_request(buffer.data(), bytes_read);
    HttpRequest request = HttpParser::parse_http(raw_request);
    std::string response = router(request, filename);
    bool conection_closed = request.version == "HTTP/1.0" || (request.headers.contains("Connection") && request.headers["Connection"] == "close");
    if (conection_closed) {
      const std::string request_line = "HTTP/1.1 200 OK\r\n";
      response = response.substr(request_line.size());
      response = "Connection: close\r\n" + response;
      response = request_line + response;
    }

    send_all(client_fd, response.data(), response.size());

    if (conection_closed) {
      close(client_fd);
      break;
    }
  }

  close(client_fd);
}

std::string router(HttpRequest& request, const std::string& filename) {
  std::cout << "Router: " << request.method << " " << request.path << " " << request.version << "\n";
  if (!request.is_valid()) {
    return HttpResponse::bad_request();
  } else if (request.path == "/") {
    return HttpResponse::ok();
  } else if (request.path.starts_with("/echo")) {
    const std::string endpoint = "/echo/";
    const std::string content = Util::trim(request.path.substr(endpoint.size())); 
    if (request.headers.contains("Accept-Encoding") && request.headers["Accept-Encoding"] == "gzip") {
      std::vector<char> compressed = compress(content);

      std::ostringstream response;
        response << "HTTP/1.1 200 OK\r\n";
        response << "Content-Type: text/plain\r\n";
        response << "Content-Encoding: gzip\r\n";
        response << "Content-Length: " << compressed.size() << "\r\n";
        response << "\r\n";

      std::string header = response.str();
      std::string body(compressed.begin(), compressed.end());
      return header + body;
    } else {
      return HttpResponse::ok(content);
    }
  } else if (request.path == "/user-agent") {
    const std::string endpoint = "/user-agent/";
    const std::string content = request.headers.contains("User-Agent") 
      ? Util::trim(request.headers["User-Agent"]) 
      : "";
    return HttpResponse::ok(content);
  } else if (request.path.starts_with("/files/")) {
    const std::string endpoint = "/files/";
    const std::string content = Util::trim(request.path.substr(endpoint.size())); 
    std::filesystem::path file_path(filename + "/" + content);

    if (request.method == "POST") {
        std::ofstream out(file_path);
        if (out.is_open() && !request.body.empty()) {
          out << request.body;
          return HttpResponse::created();  
        }
    } else {
      std::ifstream in(file_path);

      if (!in.is_open()) {
        return HttpResponse::not_found();
      } else {
        std::ostringstream oss;
        oss << in.rdbuf();
        return HttpResponse::file_ok(oss.str());
      }
    }
  }

  return HttpResponse::not_found();
}

void send_all(int sock, const void* data, size_t size) {
    const char* ptr = static_cast<const char*>(data);
    size_t total = 0;
    while (total < size) {
        ssize_t sent = send(sock, ptr + total, size - total, 0);
        if (sent <= 0) throw std::runtime_error("send failed");
        total += sent;
    }
}

std::vector<char> compress(const std::string& data) {
    z_stream strm{};
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    if (deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 16 + MAX_WBITS, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        throw std::runtime_error("deflateInit2 failed");
    }

    strm.avail_in = data.size();
    strm.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(data.data()));

    std::vector<char> compressed_data;
    const int CHUNK_SIZE = 16384;
    std::vector<char> out_buffer(CHUNK_SIZE);

    int ret;
    do {
        strm.avail_out = CHUNK_SIZE;
        strm.next_out = reinterpret_cast<Bytef*>(out_buffer.data());

        ret = deflate(&strm, Z_FINISH);
        if (ret == Z_STREAM_ERROR) {
            deflateEnd(&strm);
            throw std::runtime_error("deflate failed");
        }

        compressed_data.insert(
            compressed_data.end(),
            out_buffer.begin(),
            out_buffer.begin() + (CHUNK_SIZE - strm.avail_out)
        );
    } while (ret != Z_STREAM_END);

    deflateEnd(&strm);
    return compressed_data;
}
