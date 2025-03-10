#ifndef CPPJIEBA_SEGMENTBASE_H
#define CPPJIEBA_SEGMENTBASE_H

#include "PreFilter.hpp"

namespace cppjieba {

/// 默认的拆分文字的字符。包括一些空字符、全角逗号和句号。
const char* const SPECIAL_SEPARATORS = " \t\n\xEF\xBC\x8C\xE3\x80\x82";

using namespace limonp;

class SegmentBase {
 public:
  SegmentBase() {
  }
  virtual ~SegmentBase() {
  }

  int Init() {
    if (!ResetSeparators(SPECIAL_SEPARATORS)) {
      OBP_LOG_WARN("failed to set setparators");
      return OBP_NOT_INIT;
    }
    return OBP_SUCCESS;
  }

  virtual int Cut(const string& sentence, vector<string>& words) const = 0;

  bool ResetSeparators(const string& s) {
    symbols_.clear();
    RuneStrArray runes;
    if (!DecodeUTF8RunesInString(s, runes)) {
      OBP_LOG_WARN("UTF-8 decode failed for separators: %s", s.c_str());
      return false;
    }
    for (size_t i = 0; i < runes.size(); i++) {
      if (!symbols_.insert(runes[i].rune).second) {
        OBP_LOG_WARN("%s already exists", s.substr(runes[i].offset, runes[i].len).c_str());
        return false;
      }
    }
    return true;
  }
 protected:
  unordered_set<Rune> symbols_; /// 这个字符用来将文字拆分成多段。所以看起来这个命名并不贴切
}; // class SegmentBase

} // cppjieba

#endif
