#ifndef CPPJIEBA_HMMMODEL_H
#define CPPJIEBA_HMMMODEL_H

#include "limonp/StringUtil.hpp"
#include "Trie.hpp"

namespace cppjieba {

using namespace limonp;
typedef unordered_map<Rune, double> EmitProbMap;

struct HMMModel final {
  /*
   * STATUS:
   * 0: HMMModel::B, 1: HMMModel::E, 2: HMMModel::M, 3:HMMModel::S
   * */
  enum {B = 0, E = 1, M = 2, S = 3, STATUS_SUM = 4};

  HMMModel() {
    memset(startProb, 0, sizeof(startProb));
    memset(transProb, 0, sizeof(transProb));
  }
  ~HMMModel() {
  }
  int Init(const string& modelPath) {
    ifstream ifile(modelPath.c_str());
    if (!ifile.is_open()) {
      OBP_LOG_WARN("failed to open file: %s. error=%s", modelPath.c_str(), strerror(errno));
      return OBP_INVALID_ARGUMENT;
    }
    return Init(ifile);
  }

  int Init(istream& modelStream) {
    memset(startProb, 0, sizeof(startProb));
    memset(transProb, 0, sizeof(transProb));
    statMap[0] = 'B';
    statMap[1] = 'E';
    statMap[2] = 'M';
    statMap[3] = 'S';
    emitProbVec.push_back(&emitProbB);
    emitProbVec.push_back(&emitProbE);
    emitProbVec.push_back(&emitProbM);
    emitProbVec.push_back(&emitProbS);
    return LoadModel(modelStream);
  }

  int LoadModel(istream& ifile) {
    string line;
    vector<string> tmp;
    vector<string> tmp2;
    //Load startProb
    if (!GetLine(ifile, line)) {
      OBP_LOG_WARN("failed to get startProb line");
      return OBP_INVALID_ARGUMENT;
    }
    Split(line, tmp, " ");
    if (tmp.size() != STATUS_SUM) {
      OBP_LOG_WARN("expect field num is STATUS_SUM(%d), but got %ld. line=%s", STATUS_SUM, tmp.size(), line.c_str());
      return OBP_INVALID_ARGUMENT;
    }

    for (size_t j = 0; j< tmp.size(); j++) {
      startProb[j] = atof(tmp[j].c_str());
    }

    //Load transProb
    for (size_t i = 0; i < STATUS_SUM; i++) {
      if (!GetLine(ifile, line)) {
        OBP_LOG_WARN("failed to get line. i=%ld", i);
        return OBP_INVALID_ARGUMENT;
      }
      Split(line, tmp, " ");
      if (tmp.size() != STATUS_SUM) {
        OBP_LOG_WARN("expect field num is STATUS_SUM(%d), but got %ld. line=%s", STATUS_SUM, tmp.size(), line.c_str());
        return OBP_INVALID_ARGUMENT;
      }
      for (size_t j =0; j < STATUS_SUM; j++) {
        transProb[i][j] = atof(tmp[j].c_str());
      }
    }

    //Load emitProbB
    if (!GetLine(ifile, line)) {
      OBP_LOG_WARN("failed to read emitProbB line");
      return OBP_INVALID_ARGUMENT;
    }
    if (!LoadEmitProb(line, emitProbB)) {
      OBP_LOG_WARN("failed to load emitProbB. line=%s", line.c_str());
      return OBP_INVALID_ARGUMENT;
    }

    //Load emitProbE
    if (!GetLine(ifile, line)) {
      OBP_LOG_WARN("failed to read emitProbE line");
      return OBP_INVALID_ARGUMENT;
    }
    if (!LoadEmitProb(line, emitProbE)) {
      OBP_LOG_WARN("failed to load emitProbE. line=%s", line.c_str());
      return OBP_INVALID_ARGUMENT;
    }

    //Load emitProbM
    if (!GetLine(ifile, line)) {
      OBP_LOG_WARN("failed to read emitProbM line");
      return OBP_INVALID_ARGUMENT;
    }
    if (!LoadEmitProb(line, emitProbM)) {
      OBP_LOG_WARN("failed to load emitProbM. line=%s", line.c_str());
      return OBP_INVALID_ARGUMENT;
    }

    //Load emitProbS
    if (!GetLine(ifile, line)) {
      OBP_LOG_WARN("failed to read emitProbS line");
      return OBP_INVALID_ARGUMENT;
    }
    if (!LoadEmitProb(line, emitProbS)) {
      OBP_LOG_WARN("failed to load emitProbS. line=%s", line.c_str());
      return OBP_INVALID_ARGUMENT;
    }
    return OBP_SUCCESS;
  }
  double GetEmitProb(const EmitProbMap* ptMp, Rune key, 
        double defVal)const {
    EmitProbMap::const_iterator cit = ptMp->find(key);
    if (cit == ptMp->end()) {
      return defVal;
    }
    return cit->second;
  }
  bool GetLine(istream& ifile, string& line) {
    while (getline(ifile, line)) {
      Trim(line);
      if (line.empty()) {
        continue;
      }
      if (StartsWith(line, "#")) {
        continue;
      }
      return true;
    }
    return false;
  }
  bool LoadEmitProb(const string& line, EmitProbMap& mp) {
    if (line.empty()) {
      return false;
    }
    vector<string> tmp, tmp2;
    Unicode unicode;
    Split(line, tmp, ",");
    for (size_t i = 0; i < tmp.size(); i++) {
      Split(tmp[i], tmp2, ":");
      if (2 != tmp2.size()) {
        OBP_LOG_WARN("emitProb illegal. line=%s", line.c_str());
        return false;
      }
      if (!DecodeUTF8RunesInString(tmp2[0], unicode) || unicode.size() != 1) {
        OBP_LOG_WARN("TransCode failed. line=%s", line.c_str());
        return false;
      }
      mp[unicode[0]] = atof(tmp2[1].c_str());
    }
    return true;
  }

  char statMap[STATUS_SUM];
  double startProb[STATUS_SUM];
  double transProb[STATUS_SUM][STATUS_SUM];
  EmitProbMap emitProbB;
  EmitProbMap emitProbE;
  EmitProbMap emitProbM;
  EmitProbMap emitProbS;
  vector<EmitProbMap* > emitProbVec;
}; // struct HMMModel

} // namespace cppjieba

#endif
