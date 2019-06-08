TEMPLATE = subdirs
SUBDIRS = ipc test chat
test.depends = ipc
chat.depends = ipc