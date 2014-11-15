#define PURPLE_PLUGINS

#include <glib.h>

/* purple plugin include files */

#include "debug.h"
#include "conversation.h"
#include "signals.h"
#include "plugin.h"
#include "pluginpref.h"
#include "prefs.h"
#include "version.h"

/* define my plugin parameters */

#define PLUGIN_ID "core-darkfoxprime-akaplugin"
#define PREFS_ROOT "/plugins/core/" PLUGIN_ID
#define PREFS_ALIASES PREFS_ROOT "/aliases"

/* the default aliases preference value */

#define DEFAULT_ALIASES "my name; nickname; other name; phase to alert me; etc..."

/* My list of aliases.
 * Maintained by the aliases_cb preference callback function
 */
static GSList *aliases_list = NULL ;

/*  Process a chat message that is being received.
 *  Check the contents of *buffer to see if it contains any of my aliases.
 *  If it does, make *flags include the PURPLE_MESSAGE_NICK flag.
 *  When done, return FALSE to indicate the message should *not* be ignored.
 */
static gboolean
receiving_chat_msg_cb(PurpleAccount *account, char **sender, char **buffer, PurpleConversation *chat, PurpleMessageFlags *flags, void *data) {
    GSList *alias = aliases_list ;
    char *lcmsg = g_ascii_strdown( *buffer, -1 ) ;
    while (alias != NULL) {
        if (g_strrstr( lcmsg, alias->data ) != NULL) {
            *flags |= PURPLE_MESSAGE_NICK ;
            break ; /* out of while (alias != NULL) loop */
        }
        alias = alias->next ;
    }
    return FALSE ; /* don't ignore the message */
}

/*  Process a change in the aliases preference.
 *  Each time this preference changes, rebuild the aliases_list.
 */
static void
aliases_cb(const char *name, PurplePrefType type, gconstpointer val, gpointer data) {
    gchar *start, *next, *end ;
    /* destroy the current aliases_list, if any */
    if (aliases_list != NULL) {
        GSList *alias = aliases_list ;
        while (alias != NULL) {
            free(alias->data) ;
            alias = alias->next ;
        }
        g_slist_free(aliases_list) ;
    }
    /* start a new aliases_list */
    aliases_list = NULL ;
    /* scan through the string.  start is the start of the alias name;
     * end is one past the end of the name, and next either points to the
     * NUL character or to one past the ';' at the end of the alias name.
     */
    start = (gchar*) val ;
    while (*start) {
        if (*start == ' ') {
            start += 1 ;
        } else {
            end = start ;
            while ((*end) && (*end != ';')) {
                end += 1 ;
            }
            next = end;
            if (*next == ';') { next += 1; }
            while ((end>start) && (*(end-1) == ' ')) {
                end -= 1 ;
            }
            if (end > start) {
                /* we have an alias.  Duplicate start..(end-1) in lower case (via g_ascii_strdown), and add it to the beginning of the aliases_list. */
                aliases_list = g_slist_prepend( aliases_list, g_ascii_strdown( start, (gssize)(end - start) ) ) ;
                purple_debug_info(PLUGIN_ID, "AKA Plugin found alias \"%s\".\n", (char*)(aliases_list->data)) ;
            }
            start = next ;
        }
    }
}

/*  Initialize the plugin preferences.
 *  Create the root preference directory if needed.
 *  If the aliases preference exists, load it;
 *  otherwise, initialize it from the default aliases value.
 *  Then call the aliases_cb function to build the aliases_list.
 */
static void
init_prefs(PurplePlugin *plugin) {
    const char *aliases;
    if (!purple_prefs_exists(PREFS_ROOT)) {
        purple_prefs_add_none(PREFS_ROOT) ;
    }
    if (purple_prefs_exists(PREFS_ALIASES)) {
        aliases = purple_prefs_get_string(PREFS_ALIASES) ;
    } else {
        purple_prefs_add_string(PREFS_ALIASES, DEFAULT_ALIASES) ;
        aliases = DEFAULT_ALIASES ;
    }
    aliases_cb(PREFS_ALIASES, PURPLE_PREF_STRING, aliases, NULL) ;
    /* connect aliases_cb to be called whenever the preference changes */
    purple_prefs_connect_callback(plugin, PREFS_ALIASES, (PurplePrefCallback)(&aliases_cb), NULL) ;
}

/*  Initialize the plugin.
 */
static void
init_plugin(PurplePlugin *plugin) {
    init_prefs(plugin) ;
}

/*  Register any custom commands provided by the plugin.
 */
static void
register_cmds(PurplePlugin *plugin) {
}

/*  Connect any signal handlers used by the plugin.
 */
static void
connect_signals(PurplePlugin *plugin) {
    void *conv_handle = purple_conversations_get_handle() ;
    purple_signal_connect(conv_handle, "receiving-chat-msg", plugin, PURPLE_CALLBACK(receiving_chat_msg_cb), NULL) ;
}

/*  Load the plugin.
 *  Register plugin commands and connect signal handlers.
 */
static gboolean
plugin_load(PurplePlugin *plugin) {
    purple_debug_info(PLUGIN_ID, "AKA Plugin Loaded.\n") ;
    register_cmds(plugin) ;
    connect_signals(plugin) ;
    return TRUE ;
}

/*  Unload the plugin.
 *  Currently does nothing.
 */
static gboolean
plugin_unload(PurplePlugin *plugin) {
    purple_debug_info(PLUGIN_ID, "AKA Plugin Unloaded.\n") ;
    return TRUE ;
}

/*  Generate the preference frame for the plugin.
 *  This consists of a header label, the aliases string input,
 *  and an information label.
 */
static PurplePluginPrefFrame *
prefs_info_cb(PurplePlugin *plugin) {
    PurplePluginPrefFrame *frame ;
    PurplePluginPref *ppref ;

    frame = purple_plugin_pref_frame_new() ;

    ppref = purple_plugin_pref_new_with_label("Enter your aliases, separated by semicolons (;), in this text box.") ;
    purple_plugin_pref_frame_add(frame, ppref) ;
    ppref = purple_plugin_pref_new_with_name_and_label(
        PREFS_ALIASES, "Aliases"
    ) ;
    purple_plugin_pref_set_max_length(ppref, 256) ;
    purple_plugin_pref_frame_add(frame, ppref) ;
    ppref = purple_plugin_pref_new_with_label("Any chat message that contains one of the words or phrases here\nwill be treated as if it was addressed to your username.") ;
    purple_plugin_pref_frame_add(frame, ppref) ;

    return frame ;
}

/*  The preference information block.
 *  The important part of this is the callback, which points to the
 *  prefs_info_cb function to generate the preference frame.
 */
static PurplePluginUiInfo prefs_info = {
    /* callback */              prefs_info_cb,
    /* page_num (Reserved) */   0,
    /* frame (Reserved) */      NULL,
    /* Padding */               NULL,
    /* Padding */               NULL,
    /* Padding */               NULL,
    /* Padding */               NULL
} ;

/*  The plugin information block.  */
static PurplePluginInfo info = {
    /* magic */                 PURPLE_PLUGIN_MAGIC,
    /* compat major version */  PURPLE_MAJOR_VERSION,
    /* compat minor version */  PURPLE_MINOR_VERSION,
    /* plugin type */           PURPLE_PLUGIN_STANDARD,
    /* reserved for ui */       NULL,
    /* plugin flags */          0,
    /* reserved for deps */     NULL,
    /* priority */              PURPLE_PRIORITY_DEFAULT,

    /* plugin id */             "core-darkfoxprime-aka",
    /* plugin name */           "AKA Plugin",
    /* plugin version */        "0.2",

    /* summary */               "Recognize aliases said in chats as also being you.",
    /* description */           "This plugin listens for chat messages and, when a message contains any of the aliases configured in its preferences, marks the message as being said to you.",
    /* author */                "Johnson Earls <darkfoxprime@gmail.com>",
    /* URL */                   "http://github.com/darkfoxprime/akaplugin",

    /* plugin_load */           plugin_load,
    /* plugin_unload */         plugin_unload,
    /* plugin_destroy */        NULL,

    /* ui info */               NULL,
    /* loader/protocol info */  NULL,
    /* prefs info */            &prefs_info,
    /* plugin_actions */        NULL,
    /* reserved */              NULL,
    /* reserved */              NULL,
    /* reserved */              NULL,
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
PURPLE_INIT_PLUGIN(akaplugin, init_plugin, info)
