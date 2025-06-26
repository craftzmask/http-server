#include <string>
#include <vector>

struct Util {
public:
  static std::vector<std::string> split(const std::string &s,
                                        const std::string &delimiter) {
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

  static std::string trim(const std::string &s) {
    const size_t start = s.find_first_not_of(" \t\n\r\f\v");
    const size_t end = s.find_last_not_of(" \t\n\r\f\v");

    if (start != std::string::npos && end != std::string::npos) {
      return s.substr(start, end - start + 1);
    }

    return "";
  }
};