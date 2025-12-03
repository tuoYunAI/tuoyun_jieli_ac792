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


#define RTL8201_TIMEOUT     (20000000UL)

#define	RTL8201_BMCR 		0	/* Basic Mode Control Register */
#define RTL8201_BMSR		1	/* Basic Mode Status Register */
#define RTL8201_PHYID1		2	/* PHY Idendifier Register 1 */
#define RTL8201_PHYID2		3	/* PHY Idendifier Register 2 */
#define RTL8201_ANAR		4	/* Auto_Negotiation Advertisement Register  */
#define RTL8201_ANLPAR		5	/* Auto_negotiation Link Partner Ability Register */
#define RTL8201_ANER		6	/* Auto-negotiation Expansion Register  */
#define RTL8201_TEST		25

#define RTL8201_PSR         31  /*页选择*/

#define RTL8201_RMSR        16  /*  7 page    RMII Mode Setting Register*/


/* --Bit definitions: RTL8201_BMCR */
#define RTL8201_RESET  	          (1 << 15)	/* 1= Software Reset; 0=Normal Operation */
#define RTL8201_LOOPBACK          (1 << 14)	/* 1=loopback Enabled; 0=Normal Operation */
#define RTL8201_SPEED_SELECT      (1 << 13)	/* 1=100Mbps; 0=10Mbps */
#define RTL8201_AUTONEG	          (1 << 12)
#define RTL8201_POWER_DOWN        (1 << 11)
#define RTL8201_RESTART_AUTONEG   (1 << 9)
#define RTL8201_DUPLEX_MODE       (1 << 8)

/*--Bit definitions: RTL8201_BMSR */
#define RTL8201_100BASE_T4        (1 << 15)
#define RTL8201_100BASE_TX_FD     (1 << 14)
#define RTL8201_100BASE_T4_HD     (1 << 13)
#define RTL8201_10BASE_T_FD       (1 << 12)
#define RTL8201_10BASE_T_HD       (1 << 11)
#define RTL8201_MF_PREAMB_SUPPR   (1 << 6)
#define RTL8201_AUTONEG_COMP      (1 << 5)
#define RTL8201_REMOTE_FAULT      (1 << 4)
#define RTL8201_AUTONEG_ABILITY   (1 << 3)
#define RTL8201_LINK_STATUS       (1 << 2)
#define RTL8201_JABBER_DETECT     (1 << 1)
#define RTL8201_EXTEND_CAPAB      (1 << 0)

/*--definitions: RTL8201_PHYID1 */
#define RTL8201_PHYID1_OUI	 0x0000
#define RTL8201_PHYID2_OUI	 0x8201

/*--Bit definitions: RTL8201_ANAR, RTL8201_ANLPAR */
#define RTL8201_NP               (1 << 15)
#define RTL8201_ACK              (1 << 14)
#define RTL8201_RF               (1 << 13)
#define RTL8201_FCS              (1 << 10)
#define RTL8201_T4               (1 << 9)
#define RTL8201_TX_FDX           (1 << 8)
#define RTL8201_TX_HDX           (1 << 7)
#define RTL8201_10_FDX           (1 << 6)
#define RTL8201_10_HDX           (1 << 5)
#define RTL8201_AN_IEEE_802_3	0x0001

/*--Bit definitions: RTL8201_ANER */
#define RTL8201_PDF              (1 << 4)
#define RTL8201_LP_NP_ABLE       (1 << 3)
#define RTL8201_NP_ABLE          (1 << 2)
#define RTL8201_PAGE_RX          (1 << 1)
#define RTL8201_LP_AN_ABLE       (1 << 0)


/*--Bit definitions: RTL8201_TEST */
#define RTL8201_LINK100		    (1 << 0)
#define RTL8201_LINK10		    (1 << 1)
#define RTL8201_RMII_MODE       (1 << 10)
#define RTL8201_RMII_CKLIN      (1 << 11)
#define RTL8201_RMII_RXD_TRIG   (1 << 13)


/*--Bit definitions: RTL8201_DSCR */
#define RTL8201_BP4B5B           (1 << 15)
#define RTL8201_BP_SCR           (1 << 14)
#define RTL8201_BP_ALIGN         (1 << 13)
#define RTL8201_BP_ADPOK         (1 << 12)
#define RTL8201_REPEATER         (1 << 11)
#define RTL8201_TX               (1 << 10)
#define RTL8201_RMII_ENABLE      (1 << 8)
#define RTL8201_F_LINK_100       (1 << 7)
#define RTL8201_SPLED_CTL        (1 << 6)
#define RTL8201_COLLED_CTL       (1 << 5)
#define RTL8201_RPDCTR_EN        (1 << 4)
#define RTL8201_SM_RST           (1 << 3)
#define RTL8201_MFP_SC           (1 << 2)
#define RTL8201_SLEEP            (1 << 1)
#define RTL8201_RLOUT            (1 << 0)

/*--Bit definitions: RTL8201_DSCSR */
#define RTL8201_100FDX           (1 << 15)
#define RTL8201_100HDX           (1 << 14)
#define RTL8201_10FDX            (1 << 13)
#define RTL8201_10HDX            (1 << 12)

/*--Bit definitions: RTL8201_10BTCSR */
#define RTL8201_LP_EN           (1 << 14)
#define RTL8201_HBE             (1 << 13)
#define RTL8201_SQUELCH         (1 << 12)
#define RTL8201_JABEN           (1 << 11)
#define RTL8201_10BT_SER        (1 << 10)
#define RTL8201_POLR            (1 << 0)


/*--Bit definitions: RTL8201_MDINTR */
#define RTL8201_INTR_PEND       (1 << 15)
#define RTL8201_FDX_MASK        (1 << 11)
#define RTL8201_SPD_MASK        (1 << 10)
#define RTL8201_LINK_MASK       (1 << 9)
#define RTL8201_INTR_MASK       (1 << 8)
#define RTL8201_FDX_CHANGE      (1 << 4)
#define RTL8201_SPD_CHANGE      (1 << 3)
#define RTL8201_LINK_CHANGE     (1 << 2)
#define RTL8201_INTR_STATUS     (1 << 0)


static void rtl8201_set_mii_rmii_mode(const struct eth_platform_data *data)
{
    u16 cr = eth_mii_read(&data->rmii_bus, RTL8201_PSR);

    log_info("page = %d", cr);

    cr = 0x7; //Settine 7 Page
    eth_mii_write(&data->rmii_bus, RTL8201_PSR, cr);

    log_info("page = %d", cr);

    cr = eth_mii_read(&data->rmii_bus, RTL8201_RMSR);

    log_info("RMSR = %d", cr);

    if (data->mode == MII_MODE) {
        log_info("MII_MODE");
        cr &= ~BIT(3);
    } else { //RTL8201(EL) only
        log_info("RMII_MODE");
        cr |= BIT(3);
        cr &= ~BIT(12);
    }

    eth_mii_write(&data->rmii_bus, RTL8201_RMSR, cr);

    eth_mii_write(&data->rmii_bus, RTL8201_PSR, 0);
}

static void rtl8201_set_speed_mode(const struct eth_platform_data *data)
{
    u16 cr = eth_mii_read(&data->rmii_bus, RTL8201_BMCR);

    log_info("RTL8201 BMCR read=  0x%x", cr);

    switch (data->speed) {
    case PHY_FULLDUPLEX_100M:
        cr &= ~RTL8201_AUTONEG;         // Clear auto negotiate bit
        cr |= RTL8201_SPEED_SELECT;     // set fast eth. bit
        cr |= RTL8201_DUPLEX_MODE;      //set full dplx
        eth_mii_write(&data->rmii_bus, RTL8201_BMCR, cr);
        break;
    case PHY_HALFDUPLEX_100M:
        cr &= ~RTL8201_AUTONEG;         // Clear auto negotiate bit
        cr |= RTL8201_SPEED_SELECT;     // set fast eth. bit
        cr &= ~RTL8201_DUPLEX_MODE;      //Clear full dplx
        eth_mii_write(&data->rmii_bus, RTL8201_BMCR, cr);
        break;
    case PHY_FULLDUPLEX_10M:
        cr &= ~RTL8201_AUTONEG;             // Clear auto negotiate bit
        cr &= ~RTL8201_SPEED_SELECT;        // Clear fast eth. bit
        cr &= ~RTL8201_DUPLEX_MODE;          //Clear full dplx
        eth_mii_write(&data->rmii_bus, RTL8201_BMCR, cr);
        break;
    case PHY_HALFDUPLEX_10M:
        cr &= ~RTL8201_AUTONEG;             // Clear auto negotiate bit
        cr &= ~RTL8201_SPEED_SELECT;        // Clear fast eth. bit
        cr |= RTL8201_DUPLEX_MODE;          // set full dplx
        eth_mii_write(&data->rmii_bus, RTL8201_BMCR, cr);
        break;
    case PHY_AUTONEGOTINATION:
        cr |= RTL8201_AUTONEG;          // set auto negotiate bit
        eth_mii_write(&data->rmii_bus, RTL8201_BMCR, cr);
        while (!(eth_mii_read(&data->rmii_bus, RTL8201_BMSR) & RTL8201_AUTONEG_COMP)) {
            ;//wait auto negotiation completed
        }
        break;
    default:
        break;
    }

    log_info("RTL8201 BMCR write= 0x%x", eth_mii_read(&data->rmii_bus, RTL8201_BMCR));
}

static int rtl8201_reset(const struct eth_platform_data *data)
{
    u32 reset_to = RTL8201_TIMEOUT;

    u16 cr = eth_mii_read(&data->rmii_bus, RTL8201_BMCR);
    if (cr & RTL8201_POWER_DOWN) {
        return -1;
    }

    cr |= RTL8201_RESET; // set reset bit
    eth_mii_write(&data->rmii_bus, RTL8201_BMCR, cr);
    while (eth_mii_read(&data->rmii_bus, RTL8201_BMCR) & RTL8201_RESET) {
        if (reset_to-- == 0) {
            return -1;
        }
    }
    //cr &= ~RTL8201_AUTONEG;
    // eth_mii_write(&data->rmii_bus, RTL8201_BMCR, cr);
    log_info("rtl8201 reset -> 0x%x", cr);

    return 0;
}

static int rtl8201_IsPhyConnected(const struct eth_platform_data *data)
{
    u16 status = eth_mii_read(&data->rmii_bus, RTL8201_BMSR);
    //log_info("rtl8201 connect status  0x%x \n",status);
    if (status & RTL8201_LINK_STATUS) {
        return 0;
    }

    return -1;
}

static void rtl8201_GetLinkSpeed(const struct eth_platform_data *data)
{
    u16 stat1, stat2;

    if (!(eth_mii_read(&data->rmii_bus, RTL8201_BMSR) & RTL8201_LINK_STATUS)) {
        log_info("cable disconnected");
        return;
    }

    stat1 = eth_mii_read(&data->rmii_bus, RTL8201_BMCR);
    stat2 = eth_mii_read(&data->rmii_bus, RTL8201_TEST);
    if ((stat1 & RTL8201_DUPLEX_MODE) && (stat2 & RTL8201_LINK100)) {
        log_info("100baseTX Full Duplex");
        return;
    }

    if ((stat1 & RTL8201_DUPLEX_MODE) && (stat2 & RTL8201_LINK10)) {
        log_info("10baseT Full Duplex");
        return;
    }

    if ((!(stat1 & RTL8201_DUPLEX_MODE)) && (stat2 & RTL8201_LINK100)) {
        log_info("100baseTX Half Duplex");
        return;
    }

    if ((!(stat1 & RTL8201_DUPLEX_MODE)) && (stat2 & RTL8201_LINK10)) {
        log_info("10baseT Half Duplex");
        return;
    }
}

static u16 rtl8201_read_identifier_reg1(const struct software_rmii *rmii_bus)
{
    // Hardset PHY to just use 10Mbit mode
    return eth_mii_read(rmii_bus, RTL8201_PHYID1);
}

static u16 rtl8201_read_identifier_reg2(const struct software_rmii *rmii_bus)
{
    // Hardset PHY to just use 10Mbit mode
    return eth_mii_read(rmii_bus, RTL8201_PHYID2);
}

static int rtl8201_check_name(const struct eth_platform_data *data)
{
    if (!strcmp((const char *)data->name, "rtl8201e")) {
        return 51221;
    } else if (!strcmp((const char *)data->name, "rtl8201f")) {
        return 51222;
    }
    return 0;
}

static int rtl8201_InitPhy(const struct eth_platform_data *data)
{
    if (data == NULL) {
        return -1;
    }

    if (rtl8201_reset(data)) {
        return -1;
    }

    u32 driver_id = rtl8201_check_name(data);
    log_info("id1 :%d   id2 :%d ", rtl8201_read_identifier_reg1(&data->rmii_bus)
             , rtl8201_read_identifier_reg2(&data->rmii_bus));

    u32 device_id = rtl8201_read_identifier_reg2(&data->rmii_bus);
    if (driver_id != device_id) {
        log_info("rtl8201 driver no match device");
        return -1;
    }

    rtl8201_set_mii_rmii_mode(data);
    /* rtl8201_set_speed_mode(data); */
    /* rtl8201_GetLinkSpeed(data); */
    return 0;
}

static void rtl8201_enable_loopback(const struct eth_platform_data *data)
{
    u16 cr = eth_mii_read(&data->rmii_bus, RTL8201_BMCR);

    cr |= RTL8201_LOOPBACK;

    eth_mii_write(&data->rmii_bus, RTL8201_BMCR, cr);
}

REGISTER_NET_PHY_DEVICE(rtl8201f_dev_ops) = {
    .name           = "rtl8201f",
    .init           = rtl8201_InitPhy,
    .is_connect     = rtl8201_IsPhyConnected,
    .get_link_speed = rtl8201_GetLinkSpeed,
    .set_link_speed = rtl8201_set_speed_mode,
};

#endif
