
		. = ALIGN(4);
		update_code_begin = .;

		*(.bt_updata_ram_code)
		*(.update_const)
		*(.update_code)

		update_code_end = .;
