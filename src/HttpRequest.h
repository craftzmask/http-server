#pragma once
#include <string>
#include <unordered_map>

struct HttpRequest {
  std::string method;
  std::string path;
  std::string version;
  std::unordered_map<std::string, std::string> headers;
  std::string body;

  bool is_valid() const {
    return !method.empty() && !path.empty() && !version.empty();
  }
};