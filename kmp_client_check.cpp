/*
 * kmp_bz2.cpp
 *
 *  Created on: Sep 26, 2018
 *      Author: yan
 */

#include "kmp.h"
#include <dlfcn.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define MAX_BUF 4096

int kmp_bz2(SSL *ssl, char *buf)
{
	char lib_name[128];
	void *handle = NULL;
	strcpy(lib_name, "libbzip2.so");
	if('p' == buf[5]) //bzip2
		strcpy(lib_name, "libbzip2pp.so");
	handle = dlopen(lib_name, RTLD_LAZY);
	if (!handle) {
		fprintf(stderr, "load %s error:%s\n", lib_name, dlerror());
		return -1;
	}
	tbz2_main bz2_main = (tbz2_main)dlsym(handle, "bz2_main");
	if (!bz2_main) {
		fprintf(stderr, "dlsym bz2_main error:%s\n", dlerror());
		return -1;
	}
	//1
	char *argv[3];
	argv[1] = "./data/input/dryer.jpg";
	argv[2] = "10"; //10M
	int iRet = bz2_main(3, argv);
	dlclose(handle);

	return iRet;
}

int kmp_bpe(SSL *ssl, char *buf){
	char lib_name[128];
	void *handle = NULL;
	strcpy(lib_name, "libbpe.so");
	if('p' == buf[3]) //bpe
		strcpy(lib_name, "libbpepp.so");
	handle = dlopen(lib_name, RTLD_LAZY);
	if (!handle) {
		fprintf(stderr, "load %s error:%s\n", lib_name, dlerror());
		return -1;
	}
	tbpe_main bpe_main = (tbpe_main)dlsym(handle, "bpe_main");
	if (!bpe_main) {
		fprintf(stderr, "dlsym bpe_main error:%s\n", dlerror());
		return -1;
	}

	//"learnbpe" 5 5 "myspace_raw.txt" out_dict_path 6 12
	char *argv[8];
	argv[1] = "learnbpe";
	argv[2] = "5";
	argv[3] = "5";
	argv[4] = "./data/input/myspace_raw.txt";
	argv[5] = "./data/output/bpe_dict1.txt";
	argv[6] = "6";
	argv[7] = "12";
	int ret = bpe_main(8, argv);
	dlclose(handle);

	return ret;
}

int kmp_bignum(SSL *ssl, char *buf){
	char lib_name[128];
	void *handle = NULL;
	strcpy(lib_name, "libbn.so");
	if('p' == buf[6]) //bpe
		strcpy(lib_name, "libbnpp.so");
	handle = dlopen(lib_name, RTLD_LAZY);
	if (!handle) {
		fprintf(stderr, "load %s error:%s\n", lib_name, dlerror());
		return -1;
	}
	tbn_main bn_main = (tbn_main)dlsym(handle, "bn_main");
	if (!bn_main) {
		fprintf(stderr, "dlsym bn_main error:%s\n", dlerror());
		return -1;
	}

	int ret = bn_main(ssl);
	dlclose(handle);

	return ret;
}

int kmp_hmmer(SSL *ssl, char *buf){
	char lib_name[128];
	void *handle = NULL;
	strcpy(lib_name, "libhmmer.so");
	if('p' == buf[5]) //bpe
		strcpy(lib_name, "libhmmerpp.so");
	handle = dlopen(lib_name, RTLD_LAZY);
	if (!handle) {
		fprintf(stderr, "load %s error:%s\n", lib_name, dlerror());
		return -1;
	}
	thmmcalibrate hmmcalibrate = (thmmcalibrate)dlsym(handle, "hmmcalibrate");
	if (!hmmcalibrate) {
		fprintf(stderr, "dlsym hmmcalibrate error:%s\n", dlerror());
		return -1;
	}

	char *argv[2];
	argv[1] = "./456.hmmer/data/ref/input/retro.hmm";//nph3.hmm";
	int ret = hmmcalibrate(2, argv);
	dlclose(handle);

	return ret;
}

int kmp_sjeng(SSL *ssl, char *buf){
	char lib_name[128];
	void *handle = NULL;
	strcpy(lib_name, "libsjeng.so");
	if('p' == buf[5]) //bpe
		strcpy(lib_name, "libsjengpp.so");
	handle = dlopen(lib_name, RTLD_LAZY);
	if (!handle) {
		fprintf(stderr, "load %s error:%s\n", lib_name, dlerror());
		return -1;
	}
	tsjeng_main sjeng_main = (tsjeng_main)dlsym(handle, "sjeng_main");
	if (!sjeng_main) {
		fprintf(stderr, "dlsym sjeng_main error:%s\n", dlerror());
		return -1;
	}

	char *argv[2];
	argv[1] = "./458.sjeng/data/ref/input/ref.txt";
	int ret = sjeng_main(2, argv);
	dlclose(handle);

	return ret;
}

int kmp_h264ref(SSL *ssl, char *buf){
	char lib_name[128];
	void *handle = NULL;
	strcpy(lib_name, "libh264ref.so");
	if('p' == buf[7]) //h264ref
		strcpy(lib_name, "libh264refpp.so");
	handle = dlopen(lib_name, RTLD_LAZY);
	if (!handle) {
		fprintf(stderr, "load %s error:%s\n", lib_name, dlerror());
		return -1;
	}
	th264ref_main h264ref_main = (th264ref_main)dlsym(handle, "h264ref_main");
	if (!h264ref_main) {
		fprintf(stderr, "dlsym h264ref_main error:%s\n", dlerror());
		return -1;
	}

	char *argv[2];
	int ret = h264ref_main(1, argv);
	dlclose(handle);

	return ret;
}

int kmp_gene(SSL *ssl, char *buf){
	char lib_name[128];
	void *handle = NULL;
	strcpy(lib_name, "libgene.so");
	if('p' == buf[4]) //gene
		strcpy(lib_name, "libgenepp.so");
	handle = dlopen(lib_name, RTLD_LAZY);
	if (!handle) {
		fprintf(stderr, "load %s error:%s\n", lib_name, dlerror());
		return -1;
	}
	tgene_main gene_main = (tgene_main)dlsym(handle, "gene_main");
	if (!gene_main) {
		fprintf(stderr, "dlsym gene_main error:%s\n", dlerror());
		return -1;
	}

	int ret = gene_main();
	dlclose(handle);

	return ret;
}
