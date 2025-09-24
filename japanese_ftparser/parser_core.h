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

#pragma once

#include "oceanbase/ob_plugin_ftparser.h"
#include <string>
#include <vector>

// Forward declaration to avoid circular dependency
namespace oceanbase { namespace ftparser { class JNIBridge; } }

/**
 * @defgroup FtParserCore Fulltext Parser Core
 * @brief Core implementation of multilingual fulltext parser
 * @{
 */

namespace oceanbase {
namespace ftparser {

/**
 * Token information structure
 * @details Stores information about a single token extracted from text
 */
struct TokenInfo {
    std::string word;           // The token text
    int64_t byte_length;        // Length in bytes (UTF-8)
    int64_t char_length;        // Length in characters
    int64_t frequency;          // Token frequency (usually 1)
    size_t start_position;      // Start position in original text
    size_t end_position;        // End position in original text
    
    TokenInfo() : byte_length(0), char_length(0), frequency(1), 
                  start_position(0), end_position(0) {}
                  
    TokenInfo(const std::string& w, int64_t bl, int64_t cl, int64_t freq = 1)
        : word(w), byte_length(bl), char_length(cl), frequency(freq),
          start_position(0), end_position(0) {}
};

/**
 * Fulltext Parser Core Class
 * @details Main implementation class for multilingual fulltext parsing.
 * This class handles the segmentation of text into tokens using JNI
 * to call Java-based segmentation libraries.
 */
class ObFTParser final
{
public:
    /**
     * Constructor
     */
    ObFTParser();
    
    /**
     * Destructor
     * @details Automatically cleans up resources
     */
    virtual ~ObFTParser();
    
    /**
     * Initialize the parser with text from OceanBase
     * @param param ObPluginFTParserParamPtr containing the text to be parsed
     * @return OBP_SUCCESS on success, error code on failure
     * @details This method extracts the text from the OceanBase parameter,
     * initializes the JNI bridge, and performs the initial segmentation.
     */
    int init(ObPluginFTParserParamPtr param);
    
    /**
     * Reset the parser state
     * @details Clears all internal state and releases resources
     */
    void reset();
    
    /**
     * Get the next token from the parsed text
     * @param word Output parameter for the token text
     * @param word_len Output parameter for the token byte length
     * @param char_len Output parameter for the token character length
     * @param word_freq Output parameter for the token frequency
     * @return OBP_SUCCESS on success, OBP_ITER_END when no more tokens, error code on failure
     * @details This method returns tokens one by one from the pre-segmented text.
     * It maintains internal state to track the current position.
     */
    int get_next_token(const char *&word,
                      int64_t &word_len,
                      int64_t &char_len,
                      int64_t &word_freq);
    
    /**
     * Get the number of tokens available
     * @return Number of tokens in the current text
     */
    size_t get_token_count() const { return tokens_.size(); }
    
    /**
     * Check if the parser is initialized
     * @return true if initialized, false otherwise
     */
    bool is_initialized() const { return is_inited_; }

private:
    // JNI bridge pointer (actual implementation in jni_bridge.h)
    oceanbase::ftparser::JNIBridge* jni_bridge_;  // JNI bridge to Java segmentation library (managed by singleton)
    std::vector<TokenInfo> tokens_;      // Cached segmentation results
    size_t current_index_;               // Current token index
    std::string original_text_;          // Original text for reference
    bool is_inited_;                     // Initialization flag
    
    // Character set information from OceanBase
    ObPluginCharsetInfoPtr charset_info_;
    
    /**
     * Perform text segmentation using JNI
     * @param text The text to be segmented
     * @return OBP_SUCCESS on success, error code on failure
     * @details This method uses the JNI bridge to call Java segmentation
     * libraries and caches the results in the tokens_ vector.
     */
    int perform_segmentation(const std::string& text);
    
    /**
     * Calculate the character length of a UTF-8 string
     * @param utf8_text The UTF-8 encoded text
     * @return Number of characters (not bytes)
     * @details Multi-byte UTF-8 text uses multiple bytes per character, so byte count
     * and character count are different. This method correctly counts
     * UTF-8 characters.
     */
    int64_t calculate_utf8_char_length(const std::string& utf8_text);
    
    /**
     * Validate and normalize a token
     * @param token The token to validate
     * @return true if the token is valid, false otherwise
     * @details Performs basic validation such as checking for empty tokens,
     * whitespace-only tokens, and other invalid patterns.
     */
    bool validate_token(const std::string& token);
    
    /**
     * Initialize the JNI bridge
     * @return OBP_SUCCESS on success, error code on failure
     */
    int initialize_jni_bridge();
    
    /**
     * Cleanup the JNI bridge
     */
    void cleanup_jni_bridge();
    
    // Disable copy constructor and assignment operator
    ObFTParser(const ObFTParser&) = delete;
    ObFTParser& operator=(const ObFTParser&) = delete;
};

} // namespace ftparser
} // namespace oceanbase

// Global alias for backward compatibility and easier usage
using ObThaiFTParser = oceanbase::ftparser::ObFTParser;

/** @} */