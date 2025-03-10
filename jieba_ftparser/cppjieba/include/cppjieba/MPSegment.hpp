#ifndef CPPJIEBA_MPSEGMENT_H
#define CPPJIEBA_MPSEGMENT_H

#include <algorithm>
#include <set>
#include "DictTrie.hpp"
#include "SegmentTagged.hpp"
#include "PosTagger.hpp"

namespace cppjieba {

/**
 * 最大概率分词算法
 * @details MP for Maximum Probability
 */
class MPSegment final: public SegmentTagged {
 public:
  MPSegment() = default;

  virtual ~MPSegment() {
    if (isNeedDestroy_) {
      delete dictTrie_;
      dictTrie_ = nullptr;
    }
  }

  int Init(const string& dictPath, const string& userDictPath = "") {
    int ret = OBP_SUCCESS;
    ret = SegmentTagged::Init();
    if (ret != OBP_SUCCESS) {
      OBP_LOG_WARN("failed to init segment tagged. ret=%d", ret);
      return ret;
    }
    DictTrie *dictTrie = new (std::nothrow) DictTrie;
    if (nullptr == dictTrie) {
      OBP_LOG_WARN("failed to allocate memory. size=%ld", sizeof(DictTrie));
      return OBP_ALLOCATE_MEMORY_FAILED;
    }
    ret = dictTrie->Init(dictPath, userDictPath);
    if (ret != OBP_SUCCESS) {
      OBP_LOG_WARN("failed to init dict trie. dict=%s, user dict=%s, ret=%d",
                   dictPath.c_str(), userDictPath.c_str(), ret);
      delete dictTrie;
      dictTrie = nullptr;
      return ret;
    }

    dictTrie_ = dictTrie;
    isNeedDestroy_ = true;
    return ret;
  }
  int Init(const DictTrie* dictTrie) {
    int ret = SegmentTagged::Init();
    if (ret != OBP_SUCCESS) {
      OBP_LOG_WARN("failed to init segment tagged. ret=%d", ret);
      return ret;
    }
    if (nullptr == dictTrie) {
      OBP_LOG_WARN("invalid argument: dict trie is null");
      return OBP_INVALID_ARGUMENT;
    }
    dictTrie_ = dictTrie;
    isNeedDestroy_ = false;
    return OBP_SUCCESS;
  }
  
  int Cut(const string& sentence, vector<string>& words) const override {
    return Cut(sentence, words, MAX_WORD_LENGTH);
  }

  int Cut(const string& sentence,
        vector<string>& words,
        size_t max_word_len) const {
    vector<Word> tmp;
    int ret = Cut(sentence, tmp, max_word_len);
    if (OBP_SUCCESS == ret){
      GetStringsFromWords(tmp, words);
    }
    return ret;
  }
  int Cut(const string& sentence, 
        vector<Word>& words, 
        size_t max_word_len = MAX_WORD_LENGTH) const {
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
      ret = Cut(range.begin, range.end, wrs, max_word_len);
      if (ret != OBP_SUCCESS) {
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
           vector<WordRange>& words,
           size_t max_word_len = MAX_WORD_LENGTH) const {
    vector<Dag> dags;
    dictTrie_->Find(begin, 
          end, 
          dags,
          max_word_len);
    int ret = CalcDP(dags);
    if (ret != OBP_SUCCESS) {
      OBP_LOG_WARN("failed to calc dp. ret=%d", ret);
      return ret;
    }
    return CutByDag(begin, end, dags, words);
  }

  const DictTrie* GetDictTrie() const {
    return dictTrie_;
  }

  bool Tag(const string& src, vector<pair<string, string> >& res) const {
    return tagger_.Tag(src, res, *this);
  }

  bool IsUserDictSingleChineseWord(const Rune& value) const {
    return dictTrie_->IsUserDictSingleChineseWord(value);
  }
 private:
  int CalcDP(vector<Dag>& dags) const {
    int ret = OBP_SUCCESS;
    size_t nextPos;
    const DictUnit* p;
    double val;

    for (vector<Dag>::reverse_iterator rit = dags.rbegin(); rit != dags.rend(); rit++) {
      rit->pInfo = NULL;
      rit->weight = MIN_DOUBLE;
      if (rit->nexts.empty()) {
        OBP_LOG_WARN("got empty rit->nexts");
        return OBP_INVALID_ARGUMENT;
      }
      for (LocalVector<pair<size_t, const DictUnit*> >::const_iterator it = rit->nexts.begin(); it != rit->nexts.end(); it++) {
        nextPos = it->first;
        p = it->second;
        val = 0.0;
        if (nextPos + 1 < dags.size()) {
          val += dags[nextPos + 1].weight;
        }

        if (p) {
          val += p->weight;
        } else {
          val += dictTrie_->GetMinWeight();
        }
        if (val > rit->weight) {
          rit->pInfo = p;
          rit->weight = val;
        }
      }
    }
    return ret;
  }
  int CutByDag(RuneStrArray::const_iterator begin, 
        RuneStrArray::const_iterator end, 
        const vector<Dag>& dags, 
        vector<WordRange>& words) const {
    size_t i = 0;
    while (i < dags.size()) {
      const DictUnit* p = dags[i].pInfo;
      if (p) {
        if (p->word.size() < 1) {
          OBP_LOG_WARN("invalid word(num:%ld)", p->word.size());
          return OBP_INVALID_ARGUMENT;
        }
        if (begin + i + p->word.size() - 1 >= end) {
          OBP_LOG_WARN("out of range. end-begin=%ld, p->word.size=%ld", end-begin, p->word.size());
          return OBP_PLUGIN_ERROR;
        }
        WordRange wr(begin + i, begin + i + p->word.size() - 1);
        words.push_back(wr);
        i += p->word.size();
      } else { //single chinese word
        if (begin + i >= end) {
          OBP_LOG_WARN("out of range. end-begin=%ld, i=%ld", end-begin, i);
          return OBP_PLUGIN_ERROR;
        }
        WordRange wr(begin + i, begin + i);
        words.push_back(wr);
        i++;
      }
    }
    return OBP_SUCCESS;
  }

  const DictTrie* dictTrie_ = nullptr;
  bool isNeedDestroy_ = false;
  PosTagger tagger_;

}; // class MPSegment

} // namespace cppjieba

#endif
