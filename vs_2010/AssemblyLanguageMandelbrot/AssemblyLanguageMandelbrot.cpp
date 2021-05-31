#include "stdafx.h"
#include "AssemblyLanguageMandelbrot.h"

    
ManJulFractal *             g0_pFractal = nullptr;


const DWORD D3DFVF_TLVERTEX = D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1;



struct TLVERTEX
{
    float x, y, z;
    float rhw;
    D3DCOLOR color;
    float u;
    float v;
};



UINT gvGetDefaultSize()
{
    UINT edgeLength = 800;
    return edgeLength;
}




using std::wstring;

void AlertFailure(HRESULT hr, wstring messageString)
{
        std::wstring errorString = DXGetErrorString(hr);
        std::wstring errorDesc = DXGetErrorDescription(hr);
        messageString += errorString;
        messageString += L"\n";
        messageString += errorDesc;
        MessageBox(0, messageString.c_str(), L"Error!", MB_OK);             
        DebugBreak();
}











RTTPair::RTTPair()
{
    HRESULT hr = D3DXCreateTexture( 
        g0_pFractal->GetD3D9Device(),            
        gvGetDefaultSize(), 
        gvGetDefaultSize(), 
        1,  // MipLevels = 1:  THIS WAS THE KEY!!! Using zero for MipLevels broke everything!!! Use 1!!!
        D3DUSAGE_RENDERTARGET,      
        D3DFMT_A32B32G32R32F,   // Format;
        D3DPOOL_DEFAULT,        // Pool;
        &(this->rttTexture)
    );

    if(FAILED(hr)) { AlertFailure(hr, L"create texture for RTT texture"); }
}

RTTPair::~RTTPair()
{
    if(rttSurface) rttSurface->Release();

    if(rttTexture) rttTexture->Release();
}

void RTTPair::GetSurfaceLevel()
{
    HRESULT hr = this->rttTexture->GetSurfaceLevel(
            0, 
            &(this->rttSurface)
    );

    if(FAILED(hr)) { AlertFailure(hr, L"GetSurfaceLevel for rtt surface"); }
}
    
IDirect3DTexture9 *     RTTPair::GetTexturePointer()
{
    return this->rttTexture;
}

IDirect3DSurface9 *     RTTPair::GetSurfacePointer()
{
    return this->rttSurface;
}










ManJulFractal::ManJulFractal() : 
    g0_direct3D(nullptr), 
    direct3DDevice(nullptr), 
    initXYTexture(nullptr), 
    g0_VertexBuffer(nullptr), 
    g0_curr_PS_Id(0), 
    m_OrigBackbufferColorTarget(nullptr)
{
    m_pControlledRect = &m_frMandelbrotView;
    m_eFractalType = MANDELBROT;
    m_fColorScale = 0.046031f;
    m_dJuliaX = 0.0;
    m_dJuliaY = 0.0;
    m_bShowOrbitDest = false;
}
   





 

ManJulFractal::~ManJulFractal()
{
}









ShaderManager * ManJulFractal::GetShaderManager()
{
    return &(this->g0_ShaderManager);
}








LPDIRECT3DDEVICE9 ManJulFractal::GetD3D9Device()
{
    return direct3DDevice;
}










void ManJulFractal::InitializeWindowAndDevice(HINSTANCE hInstance, int nCmdShow)
{
    WNDCLASSEX wc = 
    {
        sizeof(WNDCLASSEX), 
        CS_VREDRAW | CS_HREDRAW | CS_OWNDC, 
        WndProc, 
        0, 
        0, 
        hInstance, 
        NULL, 
        NULL, 
        (HBRUSH)(COLOR_WINDOW + 1), 
        NULL, 
        L"DX9_CLASS", 
        NULL
    };
    
    if(!RegisterClassEx(&wc))
    {
        MessageBox(0, L"RegisterClassEx failed", L"Error!", MB_OK);
        throw;
    }

    int edge = gvGetDefaultSize();
    RECT rc = { 0, 0, static_cast<LONG>(edge), static_cast<LONG>(edge) };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    hWnd = CreateWindowW(
        L"DX9_CLASS", 
        L"DirectX 9 RTT (Render-to-texture)",
        WS_OVERLAPPEDWINDOW, 
        128, 
        128, 
        rc.right - rc.left,
        rc.bottom - rc.top, 
        NULL, 
        NULL, 
        hInstance, 
        NULL
    );

    if(!hWnd)
    {
        MessageBox(0, L"RegisterClassEx failed", L"Error!", MB_OK);
        throw;
    }

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    // =================================================================
    // =================================================================
    
    g0_direct3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (!g0_direct3D)
    {
        MessageBox(0, L"Direct3DCreate9 failed", L"Error!", MB_OK);
        throw;
    }
    
    D3DPRESENT_PARAMETERS PresentParams;
    memset(&PresentParams, 0, sizeof(D3DPRESENT_PARAMETERS));

    PresentParams.Windowed = true;
    PresentParams.SwapEffect = D3DSWAPEFFECT_DISCARD;

    HRESULT hr = g0_direct3D->CreateDevice(
        D3DADAPTER_DEFAULT, 
        D3DDEVTYPE_HAL, 
        hWnd, 
        D3DCREATE_HARDWARE_VERTEXPROCESSING,  // TODO: pure...
        &PresentParams, 
        &direct3DDevice
    );

    if (FAILED(hr))
    {
        AlertFailure(hr, L"FAIL CreateDevice() \n");
        throw;
    }

    D3DCAPS9  pCaps;

    hr = g0_direct3D->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &pCaps);

    if (FAILED(hr))
    {
        AlertFailure(hr, L"FAIL GetDeviceCaps() \n");
        throw;
    }

    // ps.3.0 must be supported: 

    if(pCaps.PixelShaderVersion < D3DPS_VERSION(3,0))
    {
        AlertFailure(E_FAIL, L"FAIL: no ps.3.0 support.  \n");
        throw;
    }

    printf("\nVerified ps.3.0 support.\n");

    // fp32 render targets must be supported:

    UINT                    u_Adapter = pCaps.AdapterOrdinal;
    D3DDISPLAYMODE          Mode;
    D3DFORMAT               AdapterFormat = D3DFMT_UNKNOWN;

    hr = g0_direct3D->GetAdapterDisplayMode(u_Adapter, &Mode);
   
    if (FAILED(hr))
    {
        AlertFailure(hr, L"FAIL GetAdapterDisplayMode() \n");
        throw;
    }

    AdapterFormat = Mode.Format;

    hr = g0_direct3D->CheckDeviceFormat(
        u_Adapter, 
        pCaps.DeviceType, 
        AdapterFormat, 
        D3DUSAGE_RENDERTARGET, 
        D3DRTYPE_TEXTURE, 
        D3DFMT_A32B32G32R32F
    );

    if(FAILED(hr))
    {
        AlertFailure(hr, L"FAIL: no support for fp32 render targets.  \n");
        throw;
    }

    printf("\nVerified support for fp32 render targets.\n");
}









void ManJulFractal::CreateInitialValuesTexture()
{
    HRESULT hr = S_OK;

    hr = D3DXCreateTexture(
        this->direct3DDevice, 
        gvGetDefaultSize(), 
        gvGetDefaultSize(), 
        1,  // MipLevels
        D3DUSAGE_DYNAMIC,      
        D3DFMT_A32B32G32R32F,   // Format;
        D3DPOOL_DEFAULT,        // Pool;
        &(this->initXYTexture)
    );

    if(FAILED(hr))
    {
        AlertFailure(hr, L"FAIL CreateTexture for Initial Values texture.\n");
        throw; 
    }
}














HRESULT ManJulFractal::SetInitialValuesTexture()
{
    HRESULT hr = S_OK;

    if(initXYTexture == nullptr)
    { 
        hr = E_FAIL;
        AlertFailure(hr, L"FAIL Initial Values texture is nullptr. \n");
        return hr;
    }

    D3DLOCKED_RECT lockedr;
    hr = initXYTexture->LockRect(0, &lockedr, NULL, D3DLOCK_DISCARD); // D3DLOCK_DISCARD only for dynamic textures; 
    if(FAILED(hr)) 
    { 
        AlertFailure(hr, L"FAIL LockRect() for Initial Values texture. \n");
        return hr;
    }

    UINT vpHeight = 0;
    UINT vpWidth = 0;
    D3DSURFACE_DESC desc;
    initXYTexture->GetLevelDesc(0, &desc);
    vpHeight = desc.Height;
    vpWidth = desc.Width;

    FRECT *             pR;
    if(m_eFractalType == JULIA)
    {
        pR = &m_frJuliaView;
    }
    else
    {
        pR = &m_frMandelbrotView;
    }

    float               fx = 0.00f;
    float               fy = 0.00f;
    D3DXVECTOR4 *       pVec4;

    for(UINT iy = 0; iy < vpHeight; iy++)
    {
        pVec4 = (D3DXVECTOR4*)((BYTE*)(lockedr.pBits) + lockedr.Pitch * iy);
        fy = pR->bottom + iy * (pR->top - pR->bottom) / (vpHeight - 1.0f);

        for(UINT ix = 0; ix < vpWidth; ix++)
        {
            fx = pR->left + ix * (pR->right - pR->left) / (vpWidth - 1.0f);
            *pVec4 = D3DXVECTOR4(fx, fy, 0.0f, 1.0f);
            pVec4++;
        }
    }
    initXYTexture->UnlockRect( 0 );
    return hr;
}







void ManJulFractal::CreateVertexBuffer()
{
    direct3DDevice->SetVertexShader(NULL);
    direct3DDevice->SetFVF(D3DFVF_TLVERTEX);

    HRESULT hr = direct3DDevice->CreateVertexBuffer(
        sizeof(TLVERTEX) * 4, 
        NULL, 
        D3DFVF_TLVERTEX, 
        D3DPOOL_MANAGED, 
        &(this->g0_VertexBuffer), 
        NULL
    );

    if(FAILED(hr))
    {
        AlertFailure(hr, L"FAIL Create Vertex Buffer. \n");
        throw; 
    }

    direct3DDevice->SetStreamSource(0, g0_VertexBuffer, 0, sizeof(TLVERTEX));

    // =================================================================
    // =================================================================
    //          Enumerate the Vertices
    //
    // The size of the win32 window's client rect can be obtained 
    // either from GetClientRect() or GetViewport(), 
    // and should agree with the values returned by gvGetDefaultSize(). 
    // =================================================================

    D3DCOLOR vertexColour = D3DCOLOR_ARGB(255, 255, 255, 255);

    UINT edge = gvGetDefaultSize(); // <------- TODO: change to viewport

    // D3DVIEWPORT9 viewport;
    // direct3DDevice->GetViewport( &viewport );
    // RECT clientR;
    // GetClientRect(hWnd, &clientR);

    float meelg = 0.5f;

    UINT fieldLeft = 0; 
    UINT fieldTop = 0; 
    RECT rDest = {0};
    rDest.left = fieldLeft;
    rDest.top = fieldTop;
    rDest.bottom = fieldTop + edge;
    rDest.right = fieldLeft + edge;
    
    TLVERTEX* vertices;
    
    this->g0_VertexBuffer->Lock(0, 0, (void**)&vertices, NULL); // Lock the vertex buffer; 
    
    vertices[0].color = vertexColour;
    vertices[0].x = (float) rDest.left - meelg;
    vertices[0].y = (float) rDest.top - meelg;
    vertices[0].z = 0.0f;
    vertices[0].rhw = 1.0f;
    vertices[0].u = 0.0f;
    vertices[0].v = 0.0f;
    
    vertices[1].color = vertexColour;
    vertices[1].x = (float) rDest.right - meelg;
    vertices[1].y = (float) rDest.top - meelg;
    vertices[1].z = 0.0f;
    vertices[1].rhw = 1.0f;
    vertices[1].u = 1.0f;
    vertices[1].v = 0.0f;
    
    vertices[2].color = vertexColour;
    vertices[2].x = (float) rDest.right - meelg;
    vertices[2].y = (float) rDest.bottom - meelg;
    vertices[2].z = 0.0f;
    vertices[2].rhw = 1.0f;
    vertices[2].u = 1.0f;
    vertices[2].v = 1.0f;
    
    vertices[3].color = vertexColour;
    vertices[3].x = (float) rDest.left - meelg;
    vertices[3].y = (float) rDest.bottom - meelg;
    vertices[3].z = 0.0f;
    vertices[3].rhw = 1.0f;
    vertices[3].u = 0.0f;
    vertices[3].v = 1.0f;
    
    this->g0_VertexBuffer->Unlock(); // Unlock the vertex buffer; 

    return; 
}













void ManJulFractal::CreateRTTTargets()
{
    // Create the RTT target texture/surface pairs: 

    m_OrbitTargets[0] = new RTTPair();
    m_OrbitTargets[1] = new RTTPair();
}















void ManJulFractal::RTT_RenderToTexture(DX::StepTimer const& pTimer)
{
    static double gEla = 0.0;
    gEla += pTimer.GetElapsedSeconds();
    if(gEla < 0.3)
    {
        return;
    }
    else
    {
        gEla = 0.0;
    }

    HRESULT hr = S_OK;


    if (m_OrigBackbufferColorTarget == nullptr)
    {
        // Save the original render target for later: 
        hr = direct3DDevice->GetRenderTarget(
            0,                                  //  RenderTargetIndex  
            &m_OrigBackbufferColorTarget        // IDirect3DSurface9 **ppRenderTarget
        );
        if(FAILED(hr)) { AlertFailure(hr, L"FAIL GetRenderTarget for original rt\n"); }
    }

    // The RTT phase: read from initXYTexture and write to rttTexture: 

    direct3DDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
    direct3DDevice->BeginScene();


    if (m_eFractalType == JULIA)
    {
        hr = direct3DDevice->SetPixelShader(g0_ShaderManager.GetPixelShader("JuliaIteratorPixelShader"));
        if(FAILED(hr)) { AlertFailure(hr, L"FAIL SetPixelShader JULIA\n"); }
        direct3DDevice->SetPixelShaderConstantF(3, D3DXVECTOR4(m_dJuliaX, m_dJuliaY, 0.0f, 0.0f), 1); // Set position of Julia point;
    }
    else
    {
        hr = direct3DDevice->SetPixelShader(g0_ShaderManager.GetPixelShader("MandelbrotIteratorPixelShader"));
        if(FAILED(hr)) { AlertFailure(hr, L"FAIL SetPixelShader MANDELBROT\n"); }
    }

    IDirect3DTexture9 *         pSrcTexture = nullptr;
    IDirect3DSurface9 *         pDestSurf = nullptr;
    RTTPair *                   rtDest = nullptr;
    int                         maxLoops = 11; // or use 11; 

    for (int kIdx = 0; kIdx < maxLoops; kIdx++)
    {
        if (kIdx == 0)
        {
            pSrcTexture = initXYTexture;
        }
        else
        {
            pSrcTexture = rtDest->GetTexturePointer();
        }

        direct3DDevice->SetTexture(0, initXYTexture);   // constant offset for z^2+c
        direct3DDevice->SetTexture(1, pSrcTexture);     // state of the simulation

        int kAlter = (kIdx % 2 == 0) ? 0 : 1; // for ping-pong between targets
        rtDest = m_OrbitTargets[kAlter];
        rtDest->GetSurfaceLevel();
        pDestSurf = rtDest->GetSurfacePointer();
        hr = direct3DDevice->SetRenderTarget(0, pDestSurf);
        if(FAILED(hr)) { AlertFailure(hr, L"SetRenderTarget to rttSurface"); }

        // optional:  hr = direct3DDevice->SetDepthStencilSurface(NULL);

        direct3DDevice->DrawPrimitive (D3DPT_TRIANGLEFAN, 0, 2);
    }

    // =================================================================
    // 
    // The usual rendering phase: read from rttTexture and write to the screen:
    //  

    direct3DDevice->SetTexture (0, rtDest->GetTexturePointer());
    // optional: direct3DDevice->SetTexture (1, NULL);

    hr = direct3DDevice->SetRenderTarget(0, m_OrigBackbufferColorTarget); // Revert the render target back to the screen;
    if(FAILED(hr)) { AlertFailure(hr, L"SetRenderTarget back to original rt"); }

    // optional:  hr = direct3DDevice->SetDepthStencilSurface();

    if(m_bShowOrbitDest)
    {
        hr = direct3DDevice->SetPixelShader(g0_ShaderManager.GetPixelShader("OrbitVisualizerPixelShader"));
        if(FAILED(hr)) { AlertFailure(hr, L"FAIL SetPixelShader OrbitVisualizerPixelShader\n"); }
    }
    else
    {
        hr = direct3DDevice->SetPixelShader(g0_ShaderManager.GetPixelShader("MandelbrotVisualizerPixelShader"));
        if(FAILED(hr)) { AlertFailure(hr, L"FAIL SetPixelShader MandelbrotVisualizerPixelShader\n"); }
    }

    // 
    // Set the constant color multiplier in Pixel Shader Constant Register 1: 
    // (remember to set alpha channel to 1)...
    // (a good value of maxiter is 64.f): float maxiter = 44.f;
    // 
    float maxiter = 44.f;

    if(m_eFractalType == JULIA)
    {
        maxiter = m_fColorScale * 300.0f;
    }
    else
    {
        maxiter = m_fColorScale * 2300.0f;
    }

    // red:  direct3DDevice->SetPixelShaderConstantF(1, D3DXVECTOR4( 1.0f/maxiter, 0.0f, 0.0f, 1.0f ), 1 );
    // ice:  direct3DDevice->SetPixelShaderConstantF(1, D3DXVECTOR4( 1.0f/maxiter, 2.0f/maxiter, 3.0f/maxiter, 1.0f ), 1 );
    direct3DDevice->SetPixelShaderConstantF(1, D3DXVECTOR4( 1.0f/maxiter, 2.0f/maxiter, 8.0f/maxiter, 1.0f ), 1 );

    direct3DDevice->DrawPrimitive (D3DPT_TRIANGLEFAN, 0, 2);

    direct3DDevice->EndScene();

    direct3DDevice->Present(NULL, NULL, NULL, NULL);
}








void ManJulFractal::RenderByMode()
{
    this->m_timer.Tick([&]()
    {
        this->RTT_RenderToTexture(m_timer);
    });
}








void ManJulFractal::Cleanup()
{
    if (g0_VertexBuffer)
    {
        g0_VertexBuffer->Release();
    }
    if (initXYTexture)
    {
        initXYTexture->Release();
    }
    if (direct3DDevice)
    {
        direct3DDevice->Release();
    }
    if (g0_direct3D)
    {
        g0_direct3D->Release();
    }
}



// =====================================================================
// =====================================================================
// =====================================================================



HRESULT ManJulFractal::SwitchFractals()
{
    HRESULT hr = S_OK;

    if(m_eFractalType == MANDELBROT)
    {
        m_eFractalType = JULIA;
        m_pControlledRect = &m_frJuliaView;
        SetInitialView();
    }
    else        
    {
        m_eFractalType = MANDELBROT;
        m_pControlledRect = &m_frMandelbrotView;
    }

    hr = SetInitialValuesTexture();

    if(FAILED(hr))
    {
        AlertFailure(hr, L"FAIL to initialize target.\n"); 
    }

    ResetIterations();

    SetColorScaleBasedOnViewAreaWidth();

    return hr;
}














void ManJulFractal::ResetIterations()
{
    ; 

    // m_uIterationCount = 0;
    // m_bSingleStep = true;
    // m_uCurrentOrbitSrc = 0;
    // m_uCurrentOrbitTarget = 1;
}




















HRESULT ManJulFractal::ZoomIntoRect(POINT pt)
{
    // POINT in window coords

    HRESULT hr = S_OK;

    float x,y;


    D3DSURFACE_DESC desc;
    initXYTexture->GetLevelDesc(0, &desc);

    x = ((float)pt.x) / ((float)desc.Width);
    y = ((float)pt.y) / ((float)desc.Height);

    printf("\ncursor pos: %f %f\n", x, y);
        

    FRECT * pR;
    switch( m_eFractalType )
    {
    case MANDELBROT :
        pR = &m_frMandelbrotView;
        break;
    case JULIA : 
        pR = &m_frJuliaView;
        break;
    }

    float wx, wy;
    wx = pR->right  - pR->left;
    wy = pR->top    - pR->bottom;
    x = pR->left    + x * wx;
    y = pR->bottom  + y * wy;
    if( m_eFractalType != JULIA )
    {
        // Set Julia set points based on where user clicked on Mandelbrot fractal
        m_dJuliaX = x;
        m_dJuliaY = y;
    }

    printf("view space pos: %f %f\n", x, y);

    // zoom factor
    wx = wx / 1.5f;
    wy = wy / 1.5f;
    pR->right   = x + wx/2.0f;
    pR->left    = x - wx/2.0f;
    pR->top     = y + wy/2.0f;
    pR->bottom  = y - wy/2.0f;

    printf("new view space: xmin= %f  xmax= %f  ymin= %f  ymax= %f\n", 
            m_frMandelbrotView.left, m_frMandelbrotView.right, 
            m_frMandelbrotView.bottom, m_frMandelbrotView.top);

    hr = SetInitialValuesTexture();

    if(FAILED(hr))
    {
        AlertFailure(hr, L"FAIL to initialize target.\n"); 
    }

    ResetIterations();

    SetColorScaleBasedOnViewAreaWidth();

    return hr;
}











void ManJulFractal::GetColorScaleMinMax( float * pMin, float * pMax )
{
    if( pMin != NULL )
        *pMin = 0.005f;
    if( pMax != NULL )
        *pMax = 2.5f;
}




















// Set the color scale factor based on how much we're zoomed in:
// Uses a simple table and interpolates between values

void ManJulFractal::SetColorScaleBasedOnViewAreaWidth()
{
    if(m_pControlledRect == NULL)
    {
        return;
    }


    float wx;

    wx = m_pControlledRect->right - m_pControlledRect->left;


    // Width        Color Scale
    // 2.4f         0.046031f
    // 0.08251f     0.146018f
    // 0.001798f    0.500249f
    // 0.000025f    3.99602f


    float aws[4][2] = 
    { 
        // { 0.000025f, 3.99602f },
        { 0.000025f, 2.0f },
        { 0.001798f, 0.500249f },
        { 0.08251f,  0.146018f },
        { 2.4f,      0.046031f } 
    };

    int i;
    float sc = 0.0f;


    if( wx >= aws[3][0] )
        sc = aws[3][1];
    else if( wx <= aws[0][0] )
        sc = aws[0][1];
    else
    {
        float loww, highw, lowsc, highsc;
        for( i=0; i < 4; i++ )
        {
            if( wx > aws[i][0] )
            {
                loww = aws[i][0];
                lowsc = aws[i][1];
            }
            if( wx < aws[3-i][0] )
            {
                highw = aws[3-i][0];
                highsc = aws[3-i][1];
            }
        }
        // interpolate
        float interp;
        interp = ( wx - loww ) / ( highw - loww );
        sc = interp * ( highsc - lowsc ) + lowsc;
    }


    if( m_eFractalType == JULIA)
    {
        sc = sc * 4.0f;
    }

    m_fColorScale = sc;

    //  FMsg("m_fColorScale = %f\n", m_fColorScale );
}
















HRESULT ManJulFractal::TranslatePercent( float xpercent, float ypercent )
{
    HRESULT hr = S_OK;
    FRECT * pR;

    if( m_eFractalType == JULIA )
    {
        pR = &m_frJuliaView;
    }
    else
    {
        pR = &m_frMandelbrotView;
    }

    float wx, wy;
    wx = pR->right - pR->left;
    wy = pR->top - pR->bottom;

    float tx, ty;
    tx = wx * xpercent;
    ty = wy * ypercent;

    pR->right       += tx;
    pR->left        += tx;
    pR->top         += ty;
    pR->bottom      += ty;

    hr = SetInitialValuesTexture();
    
    if(FAILED(hr))
    {
        AlertFailure(hr, L"FAIL to initialize target.\n"); 
    }

    ResetIterations();

    return hr;
}










// zoompercent = 1.0f will double the view region

HRESULT ManJulFractal::ZoomOut( float zoompercent )
{
    HRESULT hr = S_OK;
    float wx, wy;

    FRECT * pR;

    if( m_eFractalType == JULIA )
        pR = &m_frJuliaView;
    else
        pR = &m_frMandelbrotView;


    wx = pR->right - pR->left;
    wy = pR->top - pR->bottom;


    float tx, ty;

    tx = wx * zoompercent/2.0f;
    ty = wy * zoompercent/2.0f;


    pR->right       += tx;
    pR->left        -= tx;
    pR->top         += ty;
    pR->bottom      -= ty;

    hr = SetInitialValuesTexture();
    
    if(FAILED(hr))
    {
        AlertFailure(hr, L"FAIL to initialize target.\n"); 
    }

    ResetIterations();

    SetColorScaleBasedOnViewAreaWidth();

    return hr;
}








void    ManJulFractal::SetInitialView()
{
    HRESULT             hr= S_OK;
    float               bounds = 1.2f;
    float               dx;
    FRECT *             pR;

    if( m_eFractalType == JULIA)
    {
        pR = &m_frJuliaView;
        dx = 0.0f;
    }
    else
    {
        pR = &m_frMandelbrotView;
        dx = 0.7f;
    }

    pR->bottom  = -bounds;
    pR->top     = bounds;
    pR->left    = -bounds;
    pR->right   = bounds;

    pR->left    -= dx;
    pR->right   -= dx;

    hr = SetInitialValuesTexture();
    
    if(FAILED(hr))
    {
        AlertFailure(hr, L"FAIL to initialize target.\n"); 
    }

    ResetIterations();

    SetColorScaleBasedOnViewAreaWidth();
}











void    ManJulFractal::ColorScaleIncrease()
{
    float               csf = 1.08f;
    m_fColorScale *= csf;
    printf("m_fColorScale = %f\n", m_fColorScale);
}




void    ManJulFractal::ColorScaleDecrease()
{
    float               csf = 1.08f;
    m_fColorScale /= csf;
    printf("m_fColorScale = %f\n", m_fColorScale);
}




void    ManJulFractal::ColorScaleShow()
{
    float fWidth = m_frMandelbrotView.right - m_frMandelbrotView.left;
    printf("m_fColorScale = %f      region width = %f\n", g0_pFractal->m_fColorScale, fWidth);
}












int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShow)
{
    AllocConsole();

    HANDLE handle_out = GetStdHandle(STD_OUTPUT_HANDLE);
    int hCrt = _open_osfhandle((long) handle_out, _O_TEXT);
    FILE* hf_out = _fdopen(hCrt, "w");
    setvbuf(hf_out, NULL, _IONBF, 1);
    *stdout = *hf_out;

    HANDLE handle_in = GetStdHandle(STD_INPUT_HANDLE);
    hCrt = _open_osfhandle((long) handle_in, _O_TEXT);
    FILE* hf_in = _fdopen(hCrt, "r");
    setvbuf(hf_in, NULL, _IONBF, 128);
    *stdin = *hf_in;

    g0_pFractal = new ManJulFractal;

    g0_pFractal->InitializeWindowAndDevice(hInstance, nShow);
    g0_pFractal->CreateInitialValuesTexture();
    g0_pFractal->SetInitialView();

    g0_pFractal->CreateShaders();
    g0_pFractal->CreateVertexBuffer();
    g0_pFractal->CreateRTTTargets();

    MSG msg = {0};
    while(msg.message != WM_QUIT)
    {
        if(PeekMessage( &msg, 0, 0, 0, PM_REMOVE ))
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
        else
        {   
            g0_pFractal->RenderByMode();
        }
    }

    g0_pFractal->Cleanup();
    return 0;
}










LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    POINT               mouse_pos;
    float               trans = 0.1f;

    switch (msg)
    {
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            break;
        }
        case WM_LBUTTONDOWN:
        {
            SetCapture(hWnd);
            GetCursorPos(&mouse_pos);
            ScreenToClient(hWnd, &mouse_pos);
            g0_pFractal->ZoomIntoRect(mouse_pos);
            break;
        }
        case WM_LBUTTONUP:
        {
            ReleaseCapture();
            break;
        }
        case WM_RBUTTONUP:
        {
            g0_pFractal->ZoomOut(0.35f);
            break;
        }
        case WM_KEYUP:
        {
            switch(wParam)
            {
                case 'J':
                g0_pFractal->SwitchFractals();
                break;
            }
            break;
        }
        case WM_KEYDOWN:
        {
            switch(wParam)
            {
                case VK_ESCAPE:
                    PostQuitMessage(0);
                    break;
                case VK_LEFT:
                    g0_pFractal->TranslatePercent(-trans, 0.0f);
                    break;
                case VK_RIGHT:
                    g0_pFractal->TranslatePercent(trans, 0.0f);
                    break;
                case VK_UP:
                    g0_pFractal->TranslatePercent(0.0f, -trans);
                    break;
                case VK_DOWN:           // down arrow
                    g0_pFractal->TranslatePercent(0.0f, trans);
                    break;
                case VK_PRIOR:          // pageup
                    g0_pFractal->ZoomOut(-0.35f);
                    break;
                case VK_NEXT:           // pagedown
                    g0_pFractal->ZoomOut(0.35f);
                    break;
                case 'X':
                    g0_pFractal->ColorScaleIncrease();   // means *= csf;
                    break;
                case 'Z':
                    g0_pFractal->ColorScaleDecrease();   // means  /= csf;
                    break;
                case 'D':
                    g0_pFractal->ColorScaleShow(); 
                    break;
            }
            break;
        }
        default:
        {
            break;
        }
        break;
    }
    return (DefWindowProc(hWnd, msg, wParam, lParam));
}


