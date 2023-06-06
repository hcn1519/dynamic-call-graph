#include <algorithm>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "Generator.hpp"

std::vector<std::string> split_str(const std::string &s, char delim) {
  std::stringstream ss(s);
  std::string item;
  std::vector<std::string> elems;
  while (std::getline(ss, item, delim)) {
    elems.push_back(item);
  }
  return elems;
}

std::vector<char> read_file(const char *filename) {
  std::vector<char> data{};
  std::ifstream inf{filename};

  char c;
  if (inf.is_open()) {
    while (inf.good()) {
      inf.get(c);
      data.push_back(c);
    }
  }
  inf.close();
  return data;
}

void filter_chars_and_normalize(std::vector<char> &data) {
  for (int i = 0; i < data.size(); i++) {
    if (isalnum(data[i])) {
      data[i] = std::tolower(data[i]);
    } else {
      data[i] = ' ';
    }
  }
}

Generator<std::string> scan(std::vector<char> &data) {
  std::vector<std::string> words = {};
  std::string data_str(data.begin(), data.end());
  std::vector<std::string> splited = split_str(data_str, ' ');

  for (auto s : splited) {
    co_yield s;
  }
  co_return;
}

Generator<std::string> all_words(const char *filename) {
  auto data = read_file(filename);
  filter_chars_and_normalize(data);

  auto arr = scan(data);

  for (int j = 0; arr; j++) {
    co_yield arr();
  }
}

std::vector<std::string> stop_words() {
  std::vector<std::string> words{};
  std::ifstream inf{"../stop_words.txt"};
  std::vector<std::string> stop_words{};

  while (inf) {
    std::string strInput;
    inf >> strInput;
    std::vector<std::string> splited = split_str(strInput, ',');
    stop_words.insert(stop_words.end(), splited.begin(), splited.end());
  }

  for (int n = 97; n < 123; n++) {
    char cn = n;
    std::string s(1, cn);
    stop_words.push_back(s);
  }
  stop_words.push_back("");
  return stop_words;
}

Generator<std::string> non_stop_words(const char *filename) {
  auto stopwords = stop_words();
  auto allwords_generator = all_words(filename);

  for (int i = 0; allwords_generator; i++) {
    auto word = allwords_generator();
    if (std::find(stopwords.begin(), stopwords.end(), word) ==
        stopwords.end()) {
      co_yield word;
    }
  }
}

struct ValueComparator {
  template <typename T> bool operator()(const T &lhs, const T &rhs) const {
    return lhs.second > rhs.second;
  }
};

Generator<std::vector<std::pair<std::string, int>>>
count_and_sort(const char *filename) {

  auto non_stopwords_generator = non_stop_words(filename);
  std::map<std::string, int> freq{};

  for (int i = 0; non_stopwords_generator; i++) {
    auto word = non_stopwords_generator();

    if (freq.find(word) != freq.end()) {
      freq[word] = freq[word] + 1;
    } else {
      freq[word] = 1;
    }

    if (i % 5000 == 0) {
      std::vector<std::pair<std::string, int>> combinedData{};
      for (auto s : freq) {
        combinedData.push_back(std::make_pair(s.first, s.second));
      }
      std::sort(combinedData.begin(), combinedData.end(), ValueComparator());
      co_yield combinedData;
    }
  }
  std::vector<std::pair<std::string, int>> combinedData{};
  for (auto s : freq) {
    combinedData.push_back(std::make_pair(s.first, s.second));
  }
  std::sort(combinedData.begin(), combinedData.end(), ValueComparator());
  co_yield combinedData;
}

int main(int argc, char **argv) {

  auto generator = count_and_sort(argv[1]);

  for (int i = 0; generator; i++) {
    auto pairs = generator();

    int count = 0;
    std::cout << "-----------------------------" << std::endl;
    for (auto pair : pairs) {
      if (count > 24) {
        break;
      }
      std::cout << pair.first << " - " << pair.second << std::endl;
      count += 1;
    }
  }
}
