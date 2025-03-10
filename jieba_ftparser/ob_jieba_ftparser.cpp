/*
 * Copyright (c) 2024 OceanBase.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <filesystem>
#include <unordered_map>
#include <string>
#include <exception>

#include "cppjieba/Jieba.hpp"
#include "oceanbase/ob_plugin_ftparser.h"

#ifndef LIBRARY_VERSION
#define LIBRARY_VERSION OBP_MAKE_VERSION(0, 2, 0)
#endif  // LIBRARY_VERSION

using namespace std;
using namespace cppjieba;

extern unordered_map<string, const char *> g_resources;

const char *get_dict(const char *name)
{
  auto iter = g_resources.find(name);
  if (iter == g_resources.end()) {
    return "";
  }
  return iter->second;
}
int ftparser_init(ObPluginParamPtr param)
{
  int ret = OBP_SUCCESS;

  try {
    istringstream dict_stream(string(get_dict("jieba.dict.utf8")));
    istringstream model_stream(string(get_dict("hmm_model.utf8")));
    unique_ptr<istringstream> user_dict_stream = make_unique<istringstream>(string(get_dict("user.dict.utf8")));
    istringstream idf_stream(string(get_dict("idf.utf8")));
    istringstream stop_word_stream(string(get_dict("stop_words.utf8")));

    vector<unique_ptr<istream>> user_dict_streams;
    user_dict_streams.emplace_back(std::move(user_dict_stream));

    Jieba *jieba = new (std::nothrow) Jieba();
    if (nullptr == jieba) {
      OBP_LOG_WARN("failed to create jieba object. object size=%lu", sizeof(Jieba));
      return OBP_ALLOCATE_MEMORY_FAILED;
    }
    ret = jieba->Init(dict_stream, model_stream, user_dict_streams, idf_stream, stop_word_stream);
    if (ret != OBP_SUCCESS) {
      OBP_LOG_WARN("failed to init jieba. ret=%d", ret);
      return ret;
    }

    obp_param_set_plugin_user_data(param, jieba);
  } catch (std::exception &ex) {
    OBP_LOG_WARN("catch std exception: %s", ex.what());
    ret = OBP_PLUGIN_ERROR;
  } catch (...) {
    OBP_LOG_WARN("catch unknown type exception");
    ret = OBP_PLUGIN_ERROR;
  }
  OBP_LOG_INFO("jieba parser init done. ret=%d", ret);
  return ret;
}

int ftparser_deinit(ObPluginParamPtr param)
{
  int ret = OBP_SUCCESS;
  try {
    Jieba *jieba = static_cast<Jieba *>(obp_param_plugin_user_data(param));
    if (nullptr != jieba) {
      delete jieba;
      obp_param_set_plugin_user_data(param, nullptr);
    }
  } catch (std::exception &ex) {
    OBP_LOG_WARN("catch std exception: %s", ex.what());
    ret = OBP_PLUGIN_ERROR;
  } catch (...) {
    OBP_LOG_WARN("catch unknown type exception");
    ret = OBP_PLUGIN_ERROR;
  }
  OBP_LOG_INFO("jieba parser deinit done");
  return ret;
}

struct JiebaFtparserContext
{
  vector<string> words;
  ssize_t index = -1;
};

int ftparser_scan_begin(ObPluginFTParserParamPtr param)
{
  int ret = OBP_SUCCESS;
  try {
    ObPluginCharsetInfoPtr charset = obp_ftparser_charset_info(param);
    if (OBP_SUCCESS != obp_charset_is_utf8mb4(charset)) {
      OBP_LOG_WARN("only utf8 supported, but got: %s", obp_charset_csname(charset));
      return OBP_NOT_SUPPORTED;
    }

    string sentence(obp_ftparser_fulltext(param), obp_ftparser_fulltext_length(param));

    ObPluginParamPtr plugin_param = obp_ftparser_plugin_param(param);
    Jieba *jieba = static_cast<Jieba *>(obp_param_plugin_user_data(plugin_param));
    if (nullptr == jieba) {
      OBP_LOG_WARN("parser begin failed. got a null jieba object");
      return OBP_INVALID_ARGUMENT;
    }

    JiebaFtparserContext *context = new (nothrow) JiebaFtparserContext();
    if (nullptr == context) {
      OBP_LOG_WARN("failed to allocate memory for context. size=%ld", sizeof(JiebaFtparserContext));
      return OBP_ALLOCATE_MEMORY_FAILED;
    }

    ret = jieba->Cut(sentence, context->words);
    if (ret != OBP_SUCCESS) {
      OBP_LOG_WARN("failed to cut sentence(%s), ret=%d", sentence.c_str(), ret);
      delete context;
      context = nullptr;
      return ret;
    }
    obp_ftparser_set_user_data(param, context);
    OBP_LOG_TRACE("jieba cut sentence. sentence=%s, words count=%lu", sentence.c_str(), context->words.size());
  } catch (std::exception &ex) {
    OBP_LOG_WARN("catch std exception: %s", ex.what());
    ret = OBP_PLUGIN_ERROR;
  } catch (...) {
    OBP_LOG_WARN("catch unknown type exception");
    ret = OBP_PLUGIN_ERROR;
  }
  return ret;
}

int ftparser_scan_end(ObPluginFTParserParamPtr param)
{
  int ret = OBP_SUCCESS;
  try {
    JiebaFtparserContext *context = static_cast<JiebaFtparserContext *>(obp_ftparser_user_data(param));
    if (nullptr != context) {
      delete context;
      obp_ftparser_set_user_data(param, nullptr);
    }
  } catch (std::exception &ex) {
    OBP_LOG_WARN("catch std exception: %s", ex.what());
    ret = OBP_PLUGIN_ERROR;
  } catch (...) {
    OBP_LOG_WARN("catch unknown type exception");
    ret = OBP_PLUGIN_ERROR;
  }
  return ret;
}

int ftparser_next_token(ObPluginFTParserParamPtr param,
                        char **word,
                        int64_t *word_len,
                        int64_t *char_cnt,
                        int64_t *word_freq)
{
  int ret = OBP_SUCCESS;
  try {
    if (nullptr == param) {
      return OBP_INVALID_ARGUMENT;
    }
    
    JiebaFtparserContext *context = static_cast<JiebaFtparserContext *>(obp_ftparser_user_data(param));
    if (nullptr == context) {
      return OBP_INVALID_ARGUMENT;
    }

    ObPluginParamPtr plugin_param = obp_ftparser_plugin_param(param);
    if (nullptr == plugin_param) {
      OBP_LOG_WARN("failed to get next token: plugin param is null");
      return OBP_INVALID_ARGUMENT;
    }

    Jieba *jieba = static_cast<Jieba *>(obp_param_plugin_user_data(plugin_param));
    if (nullptr == jieba) {
      OBP_LOG_WARN("parser begin failed. got a null jieba object");
      return OBP_INVALID_ARGUMENT;
    }
    
    if (word == nullptr || word_len == nullptr || char_cnt == nullptr || word_freq == nullptr) {
      return OBP_INVALID_ARGUMENT;
    }

    if (context->index >= static_cast<ssize_t>(context->words.size())) {
      return OBP_ITER_END;
    }

    context->index++;
    while (context->index < static_cast<ssize_t>(context->words.size())) {
      string &str_word = context->words[context->index];
      if (!jieba->extractor.IsStopWord(str_word)) {
        break;
      }
      context->index++;
    }
    if (context->index >= static_cast<ssize_t>(context->words.size())) {
      return OBP_ITER_END;
    }

    ObPluginCharsetInfoPtr charset = obp_ftparser_charset_info(param);
    string &str_word = context->words[context->index];
    *word = const_cast<char *>(str_word.data());
    *word_len = str_word.length();
    *char_cnt = obp_charset_numchars(charset, str_word.data(), str_word.data() + str_word.length());
    *word_freq = 1;
    OBP_LOG_TRACE("got a word:%s", str_word.c_str());
  } catch (std::exception &ex) {
    OBP_LOG_WARN("catch std exception: %s", ex.what());
    ret = OBP_PLUGIN_ERROR;
  } catch (...) {
    OBP_LOG_WARN("catch unknown type exception");
    ret = OBP_PLUGIN_ERROR;
  }
  return ret;
}

int ftparser_get_add_word_flag(uint64_t *flag)
{
  int ret = OBP_SUCCESS;
  if (flag == nullptr) {
    ret = OBP_INVALID_ARGUMENT;
  } else {
    *flag = OBP_FTPARSER_AWF_STOPWORD
            | OBP_FTPARSER_AWF_CASEDOWN
            | OBP_FTPARSER_AWF_GROUPBY_WORD;
  }
  return ret;
}

int ftparser_is_charset_supported(ObPluginCharsetInfoPtr cs)
{
  int ret = OBP_SUCCESS;
  if (cs == nullptr || OBP_SUCCESS != obp_charset_is_utf8mb4(cs)) {
    ret = OBP_NOT_SUPPORTED;
    OBP_LOG_INFO("charset is not supported: %s", cs == nullptr ? "(null)" : obp_charset_csname(cs));
  }
  return ret;
}

int plugin_init(ObPluginParamPtr plugin)
{
  int ret = OBP_SUCCESS;
  ObPluginFTParser parser = {
    .init              = ftparser_init,
    .deinit            = ftparser_deinit,
    .scan_begin        = ftparser_scan_begin,
    .scan_end          = ftparser_scan_end,
    .next_token        = ftparser_next_token,
    .get_add_word_flag = ftparser_get_add_word_flag,
    .is_charset_supported = ftparser_is_charset_supported
  };

  ret = OBP_REGISTER_FTPARSER(plugin,
                              "jieba",
                              parser,
                              "jieba full text parser for oceanbase(demo).");
  return ret;
}

OBP_DECLARE_PLUGIN(jieba_ftparser)
{
  OBP_AUTHOR_OCEANBASE,
  LIBRARY_VERSION,
  OBP_LICENSE_APACHE_V2,
  plugin_init, // init
  nullptr, // deinit
} OBP_DECLARE_PLUGIN_END;
