#pragma once
#include <string>
class VtOutSignalDef
{
public:
	VtOutSignalDef();
	~VtOutSignalDef();
	std::string SignalName;
	std::string SymbolCode;
	std::string StrategyName;
	std::string Desc;
};

