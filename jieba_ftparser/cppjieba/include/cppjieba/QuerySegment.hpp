#ifndef CPPJIEBA_QUERYSEGMENT_H
#define CPPJIEBA_QUERYSEGMENT_H

#include <algorithm>
#include <set>
#include "DictTrie.hpp"
#include "SegmentBase.hpp"
#include "FullSegment.hpp"
#include "MixSegment.hpp"
#include "Unicode.hpp"

namespace cppjieba {
class QuerySegment final: public SegmentBase {
 public:
  QuerySegment() = default;
  virtual ~QuerySegment() {
  }

  int Init(const string& dict, const string& model, const string& userDict = "") {
    int ret = SegmentBase::Init();
    if (ret != OBP_SUCCESS) {
      OBP_LOG_WARN("failed to init segment base. ret=%d", ret);
      return ret;
    }
    ret = mixSeg_.Init(dict, model, userDict);
    if (ret != OBP_SUCCESS) {
      OBP_LOG_WARN("failed to init mix seg. dict=%s, model=%s, userdict=%s. ret=%d",
                   dict.c_str(), model.c_str(), userDict.c_str(), ret);
      return ret;
    }
    trie_ = mixSeg_.GetDictTrie();

    OBP_LOG_TRACE("init query segment done. ret=%d", ret);
    return ret;
  }

  int Init(const DictTrie* dictTrie, const HMMModel* model) {
    int ret = SegmentBase::Init();
    if (ret != OBP_SUCCESS) {
      OBP_LOG_WARN("failed to init segment base. ret=%d", ret);
      return ret;
    }
    ret = mixSeg_.Init(dictTrie, model);
    if (ret != OBP_SUCCESS) {
      OBP_LOG_WARN("failed to init mix seg. ret=%d", ret);
      return ret;
    }
    trie_ = dictTrie;

    OBP_LOG_TRACE("init query segment done. ret=%d", ret);
    return ret;
  }
  
  int Cut(const string& sentence, vector<string>& words) const override {
    return Cut(sentence, words, true);
  }
  int Cut(const string& sentence, vector<string>& words, bool hmm) const {
    vector<Word> tmp;
    int ret = Cut(sentence, tmp, hmm);
    if (ret == OBP_SUCCESS) {
      GetStringsFromWords(tmp, words);
    }
    return ret;
  }
  int Cut(const string& sentence, vector<Word>& words, bool hmm = true) const {
    int ret = OBP_SUCCESS;
    PreFilter pre_filter;
    ret = pre_filter.Init(symbols_, sentence);
    if (ret != OBP_SUCCESS) {
      OBP_LOG_WARN("failed to init prefilter. ret=%d", ret);
      return ret;
    }
    PreFilter::Range range;
    vector<WordRange> wrs;
    wrs.reserve(sentence.size()/2);
    while (pre_filter.HasNext()) {
      range = pre_filter.Next();
      ret = Cut(range.begin, range.end, wrs, hmm);
      if (ret != OBP_SUCCESS) {
        OBP_LOG_WARN("failed to cut sentence. ret=%d, sentence=%s", ret, sentence.c_str());
        return ret;
      }
    }
    words.clear();
    words.reserve(wrs.size());
    GetWordsFromWordRanges(sentence, wrs, words);
    return ret;
  }
  int Cut(RuneStrArray::const_iterator begin, RuneStrArray::const_iterator end, vector<WordRange>& res, bool hmm) const {
    //use mix Cut first
    vector<WordRange> mixRes;
    int ret = mixSeg_.Cut(begin, end, mixRes, hmm);
    if (ret != OBP_SUCCESS) {
      OBP_LOG_WARN("mix seg cut fail. ret=%d", ret);
      return ret;
    }

    vector<WordRange> fullRes;
    for (vector<WordRange>::const_iterator mixResItr = mixRes.begin(); mixResItr != mixRes.end(); mixResItr++) {
      if (mixResItr->Length() > 2) {
        for (size_t i = 0; i + 1 < mixResItr->Length(); i++) {
          WordRange wr(mixResItr->left + i, mixResItr->left + i + 1);
          if (trie_->Find(wr.left, wr.right + 1) != NULL) {
            res.push_back(wr);
          }
        }
      }
      if (mixResItr->Length() > 3) {
        for (size_t i = 0; i + 2 < mixResItr->Length(); i++) {
          WordRange wr(mixResItr->left + i, mixResItr->left + i + 2);
          if (trie_->Find(wr.left, wr.right + 1) != NULL) {
            res.push_back(wr);
          }
        }
      }
      res.push_back(*mixResItr);
    }
    return ret;
  }
 private:
  bool IsAllAscii(const Unicode& s) const {
   for(size_t i = 0; i < s.size(); i++) {
     if (s[i] >= 0x80) {
       return false;
     }
   }
   return true;
  }
  MixSegment mixSeg_;
  const DictTrie* trie_ = nullptr;
}; // QuerySegment

} // namespace cppjieba

#endif
