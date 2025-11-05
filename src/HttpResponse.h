#include <sys/socket.h>
#include <string>
#include <unistd.h>

class HttpResponse {
public:
  HttpResponse(int socket_fd) : socket_fd(socket_fd) {}

  void ok(
    const std::string& body = "",
    const std::string& contentType = "text/plain"
  ) {
    std::string response = respond(body, "200 OK", contentType);
    write(this->socket_fd, response.c_str(), response.size());
  }

  void not_found() {
    std::string response = respond("", "404 Not Found");
    write(this->socket_fd, response.c_str(), response.size());
  }

private:
  std::string respond(
    const std::string& body,
    const std::string& status = "200 OK",
    const std::string& contentType = "text/plain"
  ) {
    return "HTTP/1.1 " + status + "\r\n" +
        "Content-Type: " + contentType + "\r\n" + 
        "Content-Length: " + std::to_string(body.size()) + "\r\n" +
        "Connection: close\r\n" +
        "\r\n" +
        body;
  }

private:
  int socket_fd;
};