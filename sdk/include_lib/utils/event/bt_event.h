#ifndef _BT_EVENT_H_
#define _BT_EVENT_H_

#include "event/event.h"

enum bt_event_from {
    BT_EVENT_FROM_CTRLER = 1,
    BT_EVENT_FROM_HCI,
    BT_EVENT_FROM_CON,
    BT_EVENT_FROM_TWS,
    BT_EVENT_FROM_LOCAL_TWS,
    BT_EVENT_FROM_BIG,
    BT_EVENT_FROM_AI,
    BT_EVENT_FROM_BLE,
    BT_EVENT_FROM_USER,
};

struct bt_event {
    u8 event;
    u8 args[7];
    u32 value;
};

int bt_event_notify(enum bt_event_from from, struct bt_event *event);

#endif
