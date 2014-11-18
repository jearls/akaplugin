#define PURPLE_PLUGINS

#include <glib.h>
#include <string.h>

/*  purple plugin include files  */

#include "debug.h"
#include "conversation.h"
#include "signals.h"
#include "plugin.h"
#include "pluginpref.h"
#include "prefs.h"
#include "version.h"

/*  define my plugin parameters  */

#define PLUGIN_ID "core-jme-akaplugin"
#define PLUGIN_NAME "AKA Plugin"
#define PLUGIN_VERSION "0.2"
#define PLUGIN_AUTHOR "Johnson Earls"
#define PLUGIN_URL "https://github.com/darkfoxprime/akaplugin/wiki"
#define PLUGIN_SUMMARY "Recognize aliases said in chats as also being you."
#define PLUGIN_DESCRIPTION "This plugin listens for chat messages and, when a message contains any of the aliases configured in its preferences, marks the message as being said to you."

#define PREFS_ROOT "/plugins/core/" PLUGIN_ID
#define PREFS_ALIASES PREFS_ROOT "/aliases"

/*  the default aliases preference value  */

#define DEFAULT_ALIASES "my name; nickname; other name; phase to alert me; etc..."

/*  My list of aliases.
 *  Maintained by the aliases_changed_cb preference callback function.
 */
static GSList *aliases_list = NULL ;

/*  Process a change in the aliases preference.
 *  Each time this preference changes, rebuild the aliases_list.
 */
static void
aliases_changed_cb(const char *name, PurplePrefType type, gconstpointer val, gpointer data) {
    /*  character pointers to scan the preference value.
     *  "scan" will start at the beginning of the string and advance one
     *  utf8 character at a time.
     *  "start" will point to the first non-whitespace, non-separator
     *  character found after the last separator (if any).
     *  "end" will point to the last non-whitespace, non-separator
     *  character found after "start".
     */
    gchar *scan, *start, *end ;
    /*  the character we're scanning  */
    gunichar this ;
    /*  destroy the current aliases_list (if any)
     *  so that it can be rebuilt.
     */
    if (aliases_list != NULL) {
        /*  the alias pointer to scan the list  */
        GSList *alias_ptr = aliases_list ;
        while (alias_ptr != NULL) {
            /*  free the string for this list entry  */
            free(alias_ptr->data) ;
            /*  move to the next list entry  */
            alias_ptr = alias_ptr->next ;
        }
        /*  free up the list entries themselves  */
        g_slist_free(aliases_list) ;
    }
    /*  start a new aliases_list  */
    aliases_list = NULL ;
    /*  scan through the string.
     *
     *  If scan finds a separator character and start is not NULL, add
     *  the lowercased version of the substring start..end to
     *  aliases_list.  Then reset start and end to NULL.
     *  At the end of the scan, if start is not NULL, add the lowercased
     *  version of the final substring to aliases_list.
     */
    start = end = (gchar*) NULL ;
    /*  scan through the string  */
    for (   /* start at beginning of string */
            scan = (gchar*) val ;
            /* continue while we're not at a null character */
            ((this = g_utf8_get_char(scan)) != 0) ;
            /* advance to the next character each iteration */
            scan = g_utf8_next_char(scan)
     ) {
        /*  skip spaces  */
        if (!g_unichar_isspace(this)) {
            /*  if we're at a separator, process the substring  */
            if (this == ';') {
                /*  If we have a non-empty alias, add it to aliases_list.  */
                if (start != NULL) {
                    /*  calculate the number of bytes between start
                     *  and the character after end.
                     */
                    gssize sublen = g_utf8_next_char(end) - start;
                    /*  Use g_utf8_strdown to both duplicate and
                     *  transform to lowercase the contents of the
                     *  substring starting at start and including sublen
                     *  characters, and add the duplicated string to the
                     *  beginning of the aliases_list.
                     */
                    aliases_list = g_slist_prepend( aliases_list, g_utf8_strdown( start, sublen ) ) ;
                    purple_debug_info(PLUGIN_ID, "AKA Plugin found %d byte alias \"%s\".\n", (int)sublen, (gchar*)(aliases_list->data)) ;
                    /*  reset start and end for next substring  */
                    start = end = (gchar*)NULL ;
                }
            } else {
                /*  not a separator, update start and end appropriately  */
                if (start == NULL) {
                    start = scan ;
                }
                end = scan ;
            }
        }
    }
    /*  End of the string.
     *  If we have a non-empty alias, add it to aliases_list.
     */
    if (start != NULL) {
        /*  calculate the number of bytes between start and the character
         *  after end.
         */
        gssize sublen = g_utf8_next_char(end) - start;
        /*  Use g_utf8_strdown to both duplicate and transform to
         *  lowercase the contents of the substring starting at start and
         *  including sublen characters, and add the duplicated string to
         *  the beginning of the aliases_list.
         */
        aliases_list = g_slist_prepend( aliases_list, g_utf8_strdown( start, sublen ) ) ;
        purple_debug_info(PLUGIN_ID, "AKA Plugin found %d byte alias \"%s\".\n", (int)sublen, (gchar*)(aliases_list->data)) ;
    }
    /*  Done, nothing to return.  */
    return ;
}

/*  Determine where, if at all, a UTF8 substring occurs within another
 *  UTF8 string.  Return a pointer to the start of the substring, or
 *  NULL if not found.
 */
static gchar*
find_utf8_substr(gchar *str, gchar *substr) {
    /*  record the first character of substr for scanning  */
    gunichar first = g_utf8_get_char(substr) ;
    /*  record the collation key for substr for comparison  */
    gchar* substr_key = g_utf8_collate_key(substr, -1) ;
    /*  record the length of substr, in bytes, for comparison.  do this by finding the length, in utf8 characters, converting that to a pointer, then using pointer arithmetic to find the byte length.  */
    gssize len = g_utf8_offset_to_pointer(substr, g_utf8_strlen(substr, -1)) - substr ;
    /*  scan the string looking for 'first'  */
    for (
            /*  start at the first occurrence of 'first'  */
            str = g_utf8_strchr(str, -1, first) ;
            /*  continue as long as 'first' was found  */
            str != NULL ;
            /*  for the next iteration, advance to the next occurrence
             *  of 'first'
             */
            str = g_utf8_strchr(g_utf8_next_char(str), -1, first)
    ) {
        /*  compute the collation key for the 'len'-length
         *  substring located at 'str'
         */
        gchar* this_key = g_utf8_collate_key(str, len) ;
        /*  get our comparison value  */
        int  this_comp = strcmp(substr_key, this_key) ;
        /*  free the collation key  */
        g_free(this_key) ;
        /*  if comparison was 0 (equal), break out of the scan loop  */
        if (this_comp == 0) {
            break ; /* out of for(...) loop */
        }
    }
    /*  free up the collation key  */
    g_free(substr_key) ;
    /*  return the result  */
    return(str) ;
}

/*  Process a chat message that is being received.
 *  Check the contents of *buffer to see if it contains any of my aliases.
 *  If it does, make *flags include the PURPLE_MESSAGE_NICK flag.
 *  When done, return FALSE to indicate the message should *not* be ignored.
 */
static gboolean
receiving_chat_msg_cb(PurpleAccount *account, char **sender, char **buffer, PurpleConversation *chat, PurpleMessageFlags *flags, void *data) {
    /*  pointer into the aliases list  */
    GSList *alias_ptr = aliases_list ;
    /*  a copy of the [utf8] message transformed to lowercase  */
    gchar *lcmsg = g_utf8_strdown( *buffer, -1 ) ;
    purple_debug_info(PLUGIN_ID, "AKA Plugin checking message \"%s\".\n", lcmsg) ;
    /*  loop through each alias and check to see if it exists in the message  */
    while (alias_ptr != NULL) {
        purple_debug_info(PLUGIN_ID, "AKA Plugin checking against \"%s\".\n", (char*)alias_ptr->data) ;
        if (find_utf8_substr(lcmsg, alias_ptr->data) != NULL) {
            purple_debug_info(PLUGIN_ID, "AKA Plugin found a match!\n") ;
            /*  The alias exists; set the NICK flag to indicate this
             *  message was directed at us, and break out of the alias
             *  loop.
             */
            *flags |= PURPLE_MESSAGE_NICK ;
            break ; /*  out of while (alias_ptr != NULL) loop  */
        }
        /*  remember to not infinite loop on the same alias pointer...  */
        alias_ptr = alias_ptr->next ;
    }
    /*  free our lowercase copy of the message  */
    free(lcmsg) ;
    /*  return FALSE says the message should not be ignored.  */
    return FALSE ;
}

/*  Registers any custom commands provided by the plugin.
 *  None at this time.
 */
static void
register_cmds(PurplePlugin *plugin) {
    /*  no commands registered at this time  */
    /*  Done, nothing to return  */
    return ;
}

/*  Connects signal handlers used by the plugin.
 *  Currently, this consists of only the receiving-chat-msg signal.
 */
static void
connect_signals(PurplePlugin *plugin) {
    /*  The conversation handle is used for conversation-related signals  */
    void *conv_handle = purple_conversations_get_handle() ;
    /*  Register the receiving-chat-msg signal handler  */
    purple_signal_connect(conv_handle, "receiving-chat-msg", plugin, PURPLE_CALLBACK(receiving_chat_msg_cb), NULL) ;
    /*  Done, nothing to return  */
    return ;
}

/*  Initialize the plugin preferences.
 *  Create the root preference directory if needed.
 *  If the aliases preference exists, load it;
 *  otherwise, initialize it from the default aliases value.
 *  Then call the aliases_changed_cb function to build the aliases_list.
 */
static void
init_prefs(PurplePlugin *plugin) {
    const char *aliases ;
    /*  If the root preference directory does not exist, create it  */
    if (!purple_prefs_exists(PREFS_ROOT)) {
        purple_prefs_add_none(PREFS_ROOT) ;
    }
    /*  If the alias preference exists, load it, otherwise create it from the default value  */
    if (purple_prefs_exists(PREFS_ALIASES)) {
        aliases = purple_prefs_get_string(PREFS_ALIASES) ;
    } else {
        purple_prefs_add_string(PREFS_ALIASES, DEFAULT_ALIASES) ;
        aliases = DEFAULT_ALIASES ;
    }
    /*  initialize the alias list from the alias value loaded (or created)  */
    aliases_changed_cb(PREFS_ALIASES, PURPLE_PREF_STRING, aliases, NULL) ;
    /*  connect aliases_changed_cb to be called whenever the preference
     *  changes
     */
    purple_prefs_connect_callback(plugin, PREFS_ALIASES, (PurplePrefCallback)(&aliases_changed_cb), NULL) ;
    /*  Done, nothing to return  */
    return ;
}

/*  Called by the plugin system the first time the plugin is probed.
 *  Does any one-time initialization for the plugin.
 */
static void
init_plugin_hook(PurplePlugin *plugin) {
    purple_debug_info(PLUGIN_ID, "AKA Plugin Initialized.\n") ;
    /*  Initialize the plugin's preferences  */
    init_prefs(plugin) ;
    /*  Done, nothing to return  */
    return ;
}

/*  Called by the plugin system when the plugin is loaded.
 *  Registers plugin commands and connects signal handlers.
 */
static gboolean
plugin_load_hook(PurplePlugin *plugin) {
    purple_debug_info(PLUGIN_ID, "AKA Plugin Loaded.\n") ;
    /*  register any custom plugin commands  */
    register_cmds(plugin) ;
    /*  register any signal handlers  */
    connect_signals(plugin) ;
    /*  return TRUE says continue loading the plugin  */
    return TRUE ;
}

/*  Unload the plugin.
 *  Called by the plugin system when the plugin is unloaded.
 *  Currently does nothing.
 */
static gboolean
plugin_unload_hook(PurplePlugin *plugin) {
    purple_debug_info(PLUGIN_ID, "AKA Plugin Unloaded.\n") ;
    /*  We don't need to do anything special here  */
    /*  return TRUE says continue unloading the plugin  */
    return TRUE ;
}

/*  Generate the preference frame for the plugin.
 *  This consists of a header label, the aliases string input,
 *  and an information label.
 */
static PurplePluginPrefFrame *
prefs_info_hook(PurplePlugin *plugin) {
    PurplePluginPrefFrame *pref_frame ;
    PurplePluginPref *pref_item ;

    /*  the frame in which the preferences will be presented  */
    pref_frame = purple_plugin_pref_frame_new() ;

    /*  the header label  */
    pref_item = purple_plugin_pref_new_with_label("Enter your aliases, separated by semicolons (;), in this text box.") ;
    purple_plugin_pref_frame_add(pref_frame, pref_item) ;

    /*  The input box for the list of aliases  */
    pref_item = purple_plugin_pref_new_with_name_and_label(
        PREFS_ALIASES, "Aliases"
    ) ;
    purple_plugin_pref_set_max_length(pref_item, 256) ;
    purple_plugin_pref_frame_add(pref_frame, pref_item) ;

    /*  The information label describing the content of the aliases  */
    pref_item = purple_plugin_pref_new_with_label("Any chat message that contains one of the words or phrases here\nwill be treated as if it was addressed to your username.") ;
    purple_plugin_pref_frame_add(pref_frame, pref_item) ;

    /*  return the finished frame  */
    return pref_frame ;
}

/*  The preference information block.
 *  The important part of this is the callback, which points to the
 *  prefs_info_hook function to generate the preference frame.
 */
static PurplePluginUiInfo plugin_prefs_info = {
    /* get_plugin_pref_frame */ &prefs_info_hook ,
    /* page_num (Reserved) */   0 ,
    /* frame (Reserved) */      NULL ,
    /* Padding */               NULL ,
    /* Padding */               NULL ,
    /* Padding */               NULL ,
    /* Padding */               NULL
} ;

/*  The plugin information block.
 *  This plugin uses the following hooks and info blocks:
 *    plugin_load_hook
 *    plugin_unload_hook
 *    plugin_prefs_info
 */
static PurplePluginInfo plugin_info = {
    /* magic */                 PURPLE_PLUGIN_MAGIC ,
    /* compat major version */  PURPLE_MAJOR_VERSION ,
    /* compat minor version */  PURPLE_MINOR_VERSION ,
    /* plugin type */           PURPLE_PLUGIN_STANDARD ,
    /* reserved for ui */       NULL ,
    /* plugin flags */          0 ,
    /* reserved for deps */     NULL ,
    /* priority */              PURPLE_PRIORITY_DEFAULT ,

    /* plugin id */             PLUGIN_ID ,
    /* plugin name */           PLUGIN_NAME ,
    /* plugin version */        PLUGIN_VERSION ,

    /* summary */               PLUGIN_SUMMARY ,
    /* description */           PLUGIN_DESCRIPTION ,
    /* author */                PLUGIN_AUTHOR ,
    /* URL */                   PLUGIN_URL ,

    /* plugin_load */           &plugin_load_hook ,
    /* plugin_unload */         &plugin_unload_hook ,
    /* plugin_destroy */        NULL ,

    /* ui info */               NULL ,
    /* loader/protocol info */  NULL ,
    /* prefs info */            &plugin_prefs_info ,
    /* plugin_actions */        NULL ,
    /* reserved */              NULL ,
    /* reserved */              NULL ,
    /* reserved */              NULL ,
    /* reserved */              NULL
} ;

/*  Initialize the plugin.
 *
 *  The first parameter to PURPLE_INIT_PLUGIN (akaplugin) is the plugin
 *  name.  This is not a C string, nor is it a declared type or variable.
 *  With PURPLE_PLUGINS defined, this argument is ignored.
 *  If PURPLE_PLUGINS is not defined, this argument is used by the
 *  PURPLE_INIT_PLUGIN as part of some internally-defined function names
 *  (e.g. purple_init_akaplugin_plugin).
 *
 *  The second parameter is the plugin initialization function.
 *  The third parameter is the plugin info block.
 */
PURPLE_INIT_PLUGIN(akaplugin, init_plugin_hook, plugin_info)
