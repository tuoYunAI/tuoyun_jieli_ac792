
        btctler_data_start = .;

        BTCTLER_CONTROLLER_DATA_START = .;
        *(.bt_rf.data)
        *(.hci_controller.data)
        *(.vendor_manager.data)
        BTCTLER_CONTROLLER_DATA_SIZE = ABSOLUTE(. - BTCTLER_CONTROLLER_DATA_START);

        BTCTLER_LE_CONTROLLER_DATA_START = .;
        *(.ble_hci.data)
        *(.ble_ll.data)
        *(.ble_rf.data)
        BTCTLER_LE_CONTROLLER_DATA_SIZE = ABSOLUTE(. - BTCTLER_LE_CONTROLLER_DATA_START);

        BTCTLER_CL_DATA_START = .;
        *(.lmp.data)
        *(.lmp_auth.data)
        *(.classic.data)
        BTCTLER_CL_DATA_SIZE = ABSOLUTE(. - BTCTLER_CL_DATA_START);

        btctler_data_end = .;

