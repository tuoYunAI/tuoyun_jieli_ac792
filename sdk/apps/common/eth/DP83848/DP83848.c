#include "eth/ethmac.h"
#include "app_config.h"

#if TCFG_ETH_ENABLE

#define LOG_TAG_CONST       ETH
#define LOG_TAG             "[ETH]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
/* #define LOG_CLI_ENABLE */
#include "system/debug.h"


#define DP83848_TIMEOUT     (2000UL)

/** @defgroup DP83848_PHY	BOARD: PHY status and control driver for the DP83848
 * @ingroup BOARD_PHY
 * Various functions for controlling and monitoring the status of the
 * DP83848 PHY.
 * @{
 */

/** @brief  DP83848 PHY register offsets */
#define DP8_BMCR_REG        0x0	/*!< Basic Mode Control Register */
#define DP8_BMSR_REG        0x1	/*!< Basic Mode Status Reg */
#define DP8_ANADV_REG       0x4	/*!< Auto_Neg Advt Reg  */
#define DP8_ANLPA_REG       0x5	/*!< Auto_neg Link Partner Ability Reg */
#define DP8_ANEEXP_REG      0x6	/*!< Auto-neg Expansion Reg  */
#define DP8_PHY_STAT_REG    0x10/*!< PHY Status Register  */
#define DP8_PHY_INT_CTL_REG 0x11/*!< PHY Interrupt Control Register */
#define DP8_PHY_RBR_REG     0x17/*!< PHY RMII and Bypass Register  */
#define DP8_PHY_STS_REG     0x19/*!< PHY Status Register  */

/* DP83848 Control register definitions */
#define DP8_RESET          (1 << 15)	/*!< 1= S/W Reset */
#define DP8_LOOPBACK       (1 << 14)	/*!< 1=loopback Enabled */
#define DP8_SPEED_SELECT   (1 << 13)	/*!< 1=Select 100MBps */
#define DP8_AUTONEG        (1 << 12)	/*!< 1=Enable auto-negotiation */
#define DP8_POWER_DOWN     (1 << 11)	/*!< 1=Power down PHY */
#define DP8_ISOLATE        (1 << 10)	/*!< 1=Isolate PHY */
#define DP8_RESTART_AUTONEG (1 << 9)	/*!< 1=Restart auto-negoatiation */
#define DP8_DUPLEX_MODE    (1 << 8)		/*!< 1=Full duplex mode */
#define DP8_COLLISION_TEST (1 << 7)		/*!< 1=Perform collsion test */

/* DP83848 Status register definitions */
#define DP8_100BASE_T4     (1 << 15)	/*!< T4 mode */
#define DP8_100BASE_TX_FD  (1 << 14)	/*!< 100MBps full duplex */
#define DP8_100BASE_TX_HD  (1 << 13)	/*!< 100MBps half duplex */
#define DP8_10BASE_T_FD    (1 << 12)	/*!< 100Bps full duplex */
#define DP8_10BASE_T_HD    (1 << 11)	/*!< 10MBps half duplex */
#define DP8_MF_PREAMB_SUPPR (1 << 6)	/*!< Preamble suppress */
#define DP8_AUTONEG_COMP   (1 << 5)		/*!< Auto-negotation complete */
#define DP8_RMT_FAULT      (1 << 4)		/*!< Fault */
#define DP8_AUTONEG_ABILITY (1 << 3)	/*!< Auto-negotation supported */
#define DP8_LINK_STATUS    (1 << 2)		/*!< 1=Link active */
#define DP8_JABBER_DETECT  (1 << 1)		/*!< Jabber detect */
#define DP8_EXTEND_CAPAB   (1 << 0)		/*!< Supports extended capabilities */

/* DP83848 PHY RBR MII dode definitions */
#define DP8_RBR_RMII_MODE  (1 << 5)		/*!< Use RMII mode */

/* DP83848 PHY status definitions */
#define DP8_REMOTEFAULT    (1 << 6)		/*!< Remote fault */
#define DP8_FULLDUPLEX     (1 << 2)		/*!< 1=full duplex */
#define DP8_SPEED10MBPS    (1 << 1)		/*!< 1=10MBps speed */
#define DP8_VALID_LINK     (1 << 0)		/*!< 1=Link active */

/* DP83848 PHY ID register definitions */
#define DP8_PHYID1_OUI     0x2000		/*!< Expected PHY ID1 */
#define DP8_PHYID2_OUI     0x5c90		/*!< Expected PHY ID2 */

static void dp83848_set_mii_rmii_mode(const struct eth_platform_data *data)
{
    u16 cr = eth_mii_read(&data->rmii_bus, DP8_PHY_RBR_REG);

    log_info("mode -> 0x%x", cr);

    if (data->mode == MII_MODE) {
        log_info("MII_MODE");
        cr &= ~DP8_RBR_RMII_MODE;
    } else {
        log_info("RMII_MODE");
        cr |= DP8_RBR_RMII_MODE;
    }
    eth_mii_write(&data->rmii_bus, DP8_PHY_RBR_REG, cr);

    log_info("mode2 -> 0x%x", eth_mii_read(&data->rmii_bus, DP8_PHY_RBR_REG));
}

static void dp83848_set_speed_mode(const struct eth_platform_data *data)
{
    u16 cr = eth_mii_read(&data->rmii_bus, DP8_BMCR_REG);
    log_info("set speed mode read reg 0x%x", cr);

    switch (data->speed) {
    case PHY_FULLDUPLEX_100M:
        cr &= ~DP8_AUTONEG;         // Clear auto negotiate bit
        cr |= DP8_SPEED_SELECT;     // set fast eth. bit
        cr |= DP8_DUPLEX_MODE;      //set full dplx
        eth_mii_write(&data->rmii_bus, DP8_BMCR_REG, cr);
        log_info("set speed mode write reg 0x%x", eth_mii_read(&data->rmii_bus, DP8_BMCR_REG));

        break;
    case PHY_HALFDUPLEX_100M:
        cr &= ~DP8_AUTONEG;         // Clear auto negotiate bit
        cr |= DP8_SPEED_SELECT;     // set fast eth. bit
        cr &= ~DP8_DUPLEX_MODE;      //Clear full dplx
        eth_mii_write(&data->rmii_bus, DP8_BMCR_REG, cr);
        break;
    case PHY_FULLDUPLEX_10M:
        cr &= ~DP8_AUTONEG;             // Clear auto negotiate bit
        cr &= ~DP8_SPEED_SELECT;        // Clear fast eth. bit
        cr &= ~DP8_DUPLEX_MODE;          //Clear full dplx
        eth_mii_write(&data->rmii_bus, DP8_BMCR_REG, cr);
        break;
    case PHY_HALFDUPLEX_10M:
        cr &= ~DP8_AUTONEG;             // Clear auto negotiate bit
        cr &= ~DP8_SPEED_SELECT;        // Clear fast eth. bit
        cr |= DP8_DUPLEX_MODE;          // set full dplx
        eth_mii_write(&data->rmii_bus, DP8_BMCR_REG, cr);
        break;
    case PHY_AUTONEGOTINATION:
        cr |= DP8_AUTONEG;          // set auto negotiate bit
        eth_mii_write(&data->rmii_bus, DP8_BMCR_REG, cr);
        while (!(eth_mii_read(&data->rmii_bus, DP8_BMSR_REG) & DP8_AUTONEG_COMP)) {
            ;//wait auto negotiation completed
        }
        break;
    default:
        break;
    }
}

static int dp83848_reset(const struct eth_platform_data *data)
{
    u32 reset_to = DP83848_TIMEOUT;
//u32 ret = 32;
    u16 cr;
//while(ret--)
//{
    cr = eth_mii_read(&data->rmii_bus, DP8_BMCR_REG);
//	eth_set_mdc(1);
    //	eth_set_mdio(1);
    log_info("cr -> 0x%x", cr);

//}
    if (cr & DP8_POWER_DOWN) {
        return -1;
    }

    cr |= DP8_RESET; // set reset bit
    eth_mii_write(&data->rmii_bus, DP8_BMCR_REG, cr);

    while (eth_mii_read(&data->rmii_bus, DP8_BMCR_REG) & (DP8_RESET)) {
        if (reset_to-- == 0) {
            return -1;
        }
    }

    log_info("retset -> 0x%x", eth_mii_read(&data->rmii_bus, DP8_BMCR_REG));

    return 0;
}

static int dp83848_IsPhyConnected(const struct eth_platform_data *data)
{
    if (eth_mii_read(&data->rmii_bus, DP8_BMSR_REG) & DP8_LINK_STATUS) {
        return 0;
    }
    return -1;
}

static void dp83848_GetLinkSpeed(const struct eth_platform_data *data)
{
    log_info("GetLinkSpeed -> 0x%x", eth_mii_read(&data->rmii_bus, DP8_BMSR_REG));

    if (!(eth_mii_read(&data->rmii_bus, DP8_BMSR_REG) & DP8_LINK_STATUS)) {
        log_info("cable disconnected");
        return;
    }

    u16 stat = eth_mii_read(&data->rmii_bus, DP8_PHY_STAT_REG);
    log_info("stat -> 0x%x", stat);

    if ((stat & DP8_FULLDUPLEX) && (!(stat & DP8_SPEED10MBPS))) {
        log_info("100baseTX Full Duplex");
        return;
    }

    if ((stat & DP8_FULLDUPLEX) && (stat & DP8_SPEED10MBPS)) {
        log_info("10baseT Full Duplex");
        return;
    }

    if ((!(stat & DP8_FULLDUPLEX)) && (!(stat & DP8_SPEED10MBPS))) {
        log_info("100baseTX Half Duplex");
        return;
    }

    if ((!(stat & DP8_FULLDUPLEX)) && (stat & DP8_SPEED10MBPS)) {
        log_info("10baseT Half Duplex");
        return;
    }
}

static int dp83848_InitPhy(const struct eth_platform_data *data)
{
    /* u32 connect_timeout = DP83848_TIMEOUT; */

    log_info("dp83848 reset");
    int ret = dp83848_reset(data);
    if (ret) {
        return ret;
    }
    /* puts("DP83848 Reset after\n");                 */
    /*     while(dp83848_IsPhyConnected(phyad))       */
    /*     {[> Try another time <]                    */
    /*         connect_timeout--;                     */
    /*         if (connect_timeout == 0)              */
    /*         {                                      */
    /*             oeth_puts("cable disconnected\n"); */
    /*             return -1;                         */
    /*         }                                      */
    /*     }                                          */
    dp83848_set_mii_rmii_mode(data);
    dp83848_set_speed_mode(data);
    dp83848_GetLinkSpeed(data);

    return 0;
}

void dp83848_enable_loopback(const struct eth_platform_data *data)
{
    u16 cr = eth_mii_read(&data->rmii_bus, DP8_BMCR_REG);

    cr |= DP8_LOOPBACK;

    eth_mii_write(&data->rmii_bus, DP8_BMCR_REG, cr);
}

REGISTER_NET_PHY_DEVICE(dp83848_dev_ops) = {
    .name           = "dp83848",
    .init           = dp83848_InitPhy,
    .is_connect     = dp83848_IsPhyConnected,
    .get_link_speed = dp83848_GetLinkSpeed,
    .set_link_speed = dp83848_set_speed_mode,
};

#endif
