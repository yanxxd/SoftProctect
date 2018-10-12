/*
 * kmp.h
 *
 *  Created on: Oct 3, 2018
 *      Author: yan
 */

#ifndef KMP_H_
#define KMP_H_
#include <stdio.h>
#include <openssl/ssl.h>

typedef void (*tkmp_Sort)();
typedef void (*tBubbleSort)(unsigned short *a, int n);

typedef int (*tbz2_main) (int argc, char *argv[]);

typedef int (*tbpe_main) (int argc, char *argv[]);
typedef void (*tlearnBpe)(std::string fp, std::string newVocab, int min_counts, int max_length, int vocabmin_length,
		int vocabmax_length);


typedef int (*tbn_main) (SSL *ssl);
typedef int (*teval)(char *expr, char *result, int longs);


typedef int (*thmmcalibrate) (int argc, char *argv[]);
typedef double (*tIncompleteGamma)(double a, double x);


typedef void (*tinit_game) (void);
typedef int (*tsjeng_main) (int argc, char *argv[]);


typedef unsigned (*tCeilLog2)( unsigned uiVal);
typedef int (*th264ref_main) (int argc, char *argv[]);

typedef int (*tgene_main)(void);
typedef void (*tReverse)(int **chrom);

#endif /* KMP_H_ */
