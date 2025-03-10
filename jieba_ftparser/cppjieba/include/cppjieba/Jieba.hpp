#ifndef CPPJIEAB_JIEBA_H
#define CPPJIEAB_JIEBA_H

#include "QuerySegment.hpp"
#include "KeywordExtractor.hpp"

namespace cppjieba {

class Jieba final {
 public:
  Jieba() = default;
  ~Jieba() = default;

  int Init(const string& dict_path = "", 
           const string& model_path = "",
           const string& user_dict_path = "", 
           const string& idf_path = "", 
           const string& stop_word_path = "") {
    int ret = OBP_SUCCESS;
    const string& dict_path_tmp = getPath(dict_path, "jieba.dict.utf8");
    const string& user_dict_path_tmp = getPath(user_dict_path, "user.dict.utf8");
    const string& model_path_tmp = getPath(model_path, "hmm_model.utf8");
    const string& idf_path_tmp = getPath(idf_path, "idf.utf8");
    const string& stop_word_path_tmp = getPath(stop_word_path, "stop_words.utf8");

    ret = dict_trie_.Init(dict_path_tmp, user_dict_path_tmp);
    if (ret != OBP_SUCCESS) {
      OBP_LOG_WARN("failed to init dict trie. dict path=%s, user dict=%s. ret=%d",
                   dict_path_tmp.c_str(), user_dict_path_tmp.c_str(), ret);
      return ret;
    }

    ret = model_.Init(model_path_tmp);
    if (ret != OBP_SUCCESS) {
      OBP_LOG_WARN("failed to init dict trie. model path=%s. ret=%d", model_path_tmp.c_str(), ret);
      return ret;
    }
    
    ret = mp_seg_.Init(&dict_trie_);
    if (ret != OBP_SUCCESS) {
      OBP_LOG_WARN("failed to init mp segment. ret=%d", ret);
      return ret;
    }
    ret = hmm_seg_.Init(&model_);
    if (ret != OBP_SUCCESS) {
      OBP_LOG_WARN("failed to init hmm segment. ret=%d", ret);
      return ret;
    }
    ret = mix_seg_.Init(&dict_trie_, &model_);
    if (ret != OBP_SUCCESS) {
      OBP_LOG_WARN("failed to init mix segment. ret=%d", ret);
      return ret;
    }

    ret = full_seg_.Init(&dict_trie_);
    if (ret != OBP_SUCCESS) {
      OBP_LOG_WARN("failed to init full segment. ret=%d", ret);
      return ret;
    }
    ret = query_seg_.Init(&dict_trie_, &model_);
    if (ret != OBP_SUCCESS) {
      OBP_LOG_WARN("failed to init query segment. ret=%d", ret);
      return ret;
    }

    ret = extractor.Init(&dict_trie_, &model_, idf_path_tmp, stop_word_path_tmp);
    if (ret != OBP_SUCCESS) {
      OBP_LOG_WARN("failed to init extractor. idf path=%s, stop word=%s. ret=%d",
                   idf_path_tmp.c_str(), stop_word_path_tmp.c_str(), ret);
      return ret;
    }
    return ret;
  }

  int Init(istream& dict_s, istream& model_s, vector<unique_ptr<istream>>& user_dict_s, istream& idf_s, istream& stop_word_s) {
    int ret = OBP_SUCCESS;
    ret = dict_trie_.Init(dict_s, user_dict_s);
    if (ret != OBP_SUCCESS) {
      OBP_LOG_WARN("failed to init dict trie. ret=%d", ret);
      return ret;
    }

    ret = model_.Init(model_s);
    if (ret != OBP_SUCCESS) {
      OBP_LOG_WARN("failed to init dict trie. ret=%d", ret);
      return ret;
    }
    
    ret = mp_seg_.Init(&dict_trie_);
    if (ret != OBP_SUCCESS) {
      OBP_LOG_WARN("failed to init mp segment. ret=%d", ret);
      return ret;
    }
    ret = hmm_seg_.Init(&model_);
    if (ret != OBP_SUCCESS) {
      OBP_LOG_WARN("failed to init hmm segment. ret=%d", ret);
      return ret;
    }
    ret = mix_seg_.Init(&dict_trie_, &model_);
    if (ret != OBP_SUCCESS) {
      OBP_LOG_WARN("failed to init mix segment. ret=%d", ret);
      return ret;
    }

    ret = full_seg_.Init(&dict_trie_);
    if (ret != OBP_SUCCESS) {
      OBP_LOG_WARN("failed to init full segment. ret=%d", ret);
      return ret;
    }
    ret = query_seg_.Init(&dict_trie_, &model_);
    if (ret != OBP_SUCCESS) {
      OBP_LOG_WARN("failed to init query segment. ret=%d", ret);
      return ret;
    }

    ret = extractor.Init(&dict_trie_, &model_, idf_s, stop_word_s);
    if (ret != OBP_SUCCESS) {
      OBP_LOG_WARN("failed to init extractor. ret=%d", ret);
      return ret;
    }
    return ret;
  }

  struct LocWord {
    string word;
    size_t begin;
    size_t end;
  }; // struct LocWord

  int Cut(const string& sentence, vector<string>& words, bool hmm = true) const {
    return mix_seg_.Cut(sentence, words, hmm);
  }
  int Cut(const string& sentence, vector<Word>& words, bool hmm = true) const {
    return mix_seg_.Cut(sentence, words, hmm);
  }
  int CutAll(const string& sentence, vector<string>& words) const {
    return full_seg_.Cut(sentence, words);
  }
  int CutAll(const string& sentence, vector<Word>& words) const {
    return full_seg_.Cut(sentence, words);
  }
  int CutForSearch(const string& sentence, vector<string>& words, bool hmm = true) const {
    return query_seg_.Cut(sentence, words, hmm);
  }
  int CutForSearch(const string& sentence, vector<Word>& words, bool hmm = true) const {
    return query_seg_.Cut(sentence, words, hmm);
  }
  int CutHMM(const string& sentence, vector<string>& words) const {
    return hmm_seg_.Cut(sentence, words);
  }
  int CutHMM(const string& sentence, vector<Word>& words) const {
    return hmm_seg_.Cut(sentence, words);
  }
  int CutSmall(const string& sentence, vector<string>& words, size_t max_word_len) const {
    return mp_seg_.Cut(sentence, words, max_word_len);
  }
  int CutSmall(const string& sentence, vector<Word>& words, size_t max_word_len) const {
    return mp_seg_.Cut(sentence, words, max_word_len);
  }
  
  bool Tag(const string& sentence, vector<pair<string, string> >& words) const {
    return mix_seg_.Tag(sentence, words);
  }
  int LookupTag(const string &str, string& result) const {
    return mix_seg_.LookupTag(str, result);
  }
  bool InsertUserWord(const string& word, const string& tag = UNKNOWN_TAG) {
    return dict_trie_.InsertUserWord(word, tag);
  }

  bool InsertUserWord(const string& word,int freq, const string& tag = UNKNOWN_TAG) {
    return dict_trie_.InsertUserWord(word,freq, tag);
  }

  bool DeleteUserWord(const string& word, const string& tag = UNKNOWN_TAG) {
    return dict_trie_.DeleteUserWord(word, tag);
  }
  
  bool Find(const string& word)
  {
    return dict_trie_.Find(word);
  }

  void ResetSeparators(const string& s) {
    //TODO
    mp_seg_.ResetSeparators(s);
    hmm_seg_.ResetSeparators(s);
    mix_seg_.ResetSeparators(s);
    full_seg_.ResetSeparators(s);
    query_seg_.ResetSeparators(s);
  }

  const DictTrie* GetDictTrie() const {
    return &dict_trie_;
  } 
  
  const HMMModel* GetHMMModel() const {
    return &model_;
  }

  int LoadUserDict(const vector<string>& buf)  {
    return dict_trie_.LoadUserDict(buf);
  }

  int LoadUserDict(const set<string>& buf)  {
    return dict_trie_.LoadUserDict(buf);
  }

  int LoadUserDict(const string& path)  {
    return dict_trie_.LoadUserDict(path);
  }

 private:
  static string pathJoin(const string& dir, const string& filename) {
    if (dir.empty()) {
        return filename;
    }
    
    char last_char = dir[dir.length() - 1];
    if (last_char == '/' || last_char == '\\') {
        return dir + filename;
    } else {
        #ifdef _WIN32
        return dir + '\\' + filename;
        #else
        return dir + '/' + filename;
        #endif
    }
  }

  static string getCurrentDirectory() {
    string path(__FILE__);
    size_t pos = path.find_last_of("/\\");
    return (pos == string::npos) ? "" : path.substr(0, pos);
  }

  static string getPath(const string& path, const string& default_file) {
    if (path.empty()) {
      string current_dir = getCurrentDirectory();
      string parent_dir = current_dir.substr(0, current_dir.find_last_of("/\\"));
      string grandparent_dir = parent_dir.substr(0, parent_dir.find_last_of("/\\"));
      return pathJoin(pathJoin(grandparent_dir, "dict"), default_file);
    }
    return path;
  }

  DictTrie dict_trie_;
  HMMModel model_;
  
  // They share the same dict trie and model
  MPSegment mp_seg_;
  HMMSegment hmm_seg_;
  MixSegment mix_seg_;
  FullSegment full_seg_;
  QuerySegment query_seg_;

 public:
  KeywordExtractor extractor;
}; // class Jieba

} // namespace cppjieba

#endif // CPPJIEAB_JIEBA_H
