#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>

#define BUFFER_SIZE	256
#define HEADER_SIZE	6

int append_data_to_image(const char *image_file, unsigned long app_address, const char *data, unsigned short data_len)
{
	unsigned short word_buffer[BUFFER_SIZE];
	unsigned char buffer[BUFFER_SIZE];
	int ret = 0;
	int status = 0;
	unsigned long address;
	unsigned long block_count;
	int ii;
	FILE *fp;

	if (!(fp = fopen(image_file, "r+"))) {
		fprintf(stderr, "error (%s) opening file %s\n", image_file, strerror(errno));
		ret = -1;
		return ret;
	}

	/* here's how you're going to do this: */
	/* read all record headers from the file */
	/* and dump everything that comes from the file */

	if (fread(buffer, 1, HEADER_SIZE, fp) != HEADER_SIZE) {
		fprintf(stderr, "error (%s) reading %d octets\n", strerror(errno), HEADER_SIZE);
		fclose(fp);
		fp = NULL;
		ret = -1;
		return ret;
	}

	while (status == 0) {

		/* now evaluate the header */
		if ((buffer[0] != 0x00) && (buffer[0] != 0x01)) {
			fclose(fp);
			fp = NULL;
			status = -1;
			break;
		}

		address = (((unsigned long)buffer[1] << 16) & 0x00FF0000) | 
			(((unsigned long)buffer[2] << 8) & 0x0000FF00) | 
			((unsigned long)buffer [3] & 0x000000FF);

		block_count = (buffer[4] << 8) | buffer[5];

		if (fread(word_buffer, sizeof(unsigned short), block_count, fp) != block_count) {
			fprintf(stderr, "invalid file format (%s)\n", image_file);
			fclose(fp);
			fp = NULL;	
			status = -1;
		}

		/* if all is good, dump everything you got */
		fprintf(stdout, "starting address = 0x%08x\n", address);
		fprintf(stdout, "===============================\n");
		for (ii = 0; ii < block_count; ii++) {
			fprintf(stdout, "0x%08x <= 0x%04x\n", address + ii * 2, word_buffer[ii]);
		}

		/* read the next header */
		if (fread(buffer, 1, HEADER_SIZE, fp) != HEADER_SIZE) {
			fclose(fp);
			fp = NULL;
			status = 1;
		}

		if (buffer[0] == 0xFF) {
			fprintf(stdout, "end of file\n");
			status = 0;
			if (data_len == 0)
				break;
			if (fseek(fp, -HEADER_SIZE, SEEK_CUR) == -1) {
				fprintf(stderr, "could not set file pointer\n");
				fclose(fp);
				fp = NULL;
				break;
			} else {
				buffer[0] = 0x00;
				buffer[1] = (app_address & 0x000FF000) >> 16;
				buffer[2] = (app_address & 0x0000FF00) >> 8;
				buffer[3] = (app_address & 0x000000FF);
				buffer[4] = (((data_len + 1) / 2) & 0xFF00) >> 8;
				buffer[5] = (((data_len + 1) / 2) & 0x00FF);
				if (fwrite(buffer, 1, HEADER_SIZE, fp) != HEADER_SIZE) {
					fprintf(stderr, "error writing header\n");
					fclose(fp);
					fp = NULL;
					status = 1;
				}
				if (fwrite(data, 1, data_len, fp) != data_len) {
					fprintf(stderr, "error writing data\n");
					fclose(fp);
					fp = NULL;
					status = 1;

				}
				buffer[0] = 0xFF;
				buffer[1] = 0;
				buffer[2] = 0;
				buffer[3] = 0;
				buffer[4] = 0;
				buffer[5] = 0;
				if (fwrite(buffer, 1, HEADER_SIZE, fp) != HEADER_SIZE) {
					fprintf(stderr, "error writing final header\n");
					fclose(fp);
					fp = NULL;
					status = 1;
				}
			}
		}
	}

	if (fp) fclose(fp);

	return 0;
}

int main(int argc, char **argv)
{
	char append_buffer[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 };
	unsigned long address = 0x00112230;
	int ii;

	if (argc < 2) {
		fprintf(stderr, "usage : warp_embed_dsp [image_file]\n");
		exit(-1);
	}

	for (ii = 0; ii < sizeof(append_buffer) / 2; ii+=2) {
		char tmp = append_buffer[ii];
		append_buffer[ii] = append_buffer[ii+1];
		append_buffer[ii+1] = tmp;
	}

	//return append_data_to_image(argv[1], address, append_buffer, sizeof(append_buffer) / sizeof(short));
	return append_data_to_image(argv[1], address, append_buffer, 0);
}
