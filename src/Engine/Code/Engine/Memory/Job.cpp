﻿#include "Engine/Memory/Job.hpp"

//--------------------------------------------------------------------------------------------------------------------------------------------

static int s_nextJob	= 1;

//--------------------------------------------------------------------------------------------------------------------------------------------

Job::Job()
{
	m_jobID					= s_nextJob++;
}

//--------------------------------------------------------------------------------------------------------------------------------------------

Job::~Job()
{
	
}

//--------------------------------------------------------------------------------------------------------------------------------------------