#ifndef CPPJIEBA_PRE_FILTER_H
#define CPPJIEBA_PRE_FILTER_H

#include "Trie.hpp"

namespace cppjieba {

/**
 * 把给定的文字拆分成多段，比如遇到空格、分行等
 */
class PreFilter final {
 public:
  //TODO use WordRange instead of Range
  struct Range {
    RuneStrArray::const_iterator begin;
    RuneStrArray::const_iterator end;
  }; // struct Range

  PreFilter() = default;
  ~PreFilter() {
  }

  /**
   * 初始化
   * @param[in] symbols 可以按照在这个数组中的字符进行拆分
   * @param[in] sentence 要拆分的文字
   */
  int Init(const unordered_set<Rune>& symbols, const string& sentence) {
    symbols_ = &symbols;
    if (!DecodeUTF8RunesInString(sentence, sentence_)) {
      OBP_LOG_WARN("UTF-8 decode failed for input sentence(%s)", sentence.c_str());
      return OBP_INVALID_ARGUMENT;
    }
    cursor_ = sentence_.begin();
    return OBP_SUCCESS;
  }
  bool HasNext() const {
    return cursor_ != sentence_.end();
  }

  /**
   * 获取下一个分段
   */
  Range Next() {
    Range range;

    if (nullptr == symbols_) {
      OBP_LOG_WARN("pre filter is not init. symbols is null");
      return range;
    }
    range.begin = cursor_;
    while (cursor_ != sentence_.end()) {
      if (IsIn(*symbols_, cursor_->rune)) {
        if (range.begin == cursor_) {
          cursor_ ++;
        }
        range.end = cursor_;
        return range;
      }
      cursor_ ++;
    }
    range.end = sentence_.end();
    return range;
  }
 private:
  RuneStrArray::const_iterator cursor_;
  RuneStrArray sentence_;
  const unordered_set<Rune>* symbols_ = nullptr;
}; // class PreFilter

} // namespace cppjieba

#endif // CPPJIEBA_PRE_FILTER_H
