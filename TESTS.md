# Pidgin ZNC Helper Tests

Unfortunately we don't have automated tests (patches welcome), so we need to do
them manually.

Test in both chats and queries for:
- Offline messages (watch the timestamps)
- Online messages
- Offline self messages written by a second client (watch the timestamps)
- Online self messages written by a second client
- Parted users must be italic (chats only)
- Self messages in chat history
