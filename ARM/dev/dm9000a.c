#include "type.h"
#include "dm9000x.h"
#include "arp.h"

#define CONFIG_DM9000_BASE	0x18000300 /*XMOCSN1*/
#define DM9000_DATA		0x18000304 /*ADDR2*/
#define DM9000_IO		CONFIG_DM9000_BASE
#define CONFIG_DM9000_USE_16BIT	1
#define DM9000_BASE		CONFIG_DM9000_BASE
#define DM9000_PPTR		(*((volatile unsigned short *)(DM9000_IO)))
#define DM9000_PDATA		(*((volatile unsigned short *)(DM9000_DATA)))

#define SROM_BW			(*((volatile unsigned long *)0x70000000))
#define SROM_BC1		(*((volatile unsigned long *)0x70000008))

/* ------------------------------------------------------------------------- */
#define DM9000_Tacs	(0x0)	// 0clk		address set-up
#define DM9000_Tcos	(0x4)	// 4clk		chip selection set-up
#define DM9000_Tacc	(0xE)	// 14clk	access cycle
#define DM9000_Tcoh	(0x1)	// 1clk		chip selection hold
#define DM9000_Tah	(0x4)	// 4clk		address holding time
#define DM9000_Tacp	(0x6)	// 6clk		page mode access cycle
#define DM9000_PMC	(0x0)	// normal(1data)page mode configuration
/* ------------------------------------------------------------------------- */

#define GPNCON			(*((volatile unsigned long *)0x7F008830))

#define DM9000_ID		0x90000A46
#define DM9KS_REG05		(RXCR_Discard_LongPkt|RXCR_Discard_CRCPkt)
#define DM9KS_DISINTR		IMR_SRAM_antoReturn


unsigned char buffer[1500];

unsigned char host_mac_addr[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
unsigned char mac_addr[6] = {9,8,7,6,5,4};
//unsigned char host_ip_addr[4] = {10,100,15,99};
//unsigned char ip_addr[4] = {10,100,15,202};

unsigned char host_ip_addr[4] = {192,168,1,100};
unsigned char ip_addr[4] = {192,168,1,202};

unsigned short packet_len;

void dm9000_cs_init(void)
{
	int SROM_BW_value, SROM_BC1_value;
	
	 //Data bus width control for Memory Bank1
	SROM_BW &= (~(0xf<<4));
	SROM_BW |= (0x1<<4);
	SROM_BW_value = SROM_BW;
	printf("\n\r SROM_BW_value is : %d \n\r", SROM_BW_value);	 

	//SROM BANK CONTROL REGISTER
	SROM_BC1 = ((DM9000_Tacs<<28)+(DM9000_Tcos<<24)+(DM9000_Tacc<<16)+(DM9000_Tcoh<<12)+(DM9000_Tah<<8)+(DM9000_Tacp<<4)+(DM9000_PMC));
	SROM_BC1_value = SROM_BC1;
	printf("\n\r SROM_BC1_value is : %d \n\r", SROM_BC1_value);
}

void dm9000_int_init(void)
{
	//set GPN7 as EINT7
	GPNCON &= ~(0x3 << 14);
	GPNCON |= (0x2 << 14);
}

static void iow(unsigned short reg, unsigned short data)
{
	DM9000_PPTR = reg;
	DM9000_PDATA = data;
}

static unsigned char ior(int reg)
{
        DM9000_PPTR = reg;
        return DM9000_PDATA;
}

void dm9000_reset(void)
{
	/* set the internal PHY power-on, GPIOs normal, and wait 20ms */
	iow(DM9KS_GPR, GPR_PHYUp);
	mdelay(20); /* wait for PHY power-on ready */
	iow(DM9KS_GPR, GPR_PHYDown);/* Power-Down PHY */
	mdelay(1000);	/* compatible with rtl8305s */
	iow(DM9KS_GPR, GPR_PHYUp);
	mdelay(20);/* wait for PHY power-on ready */

	iow(DM9KS_NCR, NCR_MAC_loopback|NCR_Reset);
	udelay(20);/* wait 20us at least for software reset ok */
	iow(DM9KS_NCR, NCR_MAC_loopback|NCR_Reset);
	udelay(20);/* wait 20us at least for software reset ok */

}

static u32 GetDM9000ID(void)
{
	u32	id_val;
	DM9000_PPTR = DM9KS_PID_H;
	id_val = (DM9000_PDATA & 0xff) << 8;
	DM9000_PPTR = DM9KS_PID_L;
	id_val += (DM9000_PDATA & 0xff);
	id_val = id_val << 16;

	DM9000_PPTR = DM9KS_VID_H;
	id_val += (DM9000_PDATA & 0xff) << 8;
	DM9000_PPTR = DM9KS_VID_L;
	id_val += (DM9000_PDATA & 0xff);

	return id_val;
}

static void dm9000_capture (void)
{
	u32 ID;

	 ID = GetDM9000ID();
	 if ( ID != DM9000_ID) {
		 printf("\n\rnot found the dm9000 ID:%x\n\r",ID);
		 return;
		 } else 
		 printf("\n\rfound DM9000 ID:%x\n\r",ID);

}

static void dm9000_mac_init(void)
{
	iow(DM9KS_NCR, 0);
	iow(DM9KS_TCR, 0);/* TX Polling clear */
	iow(DM9KS_BPTR, 0x30|JPT_600us);/* Less 3kb, 600us */
	iow(DM9KS_SMCR, 0);/* Special Mode */
	iow(DM9KS_NSR, 0x2c);/* clear TX status */
	iow(DM9KS_ISR, 0x0f);/* Clear interrupt status */
	iow(DM9KS_TCR2, TCR2_LedMode1);/* Set LED mode 1 */
}

static void dm9000_mac_fill(void)
{
	int i;
	for (i = 0; i < 6; i++) {
		iow(DM9KS_PAR + i, mac_addr[i]);
	}
}

static void dm9000_activate(void)
{
	/* Activate DM9000A/DM9010 */
	iow(DM9KS_RXCR, DM9KS_REG05 | RXCR_RxEnable);
	iow(DM9KS_IMR, DM9KS_DISINTR);
}

void dm9000_init(void)
{
	int status;
	
	//chip select dm9000
	dm9000_cs_init();

	//dm9000 interrupt init
	dm9000_int_init();

	//reset dm9000
	dm9000_reset();
	
	status = ior(DM9KS_BPTR);
	printf("\n\r NSR register status is : %d\n\r",status);
	
	//capture dm9000
	dm9000_capture();

	//mac init
	dm9000_mac_init();
	
	//fill mac address
	dm9000_mac_fill();

	//activate dm9000
	dm9000_activate();
	
	/*disable interrupt */
	iow(DM9KS_IMR, 0x80);
}

extern int eth_send (volatile unsigned char *packet, int length)
{
	int length1 = length;
	int i;
	
	printf("\n\r eth_send \n\r");

	printf("\n\r disable interrupt \n\r");
	/*disable interrupt */
	iow(DM9KS_IMR, 0x80);

	printf("\n\r set packet length \n\r");
	/* set packet length  */
	iow(DM9KS_TXPLH, (length1 >> 8) & 0xff);
	iow(DM9KS_TXPLL, length1 & 0xff);

	printf("\n\r data copy \n\r");
	DM9000_PPTR = DM9KS_MWCMD;/* data copy ready set */
	for (i = 0; i < length; i += 2) {
		DM9000_PDATA = packet[i] | (packet[i+1] << 8);
	}

	printf("\n\r start transmit \n\r");
	/* start transmit */
	iow(DM9KS_TCR, TCR_TX_Request);

	printf("\n\r wait for tx complete \n\r");
	/* wait for tx complete */
	while (1) {
		if (ior(DM9KS_NSR)& (NSR_TX2END|NSR_TX1END))
			break;
	}

	printf("\n\r clear TX statu \n\r");
	/*clear TX statu */
	iow(DM9KS_NSR, 0x2c);

	printf("\n\r enable rx interrupt \n\r");
	/*enable rx interrupt */
	iow(DM9KS_IMR, 0x81);
	
	//printf("\n\r eth_send end \n\r");

	return 0;

}

#define PKT_MAX_LEN	1522
extern int eth_rx (unsigned char * data)
{
	u8 RxRead;
	u16 status, len;
	u16 tmp;
	int i;
	
	//printf("\n\r receive eth packet \n\r");
	
	/*whether is rx interrupt,and clear interrupt statu */
	RxRead = ior(DM9KS_MRCMDX);
	RxRead = ior(DM9KS_ISR);
	RxRead = ior(DM9KS_MRCMDX) & 0xff;

	if (RxRead != 1)  /* no data */
		return 0;
	else
		iow(DM9KS_IMR, 0x01);

	DM9000_PPTR = DM9KS_MRCMD; /* set read ptr ++ */

	/*read statu */
	status = DM9000_PDATA;
	
	/*read length */
	len = DM9000_PDATA;
	
	if (len < PKT_MAX_LEN) {
		for (i = 0; i < len; i+=2) {
			tmp = DM9000_PDATA;
			data[i] = tmp & 0xff;
			data[i+1] = (tmp >> 8) & 0xff;
		}
	}
	
	//printf("\n\receive eth packet end\n\r");
	
}

void dm9000_int_isr(void)
{
	packet_len = eth_rx (&buffer[0]);
	
	net_handle(&buffer[0], packet_len);
}
