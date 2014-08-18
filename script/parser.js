function ParseShortCmd(cmd)
{
	if(cmd=='cm1')
	{
		eval('mm1=getMemoryBlocks()');
	}
	else if(cmd=='cm2')
	{
		eval('mm2=getMemoryBlocks();rslt=getNewExecuteMemory(mm1,mm2)');
		eval('displayMemInfo(rslt.newExes)');
	}
	else if(cmd=='lf')
	{
		eval('load("myfunc.js")');
	}
	else if(cmd=='lf2')
	{
		eval('load("test.js")');
	}
	else
	{
		print("unk cmd");
	}
}