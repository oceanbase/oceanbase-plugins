/**
 * Copyright (c) 2023 OceanBase
 * Thai Fulltext Parser Plugin - Main Entry Point
 */

#include "thai_jni_bridge.h"

extern "C" {

/**
 * Plugin initialization function
 * @param plugin Plugin parameter pointer
 * @return OBP_SUCCESS on success, error code on failure
 */
int plugin_init_thai(ObPluginParamPtr plugin)
{
    int ret = OBP_SUCCESS;
    
    if (0 == plugin) {
        ret = OBP_INVALID_ARGUMENT;
        return ret;
    }
    
    // Define the ftparser plugin descriptor
    ObPluginFTParser parser = {
        .init              = thai_ftparser_init,
        .deinit            = thai_ftparser_deinit,
        .scan_begin        = thai_ftparser_scan_begin,
        .scan_end          = thai_ftparser_scan_end,
        .next_token        = thai_ftparser_next_token,
        .get_add_word_flag = thai_ftparser_get_add_word_flag
    };
    
    // Register the ftparser plugin with OceanBase
    ret = OBP_REGISTER_FTPARSER(plugin,
                                "thai_ftparser",
                                parser,
                                "Thai language fulltext parser");
    
    return ret;
}

// Plugin declaration
OBP_DECLARE_PLUGIN(thai_ftparser)
{
    OBP_AUTHOR_OCEANBASE,                    // Plugin author
    OBP_MAKE_VERSION(1, 0, 0),              // Plugin version
    OBP_LICENSE_MULAN_PSL_V2,               // Plugin license
    plugin_init_thai,                       // Plugin initialization function
    nullptr,                                // Plugin deinitialization function (not needed)
} OBP_DECLARE_PLUGIN_END;

} // extern "C"
