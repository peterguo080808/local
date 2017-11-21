#include "arp.h"

#define HON(n) (((n & 0xff) << 8 ) | ((n & 0xff00) >> 8))

/*
 * send arp request packet
 */
 
 void arp_request(void)
 {
	 /*Constructs an arp request packet */
	memcpy(arpbuf.ethhdr.d_mac, host_mac_addr, 6);
	memcpy(arpbuf.ethhdr.s_mac, mac_addr, 6);
	arpbuf.ethhdr.type = HON(0x0806);
	
	arpbuf.hwtype = HON(1);
	arpbuf.protocol = HON(0x0800);
	arpbuf.hwlen = 6;
	arpbuf.protolen = 4;
	arpbuf.opcode = HON(1);
	memcpy(arpbuf.smac, mac_addr, 6);
	memcpy(arpbuf.sipaddr, ip_addr, 4);
	//arpbuf.dmac[6]
	memcpy(arpbuf.dipaddr, host_ip_addr, 4);
	
	packet_len = 14 + 28;

	 /*call eth_send function */
	eth_send(buffer, packet_len);
 }


 /*
  * Parse arp response packet, extract MAC
  */
unsigned char arp_process(void)
{
	int i;
	
	if (packet_len < 28)
		return 0;
	
	memcpy(host_ip_addr, arpbuf.sipaddr, 4);
	printf("\n\r host ip is : ");
	for (i = 0; i < 4; i++) {
		printf("%03d", host_ip_addr[i]);
		}
	printf("\n\r");
	
	memcpy(host_mac_addr, arpbuf.smac, 6);
	printf("\n\r host mac is : ");
	for (i = 0; i < 6; i++) {
		printf("%03d", host_ip_addr[i]);
		}
	printf("\n\r");
	
}
