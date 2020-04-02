#pragma once
class InlinePatch
{
public:
	InlinePatch();
	~InlinePatch();
	INT Patch(LPVOID lpFuncAddress, UCHAR * patch);
};

