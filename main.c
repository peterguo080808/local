int uboot_main()
{
	unsigned char buff[1024*4];

#ifdef MMU_ON
	mmu_init();
#endif

	led_init();

	button_init_ext_int();

	irq_init();

	led_on();

	Erase_NandFlash(128*1+1);

	buff[0] = 100;

	NandFlash_PageWrite(128*1+1,buff);

	buff[0] = 10;

	NandFlash_PageRead(128*1+1,buff);

	if (buff[0] == 100) {
		led_off();
	}

	while(1) ;

	return 0;
}

