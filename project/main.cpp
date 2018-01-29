#include <windows.h>
#include <DirectXMath.h> 
#include <d3d11.h>
#include <d3dcompiler.h>
#include "bth_image.h"
#include <d3d9types.h>
#include <time.h>
// Test 123 Potato
#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "d3dcompiler.lib")

using namespace DirectX;

# define M_PI           3.14159265358979323846  /* pi */

float startTime = clock();
float dt;

struct Player
{
	XMFLOAT4 camPos;
	XMFLOAT4 forward;
	XMFLOAT4 up;
	XMFLOAT4 right;
};
Player player;

float rotationX = M_PI;

//Vector3 localPos = Vector3{ 0.f, 0.f, 0.f };
//Vector3 globalPos = Vector3{ 0.f, 0.f, 0.f };

ID3D11Texture2D* gDepthStencilBuffer = nullptr;
ID3D11DepthStencilView* gDepthStencilView = nullptr;

HWND InitWindow(HINSTANCE hInstance);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

void CreatePlayer();
void Update(float dt);
HRESULT CreateDirect3DContext(HWND wndHandle);
void SetViewport();
HRESULT CreateShaders();
void CreateTexture();
void CreateTriangleData();
void CreateConstantBuffer();
void Render();

struct VS_CONSTANT_BUFFER
{
	XMMATRIX world;
	XMMATRIX view;
	XMMATRIX projection;
};
VS_CONSTANT_BUFFER VsConstData;


IDXGISwapChain* gSwapChain = nullptr;
ID3D11Device* gDevice = nullptr;
ID3D11DeviceContext* gDeviceContext = nullptr;

ID3D11RenderTargetView* gBackbufferRTV = nullptr;

ID3D11Buffer* gVertexBuffer = nullptr;
ID3D11InputLayout* gVertexLayout = nullptr;

// resources that represent shaders
ID3D11VertexShader* gVertexShader = nullptr;
ID3D11PixelShader* gPixelShader = nullptr;
ID3D11GeometryShader* gGeometryShader = nullptr;

// Texture
ID3D11ShaderResourceView* gShaderResourceView;
ID3D11SamplerState* gSamplerState;

// Used in rendering
ID3D11Buffer* gBuffer = nullptr;

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	MSG msg = { 0 };
	HWND wndHandle = InitWindow(hInstance);

	if (wndHandle)
	{
		CreateDirect3DContext(wndHandle);

		CreatePlayer();

		SetViewport();

		CreateShaders();

		CreateTexture();

		CreateTriangleData();

		CreateConstantBuffer();

		ShowWindow(wndHandle, nCmdShow);

		while (WM_QUIT != msg.message)
		{
			if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else
			{
				dt = ((float)clock() - startTime) * 0.0001f;
				startTime = (float)clock();

				Update(dt);
				Render();

				gSwapChain->Present(1, 0);
			}
		}

		gVertexBuffer->Release();

		gVertexLayout->Release();
		gVertexShader->Release();
		gPixelShader->Release();

		gShaderResourceView->Release();
		gSamplerState->Release();

		gBackbufferRTV->Release();
		gSwapChain->Release();
		gDevice->Release();
		gDeviceContext->Release();
		DestroyWindow(wndHandle);
	}

	return (int)msg.wParam;
}

void CreatePlayer()
{
	player.camPos = { 0.f, 0.f, -1.f, 1.f };
	player.forward = { 0.f, 0.f, 1.f, 1.f };
	player.up = { 0.f, 1.f, 0.f, 1.f };
	player.right = { 1.f, 0.f, 0.f, 1.f };
}

void Update(float dt)
{
	XMVECTOR xmForward = XMLoadFloat4(&player.forward);
	XMVECTOR xmUp = XMLoadFloat4(&player.up);
	XMVECTOR xmPos = XMLoadFloat4(&player.camPos);
	XMVECTOR xmRight = XMLoadFloat4(&player.right);;

	//Test for rotation
	if (GetAsyncKeyState('J') < 0)
	{
		rotationX = -10.f * dt;
		XMMATRIX rotation = XMMatrixRotationY(rotationX);
		xmForward = XMVector3TransformCoord(xmForward, rotation);
		xmUp = XMVector3TransformCoord(xmUp, rotation);
		xmRight = XMVector3Cross(xmUp, xmForward);

		XMStoreFloat4(&player.forward, xmForward);
		XMStoreFloat4(&player.right, xmRight);
		XMStoreFloat4(&player.up, xmUp);
	}

	if (GetAsyncKeyState('L') < 0)
	{
		rotationX = 10.f * dt;
		XMMATRIX rotation = XMMatrixRotationY(rotationX);
		xmForward = XMVector3TransformCoord(xmForward, rotation);
		xmUp = XMVector3TransformCoord(xmUp, rotation);
		xmRight = XMVector3Cross(xmUp, xmForward);

		XMStoreFloat4(&player.forward, xmForward);
		XMStoreFloat4(&player.right, xmRight);
		XMStoreFloat4(&player.up, xmUp);
	}

	if (GetAsyncKeyState('I') < 0)
	{
		rotationX = -10.f * dt;
		XMMATRIX rotation = XMMatrixRotationAxis(xmRight, rotationX);
		xmForward = XMVector3TransformCoord(xmForward, rotation);
		xmUp = XMVector3TransformCoord(xmUp, rotation);

		XMStoreFloat4(&player.forward, xmForward);
		XMStoreFloat4(&player.right, xmRight);
		XMStoreFloat4(&player.up, xmUp);
	}

	if (GetAsyncKeyState('K') < 0)
	{
		rotationX = 10.f * dt;
		XMMATRIX rotation = XMMatrixRotationAxis(xmRight, rotationX);
		xmForward = XMVector3TransformCoord(xmForward, rotation);
		xmUp = XMVector3TransformCoord(xmUp, rotation);
	}

	XMStoreFloat4(&player.forward, xmForward);
	XMStoreFloat4(&player.right, xmRight);
	XMStoreFloat4(&player.up, xmUp);



	if (GetAsyncKeyState('A') < 0)
		player.camPos = { 
		player.camPos.x - (player.right.x * 5 * dt),
		player.camPos.y - (player.right.y * 5 * dt),
		player.camPos.z - (player.right.z * 5 * dt),
		1.f };

	if (GetAsyncKeyState('D') < 0)
		player.camPos = {
		player.camPos.x + (player.right.x * 5 * dt),
		player.camPos.y + (player.right.y * 5 * dt),
		player.camPos.z + (player.right.z * 5 * dt),
		1.f };

	if (GetAsyncKeyState('W') < 0)
		player.camPos = {
		player.camPos.x + (player.forward.x * 5 * dt),
		player.camPos.y + (player.forward.y * 5 * dt),
		player.camPos.z + (player.forward.z * 5 * dt),
		1.f };

	if (GetAsyncKeyState('S') < 0)
		player.camPos = {
		player.camPos.x - (player.forward.x * 5 * dt),
		player.camPos.y - (player.forward.y * 5 * dt),
		player.camPos.z - (player.forward.z * 5 * dt),
		1.f };

	// EyePosition, FocusPosition, UpDirection
	VsConstData.view = XMMatrixLookAtLH(
		{ player.camPos.x, player.camPos.y, player.camPos.z },
		{ player.camPos.x + player.forward.x, player.camPos.y + player.forward.y, player.camPos.z + player.forward.z},
		{ player.up.x, player.up.y, player.up.z }
	); 
}

HWND InitWindow(HINSTANCE hInstance)
{
	WNDCLASSEX wcex = { 0 };
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.hInstance = hInstance;
	wcex.lpszClassName = L"BTH_D3D_DEMO";
	if (!RegisterClassEx(&wcex))
		return false;

	RECT rc = { 0, 0, 640, 480 };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

	HWND handle = CreateWindow(
		L"BTH_D3D_DEMO",
		L"BTH Direct3D Demo",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		rc.right - rc.left,
		rc.bottom - rc.top,
		nullptr,
		nullptr,
		hInstance,
		nullptr);

	return handle;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

HRESULT CreateDirect3DContext(HWND wndHandle)
{
	// create a struct to hold information about the swap chain
	DXGI_SWAP_CHAIN_DESC scd;

	// clear out the struct for use
	ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));

	// fill the swap chain description struct
	scd.BufferCount = 1;                                    // one back buffer
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;     // use 32-bit color
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;      // how swap chain is to be used
	scd.OutputWindow = wndHandle;                           // the window to be used
	scd.SampleDesc.Count = 4;                               // how many multisamples
	scd.Windowed = TRUE;                                    // windowed/full-screen mode

															// create a device, device context and swap chain using the information in the scd struct
	HRESULT hr = D3D11CreateDeviceAndSwapChain(NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		NULL,
		NULL,
		NULL,
		D3D11_SDK_VERSION,
		&scd,
		&gSwapChain,
		&gDevice,
		NULL,
		&gDeviceContext);

	if (SUCCEEDED(hr))
	{
		// get the address of the back buffer
		ID3D11Texture2D* pBackBuffer = nullptr;
		gSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);

		// use the back buffer address to create the render target
		gDevice->CreateRenderTargetView(pBackBuffer, NULL, &gBackbufferRTV);
		pBackBuffer->Release();

		// Depth-Stencil buffer
		D3D11_TEXTURE2D_DESC depthStencilBufferDesc;
		depthStencilBufferDesc.Width = (float)640;
		depthStencilBufferDesc.Height = (float)480;
		depthStencilBufferDesc.MipLevels = 1;
		depthStencilBufferDesc.ArraySize = 1;
		depthStencilBufferDesc.Format = DXGI_FORMAT_D32_FLOAT;
		depthStencilBufferDesc.SampleDesc.Count = 4;
		depthStencilBufferDesc.SampleDesc.Quality = 0;
		depthStencilBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		depthStencilBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		depthStencilBufferDesc.CPUAccessFlags = 0;
		depthStencilBufferDesc.MiscFlags = 0;

		hr = gDevice->CreateTexture2D(&depthStencilBufferDesc, nullptr, &gDepthStencilBuffer);
		if (FAILED(hr))
		{
			return -1;
		}

		hr = gDevice->CreateDepthStencilView(gDepthStencilBuffer, nullptr, &gDepthStencilView);
		if (FAILED(hr))
		{
			return -1;
		}

		gDeviceContext->OMSetRenderTargets(1, &gBackbufferRTV, gDepthStencilView);
	}
	return hr;
}

void SetViewport()
{
	D3D11_VIEWPORT vp;
	vp.Width = (float)640;
	vp.Height = (float)480;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	gDeviceContext->RSSetViewports(1, &vp);
}

HRESULT CreateShaders()
{
	// Binary Large OBject (BLOB), for compiled shader, and errors.
	ID3DBlob* pVS = nullptr;
	ID3DBlob* errorBlob = nullptr;

	HRESULT result = D3DCompileFromFile(
		L"Vertex.hlsl", // filename
		nullptr,		// optional macros
		nullptr,		// optional include files
		"VS_main",		// entry point
		"vs_5_0",		// shader model (target)
		D3DCOMPILE_DEBUG,	// shader compile options (DEBUGGING)
		0,				// IGNORE...DEPRECATED.
		&pVS,			// double pointer to ID3DBlob		
		&errorBlob		// pointer for Error Blob messages.
	);

	// compilation failed?
	if (FAILED(result))
	{
		if (errorBlob)
		{
			OutputDebugStringA((char*)errorBlob->GetBufferPointer());
			// release "reference" to errorBlob interface object
			errorBlob->Release();
		}
		if (pVS)
			pVS->Release();
		return result;
	}

	gDevice->CreateVertexShader(
		pVS->GetBufferPointer(),
		pVS->GetBufferSize(),
		nullptr,
		&gVertexShader
	);

	// create input layout (verified using vertex shader)
	// Press F1 in Visual Studio with the cursor over the datatype to jump
	// to the documentation online!
	// please read:
	// https://msdn.microsoft.com/en-us/library/windows/desktop/bb205117(v=vs.85).aspx
	D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
		{
			"POSITION",		// "semantic" name in shader
			0,				// "semantic" index (not used)
			DXGI_FORMAT_R32G32B32_FLOAT, // size of ONE element (3 floats)
			0,							 // input slot
			0,							 // offset of first element
			D3D11_INPUT_PER_VERTEX_DATA, // specify data PER vertex
			0							 // used for INSTANCING (ignore)
		},
		{
			"TEXCOORD", /// COLOR
			0,				// same slot as previous (same vertexBuffer)
			DXGI_FORMAT_R32G32_FLOAT,
			0,
			12,							// offset of FIRST element (after POSITION)
			D3D11_INPUT_PER_VERTEX_DATA,
			0
		},
	};

	gDevice->CreateInputLayout(inputDesc, ARRAYSIZE(inputDesc), pVS->GetBufferPointer(), pVS->GetBufferSize(), &gVertexLayout);

	// we do not need anymore this COM object, so we release it.
	pVS->Release();


	//create pixel shader
	ID3DBlob* pPS = nullptr;
	if (errorBlob) errorBlob->Release();
	errorBlob = nullptr;

	result = D3DCompileFromFile(
		L"Fragment.hlsl", // filename
		nullptr,		// optional macros
		nullptr,		// optional include files
		"PS_main",		// entry point
		"ps_5_0",		// shader model (target)
		D3DCOMPILE_DEBUG,	// shader compile options
		0,				// effect compile options
		&pPS,			// double pointer to ID3DBlob		
		&errorBlob			// pointer for Error Blob messages.
	);

	// compilation failed?
	if (FAILED(result))
	{
		if (errorBlob)
		{
			OutputDebugStringA((char*)errorBlob->GetBufferPointer());
			// release "reference" to errorBlob interface object
			errorBlob->Release();
		}
		if (pPS)
			pPS->Release();
		return result;
	}

	gDevice->CreatePixelShader(pPS->GetBufferPointer(), pPS->GetBufferSize(), nullptr, &gPixelShader);
	// we do not need anymore this COM object, so we release it.
	pPS->Release();

	//create geometry shader
	ID3DBlob* pGS = nullptr;
	D3DCompileFromFile(
		L"GeometryShader.hlsl", // filename
		nullptr,		// optional macros
		nullptr,		// optional include files
		"GS_main",		// entry point
		"gs_5_0",		// shader model (target)
		D3DCOMPILE_DEBUG,	// shader compile options
		0,				// effect compile options
		&pGS,			// double pointer to ID3DBlob
		&errorBlob		// pointer for Error Blob messages.
	);

	// compilation failed?
	if (FAILED(result))
	{
		if (errorBlob)
		{
			OutputDebugStringA((char*)errorBlob->GetBufferPointer());
			// release "reference" to errorBlob interface object
			errorBlob->Release();
		}
		if (pGS)
			pGS->Release();
		return result;
	}

	gDevice->CreateGeometryShader(pGS->GetBufferPointer(), pGS->GetBufferSize(), nullptr, &gGeometryShader);

	// we do not need anymore this COM object, so we release it.
	pGS->Release();
}

void CreateTexture()
{
	ID3D11Texture2D* texture;

	D3D11_TEXTURE2D_DESC texDesc = { 0 };
	texDesc.Width = BTH_IMAGE_WIDTH;
	texDesc.Height = BTH_IMAGE_HEIGHT;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;

	// Data from the image
	D3D11_SUBRESOURCE_DATA subData;
	subData.pSysMem = &BTH_IMAGE_DATA;
	subData.SysMemPitch = sizeof(char) * 4 * BTH_IMAGE_WIDTH;
	subData.SysMemSlicePitch = sizeof(char) * 4 * BTH_IMAGE_WIDTH * BTH_IMAGE_HEIGHT;

	gDevice->CreateTexture2D(&texDesc, &subData, &texture);
	gDevice->CreateShaderResourceView(texture, NULL, &gShaderResourceView);

	// Texture sampling
	D3D11_SAMPLER_DESC sampDesc = {};
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.MipLODBias = 1;
	sampDesc.MaxAnisotropy = 1;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;

	gDevice->CreateSamplerState(&sampDesc, &gSamplerState);

	gDeviceContext->PSSetShaderResources(0, 1, &gShaderResourceView);
	gDeviceContext->PSSetSamplers(0, 1, &gSamplerState);
}

void CreateTriangleData()
{
	struct TriangleVertex
	{
		float x, y, z;
		float u, v;
	};

	TriangleVertex triangleVertices[6] =
	{
		-0.5f, 0.5f, 0.0f,	// v0
		0.f, 0.f,			// q0

		0.5f, 0.5f, 0.0f,	// v1
		1.f, 0.f,			// q1

		-0.5f, -0.5f, 0.0f,	// v2
		0.f, 1.f,			// q2

		0.5f, 0.5f, 0.0f,	// v1
		1.f, 0.f,			// q1

		0.5f, -0.5f, 0.0f,	// v3
		1.f, 1.f,			// q3

		-0.5f, -0.5f, 0.0f,	// v4
		0.f, 1.f,			// q4



	};

	D3D11_BUFFER_DESC bufferDesc;
	memset(&bufferDesc, 0, sizeof(bufferDesc));
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(triangleVertices);

	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem = triangleVertices;
	gDevice->CreateBuffer(&bufferDesc, &data, &gVertexBuffer);
}

void CreateConstantBuffer()
{
	// buffer description
	D3D11_BUFFER_DESC cbDesc;
	cbDesc.ByteWidth = sizeof(VS_CONSTANT_BUFFER);
	cbDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbDesc.MiscFlags = 0;
	cbDesc.StructureByteStride = 0;

	// Geometry shader constant data
	VsConstData.world = 
	{
		{ 1, 0, 0, 0 },
		{ 0, 1, 0, 0 },
		{ 0, 0, 1, 0 }, 
		{ 0, 0, 0, 1 }
	};

	// EyePosition, FocusPosition, UpDirection
	VsConstData.view = XMMatrixLookAtLH(
		{ player.camPos.x, player.camPos.y, player.camPos.z },
		{ player.camPos.x + player.forward.x, player.camPos.y + player.forward.y, player.camPos.z + player.forward.z },
		{ player.up.x, player.up.y, player.up.z }
	);
	
	VsConstData.projection = XMMatrixPerspectiveFovLH(XM_PI*0.45f, (float)640 / (float)480, 0.1f, 20.f);

	gDevice->CreateBuffer(&cbDesc, nullptr, &gBuffer);
}

void Render()
{
	// clear the back buffer to a deep blue
	float clearColor[] = { 0, 0, 0, 1 };

	// Clear
	gDeviceContext->ClearRenderTargetView(gBackbufferRTV, clearColor);
	gDeviceContext->ClearDepthStencilView(gDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// pipeline
	gDeviceContext->VSSetShader(gVertexShader, nullptr, 0);
	gDeviceContext->HSSetShader(nullptr, nullptr, 0); // No hull shader
	gDeviceContext->DSSetShader(nullptr, nullptr, 0); // No domain shader
	gDeviceContext->GSSetShader(gGeometryShader, nullptr, 0); // 
	gDeviceContext->PSSetShader(gPixelShader, nullptr, 0);

	UINT vertexSize = sizeof(float) * 5; // x, y, z, u, v
	UINT offset = 0;

	// specify vertex buffert.
	gDeviceContext->IASetVertexBuffers(0, 1, &gVertexBuffer, &vertexSize, &offset);

	// specify topology
	gDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// specify how is data passed
	gDeviceContext->IASetInputLayout(gVertexLayout);

	// Mapping constant buffer
	D3D11_MAPPED_SUBRESOURCE dataPtr;
	gDeviceContext->Map(gBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &dataPtr);

	// copy memory. CPU -> GPU
	memcpy(dataPtr.pData, &VsConstData, sizeof(VS_CONSTANT_BUFFER));

	// Unmap constant buffer
	gDeviceContext->Unmap(gBuffer, 0);

	gDeviceContext->GSSetConstantBuffers(0, 1, &gBuffer);

	// issue a draw call
	gDeviceContext->Draw(6, 0);

}