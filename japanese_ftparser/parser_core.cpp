/*
 * Copyright (c) 2025 OceanBase.
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

#include "parser_core.h"
#include "jni_bridge.h"
#include <algorithm>
#include <cctype>

namespace oceanbase {
namespace ftparser {

// Forward declaration - actual implementation is in thai_jni_bridge.h/cpp
// We use the real JNI bridge now instead of the stub

// Constructor
ObFTParser::ObFTParser()
    : jni_bridge_(nullptr)
    , current_index_(0)
    , is_inited_(false)
    , charset_info_(0)
{
    OBP_LOG_TRACE("ObFTParser constructor called");
}

// Destructor
ObFTParser::~ObFTParser()
{
    reset();
    OBP_LOG_TRACE("ObFTParser destructor called");
}

// Initialize the parser
int ObFTParser::init(ObPluginFTParserParamPtr param)
{
    int ret = OBP_SUCCESS;
    
    if (is_inited_) {
        ret = OBP_INIT_TWICE;
        OBP_LOG_WARN("FT Parser already initialized. ret=%d, this=%p", ret, this);
        return ret;
    }
    
    if (0 == param) {
        ret = OBP_INVALID_ARGUMENT;
        OBP_LOG_WARN("Invalid parameter: param is null. ret=%d", ret);
        return ret;
    }
    
    // Get the text to be parsed from OceanBase
    const char *fulltext = obp_ftparser_fulltext(param);
    int64_t text_length = obp_ftparser_fulltext_length(param);
    ObPluginCharsetInfoPtr charset_info = obp_ftparser_charset_info(param);
    
    if (nullptr == fulltext || text_length < 0) {
        ret = OBP_INVALID_ARGUMENT;
        OBP_LOG_WARN("Invalid text: fulltext=%p, length=%ld. ret=%d", fulltext, text_length, ret);
        return ret;
    }
    
    if (0 == charset_info) {
        ret = OBP_INVALID_ARGUMENT;
        OBP_LOG_WARN("Invalid charset info. ret=%d", ret);
        return ret;
    }
    
    // Store the original text and charset info
    original_text_ = std::string(fulltext, text_length);
    charset_info_ = charset_info;
    
    OBP_LOG_INFO("Initializing FT Parser with text length=%ld", text_length);
    
    // Initialize JNI bridge
    ret = initialize_jni_bridge();
    if (OBP_SUCCESS != ret) {
        OBP_LOG_WARN("Failed to initialize JNI bridge. ret=%d", ret);
        reset();
        return ret;
    }
    
    // Perform text segmentation
    ret = perform_segmentation(original_text_);
    if (OBP_SUCCESS != ret) {
        OBP_LOG_WARN("Failed to perform segmentation. ret=%d", ret);
        reset();
        return ret;
    }
    
    // Initialize iteration state
    current_index_ = 0;
    is_inited_ = true;
    
    OBP_LOG_INFO("FT Parser initialized successfully. Found %zu tokens", tokens_.size());
    return OBP_SUCCESS;
}

// Reset the parser
void ObFTParser::reset()
{
    cleanup_jni_bridge();
    tokens_.clear();
    original_text_.clear();
    current_index_ = 0;
    is_inited_ = false;
    charset_info_ = 0;
    
    OBP_LOG_TRACE("FT Parser reset completed");
}

// Get next token
int ObFTParser::get_next_token(const char *&word,
                                  int64_t &word_len,
                                  int64_t &char_len,
                                  int64_t &word_freq)
{
    if (!is_inited_) {
        int ret = OBP_PLUGIN_ERROR;
        OBP_LOG_WARN("FT Parser not initialized. ret=%d, is_inited=%d", ret, is_inited_);
        return ret;
    }
    
    // Check if we have more tokens
    if (current_index_ >= tokens_.size()) {
        OBP_LOG_TRACE("No more tokens available. current_index=%zu, total_tokens=%zu", 
                      current_index_, tokens_.size());
        return OBP_ITER_END;
    }
    
    // Get the current token
    const TokenInfo& token = tokens_[current_index_];
    
    word = token.word.c_str();
    word_len = token.byte_length;
    char_len = token.char_length;
    word_freq = token.frequency;
    
    // Move to next token
    current_index_++;
    
    OBP_LOG_TRACE("Returned token[%zu]: word=%.*s, word_len=%ld, char_len=%ld, word_freq=%ld",
                  current_index_ - 1, (int)word_len, word, word_len, char_len, word_freq);
    
    return OBP_SUCCESS;
}

// Initialize JNI bridge
int ObFTParser::initialize_jni_bridge()
{
    if (nullptr != jni_bridge_) {
        OBP_LOG_WARN("JNI bridge already initialized");
        return OBP_SUCCESS;
    }
    
    // Get JNI bridge from manager (singleton pattern for JVM reuse)
    auto& manager = JNIBridgeManager::instance();
    auto bridge_ptr = manager.get_bridge();
    
    if (!bridge_ptr) {
        OBP_LOG_WARN("Failed to get JNI bridge from manager");
        return OBP_ALLOCATE_MEMORY_FAILED;
    }
    
    // Initialize the bridge if not already done
    int ret = bridge_ptr->initialize();
    if (OBP_SUCCESS != ret) {
        const auto& error = bridge_ptr->get_last_error();
        OBP_LOG_WARN("Failed to initialize JNI bridge. ret=%d, error=%s", 
                     ret, error.error_message.c_str());
        return ret;
    }
    
    jni_bridge_ = bridge_ptr.get(); // Store raw pointer for direct access
    
    OBP_LOG_INFO("JNI bridge initialized successfully");
    return OBP_SUCCESS;
}

// Cleanup JNI bridge
void ObFTParser::cleanup_jni_bridge()
{
    // We don't delete the bridge since it's managed by the singleton manager
    // The manager will handle cleanup when appropriate
    jni_bridge_ = nullptr;
}

// Perform segmentation
int ObFTParser::perform_segmentation(const std::string& text)
{
    if (nullptr == jni_bridge_) {
        OBP_LOG_WARN("JNI bridge not initialized");
        return OBP_PLUGIN_ERROR;
    }
    
    try {
        // Get tokens from JNI bridge
        OBP_LOG_INFO("=== C++ SEGMENTATION START === Input text: '%s' (length: %zu)", text.c_str(), text.length());
        std::vector<std::string> raw_tokens;
        int ret = jni_bridge_->segment(text, raw_tokens);
        
        if (ret != OBP_SUCCESS) {
            const auto& error = jni_bridge_->get_last_error();
            OBP_LOG_WARN("JNI segmentation failed. ret=%d, error=%s", 
                         ret, error.error_message.c_str());
            return ret;
        }
        
        // Process and validate tokens
        tokens_.clear();
        tokens_.reserve(raw_tokens.size());
        
        for (const auto& raw_token : raw_tokens) {
            if (!validate_token(raw_token)) {
                continue;  // Skip invalid tokens
            }
            
            TokenInfo token_info;
            token_info.word = raw_token;
            token_info.byte_length = static_cast<int64_t>(raw_token.length());
            token_info.char_length = calculate_utf8_char_length(raw_token);
            token_info.frequency = 1;
            
            tokens_.push_back(token_info);
        }
        
        OBP_LOG_INFO("=== C++ SEGMENTATION RESULT === Raw tokens: %zu, Valid tokens: %zu", 
                     raw_tokens.size(), tokens_.size());
        
        // Log each token for debugging
        for (size_t i = 0; i < raw_tokens.size(); ++i) {
            OBP_LOG_INFO("Raw token[%zu]: '%s'", i, raw_tokens[i].c_str());
        }
        for (size_t i = 0; i < tokens_.size(); ++i) {
            OBP_LOG_INFO("Valid token[%zu]: '%s' (bytes=%ld, chars=%ld)", 
                         i, tokens_[i].word.c_str(), tokens_[i].byte_length, tokens_[i].char_length);
        }
        
        return OBP_SUCCESS;
        
    } catch (const std::exception& e) {
        OBP_LOG_WARN("Exception during segmentation: %s", e.what());
        return OBP_PLUGIN_ERROR;
    }
}

// Calculate UTF-8 character length
int64_t ObFTParser::calculate_utf8_char_length(const std::string& utf8_text)
{
    int64_t char_count = 0;
    const char* p = utf8_text.c_str();
    const char* end = p + utf8_text.length();
    
    while (p < end) {
        unsigned char c = *p;
        
        if ((c & 0x80) == 0) {
            p += 1;  // ASCII character
        } else if ((c & 0xE0) == 0xC0) {
            p += 2;  // 2-byte UTF-8 character
        } else if ((c & 0xF0) == 0xE0) {
            p += 3;  // 3-byte UTF-8 character (most CJK characters)
        } else if ((c & 0xF8) == 0xF0) {
            p += 4;  // 4-byte UTF-8 character
        } else {
            p += 1;  // Invalid UTF-8, skip
            continue;
        }
        
        char_count++;
    }
    
    return char_count;
}

// Validate token
bool ObFTParser::validate_token(const std::string& token)
{
    if (token.empty()) {
        return false;
    }
    
    // For UTF-8 text (Japanese/Thai), we should be more careful about whitespace detection
    // std::isspace() only works correctly for ASCII characters
    // For now, let's only reject truly empty tokens and ASCII-only whitespace
    
    bool has_non_ascii = false;
    bool has_non_whitespace_ascii = false;
    
    for (unsigned char c : token) {
        if (c > 127) {
            // Non-ASCII character (likely Japanese/Thai), assume it's valid
            has_non_ascii = true;
        } else if (!std::isspace(c)) {
            // ASCII non-whitespace character
            has_non_whitespace_ascii = true;
        }
    }
    
    // Accept token if it has non-ASCII characters OR non-whitespace ASCII characters
    if (has_non_ascii || has_non_whitespace_ascii) {
        return true;
    }
    
    // Reject only if token is all ASCII whitespace
    return false;
}

} // namespace ftparser
} // namespace oceanbase