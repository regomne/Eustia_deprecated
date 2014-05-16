#pragma once

#include <vector>

BOOL SuspendAllThreadExpectSelf(std::vector<int>& theadIdStack);
BOOL ResumeAllThread(std::vector<int>& threadIdStack);