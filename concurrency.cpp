
#include <iostream>
#include <fstream>
#include <thread>
#include <queue>
#include <unordered_map>
#include <set>
#include <algorithm>
#include <cctype>
#include <string>
#include <sstream>
#include <vector>
#include <mutex>
#include <condition_variable>

template <typename T>
class BlockingQueue {
public:
    void enqueue(const T& item) {
        std::unique_lock<std::mutex> lock(mutex_);
        queue_.push(item);
        lock.unlock();
        condition_.notify_one();
    }

    T dequeue() {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [this]() { return !queue_.empty(); });
        T item = queue_.front();
        queue_.pop();
        return item;
    }

    bool empty() {
        std::unique_lock<std::mutex> lock(mutex_);
        return queue_.empty();
    }

private:
    std::queue<T> queue_;
    std::mutex mutex_;
    std::condition_variable condition_;
};

// Forward declarations
class ActiveWFObject;
class Message;
void send(ActiveWFObject& receiver, Message& message);

class Message {
public:
    ActiveWFObject& obj;
    std::vector<std::string> args;
    std::vector<std::pair<std::string, int>> pairs;

    Message(ActiveWFObject& object, const std::vector<std::string>& arguments, const std::vector<std::pair<std::string, int>>& keyValuePairs)
        : obj(object), args(arguments), pairs(keyValuePairs) {}
};

class ActiveWFObject {
public:
    BlockingQueue<Message> queue;
    std::thread thread;
    bool stopMe;

    ActiveWFObject() : stopMe(false) {
        thread = std::thread([this]() { run(); });
    }

    virtual ~ActiveWFObject() {
        std::cout << "join2" << std::endl;
        thread.join();
    }

    void send(Message& message) {
        queue.enqueue(message);
    }

    virtual void dispatch(Message& message) = 0;

private:
    void run() {
        while (!stopMe) {
            if (!queue.empty()) {
                Message message = queue.dequeue();
                if (message.args[0] == "die") {
                    stopMe = true;
                    break;
                }
                dispatch(message);
            }
        }
    }
};

void send(ActiveWFObject& receiver, Message& message) {
    receiver.send(message);
}

class DataStorageManager : public ActiveWFObject {
    std::string data;
    ActiveWFObject& stopWordManager;
    std::mutex mutex_;
    std::condition_variable cv_;

public:
    DataStorageManager(ActiveWFObject& swManager) : stopWordManager(swManager) {}

    void dispatch(Message& message) override {
        if (message.args[0] == "init") {
            init(message.args[1]);
        }
        else if (message.args[0] == "send_word_freqs") {
            processWords(message.obj);
        }
        else {
            ::send(stopWordManager, message);
        }
    }

private:
    void init(std::string& filePath) {
        auto str = read_file(filePath);
        filter_chars_and_normalize(str);
        std::string data_str(str.begin(), str.end());
        data = data_str;
    }

    void processWords(ActiveWFObject& recipient) {
        std::vector<std::string> words = {};
        std::vector<std::string> splited = split_str(data, ' ');
        words.insert(words.end(), splited.begin(), splited.end());

        for (const auto& word : words) {
            Message msg{ recipient, { "filter", word }, {} };
            ::send(stopWordManager, msg);
        }
        Message msg{ recipient, { "top25" }, {} };
        ::send(stopWordManager, msg);
    }

    std::vector<char> read_file(std::string& filename) {
        std::vector<char> data{};
        std::ifstream inf{ filename };

        char c;
        if (inf.is_open()) {
            while (inf.get(c)) {
                data.push_back(c);
            }
        }
        inf.close();
        return data;
    }

    void filter_chars_and_normalize(std::vector<char>& data) {
        for (int i = 0; i < data.size(); i++) {
            if (isalnum(data[i])) {
                data[i] = std::tolower(data[i]);
            }
            else {
                data[i] = ' ';
            }
        }
    }

    std::vector<std::string> split_str(const std::string& s, char delim) {
        std::stringstream ss(s);
        std::string item;
        std::vector<std::string> elems;
        while (std::getline(ss, item, delim)) {
            elems.push_back(item);
        }
        return elems;
    }
};

class StopWordManager : public ActiveWFObject {
public:
    std::set<std::string> stopWords;
    ActiveWFObject& wordFreqsManager;
    std::mutex mutex_;
    std::condition_variable cv_;

    StopWordManager(ActiveWFObject& wfManager) : stopWords(), wordFreqsManager(wfManager) {}

    void dispatch(Message& message) override {
        if (message.args[0] == "init") {
            init();
        }
        else if (message.args[0] == "filter") {
            filter(message.args[1]);
        }
        else {
            ::send(wordFreqsManager, message);
        }
    }

private:
    void init() {
        std::vector<std::string> words{};
        std::ifstream inf{ "./stop_words.txt" };
        std::set<std::string> stop_words{};

        while (inf) {
            std::string strInput;
            inf >> strInput;
            std::vector<std::string> splited = split_str(strInput, ',');
            stop_words.insert(splited.begin(), splited.end());
        }

        for (int n = 97; n < 123; n++) {
            char cn = n;
            std::string s(1, cn);
            stop_words.insert(s);
        }
        stop_words.insert("");
        stopWords = stop_words;
    }

    void filter(const std::string& word) {
        if (stopWords.find(word) == stopWords.end()) {
            Message msg{ wordFreqsManager, { "word", word }, {} };
            ::send(wordFreqsManager, msg);
        }
    }

    std::vector<std::string> split_str(const std::string& s, char delim) {
        std::stringstream ss(s);
        std::string item;
        std::vector<std::string> elems;
        while (std::getline(ss, item, delim)) {
            elems.push_back(item);
        }
        return elems;
    }
};

class WordFrequencyManager : public ActiveWFObject {

public:
    std::unordered_map<std::string, int> wordFreqs;
    std::mutex mutex_;
    std::condition_variable cv_;

    void dispatch(Message& message) override {
        if (message.args[0] == "word") {
            incrementCount(message.args[1]);
        }
        else if (message.args[0] == "top25") {
            top25(message.obj);
        }
    }

private:
    void incrementCount(const std::string& word) {
        std::unique_lock<std::mutex> lock(mutex_);
        ++wordFreqs[word];
    }

    void top25(ActiveWFObject& recipient) {
        std::vector<std::pair<std::string, int>> freqsSorted(wordFreqs.begin(), wordFreqs.end());
        std::sort(freqsSorted.begin(), freqsSorted.end(),
            [](const std::pair<std::string, int>& a, const std::pair<std::string, int>& b) {
                return a.second > b.second;
            });

        Message msg{ recipient, { "top25" }, freqsSorted };
        ::send(recipient, msg);
    }
};

class WordFrequencyController : public ActiveWFObject {
public:
    ActiveWFObject& storageManager;
    ActiveWFObject& wordFreqManager;
    ActiveWFObject& stopWordManager;

    bool completed = false;
    std::mutex mutex_;
    std::condition_variable cv_;

    WordFrequencyController(ActiveWFObject& sm, ActiveWFObject& wfm, ActiveWFObject& swm) : storageManager(sm), wordFreqManager(wfm), stopWordManager(swm) {}

    void dispatch(Message& message) override {
        if (message.args[0] == "run") {
            run();
        }
        else if (message.args[0] == "top25") {
            display(message.pairs);
        }
    }

private:
    void run() {
        Message msg{ *this, { "send_word_freqs" }, {} };
        ::send(storageManager, msg);
    }

    void display(const std::vector<std::pair<std::string, int>>& wordFreqs) {
        for (int i = 0; i < wordFreqs.size(); i++) {
            if (i > 24) {
                break;
            }
            auto pair = wordFreqs[i];
            std::cout << pair.first << " - " << pair.second << std::endl;
        }

        Message msg{ storageManager, { "die" }, {} };
        ::send(storageManager, msg);

        Message msg2{ wordFreqManager, { "die" }, {} };
        ::send(wordFreqManager, msg2);

        Message msg3{ stopWordManager, { "die" }, {} };
        ::send(stopWordManager, msg3);

        {
            std::lock_guard<std::mutex> lock(mutex_);
            completed = true;

            Message msg4{ *this, { "die" }, {} };
            ::send(*this, msg4);
        }
        cv_.notify_one();
    }
};

int main(int argc, char** argv) {
    WordFrequencyManager wordFreqManager;
    StopWordManager stopWordManager(wordFreqManager);
    Message msg1{ stopWordManager, { "init" }, {} };
    send(stopWordManager, msg1);

    DataStorageManager storageManager(stopWordManager);

    Message msg2{ storageManager, { "init", argv[1] }, {} };
    send(storageManager, msg2);

    {
        WordFrequencyController wfController(storageManager, wordFreqManager, stopWordManager);
        Message msg{ storageManager, { "run" }, {} };
        send(wfController, msg);

        std::unique_lock<std::mutex> lock(wfController.mutex_);
        wfController.cv_.wait(lock, [&wfController]() {
            return wfController.completed;
        });
    }

    return 0;
}
