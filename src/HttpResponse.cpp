#include "HttpResponse.h"
#include <format>

std::string HttpResponse::ok(const std::string_view content, const std::string_view content_type) {
  return std::format(
    "HTTP/1.1 200 OK\r\nContent-Type: {}\r\nContent-Length: {}\r\n\r\n{}",
    content_type,
    content.size(),
    content
  );
}

std::string HttpResponse::ok(const std::string_view content) {
  return ok(content, "text/plain");
}

std::string HttpResponse::ok() {
  return "HTTP/1.1 200 OK\r\n\r\n";
}

std::string HttpResponse::not_found() {
  return "HTTP/1.1 404 Not Found\r\n\r\n";
}

std::string HttpResponse::bad_request() {
  return "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
}

std::string HttpResponse::file_ok(const std::string_view file_content) {
  return ok(file_content, "application/octet-stream");
}

std::string HttpResponse::created() {
  return "HTTP/1.1 201 Created\r\n\r\n";
}