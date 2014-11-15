#define PURPLE_PLUGINS

#include <glib.h>

#include "debug.h"
#include "conversation.h"
#include "signals.h"
#include "plugin.h"
#include "pluginpref.h"
#include "prefs.h"
#include "version.h"

#define PLUGIN_ID "core-darkfoxprime-akaplugin"
#define PREFS_ROOT "/plugins/core/" PLUGIN_ID
#define PREFS_ALIASES PREFS_ROOT "/aliases"

#define DEFAULT_ALIASES "my name; nickname; other name; phase to alert me; etc..."

static GSList *aliases_list = NULL ;

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

static guint aliases_cb_id = 0 ;
static void
aliases_cb(const char *name, PurplePrefType type, gconstpointer val, gpointer data) {
    gchar *start, *next, *end ;
    if (aliases_list != NULL) {
        GSList *alias = aliases_list ;
        while (alias != NULL) {
            free(alias->data) ;
            alias = alias->next ;
        }
        g_slist_free(aliases_list) ;
    }
    aliases_list = NULL ;
    start = (gchar*) val ;
    while (*start) {
        if (*start == ' ') {
            start += 1 ;
        } else {
            end = start ;
            while ((*end) && (*end != ';')) {
                end += 1 ;
            }
            next = end + 1 ;
            while ((end>start) && (*(end-1) == ' ')) {
                end -= 1 ;
            }
            if (end > start) {
                aliases_list = g_slist_prepend( aliases_list, g_ascii_strdown( start, (gssize)(end - start) ) ) ;
                purple_debug_info(PLUGIN_ID, "AKA Plugin found alias \"%s\".\n", (char*)(aliases_list->data)) ;
            }
            start = next ;
        }
    }
}

static void
init_prefs(PurplePlugin *plugin) {
    if (!purple_prefs_exists(PREFS_ROOT)) {
        purple_prefs_add_none(PREFS_ROOT) ;
    }
    if (purple_prefs_exists(PREFS_ALIASES)) {
        aliases_cb(PREFS_ALIASES, PURPLE_PREF_STRING, purple_prefs_get_string(PREFS_ALIASES), NULL) ;
    } else {
        purple_prefs_add_string(PREFS_ALIASES, DEFAULT_ALIASES) ;
    }
    aliases_cb_id = purple_prefs_connect_callback(plugin, PREFS_ALIASES, (PurplePrefCallback)(&aliases_cb), NULL) ;
    aliases_cb(PREFS_ALIASES, PURPLE_PREF_STRING, DEFAULT_ALIASES, NULL) ;
}

static void
init_plugin(PurplePlugin *plugin) {
    init_prefs(plugin) ;
}

static void
register_cmds(PurplePlugin *plugin) {
}

static void
connect_signals(PurplePlugin *plugin) {
    void *conv_handle = purple_conversations_get_handle() ;
    purple_signal_connect(conv_handle, "receiving-chat-msg", plugin, PURPLE_CALLBACK(receiving_chat_msg_cb), NULL) ;
}

static gboolean
plugin_load(PurplePlugin *plugin) {
    purple_debug_info(PLUGIN_ID, "AKA Plugin Loaded.\n") ;
    register_cmds(plugin) ;
    connect_signals(plugin) ;
    return TRUE ;
}

static gboolean
plugin_unload(PurplePlugin *plugin) {
    purple_debug_info(PLUGIN_ID, "AKA Plugin Unloaded.\n") ;
    return TRUE ;
}

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

static PurplePluginUiInfo prefs_info = {
    /* callback */              prefs_info_cb,
    /* page_num (Reserved) */   0,
    /* frame (Reserved) */      NULL,
    /* Padding */               NULL,
    /* Padding */               NULL,
    /* Padding */               NULL,
    /* Padding */               NULL
} ;

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

PURPLE_INIT_PLUGIN(hello_world, init_plugin, info)
