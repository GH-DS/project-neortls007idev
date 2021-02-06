#pragma once

//--------------------------------------------------------------------------------------------------------------------------------------------
		
//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
//	REFER LINK - https://www.3dgep.com/learning-directx-12-1/#Fence - Theory
//				 https://docs.microsoft.com/en-us/windows/win32/api/d3d12/nn-d3d12-id3d12fence - Official Docs
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

class FenceDX12
{
public:
	FenceDX12() {};
	~FenceDX12() {};

	bool IsFenceComplete();					// Check to see if the fence's completed value has been reached.
	void WaitForFenceValue( int value );	// Stall the CPU thread until the fence value has been reached.
	void Signal();							// Insert a fence value into the command queue. The fence used to signal the command queue will have it's completed value set when that value is reached in the command queue.
	void Render();							//  Render a frame. Do not move on to the next frame until that frame's previous fence value has been reached.
};

//--------------------------------------------------------------------------------------------------------------------------------------------