function ParseShortCmd(cmd)
{
	if(cmd=='cmem1')
	{
		eval('mm1=_GetMemoryBlocks()');
	}
	else if(cmd=='cmem2')
	{
		eval('mm2=_GetMemoryBlocks();rslt=getNewExecuteMemory(mm1,mm2)');
		eval('displayObject(rslt)');
	}
	else
	{
		print("unk cmd");
	}
}