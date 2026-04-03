#include "log.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

void log_init(MessageLog *log) {
    memset(log, 0, sizeof(*log));
}

void log_add(MessageLog *log, int turn, short color_pair, const char *fmt, ...) {
    LogEntry *entry = &log->entries[log->head];
    entry->turn = turn;
    entry->color_pair = color_pair;

    va_list args;
    va_start(args, fmt);
    vsnprintf(entry->text, sizeof(entry->text), fmt, args);
    va_end(args);

    log->head = (log->head + 1) % LOG_BUFFER;
    if (log->count < LOG_BUFFER) {
        log->count++;
    }
}

const LogEntry *log_get(const MessageLog *log, int index) {
    if (index < 0 || index >= log->count) {
        return NULL;
    }
    int pos = (log->head - 1 - index + LOG_BUFFER) % LOG_BUFFER;
    return &log->entries[pos];
}
