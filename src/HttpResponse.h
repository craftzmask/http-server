#pragma once
#include <string_view>

class HttpResponse
{
public:
    static std::string ok();
    static std::string ok(const std::string_view content);
    static std::string ok(const std::string_view content, const std::string_view content_type);
    static std::string ok(const std::string_view content, const std::string_view content_type, const std::string_view content_encoding);
    static std::string file_ok(const std::string_view file_content);
    static std::string not_found();
    static std::string bad_request();
    static std::string created();
};