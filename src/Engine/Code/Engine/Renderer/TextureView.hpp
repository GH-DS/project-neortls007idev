#pragma once

//--------------------------------------------------------------------------------------------------------------------------------------------

struct ID3D12RenderTargetView;
struct ID3D12ShaderResourceView;
struct ID3D12DepthStencilView;
struct ID3D12Resource;
struct ID3D12UnorderedAccessView;

class  Texture;

//--------------------------------------------------------------------------------------------------------------------------------------------

class TextureView
{
public:
	TextureView();
	~TextureView();
	ID3D12RenderTargetView*		 GetRTVHandle() const							{ return m_rtv;  }
	ID3D12ShaderResourceView*	 GetSRVHandle() const							{ return m_srv; }
	ID3D12DepthStencilView*		 GetDSVHandle() const							{ return m_dsv; }
	ID3D12UnorderedAccessView*	 GetUAVHandle() const							{ return m_uav; }

public:
	Texture* m_owner; 

      union {
         ID3D12Resource*			m_handle;			// A01
         ID3D12ShaderResourceView*	m_srv;				// A03 - what is bound to a shader stage for reading
         ID3D12RenderTargetView*	m_rtv;				// A01 - alias as an rtv
		 ID3D12DepthStencilView*	m_dsv;
      	 ID3D12UnorderedAccessView* m_uav;
      }; 

private:

};