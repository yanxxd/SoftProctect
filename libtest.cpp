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
	unsigned int randnum = get_rand_by_time(0);
	g_checksum = hash1 ^ hash2 ^ randnum;
	free(ins);
	//printf("g_checksum=%d\n", g_checksum);
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
	unsigned int randnum = get_rand_by_time(0);
	g_checksum = (hash1 ^ hash2 ^ randnum ^ len);
	free(ins);
	return str[len];
}

//#define KMP_ENABLE
#ifndef KMP_DISABLE
extern "C" void BubbleSort(unsigned short *a, int n) {

	struct timeval start1, end1, start2, end2, start3, end3, start4, end4;
	/******************** 1. 保存参数 ********************/
	gettimeofday(&start1, NULL);
	unsigned int uiHash = 0;
	char tmp[1024];
	*(unsigned int*) tmp = g_checksum;
	strcpy(tmp + 4, "BubbleSort");
	*(int*) (tmp + 14) = n;
	memcpy(tmp + 18, a, n * 2);
	gettimeofday(&end1, NULL);
	/******************** 保存参数 end ********************/

	/******************** 2. 排序算法代码 ********************/
	gettimeofday(&start2, NULL);
	int i, j;
	for (i = n - 1; i > 0; --i)          //从n-1循环的到0，也是n次
		for (j = 0; j < i; j++)
			if (a[j] < a[j + 1]) {
				unsigned int temp = a[j]; uiHash ^= temp;
				a[j] = a[j + 1];
				a[j + 1] = temp;
			}
	gettimeofday(&end2, NULL);
	/******************** 排序算法代码 end********************/

	/******************** 3. 与服务器通信 ********************/
	gettimeofday(&start3, NULL);
	int is_check = 1;
	int delta;
	if (!g_is_server) {
		//1. 把参数发给服务端
		if (SSL_write(g_ssl, tmp, 18 + n * 2) <= 0) {
			fprintf(stderr, "kmp_BubbleSort:fail to send param.");
			return;
		}
		//2. 接收参数x
		if (recv_seed_delta(g_ssl, &is_check, &delta)) {
			fprintf(stderr, "kmp_BubbleSort:fail to recv x.\n");
			return;
		}
	}
	gettimeofday(&end3, NULL);
	/******************** 与服务器通信 end ********************/

	/******************** 4. 校验 end ********************/
	gettimeofday(&start4, NULL);
	//3. 是否校验这个函数
	if (is_check) {
		memcpy(tmp, (char*) BubbleSort, 200);
		*(int*)(tmp + 200) = n;
		memcpy(tmp + 204, a, n * 2);
		//dbg_printf("\nBubbleSort(%d..., %d)", a[0], n);
		unsigned int hash1 = easy_hash((unsigned char*) tmp, 204 + n * 2);
		unsigned int hash2 = easy_hash((unsigned char*) g_cha_input, strlen(g_cha_input));
		unsigned int randnum = get_rand_by_time(0);
		g_checksum = (uiHash ^ hash1 ^ hash2 ^ randnum);
		//dbg_fprintf(stdout, "\nkmp_BubbleSort:%x ^ %x ^ %x ^ %x = %x.", uiHash, hash1, hash2, randnum, g_checksum);
		fprintf(stdout, "\n\nhash结果:%08X", g_checksum);
	}
	gettimeofday(&end4, NULL);
	/******************** 校验 end ********************/

	struct timeval r1, r2, r3, r4;
	timeval_us_sub(&start1, &end1, &r1);
	timeval_us_sub(&start2, &end2, &r2);
	timeval_us_sub(&start3, &end3, &r3);
	timeval_us_sub(&start4, &end4, &r4);
	dbg_printf("\n保存参数时间：%lds %ldus\n", r1.tv_sec, r1.tv_usec);
	dbg_printf("\n排序算法时间：%lds %ldus\n", r2.tv_sec, r2.tv_usec);
	dbg_printf("\n与服务器通信时间：%lds %ldus\n", r3.tv_sec, r3.tv_usec);
	dbg_printf("\n校验时间：%lds %ldus\n", r4.tv_sec, r4.tv_usec);
	timeval_us_add(&r1, &r2, &r1);
	timeval_us_add(&r1, &r4, &r4);
	dbg_printf("\n保护后BubbleSort执行时间：%lds %ldus\n", r4.tv_sec, r4.tv_usec);
	printf("\n");
	/******************** kmp end ********************/
}
#else
extern "C" void BubbleSort(unsigned short *a, int n) {

	struct timeval start1, end1, r1;
	gettimeofday(&start1, NULL);

	int i, j;
	for (i = n - 1; i > 0; --i)          //从n-1循环的到0，也是n次
		for (j = 0; j < i; j++)
			if (a[j] > a[j + 1]) {
				unsigned int temp = a[j];
				a[j] = a[j + 1];
				a[j + 1] = temp;
			}

	gettimeofday(&end1, NULL);
	timeval_us_sub(&start1, &end1, &r1);
	dbg_printf("\n原BubbleSort函数执行时间：%lds %ldus\n", r1.tv_sec, r1.tv_usec);
	printf("\n");
}
#endif

extern "C" void kmp_Sort() {
	srand(time(0));
	//int n = rand() % 50 + 50;
	//unsigned short *a = new unsigned short[n];
	int i;
	unsigned short a[] = {17052,36120,34357,44436,44963,42593,9889,50074,9942,32874,47151,59209,12590,147,19923,
			2121,1980,64970,19426,42456,6563,24973,1726,55617,31800,59641,36188,32796,11426,53241,3381,45783,32141,
			48344,22840,42030,32882,32783,9369,14497,26456,21959,14644,46380,63997,5288,48501,441,4722,2392,42897,11285,
			27365,44623,1367,59165,38729,37555,26426,50155,25260,29807,30402,57401,12615,53243,33896,45497,20490,43265,
			59994,46946,65224,9102,27790,63685,14390,10756,64126,19112,13148,41488,30397,40513,20575,31764,34142,59304,
			3784,60568,43923,29044,24839,8790,20910,37454,62033,54806,17415,16987,32535,11873,63933,32223,20975,26188
	};
	int n = sizeof(a) / sizeof(unsigned short); //要測運行時間，這裏固定大小
	printf("\n排序前的数组：\n");
	for (i = 0; i < n; i++)
		printf(" %u", a[i]);
	printf("\n");
//	printf("\n生成随机数组：\n");
//	for (i = 0; i < n; i++){
//		a[i] = (unsigned short)rand();
//		printf(" %u", a[i]);
//	}
	BubbleSort(a, n);
	printf("\n排序后的数组：\n");
	for (i = 0; i < n; i++)
		printf(" %u", a[i]);
	printf("\n");
}
