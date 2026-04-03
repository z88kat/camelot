#ifndef LOG_H
#define LOG_H

#include "common.h"

/* Message log entry */
typedef struct {
    char text[256];
    int  turn;        /* turn number when message was added */
    short color_pair; /* colour for this message */
} LogEntry;

/* Message log (ring buffer) */
typedef struct {
    LogEntry entries[LOG_BUFFER];
    int      head;    /* next write position */
    int      count;   /* total entries written (capped at LOG_BUFFER) */
} MessageLog;

/* Initialize the message log. */
void log_init(MessageLog *log);

/* Add a formatted message to the log. */
void log_add(MessageLog *log, int turn, short color_pair, const char *fmt, ...);

/* Get a log entry by index (0 = most recent). Returns NULL if out of range. */
const LogEntry *log_get(const MessageLog *log, int index);

#endif /* LOG_H */
