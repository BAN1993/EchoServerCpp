#pragma once

/*=============================================
 =	协议基础文件

 +	NOTE
		1.现在测试,协议还少,可以把所有协议放在这里

==============================================*/

#include <base/Base.h>
#include <protocol/Biosream.h>

#define HEADLEN 4

static const unsigned int xc_max_account_len = 64;
static const unsigned int xc_max_password_len = 64;
static const unsigned int xc_max_nickname_len = 64;
static const unsigned int xc_max_opt_data_len = 2048; // 现在每个操作数据最长2048

enum PROTOCOL_XYID
{
	XYID_HEARTBEAT				= 1, // 心跳
};

#define DEFXY(st, id)                                 \
	enum { XYID = id };                               \
	void reset() { memset(this, 0, sizeof(*this)); }  \
	st() { reset(); }

#define READXY(xy, buf, len) { \
	bistream bis;              \
	bis.attach(buf, len);      \
	bis >> xy; }

struct ProtocolHead
{
	unsigned short len;	// 协议长度，包含包头
	unsigned short xyid;

	void reset() { memset(this, 0, sizeof(*this)); }
	ProtocolHead(){ reset(); };

	friend bostream& operator<<(bostream& bs, const ProtocolHead& msg)
	{
		bs << msg.len;
		bs << msg.xyid;
		return bs;
	}

	friend bistream& operator>>(bistream& bs, ProtocolHead& msg)
	{
		msg.reset();
		bs >> msg.len;
		bs >> msg.xyid;
		return bs;
	}
};

struct Heartbeat
{
	DEFXY(Heartbeat, PROTOCOL_XYID::XYID_HEARTBEAT);

	unsigned int timestamp;
	
	friend bostream& operator<<(bostream& bs,const Heartbeat& msg)
	{
		bs << msg.timestamp;
		return bs;
	}

	friend bistream& operator>>(bistream& bs, Heartbeat& msg)
	{
		msg.reset();
		bs >> msg.timestamp;
		return bs;
	}
};
