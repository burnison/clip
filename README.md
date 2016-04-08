Clip
====

What is Clip
------------

Clip is a GTK-based clipboard manager. Clip was written by Richard Burnison after he became frustrated with the current
clipboard manager offerings for Linux. While similar in appearance and functionality to Parcellite, Clip offers
additional features not provided by other clipboard managers.

Availability
------------

Clip is offered under the MIT license and may be downloaded with Git at git://github.com/burnison/clip.git.



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
history item or pressing "L" when an item is selected will toggle its lock mode. Menu items displayed in a bold text are
locked.

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
may be disabled by prefixing a pattern with "(?-i)"). The first history item matching the input pattern will be
highlighted; pressing tab will move the active focus to the next match. Pressing enter or clicking on the desired menu
item will set it as active.

Positional Selection
--------------------

When pressing 0-9, the nth menu item will be selected. This behaviour is unavailable when searching and is intended
for quick access when the actual position is already known.

Deleting Individual History Items
---------------------------------

Clip provides the ability to delete individual history items. To delete a single history item, change the menu selection
to the item to be deleted. Press the "Delete" or "d" keys on your keyboard. If unlocked, the item will be removed from
the history.

Editing Values
--------------

Clip allows you to edit the value of a selected entry by pressing "e" or "E". This will pop-up a dialog in which you can
make any changes to the desired string. While "e" only provides a way to edit the selected entry, pressing "E" will
additionally promote the edited value to the active clipboard value.

Record Joining
--------------

When a menu entry is currently selected, pressing 'J' will cause the top record to merge with the next record, similar
to the join behaviour of Vi. Joining a row will not activate it, allowing multiple joins to be made in one trip to the
dialog. Finally, the usage counter is inherited from the left-most entry as opposed to merging the counters. This was
done to prevent entries from remaining around for too long.

Changing Case
-------------

Similar to Vi, Clip can change the case of a record by pressing "u" or "U" when a record is selected. To change the case
to lower case, press "u"; to change case to upper, press "U". Changing case of the head record does not activate it.

Trimming
--------

In addition to the dynamic trimming feature, pressing "t" will trim both left and right sides of a selected entry. This
operation will not modify the current clipboard value.

Marking / Tagging
-----------------

Clip supports the ability to mark (or tag) an entry for quick selection.  To mark a selected record, press 'm' and then
the desired mark letter. For example, pressing 'ma' will mark the record with the tag, 'a'. The mark will be displayed
immediately to the left of the record.

To activate a marked record, press the gave character, '`' followed by the desired mark. For example, to select the
previously-selected 'a' record, press '\`a'. If the mark is found, the correlated record will be activated.

Masking
-------

When a semi-sensitive value is present on your clipboard, you may toggle a display mask by pressing '*' with the
target menu item highlighted. This will mask-out the displayed text. Be aware that masked values are *not* encrypted
or in any way obfuscated in Clip's database. Because the actual displayed value is rendered unreadable, masking works
best when used in conjunction with marks/tags.


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
primary clipboard selection to become unselected in some applications (such as rxvt) iff Clip is compiled with syncing.

Similarity Detection
--------------------

Clip employs a duplication detection mechanism that computes the Levenshtein distance between existing entries and new
entries to determine if they are logical duplicates. By default, this is only applied to the top 10% of all records so
that history doesn't become sporadic. More specifically, this implementation considers records as duplicates only if
they are similar in both time and space.

This feature is mainly intended to reduce the number of duplicate values that would otherwise clutter the history. It
works on the assumption that the last record selected is the "correct" record for a given value. Consider the following
values endered into Clip in the following order:

* @example.com
* xample.com,
* example.com

After adding all three values to Clip, Clip would only show the final record, 'example.com', which is very likely the
expected value.


Known Issues
============

* When using a window manager with "sloppy focus", you may need to set an environmental variable,
  GDK_CORE_DEVICE_EVENTS=1, when running Clip. This seems to be an upstream bug in GTK3 that prevents the pop-up dialog
  from rendering.

* Clip interacts oddly with applications that do not use text-based clipboard contents (such as InkScape and
  LibreOffice). If you do not like how these programs interact, you can temporarily disable Clip's history.
