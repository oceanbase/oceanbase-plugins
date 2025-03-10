#ifndef CPPJIEBA_KEYWORD_EXTRACTOR_H
#define CPPJIEBA_KEYWORD_EXTRACTOR_H

#include <cmath>
#include <set>
#include "MixSegment.hpp"

namespace cppjieba {

using namespace limonp;
using namespace std;

/*utf8*/
class KeywordExtractor final {
 public:
  struct Word {
    string word;
    vector<size_t> offsets;
    double weight;
  }; // struct Word

  KeywordExtractor() = default;
  ~KeywordExtractor() {
  }

  int Init(const string& dictPath, 
           const string& hmmFilePath, 
           const string& idfPath, 
           const string& stopWordPath, 
           const string& userDict = "") {
    int ret = OBP_SUCCESS;
    ret = segment_.Init(dictPath, hmmFilePath, userDict);
    if (ret != OBP_SUCCESS) {
      OBP_LOG_WARN("failed to segment init. ret=%d", ret);
      return ret;
    }

    ret = LoadIdfDict(idfPath);
    if (ret != OBP_SUCCESS) {
      OBP_LOG_WARN("failed to load idf file. file=%s, ret=%d", idfPath.c_str(), ret);
      return ret;
    }
    ret = LoadStopWordDict(stopWordPath);
    if (ret != OBP_SUCCESS) {
      OBP_LOG_WARN("failed to load stop words dict. file=%s, ret=%d", stopWordPath.c_str(), ret);
      return ret;
    }
    return ret;
  }
  int Init(const DictTrie* dictTrie, 
           const HMMModel* model,
           const string& idfPath, 
           const string& stopWordPath) {
    ifstream idfStream(idfPath);
    if (!idfStream.is_open()) {
      OBP_LOG_WARN("failed to open idf file: %s. error=%s", idfPath.c_str(), strerror(errno));
      return OBP_INVALID_ARGUMENT;
    }

    ifstream stopWordStream(stopWordPath);
    if (!stopWordStream.is_open()) {
      OBP_LOG_WARN("failed to open stop word file:%s, error=%s", stopWordPath.c_str(), strerror(errno));
      return OBP_INVALID_ARGUMENT;
    }
    return Init(dictTrie, model, idfStream, stopWordStream);
  }

  int Init(const DictTrie* dictTrie, 
           const HMMModel* model,
           istream& idfStream, 
           istream& stopWordStream) {
    int ret = segment_.Init(dictTrie, model);
    if (ret != OBP_SUCCESS) {
      OBP_LOG_WARN("failed to init segment. ret=%d", ret);
      return ret;
    }

    ret = LoadIdfDict(idfStream);
    if (ret != OBP_SUCCESS) {
      OBP_LOG_WARN("failed to load idf file. ret=%d", ret);
      return ret;
    }
    ret = LoadStopWordDict(stopWordStream);
    if (ret != OBP_SUCCESS) {
      OBP_LOG_WARN("failed to load stop words dict. ret=%d", ret);
      return ret;
    }
    return ret;
  }

  int Extract(const string& sentence, vector<string>& keywords, size_t topN) const {
    vector<Word> topWords;
    int ret = Extract(sentence, topWords, topN);
    if (ret != OBP_SUCCESS) {
      return ret;
    }
    for (size_t i = 0; i < topWords.size(); i++) {
      keywords.push_back(topWords[i].word);
    }
    return ret;
  }

  int Extract(const string& sentence, vector<pair<string, double> >& keywords, size_t topN) const {
    vector<Word> topWords;
    int ret = Extract(sentence, topWords, topN);
    if (ret != OBP_SUCCESS) {
      return ret;
    }
    for (size_t i = 0; i < topWords.size(); i++) {
      keywords.push_back(pair<string, double>(topWords[i].word, topWords[i].weight));
    }
    return ret;
  }

  int Extract(const string& sentence, vector<Word>& keywords, size_t topN) const {
    int ret = OBP_SUCCESS;
    vector<string> words;
    ret = segment_.Cut(sentence, words);
    if (ret != OBP_SUCCESS) {
      OBP_LOG_WARN("failed to extract words: segment cut failed. ret=%d, sentence=%s", ret, sentence.c_str());
      return ret;
    }

    map<string, Word> wordmap;
    size_t offset = 0;
    for (size_t i = 0; i < words.size(); ++i) {
      size_t t = offset;
      offset += words[i].size();
      if (IsSingleWord(words[i]) || stopWords_.find(words[i]) != stopWords_.end()) {
        continue;
      }
      wordmap[words[i]].offsets.push_back(t);
      wordmap[words[i]].weight += 1.0;
    }
    if (offset != sentence.size()) {
      OBP_LOG_WARN("words illegal. offset=%ld, sentence=%s (len=%ld)", offset, sentence.c_str(), sentence.length());
      return OBP_PLUGIN_ERROR;
    }

    keywords.clear();
    keywords.reserve(wordmap.size());
    for (map<string, Word>::iterator itr = wordmap.begin(); itr != wordmap.end(); ++itr) {
      unordered_map<string, double>::const_iterator cit = idfMap_.find(itr->first);
      if (cit != idfMap_.end()) {
        itr->second.weight *= cit->second;
      } else {
        itr->second.weight *= idfAverage_;
      }
      itr->second.word = itr->first;
      keywords.push_back(itr->second);
    }
    topN = min(topN, keywords.size());
    partial_sort(keywords.begin(), keywords.begin() + topN, keywords.end(), Compare);
    keywords.resize(topN);
    return ret;
  }

  bool IsStopWord(const string& word) {
    return stopWords_.find(word) != stopWords_.end();
  }
 private:
  int LoadIdfDict(const string& idfPath) {
    ifstream ifs(idfPath.c_str());
    if (!ifs.is_open()) {
      OBP_LOG_WARN("failed to open file. file=%s, error=%s", idfPath.c_str(), strerror(errno));
      return OBP_INVALID_ARGUMENT;
    }
    int ret = LoadIdfDict(ifs);
    if (ret != OBP_SUCCESS) {
      OBP_LOG_WARN("failed to load idf dict. file=%s, ret=%d", idfPath.c_str(), ret);
    }
    return ret;
  }

  int LoadIdfDict(istream& ifs) {
    string line ;
    vector<string> buf;
    double idf = 0.0;
    double idfSum = 0.0;
    size_t lineno = 0;
    for (; getline(ifs, line); lineno++) {
      buf.clear();
      if (line.empty()) {
        OBP_LOG_INFO("lineno=%ld, empty, skipped", lineno);
        continue;
      }
      Split(line, buf, " ");
      if (buf.size() != 2) {
        OBP_LOG_INFO("lineno=%ld, invalid field number:%ld", lineno, buf.size());
        continue;
      }
      idf = atof(buf[1].c_str());
      idfMap_[buf[0]] = idf;
      idfSum += idf;

    }

    if (lineno == 0) {
      OBP_LOG_WARN("got 0 line.");
      return OBP_INVALID_ARGUMENT;
    }
    idfAverage_ = idfSum / lineno;
    if (idfAverage_ <= 0) {
      OBP_LOG_WARN("idf average(%lf) is <= 0", idfAverage_);
      return OBP_INVALID_ARGUMENT;
    }
    return OBP_SUCCESS;
  }
  int LoadStopWordDict(const string& filePath) {
    ifstream ifs(filePath.c_str());
    if (!ifs.is_open()) {
      OBP_LOG_WARN("failed to open file(%s), error=%s", filePath.c_str(), strerror(errno));
      return OBP_INVALID_ARGUMENT;
    }
    return OBP_SUCCESS;
  }
  int LoadStopWordDict(istream& ifs) {
    string line;
    while (getline(ifs, line)) {
      stopWords_.insert(line);
    }
    if (stopWords_.empty()) {
      OBP_LOG_WARN("stop words is empty");
      return OBP_INVALID_ARGUMENT;
    }
    return OBP_SUCCESS;
  }

  static bool Compare(const Word& lhs, const Word& rhs) {
    return lhs.weight > rhs.weight;
  }

  MixSegment segment_;
  unordered_map<string, double> idfMap_;
  double idfAverage_;

  unordered_set<string> stopWords_;
}; // class KeywordExtractor

inline ostream& operator << (ostream& os, const KeywordExtractor::Word& word) {
  return os << "{\"word\": \"" << word.word << "\", \"offset\": " << word.offsets << ", \"weight\": " << word.weight << "}"; 
}

} // namespace cppjieba

#endif


