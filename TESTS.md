# Pidgin ZNC Helper Tests

Unfortunately we don't have automated tests (patches welcome), so we need to do
them manually.

## Requirements
- Pidgin with ZNC Helper version under test (do not start Pidgin from Cygwin,
  see [Github #8][1])
- A second IRC client (subsequently called *irssi* but you can use any other
  IRC client)
- A second ZNC client bound to the same ZNC account as Pidgin (subsequently
  called *MutterIRC* but you can use any other ZNC client)

[1]: https://github.com/kgraefe/pidgin-znc-helper/issues/8

## Test steps
1. Connect *Pidgin* through ZNC and *irssi* to the same IRC server
1. Join the same channel
1. Send a chat and a query message from *irssi* to *Pidgin* and **check the
   timestamps**
1. Disconnect *Pidgin* from ZNC
1. Send a chat message from *irssi* to *Pidgin* and wait a few seconds
1. Send a query message from *irssi* to *Pidgin* and wait a few seconds
1. Re-connect *Pidgin* to receive the messages and **check the timestamps**
1. Connect *MutterIRC* to ZNC
1. Send a chat and a query message from *MutterIRC* to *irssi* and **check the
   presence and the timestamps** in *Pidgin*
1. Check the **presence and the timestamps** in *Pidgin's* chat history
1. Disconnect *Pidgin*
1. Send a chat and a query message from *MutterIRC* to *irssi*
1. Send a chat message from *irssi* to *Pidgin*
1. Disconnect *irssi* and wait a few seconds
1. Re-connect *Pidgin* to receive the messages, **check the timestamps and that
   the nick of the irssi-user is italic**
