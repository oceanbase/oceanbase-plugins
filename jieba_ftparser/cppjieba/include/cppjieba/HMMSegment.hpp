#ifndef CPPJIBEA_HMMSEGMENT_H
#define CPPJIBEA_HMMSEGMENT_H

#include <iostream>
#include <fstream>
#include <memory.h>
#include "HMMModel.hpp"
#include "SegmentBase.hpp"

namespace cppjieba {
class HMMSegment final: public SegmentBase {
 public:
  HMMSegment() = default;
  virtual ~HMMSegment() {
    if (isNeedDestroy_) {
      delete model_;
      model_ = nullptr;
    }
  }

  int Init(const string& filePath) {
    int ret = SegmentBase::Init();
    if (ret != OBP_SUCCESS) {
      OBP_LOG_WARN("failed to init segment base. ret=%d", ret);
      return ret;
    }
    HMMModel* model = new (std::nothrow) HMMModel();
    if (nullptr == model) {
      OBP_LOG_WARN("failed to allocate memory. size=%ld", sizeof(HMMModel));
      return OBP_ALLOCATE_MEMORY_FAILED;
    }

    ret = model->Init(filePath);
    if (ret != OBP_SUCCESS) {
      OBP_LOG_WARN("failed to init hmmmodel. ret=%d", ret);
      delete model;
      model = nullptr;
      return ret;
    }
    model_ = model;
    isNeedDestroy_ = true;
    return OBP_SUCCESS;
  }

  int Init(const HMMModel* model) {
    int ret = SegmentBase::Init();
    if (ret != OBP_SUCCESS) {
      OBP_LOG_WARN("failed to init segment base. ret=%d", ret);
      return ret;
    }
    if (nullptr == model) {
      OBP_LOG_WARN("model is null");
      return OBP_INVALID_ARGUMENT;
    }
    model_ = model;
    isNeedDestroy_ = false;
    return OBP_SUCCESS;
  }
  int Cut(const string& sentence, 
        vector<string>& words) const override {
    vector<Word> tmp;
    int ret = Cut(sentence, tmp);
    if (OBP_SUCCESS == ret) {
      GetStringsFromWords(tmp, words);
    }
    return ret;
  }
  int Cut(const string& sentence, vector<Word>& words) const {
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
        OBP_LOG_WARN("failed to do cut. ret=%d", ret);
        return ret;
      }
    }
    words.clear();
    words.reserve(wrs.size());
    GetWordsFromWordRanges(sentence, wrs, words);
    return ret;
  }
  int Cut(RuneStrArray::const_iterator begin, RuneStrArray::const_iterator end, vector<WordRange>& res) const {
    int ret = OBP_SUCCESS;
    RuneStrArray::const_iterator left = begin;
    RuneStrArray::const_iterator right = begin;
    while (right != end) {
      if (right->rune < 0x80) {
        if (left != right) {
          ret = InternalCut(left, right, res);
          if (ret != OBP_SUCCESS) {
            OBP_LOG_WARN("failed to do internal cut. ret=%d", ret);
            return ret;
          }
        }
        left = right;
        do {
          right = SequentialLetterRule(left, end);
          if (right != left) {
            break;
          }
          right = NumbersRule(left, end);
          if (right != left) {
            break;
          }
          right ++;
        } while (false);
        WordRange wr(left, right - 1);
        res.push_back(wr);
        left = right;
      } else {
        right++;
      }
    }
    if (left != right) {
      ret = InternalCut(left, right, res);
    }
    return ret;
  }
 private:
  // sequential letters rule
  RuneStrArray::const_iterator SequentialLetterRule(RuneStrArray::const_iterator begin, RuneStrArray::const_iterator end) const {
    Rune x = begin->rune;
    if (('a' <= x && x <= 'z') || ('A' <= x && x <= 'Z')) {
      begin ++;
    } else {
      return begin;
    }
    while (begin != end) {
      x = begin->rune;
      if (('a' <= x && x <= 'z') || ('A' <= x && x <= 'Z') || ('0' <= x && x <= '9')) {
        begin ++;
      } else {
        break;
      }
    }
    return begin;
  }
  //
  RuneStrArray::const_iterator NumbersRule(RuneStrArray::const_iterator begin, RuneStrArray::const_iterator end) const {
    Rune x = begin->rune;
    if ('0' <= x && x <= '9') {
      begin ++;
    } else {
      return begin;
    }
    while (begin != end) {
      x = begin->rune;
      if ( ('0' <= x && x <= '9') || x == '.') {
        begin++;
      } else {
        break;
      }
    }
    return begin;
  }
  int InternalCut(RuneStrArray::const_iterator begin, RuneStrArray::const_iterator end, vector<WordRange>& res) const {
    vector<size_t> status;
    Viterbi(begin, end, status);

    RuneStrArray::const_iterator left = begin;
    RuneStrArray::const_iterator right;
    for (size_t i = 0; i < status.size(); i++) {
      if (status[i] % 2) { //if (HMMModel::E == status[i] || HMMModel::S == status[i])
        right = begin + i + 1;
        WordRange wr(left, right - 1);
        res.push_back(wr);
        left = right;
      }
    }
    return OBP_SUCCESS;
  }

  void Viterbi(RuneStrArray::const_iterator begin, 
        RuneStrArray::const_iterator end, 
        vector<size_t>& status) const {
    size_t Y = HMMModel::STATUS_SUM;
    size_t X = end - begin;

    size_t XYSize = X * Y;
    size_t now, old, stat;
    double tmp, endE, endS;

    vector<int> path(XYSize);
    vector<double> weight(XYSize);

    //start
    for (size_t y = 0; y < Y; y++) {
      weight[0 + y * X] = model_->startProb[y] + model_->GetEmitProb(model_->emitProbVec[y], begin->rune, MIN_DOUBLE);
      path[0 + y * X] = -1;
    }

    double emitProb;

    for (size_t x = 1; x < X; x++) {
      for (size_t y = 0; y < Y; y++) {
        now = x + y*X;
        weight[now] = MIN_DOUBLE;
        path[now] = HMMModel::E; // warning
        emitProb = model_->GetEmitProb(model_->emitProbVec[y], (begin+x)->rune, MIN_DOUBLE);
        for (size_t preY = 0; preY < Y; preY++) {
          old = x - 1 + preY * X;
          tmp = weight[old] + model_->transProb[preY][y] + emitProb;
          if (tmp > weight[now]) {
            weight[now] = tmp;
            path[now] = preY;
          }
        }
      }
    }

    endE = weight[X-1+HMMModel::E*X];
    endS = weight[X-1+HMMModel::S*X];
    stat = 0;
    if (endE >= endS) {
      stat = HMMModel::E;
    } else {
      stat = HMMModel::S;
    }

    status.resize(X);
    for (int x = X -1 ; x >= 0; x--) {
      status[x] = stat;
      stat = path[x + stat*X];
    }
  }

  const HMMModel* model_ = nullptr;
  bool isNeedDestroy_ = false;
}; // class HMMSegment

} // namespace cppjieba

#endif
