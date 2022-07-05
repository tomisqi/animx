#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <d3d11.h>
#include <assert.h>
#include <d3dcompiler.h>

// Define the screen resolution
#define SCREEN_WIDTH  1910
#define SCREEN_HEIGHT 1080

// Global declarations
IDXGISwapChain* swapchain;             // the pointer to the swap chain interface
ID3D11Device* dev;                     // the pointer to our Direct3D device interface
ID3D11DeviceContext* devcon;           // the pointer to our Direct3D device context
ID3D11RenderTargetView* backbuffer;    // the pointer to our back buffer
ID3D11InputLayout* pLayout;            // the pointer to the input layout
ID3D11VertexShader* pVS;               // the pointer to the vertex shader
ID3D11PixelShader* pPS;                // the pointer to the pixel shader
ID3D11Buffer* pVBuffer;                // the pointer to the vertex buffer

// A struct to define a single vertex
struct VERTEX 
{ 
    float X, Y, Z; 
    float Color[4]; 
};

// Function prototypes
void InitD3D(HWND hWnd);    // sets up and initializes Direct3D
void RenderFrame();         // renders a single frame
void CleanD3D();            // closes Direct3D and releases memory
void InitGraphics();        // creates the shape to render
void InitPipeline();        // loads and prepares the shaders

// The WindowProc function prototype
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

// The entry point for any Windows program
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
    // The handle for the window, filled by a function
    HWND hWnd;

    // This struct holds information for the window class
    WNDCLASSEX wc;

    // Clear out the window class for use
    ZeroMemory(&wc, sizeof(WNDCLASSEX));

    // Fill in the struct with the needed information
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    //wc.hbrBackground = (HBRUSH)COLOR_WINDOW; // not needed
    wc.lpszClassName = L"WindowClass1";

    // Register the window class
    RegisterClassEx(&wc);

    // Calculate the size of the client area
    RECT rect = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };    // set the size, but not the position
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);    // adjust the size

    // Create the window and use the result as the handle
    hWnd = CreateWindowEx(NULL,
                          L"WindowClass1",      // name of the window class
                          L"AnimX",             // title of the window
                          WS_OVERLAPPEDWINDOW,  // window style
                          300,                  // x-position of the window
                          0,                    // y-position of the window
                          rect.right - rect.left,                 // width of the window
                          rect.bottom - rect.top,                 // height of the window
                          NULL,                 // we have no parent window, NULL
                          NULL,                 // we aren't using menus, NULL
                          hInstance,            // application handle
                          NULL);                // used with multiple windows, NULL

    // Display the window on the screen
    ShowWindow(hWnd, nCmdShow);

    // set up and initialize Direct3D
    InitD3D(hWnd);

    // This struct holds Windows event messages
    MSG msg = {0};

    // Enter the infinite message loop
    while (TRUE)
    {
        // Check to see if any messages are waiting in the queue
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            // Translate keystroke messages into the right format
            TranslateMessage(&msg);

            // Send the message to the WindowProc function
            DispatchMessage(&msg);

            // Check to see if it's time to quit
            if (msg.message == WM_QUIT)
                break;
        }
        RenderFrame();
    }

    // clean up DirectX and COM
    CleanD3D();

    // Return this part of the WM_QUIT message to Windows
    return msg.wParam;
}

// This is the main message handler for the program
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // Sort through and find what code to run for the message given
    switch (message)
    {
        // This message is read when the window is closed
    case WM_DESTROY:
        // close the application entirely
        PostQuitMessage(0);
        return 0;
    }

    // Handle any messages the switch statement didn't
    return DefWindowProc(hWnd, message, wParam, lParam);
}

// This function initializes and prepares Direct3D for use
void InitD3D(HWND hWnd)
{
    // Create a struct to hold information about the swap chain
    DXGI_SWAP_CHAIN_DESC scd;

    // Clear out the struct for use
    ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));

    // Fill the swap chain description struct
    scd.BufferCount = 1;                                    // one back buffer
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;     // use 32-bit color
    scd.BufferDesc.Width = SCREEN_WIDTH;                    // set the back buffer width
    scd.BufferDesc.Height = SCREEN_HEIGHT;                  // set the back buffer height
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;      // how swap chain is to be used
    scd.OutputWindow = hWnd;                                // the window to be used
    scd.SampleDesc.Count = 4;                               // how many multisamples
    scd.Windowed = TRUE;                                    // windowed/full-screen mode
    scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;     // allow full-screen switching

    // Create a device, device context and swap chain using the information in the scd struct
    D3D11CreateDeviceAndSwapChain(NULL,
        D3D_DRIVER_TYPE_HARDWARE,
        NULL,
        NULL,
        NULL,
        NULL,
        D3D11_SDK_VERSION,
        &scd,
        &swapchain,
        &dev,
        NULL,
        &devcon);

    // Get the address of the back buffer
    ID3D11Texture2D* pBackBuffer;
    swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);

    // Use the back buffer address to create the render target
    dev->CreateRenderTargetView(pBackBuffer, NULL, &backbuffer);
    pBackBuffer->Release();

    // Set the render target as the back buffer
    devcon->OMSetRenderTargets(1, &backbuffer, NULL);

    // Set the viewport
    D3D11_VIEWPORT viewport;
    ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));

    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = SCREEN_WIDTH;
    viewport.Height = SCREEN_HEIGHT;

    devcon->RSSetViewports(1, &viewport);

    InitPipeline();
    InitGraphics();
}

// This is the function used to render a single frame
void RenderFrame()
{
    // Clear the back buffer to dark blue
    float color[4] = { 0.0f, 0.0f, 0.3f, 1.0f };
    devcon->ClearRenderTargetView(backbuffer, color);

    // select which vertex buffer to display
    UINT stride = sizeof(VERTEX);
    UINT offset = 0;
    devcon->IASetVertexBuffers(0, 1, &pVBuffer, &stride, &offset);

    // select which primtive type we are using
    devcon->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // draw the vertex buffer to the back buffer
    devcon->Draw(3, 0);

    // Switch the back buffer and the front buffer
    swapchain->Present(0, 0);
}

// This is the function that cleans up Direct3D and COM
void CleanD3D()
{
    swapchain->SetFullscreenState(FALSE, NULL);    // switch to windowed mode

    // Close and release all existing COM objects
    pLayout->Release();
    pVS->Release();
    pPS->Release();
    pVBuffer->Release();
    swapchain->Release();
    backbuffer->Release();
    dev->Release();
    devcon->Release();
}

// This is the function that creates the shape to render
void InitGraphics()
{
    // Create a triangle using the VERTEX struct
    VERTEX OurVertices[] =
    {
        {0.0f, 0.5f, 0.0f, {1.0f, 0.0f, 0.0f, 1.0f}},
        {0.45f, -0.5, 0.0f, {0.0f, 1.0f, 0.0f, 1.0f}},
        {-0.45f, -0.5f, 0.0f, {0.0f, 0.0f, 1.0f, 1.0f} }
    };

    // Create the vertex buffer
    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));
    bd.Usage = D3D11_USAGE_DYNAMIC;                // write access access by CPU and GPU
    bd.ByteWidth = sizeof(VERTEX) * 3;             // size is the VERTEX struct * 3
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;       // use as a vertex buffer
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;    // allow CPU to write in buffer

    dev->CreateBuffer(&bd, NULL, &pVBuffer);       // create the buffer

    // Copy the vertices into the buffer
    D3D11_MAPPED_SUBRESOURCE ms;
    devcon->Map(pVBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);    // map the buffer
    memcpy(ms.pData, OurVertices, sizeof(OurVertices));                 // copy the data
    devcon->Unmap(pVBuffer, NULL);                                      // unmap the buffer
}


// This function loads and prepares the shaders
void InitPipeline()
{
    // Load and compile the two shaders
    ID3D10Blob* VS, * PS;
    D3DCompileFromFile(L"shaders.shader", 0, 0, "VShader", "vs_4_0", 0, 0, &VS, 0);
    D3DCompileFromFile(L"shaders.shader", 0, 0, "PShader", "ps_4_0", 0, 0, &PS, 0);

    // Encapsulate both shaders into shader objects
    dev->CreateVertexShader(VS->GetBufferPointer(), VS->GetBufferSize(), NULL, &pVS);
    dev->CreatePixelShader(PS->GetBufferPointer(), PS->GetBufferSize(), NULL, &pPS);

    // Set the shader objects
    devcon->VSSetShader(pVS, 0, 0);
    devcon->PSSetShader(pPS, 0, 0);

    // Create the input layout object
    D3D11_INPUT_ELEMENT_DESC ied[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    dev->CreateInputLayout(ied, 2, VS->GetBufferPointer(), VS->GetBufferSize(), &pLayout);
    devcon->IASetInputLayout(pLayout);
}