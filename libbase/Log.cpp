#include <base/Log.h>
#include <base/Base.h>
#include <base/Thread.h>

unsigned int g_loglevel = 0;

INIT_SINGLE(Log);

void Log::setConfig(std::string filename, unsigned int filesize, unsigned int loglevel, bool screenout, bool fileout)
{
	_sFilename = filename;
	_nFileSize = filesize;
	_bStdOut = screenout;
	_bFileOut = fileout;
	splitString(_sFilename, _vecPathList, PF);
	_nNowUseSize = 0;
	g_loglevel = loglevel;
	_nNowDay = getNowDay();
	_nLastDay = 0;
}

void Log::setScreenOut(bool flag)
{
	_bStdOut = flag;
}

bool Log::doStart()
{
	_bRuning = start();
	return _bRuning;
}

void Log::log(const char* buf, unsigned int len)
{
	SelfLock l(_bufLock);

	if ((length() + len) > (MAX_LOG_BUFFER_SIZE - 200))
	{
		std::cout << "[Error]Log is too much!" << std::endl;
		_lDropCount++;
		return;
	}

	memcpy(_pLogCur, buf, len);
	_pLogCur += len;
}

Log::Log() : Thread("LogThread"), _pLogCur(_data)
{
	_bRuning = false;
	_lDropCount = 0;
}

int Log::run()
{
	while (true)
	{
		if (!logFile())
		{
			_nNowDay = getNowDay();
			sleepMillisecond(200);
		}
	}
	return 0;
}

bool Log::openFile()
{
	if (!_bFileOut)
		return true;

	// 这里不需要加锁,因为其他线程只会往缓存塞数据
	// 只有本线程会操作文件
	while (_fp.is_open())
	{
		// 如果已存在文件,但是大小或日期不符合规则,则关闭文件
		if (_nNowUseSize >= _nFileSize)
		{
			_fp.close();
			break;
		}
		if (_nLastDay != _nNowDay)
		{
			_fp.close();
			break;
		}
		break;
	}

	if (!_fp.is_open())
	{
		// 创建路劲上不存在的文件夹
		if (_vecPathList.size() > 1)
		{
			std::string tmppath = "";
			for (unsigned int i = 0; i < _vecPathList.size() - 1; i++)
			{
				tmppath += _vecPathList[i];
				tmppath += PF;
				if (!pathExist(tmppath.c_str()))
					createPath(tmppath.c_str());
			}
		}

		std::string name;
		name += _sFilename;
		name += "_";
		name += std::to_string(getNowDay());
		name += "_";
		name += getNowTimeStr();
		name += ".log";
		_fp.clear();
		_fp.open(name.c_str(), std::ios::out | std::ios::trunc);
		_nLastDay = getNowDay();
		_nNowUseSize = 0;
	}
	
	if (!_fp.is_open())
		return false;
	return true;
}

bool Log::logFile()
{
	if(!_bRuning)
		return false;

	if (!openFile())
		return false;

	{
		SelfLock l(_bufLock);

		int tmpLine = length();
		if (tmpLine <= 0)
			return false;

		memcpy(_tmpData, _data, tmpLine);

		_pLogCur = _data;

		_nNowUseSize += tmpLine;
		_tmpData[tmpLine] = 0;
	}
	
	if (_bFileOut)
		_fp << _tmpData << std::flush;

	if(_bStdOut)
		std::cout<<_tmpData << std::flush;
	
	return true;
}
