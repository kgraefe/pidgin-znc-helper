# Pidgin ZNC Helper Changes

## Version 1.9 (2019-12-20)
- Add option to hide the "playback" messages (Github #12, thanks to Patrick
  Thomas)
- Install into `$libdir/purple-2` where other libpurple2 clients can pick up
  the plugin as well.

## Version 1.8 (2019-02-25)
- Re-implement to use ZNC capabilities `znc.in/server-time-iso` and
  `znc.in/self-message`. This should be a more stable and reliable
  implementation.
- Remove Pidgin UI dependency
- Remove preference dialog and time offset feature as consequences
- Add "Uses ZNC bouncer" as an account option
- Remove French translation since it has no strings anymore and no maintainer

## Version 1.7 (2018-03-21)
- Re-implement by hooking into UI operations to improve speed especially when
  handling a lot of messages.
- Remove timestamp formatting capabilities which interferred with existing
  plugins (Message Timestamp Formats) (Github #1). As a side effect, all
  timestamps older than 20 minutes will include the date (Github #2).
- Support unloading the plugin without Pidgin restart.

## Version 1.6 (2017-02-17)
- Update plugin authors and website
- Ship AppStream metainfo file
- Add Lithuanian language

## Version 1.5.1 (2014-03-17)
- Changed the compatibility fix from last version to not be based on the
  libpurple version string as this breaks when the changes are being
  backported (and they are as they are security fixes).

## Version 1.5.0 (2014-01-29)
- Fixed compatibility issue with Pidgin 2.10.8
- Updated message parser to match modified key messages (this should be an
  option instead of being hard-coded, sorry)
- Added French and Sinhalese translation (Thanks to londumas and Thambaru
  Wijesekara)

## Version 1.4 (2012-02-29)
- Process self-posted incoming messages too. (This happens e.g. when using ZNC
  behind an irssi-proxy.) Really! ;) Please make sure to read
  https://bugs.launchpad.net/pidgin-znc-helper/+bug/900754 carefully!
	
## Version 1.3.1 (2011-11-25)
- Process self-posted incoming messages too. (This happens e.g. when using ZNC
  behind an irssi-proxy.)
- Added Spain and Russian translation

## Version 1.3 (2010-08-05)
- Don't check whether the timestamp is in the end of the string (mIRC sends
  special formatting characters that are interpreted into HTML from Pidgin
  which means that the timestamps aren't anymore at the end)
- Check whether the part of the message after timestamp contains only HTML tags
  (this means usually the timestamp was at the end of the message)
- Time offset per account including new preferences dialog

## Version 1.2 (2010-06-05)
- Added advice to preferences window
- Fix: empty messages when timestamp is not appended (#570265)

## Version 1.1 (2010-03-18)
- Display italic buddy names if buddy is offline

## Version 1.01 (2009-11-16)
- Corrected plugins website

## Version 1.0 (2009-11-16)
- Added support for personal messages
- Updated UI requirements

## Version 0.4 (2009-11-12)
- Fixed Readme for Windows (again -.-')
- Added an option to offset wrong times (for some friends :) I know it isn't
  really the plugins job)
- The plugin now blocks unloading to avoid orphan options in account settings
- New timestamp handler to avoid displaying the date on every message

## Version 0.3 (2009-11-10)
- Fixed issue with daylight saving time
- Fixed Readme files

## Version 0.2 (2009-10-05)
- Implemented own timestamp recognition to safely notice recognition failures
- Changed ZNC timestamp format to "[%Y-%m-%d %H:%M:%S]" appended to the end of
  the message which fixes a bug when replaying /me-messages

## Version 0.1 (2009-10-02)
- ZNC Helper was born.

