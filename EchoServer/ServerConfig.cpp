#include <base/Base.h>
#include <base/Utils.h>
#include <tool/ParseIni.h>
#include "ServerConfig.h"

INIT_SINGLE(ServerConfig);

bool ServerConfig::load()
{
	parserIni ini(getModulePath() + "ServerConfig.ini");
	if (!ini.load())
		return false;

	//[log]------------------------------------------------------------------
	if (!ini.getString("log", "filename", _sLogFileName))
		_sLogFileName = "server";
	// 替换文中的路劲分隔符
	replaceAllChar(_sLogFileName, "\\", PF);
	replaceAllChar(_sLogFileName, "/", PF);

	GET_INT	(ini,	"log",	"filesize",	_nLogFileSize,	256,	false);
	GET_INT	(ini,	"log",	"loglevel",	_nLogLevel,		0,		false);
	GET_BOOL(ini,	"log",	"stdout",	_nStdOut,		true,	false);
	_nLogFileSize *= (1024 * 1024);

	//[server]------------------------------------------------------------------
	GET_INT (ini,	"server",	"serverid",			_nServerID,			0,		true);
	GET_INT	(ini,	"server",	"listenport",		_nListenPort,		0,		true);
	GET_INT	(ini,	"server",	"timeout",			_nServerTimeOut,	120,	false);
	GET_INT	(ini,	"server",	"serverthreads",	_nServerThreads,	2,		false);
	GET_INT	(ini,	"server",	"clientthreads",	_nClientThreads,	2,		false);
	GET_INT	(ini,	"server",	"maxsockets",		_nMaxSockets,		5000,	false);
	GET_INT	(ini,	"server",	"pairdealcount",	_nPairDealCount,	5000,	false);
	GET_BOOL(ini,	"server",	"daemon",			_nDaemon,			false,	false);
	
	return true;
}
