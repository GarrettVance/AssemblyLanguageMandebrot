#pragma once
#include "resource.h"
#include "shaderAssembler.h"
#include "StepTimer.h"


class RTTPair
{
public:
    RTTPair();
    ~RTTPair();
    void                        GetSurfaceLevel();
    IDirect3DTexture9 *         GetTexturePointer();
    IDirect3DSurface9 *         GetSurfacePointer();
private:
    IDirect3DTexture9 *         rttTexture; 
    IDirect3DSurface9 *         rttSurface; 
};


LRESULT WINAPI              WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


class FRECT
{
public:
    float left;
    float right;
    float top;
    float bottom;
    FRECT() {};
    FRECT( float _left, float _top, float _right, float _bottom )
    {
        left = _left; right = _right; top = _top, bottom = _bottom;     
    };
};



enum FractalType
{
    MANDELBROT,
    JULIA
};





class ManJulFractal
{
public:
    ManJulFractal();
    ~ManJulFractal();
    LPDIRECT3DDEVICE9               GetD3D9Device();
    void                            InitializeWindowAndDevice(HINSTANCE hInstance, int nCmdShow);
    void                            CreateInitialValuesTexture();
    HRESULT                         SetInitialValuesTexture();
    HRESULT                         CreateShaders();
    ShaderManager *                 GetShaderManager();
    void                            CreateVertexBuffer();
    void                            CreateRTTTargets();
    void                            RTT_RenderToTexture(DX::StepTimer const& pTimer);
    void                            RenderByMode();
    void                            Cleanup();
    HRESULT                         SwitchFractals();
    HRESULT                         ZoomIntoRect(POINT pt);
    void                            GetColorScaleMinMax(float * pMin, float * pMax);
    void                            SetColorScaleBasedOnViewAreaWidth();
    HRESULT                         TranslatePercent(float xpercent, float ypercent);
    HRESULT                         ZoomOut(float zoompercent);
    void                            SetInitialView();
    void                            ResetIterations();
    void                            ColorScaleIncrease();
    void                            ColorScaleDecrease();
    void                            ColorScaleShow();

private:
    HWND                            hWnd;
    LPDIRECT3D9                     g0_direct3D;
    LPDIRECT3DDEVICE9               direct3DDevice;
    IDirect3DTexture9*              initXYTexture; 
    IDirect3DVertexBuffer9*         g0_VertexBuffer;
    ShaderManager                   g0_ShaderManager;
    UINT                            g0_curr_PS_Id; 
    DX::StepTimer                   m_timer;
    IDirect3DSurface9 *             m_OrigBackbufferColorTarget; 
    RTTPair *                       m_OrbitTargets[2];
    FractalType                     m_eFractalType;
    FRECT                           m_frMandelbrotView;     // region of space over which to compute the set
    FRECT                           m_frJuliaView;          // region of space over which to compute Julia set
    FRECT *                         m_pControlledRect;      // Either m_frJuliaView or m_frMandelbrotView
    bool                            m_bShowOrbitDest;
    float                           m_fColorScale;
    float                           m_dJuliaX;
    float                           m_dJuliaY;
};


