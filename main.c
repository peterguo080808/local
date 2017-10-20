int uboot_main()
{
	int num;

#ifdef MMU_ON
	mmu_init();
#endif

	led_init();

	button_init_ext_int();

	irq_init();

	led_on();
	
	uart_init();
	
	uart_put_char('H');
	uart_put_char('E');

	while(1) {
		printf("\n*****************************\n\r");
		printf("\n************U-Boot***********\n\r");
		printf("[1]:Download Linux Kerel from TFTP server!\n\r");
		printf("[2]:Boot Linux from RAM!\n\r");
		printf("[3]:Boot Linux from Nand Flash!\n\r");
		printf("\n Plese Select:");

		scanf("%d", &num);

		switch (num) {
			case 1:
				//tftp_load();
				break;

			case 2:
				//boot_linux_arm();
				break;

			case 3:
				//boot_linux_nand();
				break;

			default:
				printf("Error: Wrong selection!\n\r");
				break;
		}
		 
	}

	return 0;
}

