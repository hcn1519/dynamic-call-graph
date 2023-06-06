#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace basic {
    
    
std::vector<std::string> splitStr(const std::string &s, char delim) {
  std::stringstream ss(s);
  std::string item;
  std::vector<std::string> elems;
  while (std::getline(ss, item, delim)) {
    elems.push_back(item);
  }
  return elems;
}

std::set<std::string> stopWords(const char *filename) {
  std::ifstream inf{filename};
  std::vector<std::string> results{};

  while (inf) {
    std::string strInput;
    inf >> strInput;
    std::vector<std::string> splited = splitStr(strInput, ',');
    results.insert(results.end(), splited.begin(), splited.end());
  }
  return std::set<std::string>(std::begin(results), std::end(results));
}

void toLowerCase(std::string &str) {
  std::transform(str.begin(), str.end(), str.begin(),
                 [](unsigned char c) { return std::tolower(c); });
}

std::vector<std::string> retrieveTerms(std::string &s) {

  std::string nonAlpha = "";

  for (auto c : s) {
    if (!isalpha(c)) {
      nonAlpha.push_back(c);
    }
  }

  for (auto c : nonAlpha) {
    std::replace(s.begin(), s.end(), c, ' ');
  }
  std::vector<std::string> splited = splitStr(s, ' ');
  return splited;
}

std::map<int, std::vector<std::string>>
countWords(const char *filename, std::set<std::string> &stopWordSet) {

  std::ifstream inf{filename};
  std::unordered_map<std::string, int> wc{};

  while (inf) {
    std::string strInput;
    inf >> strInput;

    std::vector<std::string> terms = retrieveTerms(strInput);

    for (auto term : terms) {
      toLowerCase(term);
      if (stopWordSet.find(term) != stopWordSet.end()) {
        continue;
      }
      if (term == "" || term.length() < 2) {
        continue;
      }

      if (wc.find(term) != wc.end()) {
        wc[term] = wc[term] + 1;
      } else {
        wc[term] = 1;
      }
    }
  }

  std::map<int, std::vector<std::string>> wc2{};

  for (auto const &pair : wc) {
    if (wc2.find(pair.second) != wc2.end()) {
      wc2[pair.second].push_back(pair.first);
    } else {
      std::vector<std::string> newV{pair.first};
      wc2[pair.second] = newV;
    }
  }
  return wc2;
}

}

int main(int argc, char **argv) {
  std::set<std::string> stopWordSet = basic::stopWords("./stop_words.txt");
  char *filepath = argv[1];
  std::map<int, std::vector<std::string>> wordCounter =
      basic::countWords(filepath, stopWordSet);

  int count = 0;
  for (auto it = wordCounter.rbegin(); it != wordCounter.rend(); it++) {
    if (count > 24) {
      break;
    }
    for (auto el : it->second) {
      if (count > 24) {
        break;
      }
      std::cout << el << "  -  " << it->first << std::endl;
      count += 1;
    }
  }
}