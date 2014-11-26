AKA Plugin
==========

The AKA Plugin allows pidgin to recognize multiple names or phrases as being "your username" for the purposes of chat notifications. Use it in conjunction with the Sound Event "Someone says your username in chat" preference, and the Message Notifications plugin.

Installation
============

Download the akaplugin.dll file into a temporary location, then install it into either your personal plugins directory or the system-wide plugins directory.

Installing into personal plugins directory
------------------------------------------

Open the Windows Explorer to your AppData folder: Go to the Start menu, enter %appdata% into the search field, and press Return. You should see a .purple folder; open that. Create the plugins folder if necessary. Finally, copy the akaplugin.dll file from the download location into the plugins folder.

Installing into the system-wide plugins directory
-------------------------------------------------

Open the Windows Explorer and navigate to C:\Program Files (x86)\Pidgin\plugins . Copy the akaplugin.dll file from the download location into the plugins folder.

Usage
=====

Launch Pidgin and select the Tools → Plugins menu option. Locate and enable AKA Plugin and tell it to configure. A window will appear asking you to enter the aliases you wish to be notified for, separated by semicolons (;) - for example, bozo; clown; hey you would cause chat messages containing "bozo", "clown", or "hey you" to be treated as if they included your username.

Once your aliases are set, you can use the Pidgin preferences panel for Sounds (Someone says your username in chat), or the Message Notification plugin (for the Only when someone says your username option) to notify when someone says any of your aliases in chat.
