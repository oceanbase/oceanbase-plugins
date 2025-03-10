#ifndef CPPJIEBA_DICT_TRIE_HPP
#define CPPJIEBA_DICT_TRIE_HPP

#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <cstring>
#include <cstdlib>
#include <stdint.h>
#include <cmath>
#include <limits>
#include "oceanbase/ob_plugin_errno.h"
#include "limonp/StringUtil.hpp"
#include "Unicode.hpp"
#include "Trie.hpp"

namespace cppjieba {

using namespace limonp;

const double MIN_DOUBLE = -3.14e+100;
const double MAX_DOUBLE = 3.14e+100;
const size_t DICT_COLUMN_NUM = 3;
const char* const UNKNOWN_TAG = "";

class DictTrie final {
 public:
  enum UserWordWeightOption {
    WordWeightMin,
    WordWeightMedian,
    WordWeightMax,
  }; // enum UserWordWeightOption

  DictTrie() = default;

  ~DictTrie() {
    delete trie_;
    trie_ = nullptr;
  }

  int Init(const string& dict_path,
           const string& user_dict_paths = "",
           UserWordWeightOption user_word_weight_opt = WordWeightMedian) {
    ifstream dict_ifs(dict_path);
    if (!dict_ifs) {
      OBP_LOG_WARN("cannot open dict path. dict_path=%s, system error=%s", dict_path.c_str(), strerror(errno));
      return OBP_INVALID_ARGUMENT;
    }

    vector<string> files = limonp::Split(user_dict_paths, "|;");
    vector<unique_ptr<istream>> user_dict_streams(files.size());
    for (size_t i = 0; i < files.size(); i++) {
      auto ifs = make_unique<ifstream>(files[i]);
      if (!ifs->is_open()) {
        OBP_LOG_WARN("failed to init dict trie: user dict not exist. path=%s. system error=%s",
                     files[i].c_str(), strerror(errno));
        return OBP_INVALID_ARGUMENT;
      }
      user_dict_streams[i] = std::move(ifs);
    }
    return Init(dict_ifs, user_dict_streams, user_word_weight_opt);
  }

  int Init(istream& dict_stream,
           vector<unique_ptr<istream>>& user_dict_streams,
           UserWordWeightOption user_word_weight_opt = WordWeightMedian) {
    int ret = OBP_SUCCESS;
    ret = LoadDict(dict_stream);
    if (ret != OBP_SUCCESS) {
      OBP_LOG_WARN("failed to load dict. ret=%d", ret); 
      return ret;
    }
    freq_sum_ = CalcFreqSum(static_node_infos_);
    ret = CalculateWeight(static_node_infos_, freq_sum_);
    if (ret != OBP_SUCCESS) {
      OBP_LOG_WARN("failed to calculate weight. ret=%d", ret);
      return ret;
    }

    ret = SetStaticWordWeights(user_word_weight_opt);
    if (ret != OBP_SUCCESS) {
      OBP_LOG_WARN("failed to set static word weights. ret=%d", ret);
      return ret;
    }

    if (!user_dict_streams.empty()) {
      ret = LoadUserDict(user_dict_streams);
      if (ret != OBP_SUCCESS) {
        OBP_LOG_WARN("failed to load user dict. ret=%d", ret);
        return ret;
      }
    }
    Shrink(static_node_infos_);
    ret = CreateTrie(static_node_infos_);
    if (ret != OBP_SUCCESS) {
      OBP_LOG_WARN("failed to init dict trie: create trie failed. ret=%d", ret);
      return ret;
    }

    return ret;
  }

  bool InsertUserWord(const string& word, const string& tag = UNKNOWN_TAG) {
    DictUnit node_info;
    if (!MakeNodeInfo(node_info, word, user_word_default_weight_, tag)) {
      return false;
    }
    active_node_infos_.push_back(node_info);
    trie_->InsertNode(node_info.word, &active_node_infos_.back());
    return true;
  }

  bool InsertUserWord(const string& word, int freq, const string& tag = UNKNOWN_TAG) {
    DictUnit node_info;
    double weight = freq ? log(1.0 * freq / freq_sum_) : user_word_default_weight_ ;
    if (!MakeNodeInfo(node_info, word, weight , tag)) {
      return false;
    }
    active_node_infos_.push_back(node_info);
    trie_->InsertNode(node_info.word, &active_node_infos_.back());
    return true;
  }

  bool DeleteUserWord(const string& word, const string& tag = UNKNOWN_TAG) {
    DictUnit node_info;
    if (!MakeNodeInfo(node_info, word, user_word_default_weight_, tag)) {
      return false;
    }
    trie_->DeleteNode(node_info.word, &node_info);
    return true;
  }
  
  const DictUnit* Find(RuneStrArray::const_iterator begin, RuneStrArray::const_iterator end) const {
    return trie_->Find(begin, end);
  }

  void Find(RuneStrArray::const_iterator begin, 
        RuneStrArray::const_iterator end, 
        vector<struct Dag>&res,
        size_t max_word_len = MAX_WORD_LENGTH) const {
    trie_->Find(begin, end, res, max_word_len);
  }

  bool Find(const string& word)
  {
    const DictUnit *tmp = NULL;
    RuneStrArray runes;
    if (!DecodeUTF8RunesInString(word, runes))
    {
      OBP_LOG_WARN("faield to decode utf8 runes in string. word=%s", word.c_str());
      return false;
    }

    tmp = Find(runes.begin(), runes.end());
    return tmp != nullptr;
  }

  bool IsUserDictSingleChineseWord(const Rune& word) const {
    return IsIn(user_dict_single_chinese_word_, word);
  }

  double GetMinWeight() const {
    return min_weight_;
  }

  int InserUserDictNode(const string& line) {
    vector<string> buf;
    DictUnit node_info;
    Split(line, buf, " ");
    double weight = 0.0;
    string tag;
    if(buf.size() == 1){
      weight = user_word_default_weight_;
      tag = UNKNOWN_TAG;
    } else if (buf.size() == 2) {
      weight = user_word_default_weight_;
      tag = buf[1];
    } else if (buf.size() == 3) {
      int freq = atoi(buf[1].c_str());
      if (freq_sum_ <= 0.0) {
        return OBP_INVALID_ARGUMENT;
      }
      weight = log(1.0 * freq / freq_sum_);
      tag = buf[2];
    } else {
      OBP_LOG_WARN("invalid line. column num(%ld) is invalid(should be 1,2,3). line=%s", buf.size(), line.c_str());
      return OBP_INVALID_ARGUMENT;
    }

    if (!MakeNodeInfo(node_info, buf[0], weight, tag)) {
      OBP_LOG_WARN("failed to make node info. buf[0]=%s, weight=%lf, tag=%s, line=%s",
          buf[0].c_str(), weight, tag.c_str(), line.c_str());
      return OBP_INVALID_ARGUMENT;
    }

    static_node_infos_.push_back(node_info);
    if (node_info.word.size() == 1) {
      user_dict_single_chinese_word_.insert(node_info.word[0]);
    }
    return OBP_SUCCESS;
  }
  
  int LoadUserDict(const vector<string>& buf) {
    for (size_t i = 0; i < buf.size(); i++) {
      int ret = InserUserDictNode(buf[i]);
      if (ret != OBP_SUCCESS) {
        OBP_LOG_WARN("failed to load user dict. ret=%d", ret);
        return ret;
      }
    }
    return OBP_SUCCESS;
  }

  int LoadUserDict(const set<string>& buf) {
    std::set<string>::const_iterator iter;
    for (iter = buf.begin(); iter != buf.end(); iter++){
      int ret = InserUserDictNode(*iter);
      if (ret != OBP_SUCCESS) {
        OBP_LOG_WARN("failed to load user dict. ret=%d", ret);
        return ret;
      }
    }
    return OBP_SUCCESS;
  }

  int LoadUserDict(const string& filePaths) {
    vector<string> files = limonp::Split(filePaths, "|;");
    vector<unique_ptr<istream>> ifstreams(files.size());
    for (size_t i = 0; i < files.size(); i++) {
      const string &filename = files[i];
      auto ifs = make_unique<ifstream>(filename);
      if (!ifs->is_open()) {
        OBP_LOG_WARN("failed to open file: %s. error=%s", filename.c_str(), strerror(errno));
        return OBP_PLUGIN_ERROR;
      }
      ifstreams[i] = std::move(ifs);
    }
    return LoadUserDict(ifstreams);
  }

  int LoadUserDict(vector<unique_ptr<istream>>& dicts) {
    for (unique_ptr<istream> &isp : dicts) {
      istream &is = *isp;
      string line;
      
      while(getline(is, line)) {
        if (line.size() == 0) {
          continue;
        }
        int ret = InserUserDictNode(line);
        if (ret != OBP_SUCCESS) {
          OBP_LOG_WARN("failed to load user dict. ret=%d", ret);
          return ret;
        }
      }
    }
    return OBP_SUCCESS;
  }

 private:
  
  int CreateTrie(const vector<DictUnit>& dictUnits) {
    if (dictUnits.empty()) {
      OBP_LOG_WARN("failed to create trie: dict unit is empty");
      return OBP_INVALID_ARGUMENT;
    }

    vector<Unicode> words;
    vector<const DictUnit*> valuePointers;
    for (size_t i = 0 ; i < dictUnits.size(); i ++) {
      words.push_back(dictUnits[i].word);
      valuePointers.push_back(&dictUnits[i]);
    }

    trie_ = new (std::nothrow) Trie(words, valuePointers);
    if (nullptr == trie_) {
      OBP_LOG_WARN("failed to create trie: allocate memory failed, size=%ld", sizeof(Trie));
      return OBP_ALLOCATE_MEMORY_FAILED;
    }
    return OBP_SUCCESS;
  }

  bool MakeNodeInfo(DictUnit& node_info,
        const string& word, 
        double weight, 
        const string& tag) {
    if (!DecodeUTF8RunesInString(word, node_info.word)) {
      OBP_LOG_WARN("UTF-8 decode failed for dict word: %s", word.c_str());
      return false;
    }
    node_info.weight = weight;
    node_info.tag = tag;
    return true;
  }

  int LoadDict(const string& filePath) {
    ifstream ifs(filePath.c_str());
    if (!ifs.is_open()) {
      OBP_LOG_WARN("failed to open dict file: %s", filePath.c_str());
      return OBP_PLUGIN_ERROR;
    }
    return LoadDict(ifs);
  }

  int LoadDict(istream& is) {
    if (!is.good()) {
      return OBP_INVALID_ARGUMENT;
    }

    string line;
    vector<string> buf;

    DictUnit node_info;
    while (getline(is, line)) {
      Split(line, buf, " ");
      if (buf.size() != DICT_COLUMN_NUM) {
        OBP_LOG_WARN("invalid dict content. column num is invalid. line=%s, column num=%ld(should be %ld)",
            line.c_str(), buf.size(), DICT_COLUMN_NUM);
        return OBP_INVALID_ARGUMENT;
      }

      if (!MakeNodeInfo(node_info, buf[0], atof(buf[1].c_str()), buf[2])) {
        OBP_LOG_WARN("failed to make_node_info. buf[0]=%s, buf[1]=%s", buf[0].c_str(), buf[1].c_str());
        return OBP_INVALID_ARGUMENT;
      }
      static_node_infos_.push_back(node_info);
    }
    return OBP_SUCCESS;
  }

  static bool WeightCompare(const DictUnit& lhs, const DictUnit& rhs) {
    return lhs.weight < rhs.weight;
  }

  int SetStaticWordWeights(UserWordWeightOption option) {
    if (static_node_infos_.empty()) {
      return OBP_NOT_INIT;
    }

    vector<DictUnit> x = static_node_infos_;
    sort(x.begin(), x.end(), WeightCompare);
    min_weight_ = x[0].weight;
    max_weight_ = x[x.size() - 1].weight;
    median_weight_ = x[x.size() / 2].weight;
    switch (option) {
     case WordWeightMin:
       user_word_default_weight_ = min_weight_;
       break;
     case WordWeightMedian:
       user_word_default_weight_ = median_weight_;
       break;
     default:
       user_word_default_weight_ = max_weight_;
       break;
    }
    return OBP_SUCCESS;
  }

  double CalcFreqSum(const vector<DictUnit>& node_infos) const {
    double sum = 0.0;
    for (size_t i = 0; i < node_infos.size(); i++) {
      sum += node_infos[i].weight;
    }
    return sum;
  }

  int CalculateWeight(vector<DictUnit>& node_infos, double sum) const {
    if (sum <= 0) {
      OBP_LOG_WARN("failed to calculate weight, sum=%lf", sum);
      return OBP_INVALID_ARGUMENT;
    }

    for (size_t i = 0; i < node_infos.size(); i++) {
      DictUnit& node_info = node_infos[i];
      if (node_info.weight <= 0) {
        OBP_LOG_WARN("failed to calculate weight, node index: %ld, weight=%lf", i, node_info.weight);
        return OBP_INVALID_ARGUMENT;
      }
      node_info.weight = log(double(node_info.weight)/sum);
    }
    return OBP_SUCCESS;
  }

  void Shrink(vector<DictUnit>& units) const {
    vector<DictUnit>(units.begin(), units.end()).swap(units);
  }

  vector<DictUnit> static_node_infos_;
  deque<DictUnit> active_node_infos_; // must not be vector
  Trie * trie_ = nullptr;

  double freq_sum_;
  double min_weight_;
  double max_weight_;
  double median_weight_;
  double user_word_default_weight_;
  unordered_set<Rune> user_dict_single_chinese_word_;
};
} // namespace cppjieba

#endif // CPPJIEBA_DICT_TRIE_HPP
