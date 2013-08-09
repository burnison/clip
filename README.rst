Clip
====

What is Clip
------------

Clip is a GTK-based clipboard manager. Clip was written by Richard Burnison after he became frustrated with the current
clipboard manager offerings for Linux. While similar in appearance and functionality to Parcellite, Clip offers
additional features not provided by other clipboard managers.

Availability
------------

Clip is offered under the GNU GPL 3 license and may be downloaded with Git at git://github.com/burnison/clip.git.



Usage
=====

General Usage
-------------

Once executing, Clip will monitor your clipboard. All clipboard contents will be added to its local history, available
by pressing C-M-P. A pop-up menu will appear, navigable by mouse or keyboard. Selecting a menu item will set its value
as the current clipboard contents.

Locking History Entries
-----------------------

Clip supports the ability to lock history items, effectively making them immutable to deletion. Right clicking on a
history item will toggle its lock mode. Menu items displayed in a bold text are locked.

Clearing History
----------------

Clip's history may be cleared by selecting the "Clear" menu item. This will delete all unlocked history items. Any
locked history items will be left on the list for use.

Disabling History
-----------------

Clip also allows history to be disabled/enabled. Selecting the "Disable History" menu item will disable clipboard
history. Keep in mind this mode will not disable active clipboard selection. To re-enable history, select the "Enable
History" menu item.

Searching History
-----------------

Clip also allows a user to search for a previously-archived string by pressing "/" when the pop-up menu is visible. This
will enter search mode. Clip searches using Perl-compatible regex in a default case-insensitive mode (case insensitivity
may be disabled by prefixing a pattern with "(^i)"). The first history item matching the input pattern will be
highlighted; pressing tab will move the active focus to the next match. Pressing enter or clicking on the desired menu
item will set it as active.

Positional Selection
--------------------

When pressing 1-9, the nth menu item will be selected. This behaviour is unavailable when searching and is intended
for quick access when the actual position is already known.

Deleting Individual History Items
---------------------------------

Clip provides the ability to delete individual history items. To delete a single history item, change the menu selection
to the item to be deleted. Press the Delete key on your keyboard. If unlocked, the item will be removed from the
history.

Editing Current Value
---------------------

Clip allows you to edit the current value of the clip board by selecting the "Edit" menu item. This will pop-up a dialog
in which you can make any changes. Pressing OK will replace the current value with the changes you have made.



Features
========

Automatic Entry Clearing
------------------------

When Clip receives a request to clear the active clipboard, it will remove the current value from its history. This is
intended to realize expectations about privacy, such as those made by password stores like Keepassx. This feature may
have otherwise unwanted outcomes if third-party applications frequently clear the clipboard.

Dynamic Whitespace Trimming
---------------------------

At runtime, Clip is able to use one of four whitespace trimming strategies:

* Off - No trimming is performed.
* Left - Whitespace is trimmed from the left of each clipboard entry.
* Right - Whitespace is trimmed from the right side of each clipboard entry.
* Both - Whitespace is trimmed from both sides of each clipboard entry.

These strategies may be changed by selecting the "Trim" menu item, which indicates its active mode. This feature is 
non-destructive and will not change the source data. Keep in mind that when enabled, this feature may result in the
primary clipboard selection to become unselected in some applications (such as rxvt).



Known Issues
============

* When using a window manager with "sloppy focus", you may need to set an environmental variable,
  GDK_CORE_DEVICE_EVENTS=1, when running Clip. This seems to be an upstream bug in GTK3 that prevents
  the pop-up dialog from rendering.

* Clip interacts oddly with applications that do not use text-based clipboard contents (such as InkScape and
  LibreOffice). If you do not like how these programs interact, you can temporarily disable Clip's history.
