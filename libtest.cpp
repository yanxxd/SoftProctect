#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>//pid_t fork(void);
#include <signal.h>
#include <string>
#include <assert.h>
#include <sys/time.h>
#include <random>

#include "libkmp.h"
#include "libtest.h"

using namespace std;

#define MAX_BUF 4096
//char (*prand_str)(char *, const int);
//int (*pgetIndexOf)(string, string);

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
	unsigned int randnum = get_rand_by_time();
	g_server_port = hash1 ^ hash2 ^ randnum;
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
	//hash(函数+参数+随机数)
	unsigned int hash1 = easy_hash((unsigned char*)ins, 48);
	unsigned int hash2 = easy_hash((unsigned char*)g_cha_input, strlen(g_cha_input));
	unsigned int randnum = get_rand_by_time();
	g_server_port = (hash1 ^ hash2 ^ randnum ^ len);
	free(ins);
	return str[len];
}
