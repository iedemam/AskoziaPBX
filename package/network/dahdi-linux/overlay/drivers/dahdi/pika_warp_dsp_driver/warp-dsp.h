#ifndef __WARP_DSP_H__
#define __WARP_DSP_H__

#include <linux/types.h>

/* basic access functions */
void warp_dsp_read_memory_16(unsigned short dsp_id, unsigned int dsp_address, unsigned int num_words, unsigned short *word_values);
void warp_dsp_read_memory_16_noswap(unsigned short dsp_id, unsigned int dsp_address, unsigned int num_words, unsigned short *word_values);

void warp_dsp_read_memory_32(unsigned short dsp_id, unsigned int dsp_address, unsigned int num_words, unsigned long int *word_values);
void warp_dsp_read_memory_32_noswap(unsigned short dsp_id, unsigned int dsp_address, unsigned int num_words, unsigned long int *word_values);

void warp_dsp_write_memory_16(unsigned short dsp_id, unsigned int dsp_address, unsigned int num_words, unsigned short *word_values);
void warp_dsp_write_memory_16_noswap(unsigned short dsp_id, unsigned int dsp_address, unsigned int num_words, unsigned short *word_values);

void warp_dsp_write_memory_32(unsigned short dsp_id, unsigned int dsp_address, unsigned int num_words, unsigned long int *word_values);
void warp_dsp_write_memory_32_noswap(unsigned short dsp_id, unsigned int dsp_address, unsigned int num_words, unsigned long int *word_values);

/* other access functions  */
void warp_dsp_delay(unsigned int delay_type);
void warp_dsp_lock(unsigned short dsp_id);
void warp_dsp_unlock(unsigned short dsp_id);
void warp_dsp_cmd_lock(unsigned short dsp_id);
void warp_dsp_cmd_unlock(unsigned short dsp_id);
void warp_dsp_get_dsp_command_buffer_write_lock(unsigned short dsp_id);
void warp_dsp_get_dsp_command_buffer_read_lock(unsigned short dsp_id);

int warp_dsp_read_firmware(unsigned int file_id, unsigned char *buffer, unsigned int num_bytes);

/* transcoder packet transmission functions */
int dahdi_warp_send_packet_payload(unsigned int dsp_id, unsigned int channel_id, unsigned int encoding, unsigned char *payload, unsigned short payload_size, int queue);
int dahdi_warp_receive_packet_payload(unsigned int dsp_id, unsigned int channel_id, unsigned int encoding, unsigned char *buffer, unsigned short buffer_size, unsigned int noblock);

/* transcoder channel management functions */
int dahdi_warp_open_transcoder_channel_pair(unsigned int dsp_id, unsigned int *encoder_chan_id, unsigned int *decoder_chan_id);
int dahdi_warp_release_transcoder_channel_pair(unsigned int dsp_id, unsigned int decoder_chan_id, unsigned int encoder_chan_id);
int dahdi_warp_cleanup_transcoder_channel(unsigned int dsp_id, unsigned int channel_id);
int dahdi_warp_release_all_transcoder_channels(unsigned int dsp_id);

/* echo canceller channel management functions */
int dahdi_warp_get_echocan_channel(unsigned int dsp_id, unsigned int in_timeslot, unsigned int tap_length, unsigned int *channel_id);
int dahdi_warp_release_echocan_channel(unsigned int dsp_id, unsigned int channel_id);
int dahdi_warp_release_all_echocan_channels(unsigned int dsp_id);

/* debug/logging functions */
void dahdi_warp_log(int flag, const char *format, ...);

/* defines for logging  */
#define DSP_FS_LOG			0x00000001
#define DSP_FUNC_LOG			0x00000002
#define DSP_FIFO_LOG			0x00000004
#define DSP_HP_LOG			0x00000008
#define DSP_INT_LOG			0x00000010
#define DSP_BIST_LOG			0x00000020
#define DSP_ECAN_LOG			0x00000040
#define DSP_TCODER_LOG			0x00000080
#define DSP_HPORT_LOG			0x80000000



#endif /* __WARP_DSP_H__ */
