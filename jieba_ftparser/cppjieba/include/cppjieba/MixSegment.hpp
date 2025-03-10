#ifndef CPPJIEBA_MIXSEGMENT_H
#define CPPJIEBA_MIXSEGMENT_H

#include "MPSegment.hpp"
#include "HMMSegment.hpp"
#include "limonp/StringUtil.hpp"
#include "PosTagger.hpp"

namespace cppjieba {
class MixSegment final : public SegmentTagged {
 public:
  MixSegment() = default;

  virtual ~MixSegment() {
  }

  int Init(const string& mpSegDict, const string& hmmSegDict, const string& userDict = "")
  {
    int ret = OBP_SUCCESS;
    ret = SegmentTagged::Init();
    if (ret != OBP_SUCCESS) {
      OBP_LOG_WARN("failed to init segment tagged. ret=%d", ret);
      return ret;
    }
    ret = mpSeg_.Init(mpSegDict, userDict);
    if (ret != OBP_SUCCESS) {
      OBP_LOG_WARN("failed to init mix segment. mp seg dict=%s, user dict=%s, ret=%d",
                   mpSegDict.c_str(), userDict.c_str(), ret);
      return ret;
    }
    ret = hmmSeg_.Init(hmmSegDict);
    if (ret != OBP_SUCCESS) {
      OBP_LOG_WARN("failed to init mix segment: hmmseg init fail. hmmSegDict=%s, ret=%d",
                   hmmSegDict.c_str(), ret);
      return ret;
    }
    return ret;
  }

  int Init(const DictTrie* dictTrie, const HMMModel* model) {
    int ret = OBP_SUCCESS;
    ret = SegmentTagged::Init();
    if (ret != OBP_SUCCESS) {
      OBP_LOG_WARN("failed to init segment tagged. ret=%d", ret);
      return ret;
    }
    ret = mpSeg_.Init(dictTrie);
    if (ret != OBP_SUCCESS) {
      OBP_LOG_WARN("failed to init mix segment: mpseg init fail. dictTrie=%p, ret=%d", dictTrie, ret);
      return ret;
    }
    ret = hmmSeg_.Init(model);
    if (ret != OBP_SUCCESS) {
      OBP_LOG_WARN("failed to init mix segment: hmmseg init fail. model=%p, ret=%d", model, ret);
      return ret;
    }
    return ret;
  }
  int Cut(const string& sentence, vector<string>& words) const {
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
      OBP_LOG_WARN("failed to init pre filter. ret=%d", ret);
      return ret;
    }

    // 用prefilter 把文字分段，一段段处理
    // 通常分段处理可以减少一次获取的最大内存大小
    PreFilter::Range range;
    vector<WordRange> wrs;
    wrs.reserve(sentence.size() / 2);
    while (pre_filter.HasNext()) {
      range = pre_filter.Next();
      ret = Cut(range.begin, range.end, wrs, hmm);
      if (ret != OBP_SUCCESS) {
        OBP_LOG_WARN("failed to cut sentense. ret=%d, sentence=%s", ret, sentence.c_str());
        return ret;
      }
    }
    words.clear();
    words.reserve(wrs.size());
    GetWordsFromWordRanges(sentence, wrs, words);
    return ret;
  }

  int Cut(RuneStrArray::const_iterator begin, RuneStrArray::const_iterator end, vector<WordRange>& res, bool hmm) const {
    int ret = OBP_SUCCESS;
    if (!hmm) {
      return mpSeg_.Cut(begin, end, res);
    }

    if (end < begin) {
      OBP_LOG_WARN("failed to cut: invalid argument. end < begin:%ld", end-begin);
      return OBP_INVALID_ARGUMENT;
    }

    vector<WordRange> words;
    words.reserve(end - begin);
    mpSeg_.Cut(begin, end, words);

    vector<WordRange> hmmRes;
    hmmRes.reserve(end - begin);
    for (size_t i = 0; i < words.size(); i++) {
      //if mp Get a word, it's ok, put it into result
      if (words[i].left != words[i].right || (words[i].left == words[i].right && mpSeg_.IsUserDictSingleChineseWord(words[i].left->rune))) {
        res.push_back(words[i]);
        continue;
      }

      // if mp Get a single one and it is not in userdict, collect it in sequence
      size_t j = i;
      while (j < words.size() && words[j].left == words[j].right && !mpSeg_.IsUserDictSingleChineseWord(words[j].left->rune)) {
        j++;
      }

      // Cut the sequence with hmm
      if (j - 1 < i) {
        OBP_LOG_WARN("failed to do mix segment: j(%ld)-1 < i(%ld)", j, i);
        return OBP_PLUGIN_ERROR;
      }

      // TODO
      ret = hmmSeg_.Cut(words[i].left, words[j - 1].left + 1, hmmRes);
      if (ret != OBP_SUCCESS) {
        OBP_LOG_WARN("failed to do mix segment: hmmSeg cut fail. ret=%d", ret);
        return ret;
      }

      //put hmm result to result
      for (size_t k = 0; k < hmmRes.size(); k++) {
        res.push_back(hmmRes[k]);
      }

      //clear tmp vars
      hmmRes.clear();

      //let i jump over this piece
      i = j - 1;
    }
    return ret;
  }

  const DictTrie* GetDictTrie() const override {
    return mpSeg_.GetDictTrie();
  }

  bool Tag(const string& src, vector<pair<string, string> >& res) const override {
    return tagger_.Tag(src, res, *this);
  }

  int LookupTag(const string &str, string& result) const {
    int ret = tagger_.LookupTag(str, *this, result);
    if (ret != OBP_SUCCESS) {
      OBP_LOG_WARN("failed to lookuptag. ret=%d, str=%s", ret, str.c_str());
      return ret;
    }
    return ret;
  }

 private:
  MPSegment mpSeg_;
  HMMSegment hmmSeg_;
  PosTagger tagger_;

}; // class MixSegment

} // namespace cppjieba

#endif // CPPJIEBA_MIXSEGMENT_H
