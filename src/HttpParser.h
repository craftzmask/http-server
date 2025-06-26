#pragma once
#include "HttpRequest.h"
#include "Util.h"
#include <iostream>

struct HttpParser {
  static HttpRequest parse_http(const std::string &raw_request) {
    HttpRequest request;

    const auto lines = Util::split(raw_request, "\r\n");
    if (lines.empty()) {
      return request;
    }

    const auto request_line = Util::split(lines[0], " ");
    if (request_line.size() != 3) {
      std::cerr << "Malformed request line: " << lines[0] << '\n';
      return request;
    }

    request.method = request_line[0];
    request.path = request_line[1];
    request.version = request_line[2];

    for (int i = 1; i < lines.size(); i++) {
      if (lines[i].find(':') != std::string::npos) {
        const auto header = Util::split(lines[i], ":");
        if (header.size() == 2) {
          const auto key = Util::trim(header[0]);
          const auto value = Util::trim(header[1]);
          request.headers[key] = Util::trim(value);
          if (key == "Accept-Encoding" && value.contains("gzip")) {
            request.headers[key] = "gzip";
          }
        }
      }
    }

    if (request.method == "POST" && !lines[lines.size() - 1].empty()) {
      request.body = lines[lines.size() - 1];
    }

    return request;
  }
};