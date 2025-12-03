#include "multi_protocol_main.h"
#include "multi_box_adv.h"
#include "generic/list.h"
#include "syscfg/syscfg_id.h"

#if (THIRD_PARTY_PROTOCOLS_SEL & MULTI_BOX_ADV_EN)

typedef struct {
    u8 mac_addr[6];
    struct list_head entry;
} mac_list_t;

static struct list_head mac_head = LIST_HEAD_INIT(mac_head);

static u8 multi_box_bis_enable;
static u8 multi_box_role = MULTI_BOX_ROLE_UNKNOWN;
static u16 multi_box_number = 0;


void reset_multi_box_number(void)
{
    multi_box_bis_enable = 0;
    multi_box_role = MULTI_BOX_ROLE_UNKNOWN;
    multi_box_number = 0;
}

u16 get_multi_box_number(void)
{
    return multi_box_number;
}

u8 get_multi_box_role(void)
{
    return multi_box_role;
}

bool check_multi_box_bis_is_exist(void)
{
    return multi_box_bis_enable;
}

void set_multi_box_role(u8 role)
{
    multi_box_role = role;
}

void clear_multi_box_mac_list(void)
{
    mac_list_t *p, *n;

    list_for_each_entry_safe(p, n, &mac_head, entry) {
        list_del(&p->entry);
        free(p);
    }
}

void multi_box_adv_data_handle(u8 *mac_addr, u8 adv_data)
{
    mac_list_t *p;
    u8 self_adv_data = get_multi_box_adv_data();
    u8 new_mac_addr = 1;
    u8 self_mac_addr[6];
    u8 edr_addr[6];

    if (syscfg_read(CFG_BT_MAC_ADDR, edr_addr, 6) < 0) {
        memcpy(edr_addr, (void *)bt_get_mac_addr(), 6);
    }

    for (u8 i = 0; i < 6; ++i) {
        self_mac_addr[i] = edr_addr[5 - i];
    }

    list_for_each_entry(p, &mac_head, entry) {
        if (0 == memcmp(mac_addr, p->mac_addr, 6)) {
            new_mac_addr = 0;
            break;
        }
    }

    if (!new_mac_addr) {
        return;
    }

    if (adv_data & BIT(7)) {
        multi_box_bis_enable = 1;
    }

    p = (mac_list_t *)malloc(sizeof(mac_list_t));
    if (p) {
        memcpy(p->mac_addr, mac_addr, 6);
        list_add_tail(&p->entry, &mac_head);
        ++multi_box_number;
    }

    if (multi_box_role == MULTI_BOX_ROLE_UNKNOWN) {
        if (multi_box_bis_enable) {
            multi_box_role = MULTI_BOX_ROLE_SLAVE;
            return;
        }
        if (self_adv_data > adv_data) {
            multi_box_role = MULTI_BOX_ROLE_MASTER;
        } else if (self_adv_data == adv_data) {
            for (u8 i = 0; i < 6; ++i) {
                if (self_mac_addr[i] > mac_addr[i]) {
                    multi_box_role = MULTI_BOX_ROLE_MASTER;
                    break;
                } else if (self_mac_addr[i] < mac_addr[i]) {
                    multi_box_role = MULTI_BOX_ROLE_SLAVE;
                    break;
                }
            }
        } else {
            multi_box_role = MULTI_BOX_ROLE_SLAVE;
        }
    } else if (multi_box_role == MULTI_BOX_ROLE_MASTER) {
        if (self_adv_data < adv_data) {
            multi_box_role = MULTI_BOX_ROLE_SLAVE;
        } else if (self_adv_data == adv_data) {
            for (u8 i = 0; i < 6; ++i) {
                if (self_mac_addr[i] < mac_addr[i]) {
                    multi_box_role = MULTI_BOX_ROLE_SLAVE;
                    break;
                } else if (self_mac_addr[i] > mac_addr[i]) {
                    break;
                }
            }
        }
    } else if (multi_box_role == MULTI_BOX_ROLE_SLAVE) {

    }
}

#endif

