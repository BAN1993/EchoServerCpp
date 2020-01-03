#pragma once

#include <protocol/ProtocolBase.h>

#define GET_TEMPLATE_BUF(buf, head, xy) {\
	bostream bos;\
	bos.attach(buf + HEADLEN, sizeof(xy));\
	bos << xy;\
	head.xyid = xy.XYID;\
	head.len = (unsigned short)bos.length() + HEADLEN;\
	bos.attach(buf, HEADLEN);\
	bos << head;}
