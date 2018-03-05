#ifndef APPLICATION_H
#define APPLICATION_H

#include <windows.h>
#include <DirectXMath.h> 
#include <d3d11.h>
#include <d3dcompiler.h>
#include <d3d9types.h>
#include "DDSTextureLoader.h"

using namespace DirectX;

#define SPEED 16.f

class Application
{
private:
	HINSTANCE	g_hInstance;
	HINSTANCE	g_hPrevInstance;
	LPWSTR		g_lpCmdLine;
	int			g_nCmdShow;
	HWND		g_hwnd;
	int width;
	int height;

public:
	Application(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow, HWND wndHandle, int width, int height);
	~Application();

	bool Initialise();
	bool Update(float dt);
	void Render();

private:
	bool CreateDirect3DContext();
	bool CreateDepthBuffer();
	void SetViewport();
	bool CreateShaders();
	bool CreateConstantBuffer();
	bool CreateGBuffer();

	void CreateTexture();
	void CreateVertexBuffer();

private:
	IDXGISwapChain*	g_SwapChain;
	ID3D11Device* g_Device;
	ID3D11DeviceContext* g_DeviceContext;
	ID3D11RenderTargetView* g_RenderTargetView;

	ID3D11Buffer* g_VertexBuffer;
	ID3D11Buffer* g_QuadBuffer;

	ID3D11InputLayout* g_VertexLayout;
	ID3D11InputLayout* g_DeferredVertexLayout;

	ID3D11VertexShader* g_VertexShader;
	ID3D11VertexShader* g_DefVertexShader;

	ID3D11PixelShader* g_PixelShader;
	ID3D11PixelShader* g_DefPixelShader;
	ID3D11VertexShader* g_DeferredVertexShader;
	ID3D11PixelShader* g_DeferredPixelShader;

	ID3D11GeometryShader* g_GeometryShader;

	ID3D11PixelShader* g_PixelShaderTexture;
	ID3D11PixelShader* g_PixelShaderEverything;

	ID3D11Buffer* g_ConstantBuffer;

	ID3D11DepthStencilView* g_DepthStencilView;
	ID3D11Texture2D* g_DepthStencilBuffer;

	// G-Buffer
	ID3D11Texture2D* g_GBufferTEX[3];
	ID3D11RenderTargetView* g_GBufferRTV[3];
	ID3D11ShaderResourceView* g_GBufferSRV[3];

	ID3D11ShaderResourceView* g_ShaderResourceView;
	ID3D11SamplerState* g_SamplerState;

	struct ConstantBuffer
	{
		XMMATRIX WorldMatrix;
		XMMATRIX ViewMatrix;
		XMMATRIX ProjectionMatrix;
	};
	ConstantBuffer ObjData;

	struct VertexData
	{
		float x, y, z;
		float u, v;
		float nx, ny, nz;
	};

	float objAngle = 0.0f;
};

#endif