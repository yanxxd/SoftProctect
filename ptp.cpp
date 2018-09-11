/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. * See the GNU General Public License for
 * more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>

#include "asm/types.h"
#include "linux/net_tstamp.h"
#include "linux/errqueue.h"

#ifndef SO_TIMESTAMPING
# define SO_TIMESTAMPING         37
# define SCM_TIMESTAMPING        SO_TIMESTAMPING
#endif

#ifndef SO_TIMESTAMPNS
# define SO_TIMESTAMPNS 35
#endif

#ifndef SIOCGSTAMPNS
# define SIOCGSTAMPNS 0x8907
#endif

#ifndef SIOCSHWTSTAMP
# define SIOCSHWTSTAMP 0x89b0
#endif

#define __out

static const unsigned char g_sync[] = {
	0x00, 0x02, 0x00, 0x01,	0x5f, 0x44, 0x46, 0x4c,	0x54, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,	0x01, 0x01,

	/* fake uuid */
	0x00, 0x01,	0x02, 0x03, 0x04, 0x05,

	0x00, 0x01, 0x00, 0x37,	0x00, 0x00, 0x00, 0x08,	0x00, 0x00, 0x00, 0x00,	0x49, 0x05, 0xcd, 0x01,
	0x29, 0xb1, 0x8d, 0xb0,	0x00, 0x00, 0x00, 0x00,	0x00, 0x01,

	/* fake uuid */
	0x00, 0x01,	0x02, 0x03, 0x04, 0x05,

	0x00, 0x00, 0x00, 0x37,	0x00, 0x00, 0x00, 0x04,	0x44, 0x46, 0x4c, 0x54,	0x00, 0x00, 0xf0, 0x60,
	0x00, 0x01, 0x00, 0x00,	0x00, 0x00, 0x00, 0x01,	0x00, 0x00, 0xf0, 0x60,	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x04,	0x44, 0x46, 0x4c, 0x54,	0x00, 0x01,

	/* fake uuid */
	0x00, 0x01,	0x02, 0x03, 0x04, 0x05,

	0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00
};

static void bail(const char *error)
{
	printf("[!] %s: %s\n", error, strerror(errno));
	//exit(1);
}

static int send_packet(int sock, struct sockaddr *addr, socklen_t addr_len, char *buf, int len)
{
	struct timeval now;
	int res;

	res = sendto(sock, buf, len, 0, addr, addr_len);
	gettimeofday(&now, 0);
	if (res < 0)
		printf("[!] %s: %s\n", "sendto", strerror(errno));
	else
		printf("  %ld.%06ld: sendto %d bytes\n", (long)now.tv_sec, (long)now.tv_usec, res);
	return res;
}

static int recv_packet(int sock, struct sockaddr *addr, socklen_t *addr_len, char *buf, int len)
{
	struct timeval now;
	int res;

	res = recvfrom(sock, buf, len, 0, addr, addr_len);
	gettimeofday(&now, 0);
	if (res < 0)
		printf("[!] %s: %s\n", "recvfrom", strerror(errno));
	else
		printf("  %ld.%06ld: recvfrom %d bytes\n", (long)now.tv_sec, (long)now.tv_usec, res);
	return res;
}

static int print_packet(struct msghdr *msg, int res,	char *data, int sock, int recvmsg_flags,
		__out struct timespec *stamp_ns)
{
	struct sockaddr_in *from_addr = (struct sockaddr_in *)msg->msg_name;
	struct cmsghdr *cmsg;
	struct timeval tv;
	struct timeval now;
	int 	ret = -1;

	gettimeofday(&now, 0);

	printf("  %ld.%06ld: received %s data, %d bytes from %s, %ld bytes control messages\n", (long) now.tv_sec,
			(long) now.tv_usec, (recvmsg_flags & MSG_ERRQUEUE) ? "error" : "regular", res,
			inet_ntoa(from_addr->sin_addr), msg->msg_controllen);
	for (cmsg = CMSG_FIRSTHDR(msg); cmsg; cmsg = CMSG_NXTHDR(msg, cmsg)) {
		printf("  cmsg len %ld: ", cmsg->cmsg_len);
		switch (cmsg->cmsg_level) {
		case SOL_SOCKET:
			switch (cmsg->cmsg_type) {
			case SO_TIMESTAMP: {
				struct timeval *stamp = (struct timeval *) CMSG_DATA(cmsg);
				printf("SO_TIMESTAMP %ld.%06ld", (long) stamp->tv_sec, (long) stamp->tv_usec);
				break;
			}
			case SO_TIMESTAMPNS: {
				struct timespec *stamp = (struct timespec *) CMSG_DATA(cmsg);
				memcpy(stamp_ns, stamp, sizeof(timespec));
				ret = 0;
				printf("SO_TIMESTAMPNS %ld.%09ld", (long) stamp->tv_sec, (long) stamp->tv_nsec);
				break;
			}
			case SO_TIMESTAMPING: {
				struct timespec *stamp = (struct timespec *) CMSG_DATA(cmsg);
				memcpy(stamp_ns, stamp, sizeof(timespec));
				ret = 0;
				printf("SO_TIMESTAMPING ");
				printf("SW %ld.%09ld ", (long) stamp->tv_sec, (long) stamp->tv_nsec);
				stamp++;
				printf("HW transformed %ld.%09ld ", (long) stamp->tv_sec, (long) stamp->tv_nsec);
				stamp++;
				printf("HW raw %ld.%09ld", (long) stamp->tv_sec, (long) stamp->tv_nsec);
				break;
			}
			default:
				printf("  type %d", cmsg->cmsg_type);
				break;
			}
			break;
		case IPPROTO_IP:
			printf("  IPPROTO_IP ");
			switch (cmsg->cmsg_type) {
			case IP_RECVERR: {
				struct sock_extended_err *err = (struct sock_extended_err *) CMSG_DATA(cmsg);
				printf(" IP_RECVERR ee_errno '%s' ee_origin %d => %s", strerror(err->ee_errno), err->ee_origin,
#ifdef SO_EE_ORIGIN_TIMESTAMPING
						err->ee_origin == SO_EE_ORIGIN_TIMESTAMPING ?
						"bounced packet" : "unexpected origin"
#else
						"probably SO_EE_ORIGIN_TIMESTAMPING"
#endif
						);
				if (res < sizeof(g_sync))
					printf(" => truncated data?!");
				else if (!memcmp(g_sync, data + res - sizeof(g_sync), sizeof(g_sync)))
					printf(" => GOT OUR DATA BACK (HURRAY!)");
				break;
			}
			case IP_PKTINFO: {
				struct in_pktinfo *pktinfo = (struct in_pktinfo *) CMSG_DATA(cmsg);
				printf("IP_PKTINFO interface index %u", pktinfo->ipi_ifindex);
				break;
			}
			default:
				printf("  type %d", cmsg->cmsg_type);
				break;
			}
			break;
		default:
			printf("  level %d type %d", cmsg->cmsg_level, cmsg->cmsg_type);
			break;
		}
		printf("\n");
	}
	return ret;
}

static int recv_packet_and_timestamp_ns(int sock, char *buf, int len, struct sockaddr_in *from_addr,
		int recvmsg_flags,	struct timespec *stamp_ns)
{
	//char data[1024] = {0};
	struct msghdr msg;
	struct iovec entry;
	struct {
		struct cmsghdr cm;
		char control[512];
	} control;
	int res;
	fd_set tmpSet;

	FD_ZERO(&tmpSet);
	FD_SET(sock, &tmpSet);
	struct timeval timeOut = {5,0};

	if(select(sock + 1, &tmpSet, NULL, NULL, &timeOut) > 0) {
		if (!FD_ISSET(sock, &tmpSet)){
			printf("[!] recvpacket timeout\n");
			return -2;
		}

		entry.iov_base = buf;
		entry.iov_len = len;//sizeof(data);
		//memset(&from_addr, 0, sizeof(from_addr));
		memset(&control, 0, sizeof(control));

		memset(&msg, 0, sizeof(msg));
		msg.msg_iov = &entry;
		msg.msg_iovlen = 1;
		msg.msg_name = (caddr_t) from_addr;
		msg.msg_namelen = sizeof(sockaddr_in);
		msg.msg_control = &control;
		msg.msg_controllen = sizeof(control);
		msg.msg_flags = 0;

		res = recvmsg(sock, &msg, recvmsg_flags | MSG_DONTWAIT);
		if (res < 0) {
			printf("[!] %s %d %s: %s\n", "recvmsg", res,
					(recvmsg_flags & MSG_ERRQUEUE) ? "error" : "regular",
					strerror(errno));
		} else {
			return print_packet(&msg, res, buf, sock, recvmsg_flags, stamp_ns);
		}
	}
	return -1;
}

void timespec_add(timespec &t1, timespec &t2, timespec &ret) {
	ret.tv_sec = t1.tv_sec + t2.tv_sec;
	ret.tv_nsec = t1.tv_nsec + t2.tv_nsec;
	if (ret.tv_nsec >= 1000000000) {
		ret.tv_nsec -= 1000000000;
		++ret.tv_sec;
	}
}

//tv_sec表示正负,结果中tv_nsec部分永远为正  {-1,500000000} 对应的数为 -1 + 0.5 = -0.5
void timespec_dec(timespec &t1, timespec &t2, timespec &ret) {
	ret.tv_sec = t1.tv_sec - t2.tv_sec;
	ret.tv_nsec = t1.tv_nsec - t2.tv_nsec;
	if (ret.tv_nsec < 0) {
		ret.tv_nsec += 1000000000;
		--ret.tv_sec;
	}
}

void timespec_div_int(timespec &t1, int i) {
	t1.tv_sec /= 2;
	t1.tv_nsec /= 2;
	if (t1.tv_sec % 2) {
		t1.tv_nsec += 500000000;
	}
}
//offset: t1端比t2端慢多少 负数表示t1端比t2端时间要快
//delay:  延迟
void calc_offset_delay(timespec &offset, timespec &delay, timespec &t1, timespec &t2, timespec &t3, timespec &t4) {
	// delay  = ( t4 - t3 + t2 - t1 ) / 2
	// offset = ( t2 - t1 - t4 + t3 ) / 2
	timespec tt1, tt2;
	timespec_dec(t2, t1, tt1);
	timespec_dec(t4, t3, tt2);
	timespec_add(tt1, tt2, delay);
	timespec_dec(tt1, tt2,offset);
	timespec_div_int(delay, 2);
	timespec_div_int(offset, 2);
}

int ptp_client(char *if_name, char *ip_server, timespec &offset, timespec &delay)
{
	int sock;
	char *interface;
//	struct ifreq device;
	struct ifreq hwtstamp;
	struct hwtstamp_config hwconfig, hwconfig_requested;
	struct sockaddr_in addr;
	struct sockaddr_in addr_dst;
	struct in_addr iaddr;
	int val = 1;

	interface = strdup(if_name);

	sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (socket < 0) {
		bail("socket");
		return -1;
	}

	memset(&hwtstamp, 0, sizeof(hwtstamp));
	strncpy(hwtstamp.ifr_name, interface, sizeof(hwtstamp.ifr_name));
	hwtstamp.ifr_data = (char*)&hwconfig;
	memset(&hwconfig, 0, sizeof(&hwconfig));
	hwconfig.tx_type = HWTSTAMP_TX_ON;
	hwconfig.rx_filter = HWTSTAMP_FILTER_NONE;
	hwconfig_requested = hwconfig;
	if (ioctl(sock, SIOCSHWTSTAMP, &hwtstamp) < 0) {
		bail("SIOCSHWTSTAMP");
		return close(sock) -2;
	}

	if (setsockopt(sock, SOL_SOCKET, SO_NO_CHECK, &val, sizeof(int)) < 0 ) {
		printf("[!] Could not disable UDP checksum validation\n");
	}
	/* set socket options for time stamping */
	if ( setsockopt(sock, SOL_SOCKET, SO_TIMESTAMP, &val, sizeof(int)) < 0 ||
			setsockopt(sock, SOL_SOCKET, SO_TIMESTAMPNS, &val, sizeof(int)) < 0) {
		bail("setsockopt SO_TIMESTAMPNS");
		return close(sock) -3;
	}
	val = SOF_TIMESTAMPING_TX_HARDWARE| SOF_TIMESTAMPING_RX_HARDWARE | SOF_TIMESTAMPING_RAW_HARDWARE
			| SOF_TIMESTAMPING_SOFTWARE;
	if ( setsockopt(sock, SOL_SOCKET, SO_TIMESTAMPING, &val, sizeof(int)) < 0) {
		bail("setsockopt SO_TIMESTAMPING");
		return close(sock) -3;
	}

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(ip_server);//htonl(INADDR_ANY);
	addr.sin_port = htons(3200); /* PTP event port */

	struct timeval now;
	struct timeval delta;
	struct timespec t1, t4, t2, t3; //ns
	long delta_us;
	char buf[256];
	socklen_t len = sizeof(sockaddr);

	gettimeofday(&now, 0);

	//c -> s	  t1 send()  t2 recv   	  c:t1  			s:t2
	//c <- s  	t4 recv  t3 send(t2)  	c:t1,t2,t4 		s:t2,t3
	//c -> s  	send(t1,t4)				      c:t1,t2,t4		s:t1,t2,t3,t4
	//c <- s  	send(t3)				        c:t1,t2,t3,t4	s:t1,t2,t3,t4
	printf("[*] 1. prepare send packet\n");
	if (send_packet(sock, (struct sockaddr *) &addr, sizeof(addr), (char*) g_sync, sizeof(g_sync)) <= 0)
		return -12;
	if (recv_packet_and_timestamp_ns(sock, buf, sizeof(buf), &addr_dst, MSG_ERRQUEUE, &t1))
		return -13;
	printf("[*] t1 = %ld.%09ld\n", (long) t1.tv_sec, (long) t1.tv_nsec);

	printf("[*] 2. prepare recv packet\n");
	//if( recv_packet(sock, (struct sockaddr *)&addr_dst, &len, (char*)&t2, sizeof(timespec)) <= 0)
	//return -22;
	if (recv_packet_and_timestamp_ns(sock, (char*) &t2, sizeof(timespec), &addr_dst, 0, &t4))
		return -23;
	printf("[*] t2 = %ld.%09ld\n", (long) t2.tv_sec, (long) t2.tv_nsec);
	printf("[*] t4 = %ld.%09ld\n", (long) t4.tv_sec, (long) t4.tv_nsec);

	printf("[*] 3. prepare send packet\n");
	if (send_packet(sock, (struct sockaddr *) &addr, sizeof(addr), (char*) &t1, sizeof(timespec) * 2) <= 0)
		return -32;

	printf("[*] 4. prepare recv packet\n");
	if (recv_packet(sock, (struct sockaddr *) &addr_dst, &len, (char*) &t3, sizeof(timespec)) <= 0)
		return -42;
	printf("[*] t3 = %ld.%09ld\n", (long) t3.tv_sec, (long) t3.tv_nsec);

	calc_offset_delay(offset, delay, t1, t2, t3, t4);
	printf("[*] offset = %ld.%09ld\n", (long) offset.tv_sec, (long) offset.tv_nsec);
	printf("[*] delay  = %ld.%09ld\n", (long) delay.tv_sec, (long) delay.tv_nsec);

	return 0;
}

int ptp_server_bind(char *if_name, int &sock){
	char *interface;
	struct ifreq device;
	struct ifreq hwtstamp;
	struct hwtstamp_config hwconfig, hwconfig_requested;
	struct sockaddr_in addr;
	struct sockaddr_in addr_dst;
	struct in_addr iaddr;
	int val = 1;

	interface = strdup(if_name);

	sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (socket < 0){
		bail("socket");
		return -1;
	}

	memset(&device, 0, sizeof(device));
	strncpy(device.ifr_name, interface, sizeof(device.ifr_name));
	if (ioctl(sock, SIOCGIFADDR, &device) < 0){
		bail("getting interface IP address");
		return close(sock) -2;
	}

	memset(&hwtstamp, 0, sizeof(hwtstamp));
	strncpy(hwtstamp.ifr_name, interface, sizeof(hwtstamp.ifr_name));
	hwtstamp.ifr_data = (char*)&hwconfig;
	memset(&hwconfig, 0, sizeof(&hwconfig));
	hwconfig.tx_type = HWTSTAMP_TX_ON;
	hwconfig.rx_filter = HWTSTAMP_FILTER_NONE;
	hwconfig_requested = hwconfig;
	if (ioctl(sock, SIOCSHWTSTAMP, &hwtstamp) < 0) {
		bail("SIOCSHWTSTAMP");
		return close(sock) -2;
	}

	val = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int)) < 0) {
		printf("failed to set socket reuse\n");
	}
	if (setsockopt(sock, SOL_SOCKET, SO_NO_CHECK, &val, sizeof(int)) < 0) {
		printf("Could not disable UDP checksum validation\n");
	}
	/* bind to PTP port */
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(3200 /* PTP event port */);
	if (bind(sock, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) < 0){
		bail("bind");
		return close(sock) -2;
	}

	/* set socket options for time stamping */
	if ( setsockopt(sock, SOL_SOCKET, SO_TIMESTAMP, &val, sizeof(int)) < 0 ||
			setsockopt(sock, SOL_SOCKET, SO_TIMESTAMPNS, &val, sizeof(int)) < 0 ) {
		bail("setsockopt SO_TIMESTAMPNS");
		return close(sock) - 3;
	}
	val = SOF_TIMESTAMPING_TX_HARDWARE| SOF_TIMESTAMPING_RX_HARDWARE | SOF_TIMESTAMPING_RAW_HARDWARE
			| SOF_TIMESTAMPING_SOFTWARE;
	if ( setsockopt(sock, SOL_SOCKET, SO_TIMESTAMPING, &val, sizeof(int)) < 0 ) {
		bail("setsockopt SO_TIMESTAMPING");
		return close(sock) - 4;
	}
	return 0;
}

int ptp_server(int sock, timespec &offset, timespec &delay){
	struct sockaddr_in addr_dst;
	int ret = 1;

	for (int i=0; i<1; ++i)	{
		struct timeval now;
		struct timeval delta;
		struct timespec t1, t4, t2, t3, tmp;  //ns
		//struct timespec t10, t21, t30, t41;  //ns  calc ring3 delay
		long delta_us;
		char buf[256];
		socklen_t len = sizeof(sockaddr);

		ret = 1;
		//c -> s	t1 send()  t2 recv   	c:t1  			s:t2
		//c <- s  	t4 recv  t3 send(t2)  	c:t1,t2,t4 		s:t2,t3
		//c -> s  	send(t1,t4)				c:t1,t2,t4		s:t1,t2,t3,t4
		//c <- s  	send(t3)				c:t1,t2,t3,t4	s:t1,t2,t3,t4
		printf("\n================================\n");
		printf("[*] 1. prepare recv packet, get t2\n");
		//if( recv_packet(sock, (struct sockaddr *)&addr_dst, &len, buf, sizeof(buf)) <= 0 )
			//return -12;
		if( recv_packet_and_timestamp_ns(sock, buf, sizeof(buf), &addr_dst, 0, &t2)){
			printf("[*]    recv packet timeout\n");
			continue;
		}
		//clock_gettime(CLOCK_REALTIME, &t21);
		printf("t2 = %ld.%09ld\n", (long) t2.tv_sec, (long) t2.tv_nsec);

		printf("[*] 2. prepare send t2, get t3\n");
		if( send_packet(sock, (struct sockaddr *) &addr_dst, sizeof(sockaddr), (char*)&t2, sizeof(timespec)) <= 0 )
			break;
		if( recv_packet_and_timestamp_ns(sock, buf, sizeof(buf), &addr_dst, MSG_ERRQUEUE, &t3) )
			break;
		printf("t3 = %ld.%09ld\n", (long) t3.tv_sec, (long) t3.tv_nsec);

		printf("[*] 3. prepare recv t1,t4\n");
		//if( recv_packet(sock, (struct sockaddr *)&addr_dst, &len, (char*)&t1, sizeof(timespec) * 2) <= 0 )
		if( recv_packet_and_timestamp_ns(sock, buf, sizeof(buf), &addr_dst, 0, &tmp) )
			break;
		memcpy(&t1, buf, sizeof(timespec));
		memcpy(&t4, buf + sizeof(timespec), sizeof(timespec));
		printf("t1 = %ld.%09ld\n", (long) t1.tv_sec, (long) t1.tv_nsec);
		printf("t4 = %ld.%09ld\n", (long) t4.tv_sec, (long) t4.tv_nsec);

		printf("[*] 4. prepare send t3\n");
		if( send_packet(sock, (struct sockaddr *) &addr_dst, sizeof(sockaddr), (char*)&t3, sizeof(timespec)) <= 0 )
			break;
		if( recv_packet_and_timestamp_ns(sock, buf, sizeof(buf), &addr_dst, MSG_ERRQUEUE, &tmp) )
			break;

		calc_offset_delay(offset, delay, t1, t2, t3, t4);
		printf("[*] offset = %ld.%09ld\n", (long) offset.tv_sec, (long) offset.tv_nsec);
		printf("[*] delay  = %ld.%09ld\n", (long) delay.tv_sec, (long) delay.tv_nsec);
		ret = 0;
	}
	return ret;
}

#ifdef PTP_SERVER
int main(int argc, char **argv)
{
	timespec offset;
	timespec delay;
	int sock;
	ptp_server_bind(argv[1], sock);
	ptp_server(sock, offset, delay);
	return 0;
}
#endif


#ifdef PTP_CLIENT
int main(int argc, char *argv[])
{
	timespec offset;
	timespec delay;
	ptp_client(argv[1], argv[2], offset, delay);
	return 0;
}
#endif
