/*
 * X event name/number mapping utils
 */
#ifndef _EVENT_NAMES_H
#define _EVENT_NAMES_H

#include <stddef.h>    // for size_t

size_t event_names_size(void);
const char *event_name_by_num(int);
int event_num_by_name(const char *);

#endif /* _EVENT_NAMES_H */
