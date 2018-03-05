#include "Application.h"
#include "bth_image.h"

Application::Application(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow, HWND wndHandle, int width, int height)
{
	g_SwapChain = nullptr;
	g_Device = nullptr;
	g_DeviceContext = nullptr;
	g_RenderTargetView = nullptr;
	g_VertexBuffer = nullptr;
	g_VertexLayout = nullptr;
	g_DeferredVertexLayout = nullptr;
	g_VertexShader = nullptr;
	g_DefVertexShader = nullptr;
	g_PixelShader = nullptr;
	g_DefPixelShader = nullptr;
	g_GeometryShader = nullptr;
	g_PixelShaderTexture = nullptr;
	g_PixelShaderEverything = nullptr;
	g_DepthStencilView = nullptr;
	g_DepthStencilBuffer = nullptr;
	g_ShaderResourceView = nullptr;
	g_SamplerState = nullptr;

	this->width = width;
	this->height = height;

	// Input matrix data
	ObjData.WorldMatrix = XMMatrixIdentity();
	ObjData.ViewMatrix = XMMatrixLookAtLH({ 0.f, 0.f, -2.f }, { 0.f, 0.f, 0.f }, { 0.f, 1.f, 0.f });
	ObjData.ProjectionMatrix = XMMatrixPerspectiveFovLH(XM_PI*0.45f, (float)width / (float)height, 0.1f, 20.f);

	g_hInstance = hInstance;
	g_hPrevInstance = hPrevInstance;
	g_lpCmdLine = lpCmdLine;
	g_nCmdShow = nCmdShow;
	g_hwnd = wndHandle;
}

Application::~Application()
{
	g_VertexLayout->Release();
	g_VertexShader->Release();
	g_RenderTargetView->Release();
	g_SwapChain->Release();
	g_Device->Release();
	g_DeviceContext->Release();
	g_DepthStencilView->Release();
	g_DepthStencilBuffer->Release();
}

/* [Initialise anropas när programmet startas. Här sker alla förberedelser för att kunna köra programmet.] */
bool Application::Initialise()
{
	bool result = true;

	HRESULT hr = CreateDirect3DContext();
	if (FAILED(hr))
		result = false;

	result = CreateDepthBuffer();
	SetViewport();
	result = CreateShaders();
	CreateVertexBuffer();
	CreateTexture();
	result = CreateConstantBuffer();
	result = CreateGBuffer();

	return result;
}

/* [Uppdaterar allt i våran scen. (Rotation, rörelser, osv)] */
bool Application::Update(float dt)
{
	objAngle += (SPEED * dt);
	ObjData.WorldMatrix = XMMatrixRotationY(objAngle);

	return true;
}

/* [Renderar våran scen i 3 steg. 
1: Kopplar våran gBuffer som RenderTarget och sätter rätt InputLayout till VertexShadern. 
2: Skapar våran pipeline och ritar ut scenen till våran gBuffer.
3: Byter RenderTarget till våran backBuffer igen och ritar ut. (data från gBuffern hämtas ut i PixelShader)] */
void Application::Render()
{
	float color[4]{ 0.f, 0.f, 1.f, 1.f };
	g_DeviceContext->ClearRenderTargetView(g_RenderTargetView, color);
	for (int i = 0; i < 3; i++)
		g_DeviceContext->ClearRenderTargetView(g_GBufferRTV[i], color);

	g_DeviceContext->ClearDepthStencilView(g_DepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// Preperation for first draw call
	g_DeviceContext->OMSetRenderTargets(3, g_GBufferRTV, g_DepthStencilView);

	g_DeviceContext->IASetInputLayout(g_DeferredVertexLayout);
	g_DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	D3D11_MAPPED_SUBRESOURCE dataPtr;
	g_DeviceContext->Map(g_ConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &dataPtr);
	memcpy(dataPtr.pData, &ObjData, sizeof(ConstantBuffer));
	g_DeviceContext->Unmap(g_ConstantBuffer, 0);

	UINT vertexSize = sizeof(float) * 8; // x, y, z, u, v, nx, ny, nz
	UINT offset = 0;
	g_DeviceContext->IASetVertexBuffers(0, 1, &g_VertexBuffer, &vertexSize, &offset);

	g_DeviceContext->VSSetShader(g_DeferredVertexShader, nullptr, 0);
	g_DeviceContext->GSSetShader(g_GeometryShader, nullptr, 0);
	g_DeviceContext->PSSetShader(g_DeferredPixelShader, nullptr, 0);
	g_DeviceContext->HSSetShader(nullptr, nullptr, 0);
	g_DeviceContext->DSSetShader(nullptr, nullptr, 0);

	g_DeviceContext->GSSetConstantBuffers(0, 1, &g_ConstantBuffer);

	g_DeviceContext->PSSetShaderResources(0, 1, &g_ShaderResourceView);
	g_DeviceContext->PSSetSamplers(0, 1, &g_SamplerState);

	g_DeviceContext->Draw(6, 0);

	// Screen Quad
	g_DeviceContext->OMSetRenderTargets(1, &g_RenderTargetView, NULL);
	

	vertexSize = sizeof(float) * 8; // x, y, z, u, v, nx, ny, nz
	g_DeviceContext->IASetVertexBuffers(0, 1, &g_QuadBuffer, &vertexSize, &offset);

	g_DeviceContext->VSSetShader(g_VertexShader, nullptr, 0);
	g_DeviceContext->PSSetShader(g_PixelShader, nullptr, 0);
	g_DeviceContext->HSSetShader(nullptr, nullptr, 0);
	g_DeviceContext->DSSetShader(nullptr, nullptr, 0);
	g_DeviceContext->GSSetShader(nullptr, nullptr, 0);

	g_DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	g_DeviceContext->IASetInputLayout(g_VertexLayout);
	
	g_DeviceContext->PSSetShaderResources(0, 3, g_GBufferSRV);
	g_DeviceContext->PSSetSamplers(0, 1, &g_SamplerState);

	g_DeviceContext->Draw(4, 0);

	g_SwapChain->Present(0, 0);

	ID3D11ShaderResourceView* clear[] = { nullptr, nullptr, nullptr };
	g_DeviceContext->PSSetShaderResources(0, 3, clear);
}

/* [Skapar g_Device, g_DeviceContext, g_SwapChain, g_RenderTargetView (backbufferRTV)] */
bool Application::CreateDirect3DContext()
{
	DXGI_SWAP_CHAIN_DESC scd;
	ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));

	// fill the swap chain description struct
	scd.BufferCount = 1;                                    // one back buffer
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;     // use 32-bit color
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;      // how swap chain is to be used
	scd.OutputWindow = g_hwnd;								// the window to be used
	scd.SampleDesc.Count = 1;                               // how many multisamples
	scd.Windowed = TRUE;

	// create a device, device context and swap chain using the information in the scd struct
	HRESULT hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, NULL, NULL,
		NULL, D3D11_SDK_VERSION, &scd, &g_SwapChain, &g_Device, NULL, &g_DeviceContext);

	if (SUCCEEDED(hr))
	{
		// get the address of the back buffer
		ID3D11Texture2D* pBackBuffer = nullptr;
		g_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
		// use the back buffer address to create the render target
		g_Device->CreateRenderTargetView(pBackBuffer, NULL, &g_RenderTargetView);
		pBackBuffer->Release();
	}
	return hr;
}

bool Application::CreateDepthBuffer()
{
	D3D11_TEXTURE2D_DESC depthStencilBufferDesc;
	depthStencilBufferDesc.Width = (float)width;
	depthStencilBufferDesc.Height = (float)height;
	depthStencilBufferDesc.MipLevels = 1;
	depthStencilBufferDesc.ArraySize = 1;
	depthStencilBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilBufferDesc.SampleDesc.Count = 1;
	depthStencilBufferDesc.SampleDesc.Quality = 0;
	depthStencilBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthStencilBufferDesc.CPUAccessFlags = 0;
	depthStencilBufferDesc.MiscFlags = 0;
	
	HRESULT hr = g_Device->CreateTexture2D(&depthStencilBufferDesc, nullptr, &g_DepthStencilBuffer);
	if (FAILED(hr))
		return false;
	
	hr = g_Device->CreateDepthStencilView(g_DepthStencilBuffer, nullptr, &g_DepthStencilView);
	if (FAILED(hr))
		return false;
	
	return true;
}

void Application::SetViewport()
{
	D3D11_VIEWPORT vp;
	vp.Width = (float)width;
	vp.Height = (float)height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	g_DeviceContext->RSSetViewports(1, &vp);
}

/* [Skapar våra shaders VertexShader(behövs denna?), DeferredVertexShader, GeometryShader, PixelShader, DeferredPixelShader] */
bool Application::CreateShaders()
{
	//---------- Vertex Shader ----------//

	ID3DBlob* pVS = nullptr;
	ID3DBlob* error = nullptr;
	HRESULT hr;

	 hr = D3DCompileFromFile( L"Vertex.hlsl", nullptr, nullptr, "VS_main", "vs_5_0", 0, 0, &pVS, &error );
	if (FAILED(hr))
		return false;

	hr = g_Device->CreateVertexShader(pVS->GetBufferPointer(), pVS->GetBufferSize(), nullptr, &g_VertexShader);
	if (FAILED(hr))
			return false;

	D3D11_INPUT_ELEMENT_DESC inputDesc1[] = {
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "NORMAL",	  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	hr = g_Device->CreateInputLayout(inputDesc1, ARRAYSIZE(inputDesc1), pVS->GetBufferPointer(), pVS->GetBufferSize(), &g_VertexLayout);
	if (FAILED(hr))
		return false;

	pVS->Release();

	//---------- Deferred Vertex Shader ----------//

	pVS = nullptr;
	error = nullptr;
	hr = D3DCompileFromFile( L"DefVertex.hlsl", nullptr, nullptr, "VS_main", "vs_5_0", 0, 0, &pVS, &error );
	if (FAILED(hr))
		return false;

	hr = g_Device->CreateVertexShader(pVS->GetBufferPointer(), pVS->GetBufferSize(), nullptr, &g_DeferredVertexShader);
	if (FAILED(hr))
		return false;

	D3D11_INPUT_ELEMENT_DESC inputDesc2[] = {
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	hr = g_Device->CreateInputLayout(inputDesc2, ARRAYSIZE(inputDesc2), pVS->GetBufferPointer(), pVS->GetBufferSize(), &g_DeferredVertexLayout);
	if (FAILED(hr))
		return false;

	pVS->Release();

	//---------- Geometry Shader ----------//

	ID3DBlob* pGS = nullptr;
	error = nullptr;
	hr = D3DCompileFromFile( L"Geometry.hlsl", nullptr, nullptr, "GS_main", "gs_5_0", 0, 0, &pGS, &error	);
	if (FAILED(hr))
		return false;

	hr = g_Device->CreateGeometryShader(pGS->GetBufferPointer(), pGS->GetBufferSize(), nullptr, &g_GeometryShader);
	if (FAILED(hr))
		return false;

	pGS->Release();

	//---------- Pixel Shader ----------//

	ID3DBlob* pPS = nullptr;
	error = nullptr;
	hr = D3DCompileFromFile(L"Pixel.hlsl", nullptr, nullptr, "PS_main", "ps_5_0", 0, 0, &pPS, &error);
	if (FAILED(hr))
		return false;

	hr = g_Device->CreatePixelShader(pPS->GetBufferPointer(), pPS->GetBufferSize(), nullptr, &g_PixelShader);

	if (FAILED(hr))
		return false;

	//---------- Deferred Pixel Shader ----------//

	pPS = nullptr;
	error = nullptr;
	hr = D3DCompileFromFile( L"DefPixel.hlsl", nullptr, nullptr, "PS_main", "ps_5_0", 0, 0, &pPS, &error );
	if (FAILED(hr))
		return false;

	hr = g_Device->CreatePixelShader(pPS->GetBufferPointer(), pPS->GetBufferSize(), nullptr, &g_DeferredPixelShader);

	if (FAILED(hr))
		return false;

	pPS->Release();

	return true;
}

/* [Skapar våran konstanta buffer med structen ConstantBuffer1 (Application.h). Här behöver vi implementera för ConstantBuffer2 också. (eller kombinera dessa till en enda struct)] */
bool Application::CreateConstantBuffer()
{
	D3D11_BUFFER_DESC cbDesc;
	ZeroMemory(&cbDesc, sizeof(D3D11_BUFFER_DESC));
	cbDesc.ByteWidth = sizeof(ConstantBuffer);
	cbDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbDesc.MiscFlags = 0;
	cbDesc.StructureByteStride = 0;
	
	g_Device->CreateBuffer(&cbDesc, nullptr, &g_ConstantBuffer);

	return true;
}

/* [Skapar våran G-Buffer som vi renderar till i Render()] */
bool Application::CreateGBuffer()
{
	D3D11_TEXTURE2D_DESC textureDesc;
	ZeroMemory(&textureDesc, sizeof(textureDesc));
	textureDesc.Width = width;
	textureDesc.Height = height;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	textureDesc.MiscFlags = 0;
	textureDesc.CPUAccessFlags = 0;

	for (int i = 0; i < 3; i++)
	{
		g_Device->CreateTexture2D(&textureDesc, NULL, &g_GBufferTEX[i]);
	}

	D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
	ZeroMemory(&renderTargetViewDesc, sizeof(renderTargetViewDesc));
	renderTargetViewDesc.Format = textureDesc.Format;
	renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	renderTargetViewDesc.Texture2D.MipSlice = 0;

	for (int i = 0; i < 3; i++)
	{
		g_Device->CreateRenderTargetView(g_GBufferTEX[i], &renderTargetViewDesc, &g_GBufferRTV[i]);
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	ZeroMemory(&shaderResourceViewDesc, sizeof(shaderResourceViewDesc));
	shaderResourceViewDesc.Format = textureDesc.Format;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.Texture2D.MipLevels = 1;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;

	for (int i = 0; i < 3; i++)
	{
		g_Device->CreateShaderResourceView(g_GBufferTEX[i], &shaderResourceViewDesc, &g_GBufferSRV[i]);
	}

	return true;
}

/* [Skapar våran texture med BTH_IMAGE.H] */
void Application::CreateTexture()
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

	g_Device->CreateTexture2D(&texDesc, &subData, &texture);
	g_Device->CreateShaderResourceView(texture, NULL, &g_ShaderResourceView);

	// Texture sampling
	D3D11_SAMPLER_DESC sampDesc = {};
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.MipLODBias = 0;
	sampDesc.MaxAnisotropy = 1;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;

	g_Device->CreateSamplerState(&sampDesc, &g_SamplerState);
}

void Application::CreateVertexBuffer()
{
	VertexData triangleVertices[6] =
	{
		-0.5f, 0.5f, 0.0f,	// v0
		0.f, 0.f,			// q0
		0.f, 0.f, -1.f,
		
		0.5f, 0.5f, 0.0f,	// v1
		1.f, 0.f,			// q1
		0.f, 0.f, -1.f,
		
		-0.5f, -0.5f, 0.0f,	// v2
		0.f, 1.f,			// q2
		0.f, 0.f, -1.f,
		
		0.5f, 0.5f, 0.0f,	// v1
		1.f, 0.f,			// q1
		0.f, 0.f, -1.f,
		
		0.5f, -0.5f, 0.0f,	// v3
		1.f, 1.f,			// q3
		0.f, 0.f, -1.f,
		
		-0.5f, -0.5f, 0.0f,	// v4
		0.f, 1.f,			// q4
		0.f, 0.f, -1.f,
	};
		
	D3D11_BUFFER_DESC bufferDesc;
	memset(&bufferDesc, 0, sizeof(bufferDesc));
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(triangleVertices);
		
	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem = triangleVertices;
	g_Device->CreateBuffer(&bufferDesc, &data, &g_VertexBuffer);

	VertexData quadVertices[6] =
	{
		-1.f, 1.f, 0.f,	// v0
		0.f, 0.f,			// q0
		0.f, 0.f, -1.f,

		1.f, 1.f, 0.0f,		// v1
		1.f, 0.f,			// q1
		0.f, 0.f, -1.f,

		-1.f, -1.f, 0.f,	// v2
		0.f, 1.f,			// q2
		0.f, 0.f, -1.f,

		1.f, -1.f, 0.f,		// v3
		1.f, 1.f,			// q3
		0.f, 0.f, -1.f,
	};
	bufferDesc.ByteWidth = sizeof(triangleVertices);

	data.pSysMem = quadVertices;
	g_Device->CreateBuffer(&bufferDesc, &data, &g_QuadBuffer);
}