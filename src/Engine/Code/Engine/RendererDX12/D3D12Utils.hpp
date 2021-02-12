#pragma once
#include "Engine/Core/CoreUtils.hpp"
#include "Engine/RendererDX12/D3D12Common.hpp"

//--------------------------------------------------------------------------------------------------------------------------------------------

enum eDX12CommandListType
{
	DX12_COMMAND_LIST_TYPE_DIRECT ,
	DX12_COMMAND_LIST_TYPE_BUNDLE ,
	DX12_COMMAND_LIST_TYPE_COMPUTE ,
	DX12_COMMAND_LIST_TYPE_COPY ,
	DX12_COMMAND_LIST_TYPE_VIDEO_DECODE ,
	DX12_COMMAND_LIST_TYPE_VIDEO_PROCESS ,
	DX12_COMMAND_LIST_TYPE_VIDEO_ENCODE
};

D3D12_COMMAND_LIST_TYPE GetD3D12CommandListType( eDX12CommandListType commandListType );


// D3D12_CULL_MODE GetD3D12CullMode( eCullMode cullMode );
// D3D12_FILL_MODE GetD3D12FillMode( eRasterStateFillMode rasterFillMode );
// 
// eCullMode		GetCullModeForD3D12CullMode( D3D12_CULL_MODE D3D12CullMode );
// eRasterStateFillMode	GetFillModeForD3D12RasterState( D3D12_FILL_MODE D3D12RasterFillMode );
// eWindingOrder	GetWindingOrderForD3D12WindingOrder( BOOL D3D12WindingOrder );
// 
// D3D12_COMPARISON_FUNC GetD3D12ComparisonFunc( eCompareOp compareFunc );
// D3D12_DEPTH_WRITE_MASK GetD3D12DepthWriteMask( bool writeOnPass );