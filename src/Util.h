#include <string>
#include <vector>
#include <algorithm> // for std::transform
#include <cctype>    // for std::tolower

class Util {
public:
  static std::vector<std::string> split(
    const std::string& s,
    const std::string& deli
  ) {
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

  static std::string lower(std::string s) {
    std::transform(
      s.begin(),
      s.end(),
      s.begin(),
      [](unsigned char c) { return std::tolower(c); }
    );

    return s;
  }
};