#pragma once
#include "Engine/Renderer/RenderBuffer.hpp"

//--------------------------------------------------------------------------------------------------------------------------------------------

class IndexBuffer : public RenderBuffer
{
public:
	 IndexBuffer( RenderContext* owner , eRenderMemoryHint memHint );
	~IndexBuffer();
	void Update( uint icount , uint const* indices );
	void Update( std::vector<uint> const& indices ); // helper, calls one above

public:
};
