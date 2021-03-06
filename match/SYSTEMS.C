
#include "mplex.h"

//
// bcopy
//
void bcopy(unsigned char *source, unsigned char *dest, int k)
{
	unsigned char *from, *to;

	from = source;
	to = dest;

	while(k!=0)
	{
		*to = *from;
		to++;
		from++;
		k--;
	}
}
/*************************************************************************
	Create_Sector
	产生一个完整的sector
*************************************************************************/


void create_sector (sector, pack, sys_header,
			packet_size, inputstream, type, 
			buffer_scale, buffer_size, buffers,
			PTS, DTS, timestamps, which_streams )

Sector_struc 	 *sector;
Pack_struc	 *pack;
Sys_header_struc *sys_header;
unsigned int 	 packet_size;
FILE		 *inputstream;

unsigned char 	 type;
unsigned char 	 buffer_scale;
unsigned int 	 buffer_size;
unsigned char 	 buffers;
Timecode_struc   *PTS;
Timecode_struc   *DTS;
unsigned char 	 timestamps;
unsigned int     which_streams;

{
    int i,j,tmp;
    unsigned char *index;
    unsigned char *size_offset;

    index = sector->buf;
    sector->length_of_sector=0;
    if (pack != NULL)
    {
	i = sizeof(pack->buf);
	bcopy (pack->buf, index, i);
	index += i;
	sector->length_of_sector += i;
    }


    if (sys_header != NULL)
    {
	i = sizeof(sys_header->buf);

	if (which_streams != STREAMS_BOTH) i -= 3;

	bcopy (sys_header->buf, index, i);
	index += i;
	sector->length_of_sector += i;
    }


    *(index++) = (unsigned char)(PACKET_START)>>16;
    *(index++) = (unsigned char)(PACKET_START & 0x00ffff)>>8;
    *(index++) = (unsigned char)(PACKET_START & 0x0000ff);
    *(index++) = type;	

    
    size_offset = index;
    *(index++) = (unsigned char)((packet_size - PACKET_HEADER_SIZE)>>8);
    *(index++) = (unsigned char)((packet_size - PACKET_HEADER_SIZE)&0xff);

    *(index++) = STUFFING_BYTE;
    *(index++) = STUFFING_BYTE;
    *(index++) = STUFFING_BYTE;

    i = 0;

    if (!buffers) i +=2;
    if (timestamps == TIMESTAMPS_NO) i+=9;
    else if (timestamps == TIMESTAMPS_PTS) i+=5;

    
    for (j=0; j<i; j++)
	*(index++) = STUFFING_BYTE;

    if (buffers)
    {
	*(index++) = (unsigned char) (0x40 |
			(buffer_scale << 5) | (buffer_size >> 8));
	*(index++) = (unsigned char) (buffer_size & 0xff);
    }


    switch (timestamps)
    {
	case TIMESTAMPS_NO:
	    *(index++) = MARKER_NO_TIMESTAMPS;
	    break;
	case TIMESTAMPS_PTS:
	    buffer_timecode (PTS, MARKER_JUST_PTS, &index);
	    copy_timecode (PTS, &sector->TS);
	    break;
	case TIMESTAMPS_PTS_DTS:
	    buffer_timecode (PTS, MARKER_PTS, &index);
	    buffer_timecode (DTS, MARKER_DTS, &index);
	    copy_timecode (DTS, &sector->TS);
	    break;
    }

    
    i = (packet_size - PACKET_HEADER_SIZE - AFTER_PACKET_LENGTH);

    if (type == PADDING_STR)
    {
	for (j=0; j<i; j++)
	    *(index++)=(unsigned char) STUFFING_BYTE;
	tmp = i;
    } 
    else
    {   
	tmp = fread (index, sizeof (unsigned char), i, inputstream);
        index += tmp;

	if (tmp != i)
	{   
	    packet_size -= (i-tmp);
	    *(size_offset++) = (unsigned char)((packet_size - PACKET_HEADER_SIZE)>>8);
	    *(size_offset++) = (unsigned char)((packet_size - PACKET_HEADER_SIZE)&0xff);
	    
	}
    }


    sector->length_of_sector += packet_size;
    sector->length_of_packet_data = tmp;
    
}

/*************************************************************************
	Create_Pack

	向缓存中写入特定的报头
*************************************************************************/

void create_pack (pack, SCR, mux_rate)

Pack_struc	 *pack;
unsigned int 	 mux_rate;
Timecode_struc   *SCR;

{
    unsigned char *index;

    index = pack->buf;

    *(index++) = (unsigned char)((PACK_START)>>24);
    *(index++) = (unsigned char)((PACK_START & 0x00ff0000)>>16);
    *(index++) = (unsigned char)((PACK_START & 0x0000ff00)>>8);
    *(index++) = (unsigned char)(PACK_START & 0x000000ff);
    buffer_timecode (SCR, MARKER_SCR, &index);
    *(index++) = (unsigned char)(0x80 | (mux_rate >>15));
    *(index++) = (unsigned char)(0xff & (mux_rate >> 7));
    *(index++) = (unsigned char)(0x01 | ((mux_rate & 0x7f)<<1));
    copy_timecode (SCR,&pack->SCR);
}


/*************************************************************************
	Create_Sys_Header

	向缓存中写入特定的系统信息
*************************************************************************/

void create_sys_header (sys_header, rate_bound, audio_bound, 
			fixed, CSPS, audio_lock, video_lock,
			video_bound,
			stream1, buffer1_scale, buffer1_size,
			stream2, buffer2_scale, buffer2_size,
			which_streams)

Sys_header_struc *sys_header;
unsigned int	 rate_bound;
unsigned char	 audio_bound;
unsigned char	 fixed;
unsigned char	 CSPS;
unsigned char	 audio_lock;
unsigned char	 video_lock;
unsigned char	 video_bound;

unsigned char 	 stream1;
unsigned char 	 buffer1_scale;
unsigned int 	 buffer1_size;
unsigned char 	 stream2;
unsigned char 	 buffer2_scale;
unsigned int 	 buffer2_size;
unsigned int     which_streams;

{
    unsigned char *index;

    index = sys_header->buf;


    *(index++) = (unsigned char)((SYS_HEADER_START)>>24);
    *(index++) = (unsigned char)((SYS_HEADER_START & 0x00ff0000)>>16);
    *(index++) = (unsigned char)((SYS_HEADER_START & 0x0000ff00)>>8);
    *(index++) = (unsigned char)(SYS_HEADER_START & 0x000000ff);

    if (which_streams == STREAMS_BOTH) {
	*(index++) = (unsigned char)(SYS_HEADER_LENGTH >> 8);
	*(index++) = (unsigned char)(SYS_HEADER_LENGTH & 0xff);
    } else {
	*(index++) = (unsigned char)((SYS_HEADER_LENGTH-3) >> 8);
	*(index++) = (unsigned char)((SYS_HEADER_LENGTH-3) & 0xff);
    }

    *(index++) = (unsigned char)(0x80 | (rate_bound >>15));
    *(index++) = (unsigned char)(0xff & (rate_bound >> 7));
    *(index++) = (unsigned char)(0x01 | ((rate_bound & 0x7f)<<1));
    *(index++) = (unsigned char)((audio_bound << 2)|(fixed << 1)|CSPS);
    *(index++) = (unsigned char)((audio_lock << 7)|
		 (video_lock << 6)|0x20|video_bound);

    *(index++) = (unsigned char)RESERVED_BYTE;

    if (which_streams & STREAMS_AUDIO) {
	*(index++) = stream1;
	*(index++) = (unsigned char) (0xc0 |
		     (buffer1_scale << 5) | (buffer1_size >> 8));
	*(index++) = (unsigned char) (buffer1_size & 0xff);
    }

    if (which_streams & STREAMS_VIDEO) {
	*(index++) = stream2;
	*(index++) = (unsigned char) (0xc0 |
		     (buffer2_scale << 5) | (buffer2_size >> 8));
	*(index++) = (unsigned char) (buffer2_size & 0xff);
    }

}
