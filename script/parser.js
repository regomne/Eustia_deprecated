function ParseShortCmd(cmd)
{
	if(cmd=='cm1')
	{
		eval('mm1=_GetMemoryBlocks()');
	}
	else if(cmd=='cm2')
	{
		eval('mm2=_GetMemoryBlocks();rslt=getNewExecuteMemory(mm1,mm2)');
		eval('displayMemInfo(rslt.newExes)');
	}
	else if(cmd=='lf')
	{
		eval('load("g:/crack/myfunc.js")');
	}
	else if(cmd=='lf2')
	{
		eval('load("g:/crack/test.js")');
	}
	else
	{
		print("unk cmd");
	}
}