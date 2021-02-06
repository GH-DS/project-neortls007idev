#include "Engine/RendererDX12/D3D12Utils.hpp"


//--------------------------------------------------------------------------------------------------------------------------------------------

D3D12_COMMAND_LIST_TYPE GetD3D12CommandListType( eDX12CommandListType commandListType )
{
	switch ( commandListType )
	{
	case DX12_COMMAND_LIST_TYPE_DIRECT:
						return D3D12_COMMAND_LIST_TYPE_DIRECT;
		break;
	case DX12_COMMAND_LIST_TYPE_BUNDLE:
						return D3D12_COMMAND_LIST_TYPE_BUNDLE;
		break;
	case DX12_COMMAND_LIST_TYPE_COMPUTE:
						return D3D12_COMMAND_LIST_TYPE_COMPUTE;
		break;
	case DX12_COMMAND_LIST_TYPE_COPY:
						return D3D12_COMMAND_LIST_TYPE_COPY;
		break;
	case DX12_COMMAND_LIST_TYPE_VIDEO_DECODE:
						return D3D12_COMMAND_LIST_TYPE_VIDEO_DECODE;
		break;
	case DX12_COMMAND_LIST_TYPE_VIDEO_PROCESS:
						return D3D12_COMMAND_LIST_TYPE_VIDEO_PROCESS;
		break;
	case DX12_COMMAND_LIST_TYPE_VIDEO_ENCODE:
						return D3D12_COMMAND_LIST_TYPE_VIDEO_ENCODE;
		break;
	default:
		break;
	}
}

//--------------------------------------------------------------------------------------------------------------------------------------------