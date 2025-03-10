#ifndef CPPJIEBA_FULLSEGMENT_H
#define CPPJIEBA_FULLSEGMENT_H

#include <algorithm>
#include <set>
#include "DictTrie.hpp"
#include "SegmentBase.hpp"
#include "Unicode.hpp"

namespace cppjieba {
class FullSegment final: public SegmentBase {
 public:
  FullSegment() = default;
  virtual ~FullSegment() {
    if (isNeedDestroy_) {
      delete dictTrie_;
      dictTrie_ = nullptr;
    }
  }

  int Init(const string& dictPath) {
    int ret = OBP_SUCCESS;
    ret = SegmentBase::Init();
    if (ret != OBP_SUCCESS) {
      OBP_LOG_WARN("failed to init segment base. ret=%d", ret);
      return ret;
    }

    DictTrie *dictTrie = new (std::nothrow) DictTrie();
    if (nullptr == dictTrie) {
      OBP_LOG_WARN("failed to do full segment init: allocate DictTrie fail. size=%ld", sizeof(DictTrie));
      return OBP_ALLOCATE_MEMORY_FAILED;
    }

    ret = dictTrie->Init(dictPath);
    if (ret != OBP_SUCCESS) {
      OBP_LOG_WARN("failed to do fullSegment init: init dicttrie fail. dict path=%s, ret=%d", dictPath.c_str(), ret);
      delete dictTrie;
      dictTrie = nullptr;
      return ret;
    }

    dictTrie_ = dictTrie;
    isNeedDestroy_ = true;
    return ret;
  }

  int Init(const DictTrie* dictTrie) {
    int ret = SegmentBase::Init();
    if (ret != OBP_SUCCESS) {
      OBP_LOG_WARN("failed to init segment base. ret=%d", ret);
      return ret;
    }
    if (nullptr == dictTrie) {
      OBP_LOG_WARN("dictTrie is null");
      return OBP_INVALID_ARGUMENT;
    }

    dictTrie_ = dictTrie;
    isNeedDestroy_ = false;
    return OBP_SUCCESS;
  }

  int Cut(const string& sentence, vector<string>& words) const override {
    vector<Word> tmp;
    int ret = Cut(sentence, tmp);
    if (ret == OBP_SUCCESS) {
      GetStringsFromWords(tmp, words);
    }
    return ret;
  }
  int Cut(const string& sentence, 
        vector<Word>& words) const {
    int ret = OBP_SUCCESS;
    PreFilter pre_filter;
    ret = pre_filter.Init(symbols_, sentence);
    if (ret != OBP_SUCCESS) {
      OBP_LOG_WARN("failed to init pre filter. ret=%d", ret);
      return ret;
    }

    PreFilter::Range range;
    vector<WordRange> wrs;
    wrs.reserve(sentence.size()/2);
    while (pre_filter.HasNext()) {
      range = pre_filter.Next();
      ret = Cut(range.begin, range.end, wrs);
      if (ret != OBP_SUCCESS) {
        OBP_LOG_WARN("failed to cut(full segment). ret=%d", ret);
        return ret;
      }
    }
    words.clear();
    words.reserve(wrs.size());
    GetWordsFromWordRanges(sentence, wrs, words);
    return ret;
  }
  int Cut(RuneStrArray::const_iterator begin, 
        RuneStrArray::const_iterator end, 
        vector<WordRange>& res) const {
    if (nullptr == dictTrie_) {
      OBP_LOG_WARN("dictTrie_ is null");
      return OBP_NOT_INIT;
    }

    // result of searching in trie tree
    LocalVector<pair<size_t, const DictUnit*> > tRes;

    // max index of res's words
    size_t maxIdx = 0;

    // always equals to (uItr - begin)
    size_t uIdx = 0;

    // tmp variables
    size_t wordLen = 0;
    vector<struct Dag> dags;
    dictTrie_->Find(begin, end, dags);
    for (size_t i = 0; i < dags.size(); i++) {
      for (size_t j = 0; j < dags[i].nexts.size(); j++) {
        size_t nextoffset = dags[i].nexts[j].first;
        if (nextoffset >= dags.size()) {
          OBP_LOG_WARN("fullsegment cut fail. nextoffset(%ld) >= dags.size(%ld)", nextoffset, dags.size());
          return OBP_PLUGIN_ERROR;
        }
        const DictUnit* du = dags[i].nexts[j].second;
        if (du == NULL) {
          if (dags[i].nexts.size() == 1 && maxIdx <= uIdx) {
            WordRange wr(begin + i, begin + nextoffset);
            res.push_back(wr);
          }
        } else {
          wordLen = du->word.size();
          if (wordLen >= 2 || (dags[i].nexts.size() == 1 && maxIdx <= uIdx)) {
            WordRange wr(begin + i, begin + nextoffset);
            res.push_back(wr);
          }
        }
        maxIdx = uIdx + wordLen > maxIdx ? uIdx + wordLen : maxIdx;
      }
      uIdx++;
    }
    return OBP_SUCCESS;
  }
 private:
  const DictTrie* dictTrie_ = nullptr;
  bool isNeedDestroy_ = false;
};
}

#endif
