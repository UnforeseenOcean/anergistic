// Copyright 2010 fail0verflow <master@fail0verflow.com>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>

#include "types.h"
#include "main.h"
#include "config.h"
#include "channel.h"

//MFC channel values.
static u32 MFC_LSA;
static u32 MFC_EAH;
static u32 MFC_EAL;
static u32 MFC_Size;
static u32 MFC_TagID;
static u32 MFC_TagMask;
static u32 MFC_TagStat;

//HW_Ringbuf
static u32 RingBuf_offset = 0;
static u32 IsRingBufReadLocked = 0;

#define MFC_GET_CMD 0x40
#define MFC_SNDSIG_CMD 0xA0

void handle_hw_cmd(u32 cmd)
{
	switch (cmd)
	{
	case HW_CMD_TYPE_Reset_Ringbuf:
		printf("Reset_RingBuf\n");
		RingBuf_offset = 0;
		break;
	case HW_CMD_TYPE_ReadLock_Ringbuf:
		printf("Lock_RingBuf\n");
		IsRingBufReadLocked = 1;
		break;
	default:
		printf("unknown command: %08x\n", cmd);
	}
}

u32 handle_hw_ringbuf_read()
{
	if (IsRingBufReadLocked != 1)
	{
		u32 value = 0;
		printf("RingBuf offset: %08x\n", RingBuf_offset);
		FILE *f = fopen("ringbuf", "rb");
		fseek(f, (RingBuf_offset * 4), SEEK_SET);
		fread(&value, 4, 1, f);
		value = _ES32(value);
		printf("RingBuf value: %08x\n", value);
		RingBuf_offset++;
		fclose(f);
		return value;
	}
	return 0;
}

void handle_mfc_command(u32 cmd)
{
	printf("Local address %08x, EA = %08x:%08x, Size=%08x, TagID=%08x, Cmd=%08x\n",
		MFC_LSA, MFC_EAH, MFC_EAL, MFC_Size, MFC_TagID, cmd);
	switch (cmd)
	{
	case MFC_GET_CMD:
		printf("MFC_GET (DMA into LS)\n");
#if 0
		{
			FILE *f = fopen("dma", "rb");
			if (!f)
				exit(1);
			fseek(f, MFC_EAL, SEEK_SET);
			if (fread(ctx->ls + MFC_LSA, 1, MFC_Size, f) != MFC_Size)
			{
				printf("read error\n");
				exit(1);
			}
			fclose(f);
		}
#endif
		break;
	default:
		printf("unknown command\n");
	}
}

void handle_mfc_tag_update(u32 tag)
{
	switch (tag)
	{
	case 0:
		MFC_TagStat = MFC_TagMask;
		break;
	default:
		printf("unknown tag update\n");
		break;
	}
}

void channel_wrch(int ch, int reg)
{
	printf("CHANNEL: wrch ch%d r%d\n", ch, reg);
	u32 r = ctx->reg[reg][0];
	
	switch (ch)
	{
	case 7: //write decrementer
		break;
		
	case 16:
		printf("MFC_LSA %08x\n", r);
		MFC_LSA = r;
		break;
	case 17:
		printf("MFC_EAH %08x\n", r);
		MFC_EAH = r;
		break;
	case 18:
		printf("MFC_EAL %08x\n", r);
		MFC_EAL = r;
		break;
	case 19:
		printf("MFC_Size %08x\n", r);
		MFC_Size = r;
		break;
	case 20:
		printf("MFC_TagID %08x\n", r);
		MFC_TagID =r ;
		break;
	case 21:
		printf("MFC_Cmd %08x\n", r);
		handle_mfc_command(r);
		break;
	case 22:
		printf("MFC_WrTagMask %08x\n", r);
		MFC_TagMask = r;
		break;
	case 23:
		printf("MFC_WrTagUpdate %08x\n", r);
		handle_mfc_tag_update(r);
		break;
	case 26:
		printf("MFC_WrListStallAck %08x\n", r);
		break;
	case 27:
		printf("MFC_RdAtomicStat %08x\n", r);
		break;
	case 64:
		printf("HW_Cmd: ", r);
		handle_hw_cmd(r);
		break;
	default:
		printf("UNKNOWN CHANNEL\n");
	}
}

void channel_rdch(int ch, int reg)
{
	printf("CHANNEL: rdch ch%d r%d\n", ch, reg);
	u32 r;
	
	r = 0;
	switch (ch)
	{
	case 8: //read decrementer
		break;
	case 24:
		r = MFC_TagStat;
		printf("MFC_RdTagStat %08x\n", r);
		break;
	case 27:
		printf("MFC_RdAtomicStat %08x\n", r);
		break;
	case 73:
		printf("HW_Read_RingBuf\n");
		r = handle_hw_ringbuf_read();
		break;
	}
	ctx->reg[reg][0] = r;
	ctx->reg[reg][1] = 0;
	ctx->reg[reg][2] = 0;
	ctx->reg[reg][3] = 0;
}

int channel_rchcnt(int ch)
{
	u32 r;
	r = 0;
	switch (ch)
	{
	case 23:
		r = 1;
		break;
	case 24:
		r = 1;
		printf("MFC_RdTagStat %08x\n", r);
		break;
	case 27:
		printf("MFC_RdAtomicStat %08x\n", r);
		break;
	default:
		printf("unknown channel %d\n", ch);
	}
	return r;
}
