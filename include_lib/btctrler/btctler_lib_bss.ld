
        btctler_bss_start = .;

        BTCTLER_CONTROLLER_BSS_START = .;

        *(.bd_base)
        *(.bredr_rxtx_bulk)
        acl_tx_pool = .;
        *(.bredr_tx_bulk)
#ifdef CONFIG_BT_TX_BUFF_SIZE
        acl_tx_pool_end = acl_tx_pool + CONFIG_BT_TX_BUFF_SIZE;
#else
        acl_tx_pool_end = acl_tx_pool;
#endif
        . = acl_tx_pool_end;

        acl_rx_pool = .;
        *(.bredr_rx_bulk)
#ifdef CONFIG_BT_RX_BUFF_SIZE
        acl_rx_pool_end = acl_rx_pool + CONFIG_BT_RX_BUFF_SIZE;
#else
        acl_rx_pool_end = acl_rx_pool;
#endif
        . = acl_rx_pool_end;

        tws_bulk_pool = .;
#ifdef CONFIG_TWS_BULK_POOL_SIZE
        tws_bulk_pool_end = tws_bulk_pool + CONFIG_TWS_BULK_POOL_SIZE;
#else
        tws_bulk_pool_end = tws_bulk_pool;
#endif
        . = tws_bulk_pool_end;

        *(.bt_rf_bss)
		*(.hci_controller_bss)
		*(.hci_interface_bss)
		*(.device_manager_bss)
		*(.vendor_manager_bss)
        BTCTLER_CONTROLLER_BSS_SIZE = ABSOLUTE(. - BTCTLER_CONTROLLER_BSS_START);

        BTCTLER_LE_CONTROLLER_BSS_START = .;
        *(.ble_hci_bss)
        *(.ble_ll_bss)
        *(.ble_rf_bss)
        BTCTLER_LE_CONTROLLER_BSS_SIZE = ABSOLUTE(. - BTCTLER_LE_CONTROLLER_BSS_START);

        BTCTLER_CL_BSS_START = .;
        *(.classic_rf_bss)
        *(.classic_lmp_bss)
        *(.classic_lmp_auth_bss)
        *(.classic_lmp_bigint_bss)
        *(.classic_lmp_crypt_bss)
        *(.classic_lmp_ecdh_bss)
        *(.classic_lmp_linkbulk_bss)
        *(.classic_lmp_hmac_bss)
        *(.classic_bb_bss)
        *(.classic_hci_bss)
        BTCTLER_CL_BSS_SIZE = ABSOLUTE(. - BTCTLER_CL_BSS_START);

        btctler_bss_end = .;

