#pragma once

/*=============================================
 =	解析ini模块

 +	NOTE
		1.根据每行第一个非空字符判断类型
			[	一级标题,[...]
			#	注释
			其他都为配置内容,但必须要有个一级标题

		2.配置格式为 key = value,不需要加引号
			只支持一个=,多的会被截断
				
		3.不支持中文

==============================================*/

#include <base/Base.h>
#include <fstream>

#define GET_BASE(type, ini, title, key, value, default, failreturn) {  \
	bool GGret = ini.get##type(title, key, value);                     \
	if(!GGret) {                                                       \
		value = default;                                               \
		if(failreturn) {                                               \
			return false;                                              \
		}                                                              \
	}                                                                  \
}

/**
 * 方便读取配置的宏
 * ini:parserIni
 * title,key:string
 * default:无配置则赋为默认值
 * failreturn:无配置是否直接return false
 */
#define GET_INT(ini, title, key, value, default, failreturn) \
	GET_BASE(Int, ini, title, key, value, default, failreturn)
#define GET_BOOL(ini, title, key, value, default, failreturn) \
	GET_BASE(Bool, ini, title, key, value, default, failreturn)
#define GET_STRING(ini, title, key, value, default, failreturn) \
	GET_BASE(String, ini, title, key, value, default, failreturn)

class parserIni
{
public:
	typedef std::map<std::string, std::string>	ConfigMap;
	typedef std::map<std::string, ConfigMap>	ConfigDic;

public:
	parserIni(std::string filename)
	{
		_sFilename = filename;
	};

	bool load()
	{
		_fp.open(_sFilename.c_str(), std::ios::in);
		assert(_fp);

		std::string config;
		std::string title;
		std::string key;
		std::string value;
		std::vector<std::string> list;
		while(getline(_fp,config))
		{
			trimString(config, ' ');
			if(config.empty())
				continue;

			if(config[0]=='#')
				continue;

			trimTailAllString(config, '#');
			trimString(config, '\r');
			trimString(config, '\t');

			if(config[0]=='[')
			{
				trimHeadString(config, '[');
				trimTailString(config, ']');
				title = config;
				continue;
			}

			if(title.empty())
				continue;
			
			list.clear();
			splitString(config, list, "=", 1);
			if(list.size()<2)
				continue;

			ConfigDic::iterator ittitle = _mapConfigs.find(title);
			if(ittitle == _mapConfigs.end())
			{
				ConfigMap tmp;
				_mapConfigs.insert(
					std::pair<std::string, ConfigMap>
					(title, tmp)
				);
				ittitle = _mapConfigs.find(title);
			}
			ittitle->second.insert(
				std::pair<std::string, std::string>
				(list[0], list[1])
			);
		}

		_fp.close();
		if(_mapConfigs.size()<=0)	
			return false;
		return true;
	}

	bool getString(const std::string& title, const std::string& key, std::string& value)
	{
		ConfigDic::iterator ittitle = _mapConfigs.find(title);
		if(ittitle == _mapConfigs.end())
			return false;

		ConfigMap::iterator itkey = ittitle->second.find(key);
		if(itkey == ittitle->second.end())
			return false;

		value = itkey->second;
		return true;
	}

	bool getInt(const std::string& title, const std::string& key, int& value)
	{
		std::string ret;
		if (!getString(title, key, ret))
			return false;
		if(ret.empty())
			return false;
		value = atoi(ret.c_str());
		return true;
	}

	bool getBool(const std::string& title, const std::string& key, bool& value)
	{
		std::string ret;
		if(!getString(title, key, ret))
			return false;
		if(ret.empty())
			return false;
		stringLow(ret);
		value = (ret=="true")? true : false;
		return true;
	}

private:
	std::string _sFilename;
	std::fstream _fp;
	ConfigDic _mapConfigs;
};
