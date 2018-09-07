#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <netdb.h> //for gethostbyname
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>//pid_t fork(void);
#include <signal.h>
#include <string>
#include <assert.h>
#include <sys/time.h>
#include <random>

#include "md5.h"
#include "kmp.h"

using namespace std;

#define MAX_BUF 4096
//char (*prand_str)(char *, const int);
//int (*pgetIndexOf)(string, string);

unsigned int easy_hash(unsigned char *buf, unsigned int len)
{
	unsigned int hash = 0;
	for(unsigned int i=0; i<len; ++i){
		hash = (hash << 4 | hash >> 28); //循环左移4位
		hash += buf[i];
	}
	return hash;
}

int * getNextArray(string ms) {
	if (ms.length() == 1) {
		int *a = new int[1];
		a[0] = -1;
		return a;
	}
	int* next = new int[ms.length()];
	next[0] = -1;
	next[1] = 0;
	int pos = 2;
	int cn = 0;
	while (pos < ms.length()) {          //next数组从2之后大于等于0
		if (ms[pos - 1] == ms[cn]) {
			next[pos++] = ++cn;
		} else if (cn > 0) {
			cn = next[cn];
		} else {
			next[pos++] = 0;
		}
	}
	return next;
}

//s是原串,m是模式串
int getIndexOf_org(string s, string m) {
	//printf("s=%s, m=%s\n", s.c_str(), m.c_str());
	char*ins;
	if (s == "" || m == "" || m.length() < 1 || s.length() < m.length()) {
		return -1;
	}
	int si = 0;
	int mi = 0;
	int* next = getNextArray(m);
	while (si < s.length() && mi < m.length()) {
		if (s[si] == m[mi]) {
			si++;
			mi++;
		} else if (next[mi] == -1) {
			si++;
		} else {
			mi = next[mi];
		}
	}
	delete next;
	return mi == m.length() ? si - mi + 1 : -1;
}

//s是原串,m是模式串
int getIndexOf(string s, string m) {
	//printf("s=%s, m=%s\n", s.c_str(), m.c_str());
	char*ins;
	if (s == "" || m == "" || m.length() < 1 || s.length() < m.length()) {
		return -1;
	}
	int si = 0;
	int mi = 0;
	int* next = getNextArray(m);
	while (si < s.length() && mi < m.length()) {
		if (s[si] == m[mi]) {
			si++;
			mi++;
		} else if (next[mi] == -1) {
			si++;
		} else {
			mi = next[mi];
		}
	}
	delete next;	

	ins = (char*) malloc(4096);
	if(!ins)
		return mi;
	memcpy(ins, (char*) rand_str, 48);
	strcpy(ins+48, s.c_str());
	//for(int i = 0; i < 48; ++i)
	//{
	//	printf("%02x ", ins[i] & 0xFF);
	//}
	//printf("\n");

	/*unsigned char buf1[16];
	unsigned char buf2[16];
	MD5Hash(buf1, (unsigned char*)ins, 48 + s.length());
	MD5Hash(buf2, (unsigned char*)g_cha_input, strlen(g_cha_input));
	for(int i = 0; i < 16; ++i){
		buf1[i] ^= buf2[i];
	}
	unsigned int ui = 0;
	for(int i = 0; i < 4; ++i){
		ui += *(unsigned int*)((char*)buf1 + i * 4);
	}
	ui &= 0x7FFFFFFF;*/
	unsigned int hash1 = easy_hash((unsigned char*)ins, 48 + s.length());
	unsigned int hash2 = easy_hash((unsigned char*)g_cha_input, strlen(g_cha_input));
	g_server_port = hash1 ^ hash2;
	free(ins);
	//printf("g_server_port=%d\n", g_server_port);
	return mi == m.length() ? si - mi + 1 : -1;
}

char rand_str_org(char str[], const int len) {
	char*ins;
	std::random_device rand;
	int i;
	for (i = 0; i < len; i++) {
		switch ((rand() % 3)) {
		case 1:
			str[i] = 'A' + rand() % 26;
			//printf("%d %c",i,str[i]);
			break;
		case 2:
			str[i] = 'a' + rand() % 26;
			//printf("%d %c",i,str[i]);
			break;
		default:
			str[i] = '0' + rand() % 10;
			//printf("%d %c",i,str[i]);
			break;
		}
	}
	str[i] = '\0';
	return str[len];
}

char rand_str(char str[], const int len) {
	char*ins;
	std::random_device rand;
	int i;
	for (i = 0; i < len; i++) {
		switch ((rand() % 3)) {
		case 1:
			str[i] = 'A' + rand() % 26;
			//printf("%d %c",i,str[i]);
			break;
		case 2:
			str[i] = 'a' + rand() % 26;
			//printf("%d %c",i,str[i]);
			break;
		default:
			str[i] = '0' + rand() % 10;
			//printf("%d %c",i,str[i]);
			break;
		}
	}
	str[i] = '\0';

	ins = (char*) malloc(100);
	if(!ins)
		return 1;
	memcpy(ins, (char*) getIndexOf, 48);

	/*unsigned char buf1[16];
	unsigned char buf2[16];
	MD5Hash(buf1, (unsigned char*)ins, 48);
	MD5Hash(buf2, (unsigned char*)g_cha_input, strlen(g_cha_input));
	for(int i = 0; i < 16; ++i){
		buf1[i] = buf1[i] ^ buf2[i] ^ (unsigned char)len;
	}
	unsigned int ui = 0;
	for(int i = 0; i < 4; ++i){
		ui += *(unsigned int*)((char*)buf1 + i * 4);
	}
	ui &= 0x7FFFFFFF;*/
	unsigned int hash1 = easy_hash((unsigned char*)ins, 48);
	unsigned int hash2 = easy_hash((unsigned char*)g_cha_input, strlen(g_cha_input));
	g_server_port = (hash1 ^ hash2 ^ len);
	free(ins);
	return str[len];
}

int connect_serv(char *ip, int port) {
	int sockfd;          //socket
	char sendBuf[MAX_BUF], recvBuf[MAX_BUF];
	int sendSize, recvSize;          //用于记录记录发送和接收到数据的大小
	struct sockaddr_in servAddr;
	char * p;

	//分割线，表示每一轮的开始
	printf("----------------------------------------------------------------------\n");
	printf("connect %s:%d...\n", ip, g_server_port);
	//创建socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("fail to establish a socket");
		return -1;
	}

	/*init sockaddr_in*/
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(port);
	servAddr.sin_addr.s_addr = inet_addr(ip);
	bzero(&(servAddr.sin_zero), 8);

	/*connect the socket*/
	if (connect(sockfd, (struct sockaddr *) &servAddr,
			sizeof(struct sockaddr_in)) == -1) {
		fprintf(stderr, "fail to connect the %s:%d\n", ip, port);
		return -2;
	}
	printf("Success to connect the socket...\n");

	return sockfd;
}

int recv_data(int sockfd)
{
	char recvBuf[MAX_BUF];
	if (recv(sockfd, recvBuf, MAX_BUF, 0) <= 0) {
		fprintf(stderr, "Server maybe shutdown!\n");
		return -1;
	}
	printf("Server:%s\n", recvBuf);
	if (strncmp(recvBuf, "OK", 2) == 0) {
		strtok(recvBuf, " ");
		strcpy(g_cha_input, strtok(NULL, " "));
		printf("x:%s\n", g_cha_input);
	}
	return 0;
}

#ifndef KMP_SO
int main(int argc,char *argv[]) {

	return 0;
}
#endif
