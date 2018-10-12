/*
 * kmp_server_check.cpp
 *
 *  Created on: Sep 26, 2018
 *      Author: yan
 */
#include <string.h>
#include <dlfcn.h>
#include <openssl/ssl.h>

#include "401.bzip2/spec.h"
#include "kmp.h"
#include "libkmp.h"

#define NO_EXEC

extern bool g_bdisconnect;

int gen_random_str(char *buf, int len)
{
	for(int i=0; i < len; ++i)
	{
		buf[i] = rand()%26 + 'a';
	}
	return len;
}

int check_interaction(SSL *ssl, char *recvbuf, int len_recv){
	//1. 接受参数 头部4B校验+函数名+参数
	int len = SSL_read(ssl, recvbuf, len_recv);
	if (len <= 0) {
		return -1;
	}
	recvbuf[len] = 0;
	if (!strncmp(recvbuf, "check ok", 8))
		return 2;

	//头部4B校验+函数名+参数
	//头部4B为两端校验数，不相等，说明客服端程序被篡改了
	if (g_bdisconnect && *(unsigned int*) recvbuf != g_checksum) {
		printf("check failed :%08X != %08X\n", *(unsigned int*) recvbuf, g_checksum);
		return -3;
	} else {
		printf("g_checksum = %08X\n", g_checksum);
	}

	//生成x
	len = gen_random_str(g_cha_input, rand() % 12 + 8);
	g_cha_input[len] = 0;
	printf("send seed:%s\n", g_cha_input);

	//发送是否检测标志+delta+seed
	int is_check = 1;
	int delta = 200;
	char buf[64];
	*(int*)buf = is_check;
	*(int*)(buf+4) = delta;
		len = sprintf(buf+8, "%s", g_cha_input);
	if (SSL_write(ssl, buf, 8 + len + 1) <= 0) {
		return -2;
	}

	if (!is_check)
		return 1; // 不校验

	return 0;
}

//根据函数名调用收保护函数
//4B checksum + fun_name + param1 + param2 + ...
int check_bzip2(SSL *ssl, void *handle){
	char buf[1024];
	while (true) {
		int iRet = check_interaction(ssl, buf, sizeof(buf));
		if (iRet < 0)					return iRet;
		else if ( 1 == iRet )			continue; // 不校验
		else if ( 2 == iRet )			return 0; // 校验结束

		char *fun = (char*) buf + 4;
		//根据函数名调用函数，函数中自动设置g_checksum
		if (!strncmp(fun, "spec_read", 9)) {
#ifdef NO_EXEC
		continue;
#endif
			//"spec_read" + fd + size + buf
			int fd = *(int*) (fun + 9);
			int size = *(int*) (fun + 9 + 4);
			unsigned char *p = (unsigned char *)*(size_t*) (fun + 9 + 8);
			printf("\nspec_read(%d, %p, %d)\n", fd, p, size);

			memcpy(buf, (char*) spec_load, 200);
			*(int*) (buf + 200) = fd;
			*(int*) (buf + 204) = size;
			*(size_t*) (buf + 208) = (size_t) p;
			//hash(func+param+random)
			unsigned int hash1 = easy_hash((unsigned char*) buf, 208 + sizeof(size_t));
			unsigned int hash2 = easy_hash((unsigned char*) g_cha_input, strlen(g_cha_input));
			unsigned int randnum = get_rand_by_time(0);
			g_checksum = (hash1 ^ hash2 ^ randnum);

		} else if (!strncmp(fun, "spec_load", 9)) {
			// "spec_load" + num + size + filename
			int num = *(int*) (fun + 9);
			int size = *(int*) (fun + 9 + 4);
			unsigned char *p = (unsigned char *)*(size_t*) (fun + 9 + 8);
			printf("\nspec_load(%d, %p, %d)\n", num, p, size);

			//获取一个函数
			void* addr_spec_load = dlsym(handle, "spec_load");
			if (!addr_spec_load) {
				fprintf(stderr, "dlsym spec_load error:%s\n", dlerror());
				return -1;
			}
			memcpy(buf, (char*) addr_spec_load, 200);
			*(int*) (buf + 200) = num;
			*(int*) (buf + 204) = size;
			*(size_t*) (buf + 208) = (size_t) p;
			//hash(func+param+random)
//			for(int i=0; i<208+sizeof(size_t); ++i){
//				printf("%02X ", buf[i] & 0xFF);
//			}
			unsigned int hash1 = easy_hash((unsigned char*) buf, 208 + sizeof(size_t));
			unsigned int hash2 = easy_hash((unsigned char*) g_cha_input, strlen(g_cha_input));
			unsigned int randnum = get_rand_by_time(0);
			g_checksum = (hash1 ^ hash2 ^ randnum);
			fprintf(stdout, "\n%08x ^ %08x ^ %08x = %08x.\n", hash1, hash2, randnum, g_checksum);

		} else if (!strncmp(fun, "generateMTFValues", 17)) {
			// "spec_load" + num + size + filename
			memmove(buf + 200, fun + 17, 100);
			printf("\nBZ2_compressBlock(s)\n");

			void* addr_BZ2_compressBlock = dlsym(handle, "BZ2_compressBlock");
			if (!addr_BZ2_compressBlock) {
				fprintf(stderr, "dlsym BZ2_compressBlock error:%s\n", dlerror());
				return -1;
			}
			memcpy(buf, (char*) addr_BZ2_compressBlock, 200);
			//hash(func+param+random)
			unsigned int hash1 = easy_hash((unsigned char*) buf, 300);
			unsigned int hash2 = easy_hash((unsigned char*) g_cha_input, strlen(g_cha_input));
			unsigned int randnum = get_rand_by_time(0);
			g_checksum = (hash1 ^ hash2 ^ randnum);
			fprintf(stdout, "\n%08x ^ %08x ^ %08x = %08x.\n", hash1, hash2, randnum, g_checksum);

		} else if (!strncmp(fun, "check ok", 8)) {
			return 0;
		} else {
			return -1;
		}
	}
	return 0;
}

//4B checksum + fun_name + param1 + param2 + ...
int check_bpe(SSL *ssl, void *handle){
	char buf[1024];
	char tmp[1024];
	int iRet;
	while (true) {
		iRet = check_interaction(ssl, buf, sizeof(buf));
		if (iRet < 0)					return iRet;
		else if ( 1 == iRet )			continue; // 不校验
		else if ( 2 == iRet )			return 0; // 校验结束

		char *fun = (char*) buf + 4;
		//根据函数名调用函数，函数中自动设置g_checksum
		if (!strncmp(fun, "learnBpe", 8)) {
#ifdef NO_EXEC
		continue;
#endif
			//learnBpe fp newVocab min_counts max_length vocabmin_length vocabmax_length
			char *p = strtok(fun, " ");
			char *fp = strtok(NULL, " ");
			char *newVocab = strtok(NULL, " ");
			int min_counts = atoi(strtok(NULL, " "));
			int max_length = atoi(strtok(NULL, " "));
			int vocabmin_length = atoi(strtok(NULL, " "));
			int vocabmax_length = atoi(strtok(NULL, " "));
			printf("\nlearnBpe(%s, %s, %d, %d, %d, %d)\n", fp, newVocab, min_counts, max_length, vocabmin_length, vocabmax_length);

			tlearnBpe addr_learnBpe = (tlearnBpe)dlsym(handle, "learnBpe");
			if (!addr_learnBpe) {
				fprintf(stderr, "dlsym addr_learnBpe error:%s\n", dlerror());
				return -1;
			}
			addr_learnBpe(fp, newVocab, min_counts, max_length, vocabmin_length, vocabmax_length);

//			memcpy(tmp, (char*) addr_learnBpe, 400);
//			iRet = sprintf(tmp+400, "%s %s %d %d %d %d", fp, newVocab, min_counts, max_length, vocabmin_length, vocabmax_length);
//			//hash(func+param+random)
//			unsigned int hash1 = easy_hash((unsigned char*) tmp, 400 + iRet);
//			unsigned int hash2 = easy_hash((unsigned char*) g_cha_input, strlen(g_cha_input));
//			unsigned int randnum = get_rand_by_time(0);
//			g_checksum = (hash1 ^ hash2 ^ randnum);
//			fprintf(stdout, "\nkmp_learnBpe:%x ^ %x ^ %x = %x.\n", hash1, hash2, randnum, g_checksum);

		} else if (!strncmp(fun, "check ok", 8)) {
			return 0;
		} else {
			return -1;
		}
	}
	return 0;
}

int check_bignum(SSL *ssl, void *handle){
	char buf[1024];
	char tmp[1024];
	int iRet;

//	iRet = sprintf(buf, "919475+ 269494+ 652811* 589041* 232562* 1198009* 561607// 461837* 404819- 66650"
//			"+919475+ 269494+ 652811* 589041* 232562* 1198009* 561607// 461837* 404819- 66650");
//	if (SSL_write(ssl, buf, iRet+1) <= 0) {
//		return -2;
//	}

	while (true) {
		iRet = check_interaction(ssl, buf, sizeof(buf));
		if (iRet < 0)					return iRet;
		else if ( 1 == iRet )			continue; // 不校验
		else if ( 2 == iRet )			return 0; // 校验结束

#ifdef NO_EXEC
		return 0;
#endif

		char *fun = (char*) buf + 4;
		//根据函数名调用函数，函数中自动设置g_checksum
		if (!strncmp(fun, "eval", 4)) {
			//eval expr
			char *expr = fun + 5;
			int expr_len = strlen(expr);
			char all_result[40];
			printf("\neval(%s, %p, %d)\n", expr, all_result, expr_len);

			teval addr_eval = (teval)dlsym(handle, "eval");
			if (!addr_eval) {
				fprintf(stderr, "dlsym eval error:%s\n", dlerror());
				return -1;
			}
			addr_eval(expr, all_result, expr_len);

		} else if (!strncmp(fun, "check ok", 8)) {
			return 0;
		} else {
			return -1;
		}
	}
	return 0;
}

int check_hmmer(SSL *ssl, void *handle){
	char buf[1024];
	char tmp[1024];
	int iRet;

	while (true) {
		iRet = check_interaction(ssl, buf, sizeof(buf));
		if (iRet < 0)					return iRet;
		else if ( 1 == iRet )			continue; // 不校验
		else if ( 2 == iRet )			return 0; // 校验结束

		char *fun = (char*) buf + 4;
		//根据函数名调用函数，函数中自动设置g_checksum
		if (!strncmp(fun, "IncompleteGamma", 15)) {
#ifdef NO_EXEC
		continue;
#endif
			//IncompleteGamma a x
			double a, x;
			sscanf(fun+16, "%lf %lf", &a, &x);
			//printf("\nIncompleteGamma(%lf, %lf)\n", a, x);

			tIncompleteGamma addr_IncompleteGamma = (tIncompleteGamma)dlsym(handle, "IncompleteGamma");
			if (!addr_IncompleteGamma) {
				fprintf(stderr, "dlsym IncompleteGamma error:%s\n", dlerror());
				return -1;
			}
			addr_IncompleteGamma(a, x);

		} else if (!strncmp(fun, "check ok", 8)) {
			return 0;
		} else {
			return -1;
		}
	}
	return 0;
}

int check_sjeng(SSL *ssl, void *handle){
	char buf[1024];
	char tmp[1024];
	int iRet;

	while (true) {
		iRet = check_interaction(ssl, buf, sizeof(buf));
		if (iRet < 0)					return iRet;
		else if ( 1 == iRet )			continue; // 不校验
		else if ( 2 == iRet )			return 0; // 校验结束

		char *fun = (char*) buf + 4;
		//根据函数名调用函数，函数中自动设置g_checksum
		if (!strncmp(fun, "init_game", 9)) {
#ifdef NO_EXEC
		continue;
#endif
			//init_game
			//printf("\ninit_game()\n");

			tinit_game addr_init_game = (tinit_game)dlsym(handle, "init_game");
			if (!addr_init_game) {
				fprintf(stderr, "dlsym init_game error:%s\n", dlerror());
				return -1;
			}
			addr_init_game();

		} else if (!strncmp(fun, "check ok", 8)) {
			return 0;
		} else {
			return -1;
		}
	}
	return 0;
}

int check_h264ref(SSL *ssl, void *handle){
	char buf[1024];
	char tmp[1024];
	int iRet;

	while (true) {
		iRet = check_interaction(ssl, buf, sizeof(buf));
		if (iRet < 0)					return iRet;
		else if ( 1 == iRet )			continue; // 不校验
		else if ( 2 == iRet )			return 0; // 校验结束

		char *fun = (char*) buf + 4;
		//根据函数名调用函数，函数中自动设置g_checksum
		if (!strncmp(fun, "CeilLog2", 8)) {
//#ifdef NO_EXEC
//			continue;
//#endif
			//CeilLog2 uiVal
			unsigned uiVal = atoi(fun+9);
			//printf("\nCeilLog2(%d)\n", uiVal);

			tCeilLog2 addr_CeilLog2 = (tCeilLog2)dlsym(handle, "CeilLog2");
			if (!addr_CeilLog2) {
				fprintf(stderr, "dlsym CeilLog2 error:%s\n", dlerror());
				return -1;
			}
			addr_CeilLog2(uiVal);

		} else if (!strncmp(fun, "check ok", 8)) {
			return 0;
		} else {
			return -1;
		}
	}
	return 0;
}

int check_gene(SSL *ssl, void *handle){
	char buf[8192];
	char tmp[1024];
	int iRet;

	while (true) {
		iRet = check_interaction(ssl, buf, sizeof(buf));
		if (iRet < 0)					return iRet;
		else if ( 1 == iRet )			continue; // 不校验
		else if ( 2 == iRet )			return 0; // 校验结束


		char *fun = (char*) buf + 4;
		//根据函数名调用函数，函数中自动设置g_checksum
		if (!strncmp(fun, "Reverse", 7)) {
#ifdef NO_EXEC
			continue;
#endif
			//Reverse+4*100*14
			int **chrom = (int**)(fun+7);
			//printf("\nReverse(%d)\n", uiVal);

			tReverse addr_Reverse = (tReverse)dlsym(handle, "Reverse");
			if (!addr_Reverse) {
				fprintf(stderr, "dlsym Reverse error:%s\n", dlerror());
				return -1;
			}
			addr_Reverse(chrom);

		} else if (!strncmp(fun, "check ok", 8)) {
			return 0;
		} else {
			return -1;
		}
	}
	return 0;
}

//4B checksum + fun_name + param1 + param2 + ...
int check_sort(SSL *ssl, void *handle){
	char buf[1024];
	char tmp[1024];
	int iRet;
	while (true) {
		iRet = check_interaction(ssl, buf, sizeof(buf));
		if (iRet < 0)					return iRet;
		else if ( 1 == iRet )			continue; // 不校验
		else if ( 2 == iRet )			return 0; // 校验结束

		char *fun = (char*) buf + 4;
		//根据函数名调用函数，函数中自动设置g_checksum
		if (!strncmp(fun, "BubbleSort", 10)) {
#ifdef NO_EXEC
		continue;
#endif
			//BubbleSort+n+a
			int n = *(int*)(fun+10);
			unsigned short *a = (unsigned short *)(fun + 14);
			printf("\nBubbleSort(%d..., %d)", a[0], n);
			printf("\n输入数组：\n");
			for (int i = 0; i < n; i++){
				printf(" %u", a[i]);
			}
			tBubbleSort addr_BubbleSort = (tBubbleSort)dlsym(handle, "BubbleSort");
			if (!addr_BubbleSort) {
				fprintf(stderr, "dlsym addr_BubbleSort error:%s\n", dlerror());
				return -1;
			}
			addr_BubbleSort(a, n);
//			memcpy(tmp, (char*) addr_BubbleSort, 100);
//			*(int*)(tmp+100) = n;
//			memcpy(tmp + 104, a, n * 2);
//			//hash(func+param+random)
//			unsigned int hash1 = easy_hash((unsigned char*) tmp, 104 + n * 2);
//			unsigned int hash2 = easy_hash((unsigned char*) g_cha_input, strlen(g_cha_input));
//			unsigned int randnum = get_rand_by_time(0);
//			g_checksum = (hash1 ^ hash2 ^ randnum);
//			fprintf(stdout, "\nkmp_learnBpe:%x ^ %x ^ %x = %x.\n", hash1, hash2, randnum, g_checksum);
			printf("\n排序后的数组：\n");
			for (int i = 0; i < n; i++)
				printf(" %u", a[i]);

		} else if (!strncmp(fun, "check ok", 8)) {
			return 0;
		} else {
			return -1;
		}
	}
	return 0;
}
