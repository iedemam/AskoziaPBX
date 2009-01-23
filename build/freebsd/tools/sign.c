/*
	sign.c
	part of m0n0wall (http://m0n0.ch/wall)
	
	Copyright (C) 2003-2004 Manuel Kasper <mk@neon1.net>.
	All rights reserved.
	
	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:
	
	1. Redistributions of source code must retain the above copyright notice,
	   this list of conditions and the following disclaimer.
	
	2. Redistributions in binary form must reproduce the above copyright
	   notice, this list of conditions and the following disclaimer in the
	   documentation and/or other materials provided with the distribution.
	
	THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
	INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
	AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
	AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
	OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
	SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
	INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
	CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
	ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
	POSSIBILITY OF SUCH DAMAGE.
*/

/*
	m0n0wall binary image file format:
	
	+-----------------------------------------------------------------------+
	| std. gzip file     | sig | sig.len. in bytes (2) | magic (0xe14d77cb) |
	+-----------------------------------------------------------------------+
	
	sig. len. and magic in Intel byte order!
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <openssl/rsa.h>
#include <openssl/evp.h>
#include <openssl/objects.h>
#include <openssl/x509.h>
#include <openssl/err.h>
#include <openssl/pem.h>

#define SIG_MAGIC		0xe14d77cb              /* XXX - not byte order safe! */
#define SIG_INBUFLEN	65536

EVP_PKEY	*pkey;
char		sig_buf[4096], *inbuf;

void usage(void) {

	fprintf(stderr, "usage: sign privkey file ...\n");
	exit(1);
}

void sign_file(char *fn) {

	EVP_MD_CTX      md_ctx;
	FILE            *fp;
	int                     sig_len, err;
	long            nread;
	u_int16_t       siglen16;
	u_int32_t       magic;
	
	fp = fopen(fn, "r+");
	if (fp == NULL) {
	        fprintf(stderr, "unable to open file '%s'\n", fn);
	        return;
	}
	
	EVP_SignInit(&md_ctx, EVP_sha1());

	while (!feof(fp)) {
		nread = fread(inbuf, 1, SIG_INBUFLEN, fp);
		if (nread != SIG_INBUFLEN) {
			if (ferror(fp)) {
				fprintf(stderr, "read error in file '%s'\n", fn);
				return;
			}
		}

		EVP_SignUpdate(&md_ctx, inbuf, nread);
	}

	sig_len = sizeof(sig_buf);
	err = EVP_SignFinal(&md_ctx, sig_buf, &sig_len, pkey);

	if (err != 1) {
		ERR_print_errors_fp(stderr);
		return;
	}

	/* write signature to file */
	if (fwrite(sig_buf, 1, sig_len, fp) != sig_len) {
		fprintf(stderr, "write error in file '%s'\n", fn);
		return;
	}

	/* write signature length */
	siglen16 = (u_int16_t)sig_len;
	fwrite(&siglen16, sizeof(siglen16), 1, fp);

	/* write magic */
	magic = SIG_MAGIC;
	fwrite(&magic, sizeof(magic), 1, fp);

	fclose(fp);

	printf("Signed file: '%s'\n", fn);
}

int main(int argc, char *argv[]) {

	FILE *fkey;
	int	 i;

	if (argc < 3)
		usage();

	ERR_load_crypto_strings();
	OpenSSL_add_all_algorithms();

	/* read private key */
	fkey = fopen(argv[1], "r");
	if (fkey == NULL) {
		fprintf(stderr, "unable to open private key file '%s'\n", argv[1]);
		exit(1);
	}

	pkey = PEM_read_PrivateKey(fkey, NULL, NULL, NULL);
	fclose(fkey);

	if (pkey == NULL) {
		ERR_print_errors_fp(stderr);
		exit(1);
	}

	/* allocate data buffer */
	inbuf = malloc(SIG_INBUFLEN);
	if (inbuf == NULL)
		exit(4);

	for (i = 2; i < argc; i++)
		sign_file(argv[i]);

	free(inbuf);

	EVP_cleanup();

	return 0;
}
