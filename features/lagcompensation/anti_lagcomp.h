#pragma once

#include "..\..\includes.hpp"
#include "..\..\sdk\structs.hpp"

class anti_lagcomp : public singleton <anti_lagcomp>
{
public:
	void createmove(ClientFrameStage_t stage);
};