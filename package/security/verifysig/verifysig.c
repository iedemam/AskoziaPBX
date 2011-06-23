/*
	$Id: verifysig.c 52 2006-02-07 14:35:14Z mkasper $
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
	
	WARNING: in the process of verifying the signature, this program actually
	removes it from the file - this is to facilitate later processing where
	it might confuse other programs (gzip just warns about trailing garbage,
	but we might sign other files in the future...).
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <openssl/rsa.h>
#include <openssl/evp.h>
#include <openssl/objects.h>
#include <openssl/x509.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <unistd.h>

#define SIG_MAGIC		0xe14d77cb		/* XXX - not byte order safe! */
#define SIG_INBUFLEN	65536

void usage(void) {
	
	fprintf(stderr, "usage: verifysig pubkey file key\n\n"
					"return values:    0 -> signature verified OK\n"
					"                  1 -> signature invalid\n"
					"                  2 -> no signature found\n"
					"                  3 -> signature verification error\n"
					"                  4 -> other error\n");
	exit(4);
}

int main(int argc, char *argv[]) {
	
	FILE		*fin, *fkey, *sigkey;
	u_int16_t	siglen;
	u_int32_t	magic;
	long		nread, ndata;
	char		*sigbuf, *inbuf;
	EVP_PKEY	*pkey;
	EVP_MD_CTX	ctx;
	int			err, retval;
	
	if (!(argc == 3 || argc == 4))
		usage();
	
	ERR_load_crypto_strings();
	
	/* open file and check for magic */
	fin = fopen(argv[2], "r+");
	if (fin == NULL) {
		fprintf(stderr, "unable to open file '%s'\n", argv[2]);
		exit(5);
	}
	
	
	/* read signature key if available */
	if(argc == 4)
	{
		sigkey = fopen(argv[3], "r");
		if (sigkey == NULL) {
			fprintf(stderr, "unable to open signature key file '%s'\n", argv[3]);
			exit(6);
		}
	}
	
	if(argc == 3)
	{
		fseek(fin, -(sizeof(magic)), SEEK_END);
		fread(&magic, sizeof(magic), 1, fin);
	} else {
		fseek(sigkey, -(sizeof(magic)), SEEK_END);
		fread(&magic, sizeof(magic), 1, sigkey);
	}
		
	if (magic != SIG_MAGIC) {
		fclose(fin);
		exit(2);
	}
	
	if(argc == 3)
	{
		/* magic is good; get signature length */	
		fseek(fin, -(sizeof(magic) + sizeof(siglen)), SEEK_END);	
		fread(&siglen, sizeof(siglen), 1, fin);
	} else {
		/* magic is good; get signature length */	
		fseek(sigkey, -(sizeof(magic) + sizeof(siglen)), SEEK_END);	
		fread(&siglen, sizeof(siglen), 1, sigkey);
	}
	
	/* read public key */
	fkey = fopen(argv[1], "r");
	if (fkey == NULL) {
		fprintf(stderr, "unable to open public key file '%s'\n", argv[1]);
		exit(7);
	}
	
	pkey = PEM_read_PUBKEY(fkey, NULL, NULL, NULL);
	fclose(fkey);
	
	if (pkey == NULL) {
		ERR_print_errors_fp(stderr);
		exit(8);
	}
	
	/* check if siglen is sane */
	if ((siglen == 0) || (siglen > EVP_PKEY_size(pkey)))
		exit(3);
	
	/* got signature length; read signature */
	sigbuf = malloc(siglen);
	if (sigbuf == NULL)
		exit(9);
	
	
	if(argc == 3)
	{
		fseek(fin, -(sizeof(magic) + sizeof(siglen) + siglen), SEEK_END);	
		if (fread(sigbuf, 1, siglen, fin) != siglen)
		exit(10);
		
		/* signature read; truncate file to remove sig */
		fseek(fin, 0, SEEK_END);
		ndata = ftell(fin) - (sizeof(magic) + sizeof(siglen) + siglen);
		ftruncate(fileno(fin), ndata);
	}
	
	if(argc == 4)
	{
		rewind(sigkey);
		while (!feof(sigkey)) {
			nread = fread(sigbuf, 1, SIG_INBUFLEN, sigkey);
			if (nread != SIG_INBUFLEN) {
				if (ferror(sigkey)) {
					fprintf(stderr, "read error in file '%s'\n", argv[2]);
					exit(11);
				}
			}
		}
	}

	/* verify the signature now */
	EVP_VerifyInit(&ctx, EVP_sha1());
	
	/* allocate data buffer */
	inbuf = malloc(SIG_INBUFLEN);
	if (inbuf == NULL)
		exit(13);
	
	rewind(fin);
	while (!feof(fin)) {
		nread = fread(inbuf, 1, SIG_INBUFLEN, fin);
		if (nread != SIG_INBUFLEN) {
			if (ferror(fin)) {
				fprintf(stderr, "read error in file '%s'\n", argv[2]);
				exit(14);
			}
		}
		
		EVP_VerifyUpdate(&ctx, inbuf, nread);
	}
	
	err = EVP_VerifyFinal(&ctx, sigbuf, siglen, pkey);
	EVP_PKEY_free(pkey);
	
	if (err == 1)
		retval = 0;		/* correct signature */
	else if (err == 0)
		retval = 1;		/* invalid signature */
	else
		retval = 3;		/* error */
	
	free(inbuf);
	free(sigbuf);
	fclose(fin);
	
	return retval;
}
