#include <string>
#include <unordered_map>
#include <vector>

#include "Util.h"

class HttpRequest {

public:
  HttpRequest(const std::string& requestString) {
    std::vector<std::string> parts = Util::split(requestString, "\r\n");
    std::vector<std::string> requestLineParts = Util::split(parts[0], " ");

    this->method_ = requestLineParts[0];
    this->path_ = requestLineParts[1];
    this->version_ = requestLineParts[2];
    
    for (int i = 1; i < parts.size(); i++) {
      const std::string& headerLine = parts[i];
      if (headerLine.empty()) {
        break;
      }

      const std::vector<std::string> headerParts = Util::split(headerLine, ": ");
      const std::string& headerName = Util::lower(headerParts[0]);
      const std::string& headerValue = headerParts[1];
      this->headers_[headerName] = headerValue;
    }
  }

  const std::string& version() { return version_; }

  const std::string& method() { return method_; }

  const std::string& path() { return path_; }

  const std::string& body() { return body_; }

  const std::string echo_str() {
    std::string path = "/echo/";
    std::string str = path_.substr(path.size());
    return str;
  }

  const std::string header(const std::string& name) {
    if (headers_.contains(name)) {
      return headers_[Util::lower(name)];
    }
    return {};
  }

private:
  std::string version_;
  std::string method_;
  std::string path_;
  std::string body_;
  std::unordered_map<std::string, std::string> headers_;
};