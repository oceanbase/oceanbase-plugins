/**
 * Copyright (c) 2023 OceanBase
 * Japanese Fulltext Parser Plugin - Main Entry Point
 */

#include "japanese_jni_bridge.h"

extern "C" {

/**
 * Plugin initialization function
 * @param plugin Plugin parameter pointer
 * @return OBP_SUCCESS on success, error code on failure
 */
int plugin_init_jp(ObPluginParamPtr plugin)
{
    int ret = OBP_SUCCESS;
    
    if (0 == plugin) {
        ret = OBP_INVALID_ARGUMENT;
        return ret;
    }
    
    // Define the ftparser plugin descriptor
    ObPluginFTParser parser = {
        .init              = japanese_ftparser_init,
        .deinit            = japanese_ftparser_deinit,
        .scan_begin        = japanese_ftparser_scan_begin,
        .scan_end          = japanese_ftparser_scan_end,
        .next_token        = japanese_ftparser_next_token,
        .get_add_word_flag = japanese_ftparser_get_add_word_flag
    };
    
    // Register the ftparser plugin with OceanBase
    ret = OBP_REGISTER_FTPARSER(plugin,
                                "japanese_ftparser",
                                parser,
                                "Japanese language fulltext parser");
    
    return ret;
}

// Plugin declaration
OBP_DECLARE_PLUGIN(japanese_ftparser)
{
    OBP_AUTHOR_OCEANBASE,                    // Plugin author
    OBP_MAKE_VERSION(0, 0, 1),              // Plugin version
    OBP_LICENSE_MULAN_PSL_V2,               // Plugin license
    plugin_init_jp,                         // Plugin initialization function
    nullptr,                                // Plugin deinitialization function (not needed)
} OBP_DECLARE_PLUGIN_END;

} // extern "C"
