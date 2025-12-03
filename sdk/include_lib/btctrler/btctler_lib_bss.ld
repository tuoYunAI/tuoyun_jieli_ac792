
        btctler_bss_start = .;

        BTCTLER_CONTROLLER_BSS_START = .;
        *(.bt_rf.data.bss)
        *(.hci_controller.data.bss)
        *(.vendor_manager.data.bss)
        BTCTLER_CONTROLLER_BSS_SIZE = ABSOLUTE(. - BTCTLER_CONTROLLER_BSS_START);

        BTCTLER_LE_CONTROLLER_BSS_START = .;
        *(.ble_hci.data.bss)
        *(.ble_ll.data.bss)
        *(.ble_rf.data.bss)
        BTCTLER_LE_CONTROLLER_BSS_SIZE = ABSOLUTE(. - BTCTLER_LE_CONTROLLER_BSS_START);

        BTCTLER_CL_BSS_START = .;
        *(.lmp.data.bss)
        *(.lmp_auth.data.bss)
        *(.classic.data.bss)
        BTCTLER_CL_BSS_SIZE = ABSOLUTE(. - BTCTLER_CL_BSS_START);

        btctler_bss_end = .;

