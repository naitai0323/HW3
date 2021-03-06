#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <pcap.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <string.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>

#define MAC_ADDRSTRLEN 2*6+5+1
void dump_ethernet(u_int32_t length, const u_char *content);
void dump_ip(u_int32_t length, const u_char *content);
void dump_tcp(u_int32_t length, const u_char *content);
void dump_udp(u_int32_t length, const u_char *content);
void pcap_callback(u_char *arg, const struct pcap_pkthdr *header, const u_char *content);
char *mac_ntoa(u_char *d);
char *ip_ntoa(void *i);
char *ip_ttoa(u_int8_t flag);
char *ip_ftoa(u_int16_t flag);
char *tcp_ftoa(u_int8_t flag);

int specify = 0;
int decide;
char flag[4] = "";

int main(int argc, const char * argv[]) {
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *handle = NULL;
    char *device = "ens33";
    bpf_u_int32 net, mask;
    struct bpf_program fcode;
    char filename[100]="";
    strcpy(filename,argv[2]);
    handle = pcap_open_offline(filename, errbuf);
    if(!handle) {
        fprintf(stderr, "pcap_open_offline(): %s\n", errbuf);
        exit(1);
    }
    //start capture
    int in = 0;
    pcap_loop(handle,65535, pcap_callback, (u_char*)&in);
    //free
    pcap_close(handle);
    return 0;
}


char *mac_ntoa(u_char *d) {
    static char str[MAC_ADDRSTRLEN];

    snprintf(str, sizeof(str), "%02x:%02x:%02x:%02x:%02x:%02x", d[0], d[1], d[2], d[3], d[4], d[5]);

    return str;
}//end mac_ntoa

char *ip_ntoa(void *i) {
    static char str[INET_ADDRSTRLEN];

    inet_ntop(AF_INET, i, str, sizeof(str));

    return str;
}//end ip_ntoa

void pcap_callback(u_char *arg, const struct pcap_pkthdr *header, const u_char *content) {

    struct ip *ip = (struct ip *)(content + ETHER_HDR_LEN);
    static int d = 0;
    u_char protocol = ip->ip_p;
    printf("No. %d\n", ++d);
    //print header
    printf("Recieved time: %s", ctime((const time_t *)&header->ts.tv_sec));
    printf("Length: %d bytes\n", header->len);
    printf("Capture length: %d bytes\n", header->caplen);

    //dump ethernet
    dump_ethernet(header->caplen, content);
    printf("\n");
}//end pcap_callback

void dump_ethernet(u_int32_t length, const u_char *content) {
    struct ether_header *ethernet = (struct ether_header *)content;
    char dst_mac_addr[MAC_ADDRSTRLEN] = {};
    char src_mac_addr[MAC_ADDRSTRLEN] = {};
    u_int16_t type;
    //copy header
    strncpy(dst_mac_addr, mac_ntoa(ethernet->ether_dhost), sizeof(dst_mac_addr));
    strncpy(src_mac_addr, mac_ntoa(ethernet->ether_shost), sizeof(src_mac_addr));
    type = ntohs(ethernet->ether_type);

    //print
    if(type <= 1500)
        printf("IEEE 802.3 Ethernet Frame:\n");
    else
        printf("Ethernet Frame:\n");
    printf(" - Destination MAC Address: %s\n", dst_mac_addr);
    printf(" - Source MAC Address: %s\n", src_mac_addr);
    if (type < 1500)
        printf(" - Length: %u\n", type);
    else
        printf(" - Ethernet Type: 0x%04x\n", type);
    switch (type) {
        case ETHERTYPE_ARP:
            printf("Next is ARP\n");
            break;

        case ETHERTYPE_IP:
            dump_ip(length, content);
            break;

        case ETHERTYPE_REVARP:
            printf("Next is RARP\n");
            break;

        case ETHERTYPE_IPV6:
            printf("Next is IPv6\n");
            break;

        default:
            printf("Next is %#06x", type);
            break;
    }//end switch
}//end dump_ethernet


void dump_ip(u_int32_t length, const u_char *content) {
    struct ip *ip = (struct ip *)(content + ETHER_HDR_LEN);
    u_char protocol = ip->ip_p;
    printf("Protocol: IP\n");
    printf(" - Source IP Address: %s\n",  ip_ntoa(&ip->ip_src));
    printf(" - Destination IP Address: %s\n", ip_ntoa(&ip->ip_dst));
    switch (protocol) {
        case IPPROTO_UDP:
            dump_udp(length, content);
            break;

        case IPPROTO_TCP:
            dump_tcp(length, content);
            break;

        case IPPROTO_ICMP:
            printf("Next is ICMP\n");
            break;

        default:
            printf("Next is %d\n", protocol);
            break;
    }//end switch
}//end dump_ip

void dump_tcp(u_int32_t length, const u_char *content) {
    struct ip *ip = (struct ip *)(content + ETHER_HDR_LEN);
    struct tcphdr *tcp = (struct tcphdr *)(content + ETHER_HDR_LEN + (ip->ip_hl << 2));
    u_int16_t source_port = ntohs(tcp->th_sport);
    u_int16_t destination_port = ntohs(tcp->th_dport);
    printf("Protocol: TCP\n");
    printf(" - Source Port: %u\n", source_port);
    printf(" - Destination Port: %u\n", destination_port);
    
    printf("\n");
}

void dump_udp(u_int32_t length, const u_char *content) {
    struct ip *ip = (struct ip *)(content + ETHER_HDR_LEN);
    struct udphdr *udp = (struct udphdr *)(content + ETHER_HDR_LEN + (ip->ip_hl << 2));
    u_int16_t source_port = ntohs(udp->uh_sport);
    u_int16_t destination_port = ntohs(udp->uh_dport);
    printf("Protocol: UDP\n");
    printf(" - Source Port: %u\n", source_port);
    printf(" - Destination Port: %u\n", destination_port);
    printf("\n");
}//end dump_upp
