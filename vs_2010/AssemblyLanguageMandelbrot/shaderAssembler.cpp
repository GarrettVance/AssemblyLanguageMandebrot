#include "stdafx.h"
#include "AssemblyLanguageMandelbrot.h"
#include "shaderAssembler.h"

extern ManJulFractal *             g0_pFractal; 




// see page 313 of 09-VertexShaders.pdf
static const char VertexShaderASMCode[] = \
    "vs.3.0\n"
    "dcl_position v0  \n"
    "dcl_texcoord0 v1 \n"
    "dcl_position o0  \n"
    "dcl_texcoord0 o1 \n"
    "mov o0, v0      \n"
    "mov o1, v1      \n"    ;




ShaderDescriptor::ShaderDescriptor()
{
    sName = "Empty";
    sType = ST_Unknown;
    sASMSource = "Empty";
    sVertexShader = nullptr;
    sPixelShader = nullptr;
}


ShaderDescriptor::~ShaderDescriptor()
{
}


ShaderDescriptor::ShaderDescriptor(std::string pName, ShaderType pType, std::string pASMSource)
{
    sName = pName;
    sType = pType;
    sASMSource = pASMSource;
    sVertexShader = nullptr;
    sPixelShader = nullptr;

    if (pType == ST_VertexShader)
    {
        this->gvAssembleVertexShader();
    }
    else if (pType == ST_PixelShader)
    {
        this->gvAssemblePixelShader();
    }
}


std::string ShaderDescriptor::GetName()
{
    return sName;
}


IDirect3DVertexShader9 *    ShaderDescriptor::GetVertexShader()
{
    return sVertexShader;
}


IDirect3DPixelShader9 *    ShaderDescriptor::GetPixelShader()
{
    return sPixelShader;
}




HRESULT ShaderDescriptor::gvAssembleVertexShader()
{
    HRESULT hr = D3D_OK; 
    LPD3DXBUFFER VertexShaderObjectCode;
    LPD3DXBUFFER VertexShaderErrorBuffer = nullptr;

    hr = D3DXAssembleShader(
        this->sASMSource.c_str(),  // pointer to buffer containing the shader assembly language source code. 
        this->sASMSource.length(), // Length in bytes of shader assembly language source code.
        0,  // optional array of D3DXMACRO structures; 
        0,  // optional ID3DXInclude interface pointer; 
        D3DXSHADER_USE_LEGACY_D3DX9_31_DLL,   // compile options (see D3DXSHADER flags);
        &VertexShaderObjectCode,  // the binary object code as a value of type LPD3DXBUFFER*;
        &VertexShaderErrorBuffer   // optional buffer of errors as a value of type LPD3DXBUFFER*;
    );
    
    if (VertexShaderErrorBuffer)
    {
        FILE *file;
        errno_t retVal = fopen_s(&file, "debugVertexShader.log", "a");
        char *p = (char*)VertexShaderErrorBuffer->GetBufferPointer();
        fprintf (file,"\n%s error : %s \n", p );
        fclose (file);
    }

    if(FAILED(hr))
    {
        std::cout << "FAIL AssembleShader for Vertex Shader" << std::endl;
    }
    else
    {
        hr = g0_pFractal->GetD3D9Device()->CreateVertexShader(
            (DWORD *)VertexShaderObjectCode->GetBufferPointer(), 
            &(this->sVertexShader)
        );

        VertexShaderObjectCode->Release();

        if(FAILED(hr))
        {
            std::cout << "FAIL CreateVertexShader" << std::endl;
        }
    }
    return hr;
}






HRESULT ShaderDescriptor::gvAssemblePixelShader()
{
    HRESULT hr = D3D_OK;
    LPD3DXBUFFER PixelShaderObjectCode;
    LPD3DXBUFFER PixelShaderErrorBuffer = nullptr;

    hr = D3DXAssembleShader(
        this->sASMSource.c_str(),  // pointer to buffer containing the shader assembly language source code. 
        this->sASMSource.length(), // Length in bytes of shader assembly language source code.
        0,  // optional array of D3DXMACRO structures; 
        0,  // optional ID3DXInclude interface pointer; 
        D3DXSHADER_USE_LEGACY_D3DX9_31_DLL,   // compile options (see D3DXSHADER flags);
        &PixelShaderObjectCode,  // the binary object code as a value of type LPD3DXBUFFER*;
        &PixelShaderErrorBuffer   // optional buffer of errors as a value of type LPD3DXBUFFER*;
    );

    if (PixelShaderErrorBuffer)
    {
        FILE *file;
        errno_t retVal = fopen_s(&file, "debugPixelShader.log", "a");
        char *p = (char*)PixelShaderErrorBuffer->GetBufferPointer();
        fprintf (file,"\n%s error : %s \n", p );
        fclose (file);
    }

    if(FAILED(hr))
    {
        std::cout << "FAIL AssembleShader for Pixel Shader" << std::endl;
        throw;
    }
    else
    {
        hr = g0_pFractal->GetD3D9Device()->CreatePixelShader(
            (DWORD*)PixelShaderObjectCode->GetBufferPointer(), 
            &(this->sPixelShader)
        );

        PixelShaderObjectCode->Release();

        if(FAILED(hr))
        {
            std::cout << "FAIL CreatePixelShader" << std::endl;
            throw;
        }
    }
    return hr;
}



ShaderManager::ShaderManager()
{
    sShaders = new std::vector<ShaderDescriptor *>();
}


ShaderManager::~ShaderManager()
{
}


void ShaderManager::AddShader(ShaderDescriptor * pSD)
{
    sShaders->push_back(pSD);
}


IDirect3DVertexShader9 *    ShaderManager::GetVertexShader(std::string pName)
{
    IDirect3DVertexShader9 *                    ret = nullptr;
    std::vector<ShaderDescriptor *>::iterator   iter; 

    for(iter = this->sShaders->begin(); iter != this->sShaders->end();  iter++)
    {
        if((*iter)->GetName().compare(pName) == 0)
        {
            ret = (*iter)->GetVertexShader();
        }
    }
    return ret;
}


IDirect3DPixelShader9 *    ShaderManager::GetPixelShader(std::string pName)
{
    IDirect3DPixelShader9 *                    ret = nullptr;
    std::vector<ShaderDescriptor *>::iterator   iter; 

    for(iter = this->sShaders->begin(); iter != this->sShaders->end();  iter++)
    {
        if((*iter)->GetName().compare(pName) == 0)
        {
            ret = (*iter)->GetPixelShader();
        }
    }
    return ret;
}


void ShaderManager::ListAll()
{
    std::vector<ShaderDescriptor *>::iterator   iter; 
        
    std::cout << std::endl; 

    for(iter = this->sShaders->begin(); iter != this->sShaders->end();  iter++)
    {
        std::cout << (*iter)->GetName() << std::endl;
    }

    std::cout << std::endl; 
}











HRESULT ManJulFractal::CreateShaders()
{
    HRESULT                 resPS = D3D_OK; 
    ShaderDescriptor *      ptrSD = nullptr;


    std::string ASM0 =  std::string("ps.3.0 "); 
    ASM0.append("def c0, 2, -1366, -768, 0.00520833349          ");
    ASM0.append("def c1, 10, 0, 1, 0                            "); 
    ASM0.append("def c2, 0.159154937, 0.5, 6.28318548, -3.14159274      "); 
    ASM0.append("dcl vPos.xy                        "); 
    ASM0.append("mad r0.xy, vPos, c0.x, c0.yzzw     "); 
    ASM0.append("mul r0.xy, r0, c0.w                "); 
    ASM0.append("mul r0.z, r0_abs.y, r0_abs.x       "); 
    ASM0.append("mad r0.z, r0.z, c2.x, c2.y         "); 
    ASM0.append("frc r0.z, r0.z                     "); 
    ASM0.append("mad r0.z, r0.z, c2.z, c2.w         "); 
    ASM0.append("sincos r1.x, r0.z                  "); 
    ASM0.append("mul r0.z, r1.x, c1.x               "); 
    ASM0.append("lrp r1.x, r0.z, r0_abs.y, r0_abs.x "); 
    ASM0.append("add oC0.x, -r1_abs.x, c1.x         "); 
    ASM0.append("mov oC0.yzw, c1.xyyz               ");
    ptrSD = new ShaderDescriptor("DebugPixelShader", ST_PixelShader, ASM0);
    g0_pFractal->GetShaderManager()->AddShader(ptrSD);





    // ghv: Normalize the texture coords to color values in [0, 1]: 

    std::string ASM1 =  std::string("ps.3.0             \n"); 
    ASM1.append("def c0, 3.0, 3.0, 3.0, 3.0                     \n");
    ASM1.append("def c1, 0.1667, 0.1667, 0.1667, 0.1667         \n");
    ASM1.append("dcl_texcoord0        v0.xy                    \n");
    ASM1.append("dcl_2d     s0                                  \n");
    ASM1.append("//                                             \n");
    ASM1.append("texld  r0, v0, s0                            \n");
    ASM1.append("//                                             \n");
    ASM1.append("add r0, r0, c0                                 \n");
    ASM1.append("//                                             \n");
    ASM1.append("mul  r0, r0, c1                                \n");
    ASM1.append("//                                             \n");
    ASM1.append("mov     oC0, r0                                \n");
    ptrSD = new ShaderDescriptor("NormalizedColorPixelShader", ST_PixelShader, ASM1);
    g0_pFractal->GetShaderManager()->AddShader(ptrSD);





    // ghv: REVERSE the Texture Coordinates: 
    // Normalize the texture coords to color values in [1, 0]: 
    //
    // ghv: Change the law from y = (x + 3) / ( 2 * bounds)
    // to
    // y = -x / (2 * bounds) + 0.5;
    // This can be written 
    // y = -0.1667x + 0.5;

    std::string ASM2 =  std::string("ps.3.0             \n"); 
    ASM2.append("def c0, 0.5, 0.5, 0.5, 0.5                     \n");
    ASM2.append("def c1, 0.1667, 0.1667, 0.1667, 0.1667         \n");
    ASM2.append("dcl_texcoord0        v0.xy                    \n");
    ASM2.append("dcl_2d     s0                                  \n");
    ASM2.append("//                                             \n");
    ASM2.append("texld  r0, v0, s0                            \n");
    ASM2.append("//                                             \n");
    ASM2.append("mul  r0, r0, -c1                                \n");
    ASM2.append("//                                             \n");
    ASM2.append("add r0, r0, c0                                 \n");
    ASM2.append("//                                             \n");
    ASM2.append("mov     oC0, r0                                \n");
    ptrSD = new ShaderDescriptor("NormalizedReverseColorPixelShader", ST_PixelShader, ASM2);
    g0_pFractal->GetShaderManager()->AddShader(ptrSD);
    



    // ghv: Pixel Shader for RTT Render To Texture mode: 

    std::string ASM3 =  std::string("ps.3.0             \n"); 
    ASM3.append("def c0, 0.0, 0.0, 3.0, 3.0                     \n");  // color blue;
    ASM3.append("def c1, 3.0, 0.0, 0.0, 3.0                     \n");  // color red;
    ASM3.append("def c2, -1.0, 0.1667, 0.1667, 0.1667         \n");
    ASM3.append("dcl_texcoord0        v0.xy                    \n");
    ASM3.append("dcl_2d     s0                                  \n");
    ASM3.append("//                                             \n");
    ASM3.append("texld  r0, v0, s0                            \n");
    ASM3.append("//                                             \n");
    ASM3.append("if_lt r0.x, c2.x                         \n");
    ASM3.append("mov r0,  c0                              \n");
    ASM3.append("else                              \n");
    ASM3.append("mov r0,  c1                              \n");
    ASM3.append("endif                              \n");
    ASM3.append("//                                             \n");
    ASM3.append("mov     oC0, r0                                \n");
    ptrSD = new ShaderDescriptor("RTTPixelShader", ST_PixelShader, ASM3);
    g0_pFractal->GetShaderManager()->AddShader(ptrSD);



    // ghv: Mandelbrot iterations:

    // Comments:
    // Looping is not used for the iterations since it adds a small overhead.

    // This shader computes several iterations of the equation that generates the Mandelbrot set.
    // The floating point texture data read from s1 and s2 are treated as complex numbers (Z) and each
    // iteration computes Z' = Z^2 + C to generate the fractal.  Applying that computation over and over
    // causes each point (the coordinate stored at each pixel) to fly around in an orbit that will either
    // decay to zero or fly away to infinity.  If the point flies away to a certain distance (stored as 
    // distance^2 in c1.x) the iteration stops and the number of iterations required to reach that distance
    // is stored in the .z component of the output.  To visualize the fractal, a separate pixel shader reads
    // the iteration count and uses it to generate a color ramp.

    // The Z' = Z^2 + C operation on complex numbers Z = x + i*y is:
    // Z' = x*x + i*y*i*y + 2*x*i*y + cx + cy*i
    // For complex numbers, the imaginary part i = sqrt(-1), so i*i = -1, and the formula becomes:
    // Z' = x*x - y*y + 2*x*i*y + cx + cy*i
    // Separating into real (x) and imaginary (y) parts:
    // x' = x*x - y*y + cx
    // y' = 2*x*y + cy

    // Texture s0 contains the floating point starting coordinates for the fractal iteration.
    // Texture s1 contains the current location of the point as it iterates through it's orbit. 

    std::string ASM5 = std::string("ps.3.0                                                                                      \n");
    ASM5.append("def c0, 2.0f, 1.0, 1.0, 0.0                                                                                    \n");
    ASM5.append("def c1, 5.0f, 0.0, 0.0, 0.0                                                                                  \n");
    ASM5.append("def c2, 1.0f, -1.0f, 1.0f, 0.0                                                                                 \n");
    ASM5.append("dcl_2d s0              // const offset for z^2+c                                                               \n");
    ASM5.append("dcl_2d s1              // current orbit point                                                                  \n");
    ASM5.append("dcl_texcoord0  v0.xyzw                                                                                         \n");
    ASM5.append("texld  r0, v0, s0      // constant offset                                                                      \n");
    ASM5.append("texld  r1, v0, s1      // orbit point                                                                          \n");
    ASM5.append("mul        r2,    r1.xyxx, r1.xyxy                                                                             \n");
    ASM5.append("mad        r2.xz, r2.y, c2.ywx, r2.x   // x = x*x - y*y, z = x*x + y*y                                         \n");
    ASM5.append("if_lt r2.z, c1.x                           // if not escaped                                                   \n");
    ASM5.append("   add     r2.y,  r2.w, r2.w               // 2*x*y                                                            \n");
    ASM5.append("   add     r2.xy, r2.xy, r0.xy             // x += cx, y += cy                                                 \n");
    ASM5.append("   mad     r2.zw, r1.z, c0.wwyw, c0.wwyw   // increment r1.z (the iteration count), and set w = 0              \n");
    ASM5.append("   //-- another iteration                                                                                      \n");
    ASM5.append("   mul     r1,    r2.xyxx, r2.xyxy                                                                             \n");
    ASM5.append("   mad     r1.xz, r1.y, c2.ywx, r1.x       // x = x*x - y*y, z = x*x + y*y                                     \n");
    ASM5.append("   if_lt r1.z, c1.x                            // if not escaped                                               \n");
    ASM5.append("       add     r1.y,  r1.w, r1.w               // 2*x*y                                                        \n");
    ASM5.append("       add     r1.xy, r1.xy, r0.xy             // x += cx, y += cy                                             \n");
    ASM5.append("       mad     r1.zw, r2.z, c0.wwyw, c0.wwyw   // increment r1.z (the iteration count), and set w = 0          \n");
    ASM5.append("                                                                                                               \n");
    ASM5.append("       //-- another iteration                                                                                  \n");
    ASM5.append("       mul     r2,    r1.xyxx, r1.xyxy                                                                         \n");
    ASM5.append("       mad     r2.xz, r2.y, c2.ywx, r2.x   // x = x*x - y*y, z = x*x + y*y                                     \n");
    ASM5.append("       if_lt r2.z, c1.x                            // if not escaped                                           \n");
    ASM5.append("           add     r2.y,  r2.w, r2.w               // 2*x*y                                                    \n");
    ASM5.append("           add     r2.xy, r2.xy, r0.xy             // x += cx, y += cy                                         \n");
    ASM5.append("           mad     r2.zw, r1.z, c0.wwyw, c0.wwyw   // increment r1.z (the iteration count), and set w = 0      \n");
    ASM5.append("                                                                                                               \n");
    ASM5.append("           //-- another iteration                                                                              \n");
    ASM5.append("           mul     r1,    r2.xyxx, r2.xyxy                                                                     \n");
    ASM5.append("           mad     r1.xz, r1.y, c2.ywx, r1.x       // x = x*x - y*y, z = x*x + y*y                             \n");
    ASM5.append("           if_lt r1.z, c1.x                            // if not escaped                                       \n");
    ASM5.append("               add     r1.y,  r1.w, r1.w               // 2*x*y                                                \n");
    ASM5.append("               add     r1.xy, r1.xy, r0.xy             // x += cx, y += cy                                     \n");
    ASM5.append("               mad     r1.zw, r2.z, c0.wwyw, c0.wwyw   // increment r1.z (the iteration count), and set w = 0  \n");
    ASM5.append("                                                                                                               \n");
    ASM5.append("               //-- another iteration                                                                          \n");
    ASM5.append("               mul     r2,    r1.xyxx, r1.xyxy                                                                 \n");
    ASM5.append("               mad     r2.xz, r2.y, c2.ywx, r2.x   // x = x*x - y*y, z = x*x + y*y                             \n");
    ASM5.append("               if_lt r2.z, c1.x                            // if not escaped                                   \n");
    ASM5.append("                   add     r2.y,  r2.w, r2.w               // 2*x*y                                            \n");
    ASM5.append("                   add     r2.xy, r2.xy, r0.xy             // x += cx, y += cy                                 \n");
    ASM5.append("                   mad     r2.zw, r1.z, c0.wwyw, c0.wwyw   // increment r1.z (the iteration count), and set w = 0                      \n");
    ASM5.append("               else                                                                                                                    \n");
    ASM5.append("                   mad     r2, r1, c0.yyyw, c0.wwwz    // If escaped, copy the point coordinate and iteration count, and set .w to 1   \n");
    ASM5.append("               endif                                                                                                                   \n");
    ASM5.append("           else                                                                                                                        \n");
    ASM5.append("               mad     r2, r2, c0.yyyw, c0.wwwz    // If escaped, copy the point coordinate and iteration count, and set .w to 1       \n");
    ASM5.append("           endif                                                                                                                       \n");
    ASM5.append("       else                                                                                                                            \n");
    ASM5.append("           mad     r2, r1, c0.yyyw, c0.wwwz    // If escaped, copy the point coordinate and iteration count, and set .w to 1           \n");
    ASM5.append("       endif                                                                                                                           \n");
    ASM5.append("   else                                                                                                                                \n");
    ASM5.append("       mad     r2, r2, c0.yyyw, c0.wwwz    // If escaped, copy the point coordinate and iteration count, and set .w to 1               \n");
    ASM5.append("   endif                                                                                                                               \n");
    ASM5.append("else                                                                                                                                   \n");
    ASM5.append("   mad     r2, r1, c0.yyyw, c0.wwwz    // If escaped, copy the point coordinate and iteration count, and set .w to 1                   \n");
    ASM5.append("endif                                                                                                                                  \n");
    ASM5.append("                                                                                                                                       \n");
    ASM5.append("mov oC0, r2                                                                                                                            \n");
    ptrSD = new ShaderDescriptor("MandelbrotIteratorPixelShader", ST_PixelShader, ASM5);
    g0_pFractal->GetShaderManager()->AddShader(ptrSD);


    // ghv: Visualize the Mandelbrot Set:

    std::string ASM6 =  std::string("ps.2.x             \n"); 
    ASM6.append("//                                             \n");
    ASM6.append("dcl        t0.xy                             \n");
    ASM6.append("dcl_2d     s0                                  \n");
    ASM6.append("//                                             \n");
    ASM6.append("//                                             \n");
    ASM6.append("texld  r0, t0, s0                              \n");
    ASM6.append("//                                             \n");
    ASM6.append("mul    r0, r0, r0.w                            \n");
    ASM6.append("//                                             \n");
    ASM6.append("mul    r0, r0.z, c1                            \n");
    ASM6.append("//                                             \n");
    ASM6.append("mov     oC0, r0                                \n");
    ptrSD = new ShaderDescriptor("MandelbrotVisualizerPixelShader", ST_PixelShader, ASM6);
    g0_pFractal->GetShaderManager()->AddShader(ptrSD);







    std::string ASM7 = std::string("ps.3.0                                                                                      \n");
    ASM7.append("def c0, 2.0f, 1.0, 1.0, 0.0                                                                                    \n");
    ASM7.append("def c1, 500.0f, 0.0, 0.0, 0.0                                                                                  \n");
    ASM7.append("def c2, 1.0f, -1.0f, 1.0f, 0.0                                                                                 \n");
    ASM7.append("                                                                                                               \n");
    ASM7.append("dcl_2d s0              // const offset for z^2+c                                                               \n");
    ASM7.append("dcl_2d s1              // current orbit point                                                                  \n");
    ASM7.append("dcl_texcoord0  v0.xyzw                                                                                         \n");
    ASM7.append("                                                                                                               \n");
    ASM7.append("texld  r0, v0, s0      // constant offset                                                                      \n");
    ASM7.append("texld  r1, v0, s1      // orbit point                                                                          \n");
    ASM7.append("                                                                                                               \n");
    ASM7.append("mul        r2,    r1.xyxx, r1.xyxy                                                                             \n");
    ASM7.append("mad        r2.xz, r2.y, c2.ywx, r2.x   // x = x*x - y*y, z = x*x + y*y                                         \n");
    ASM7.append("                                                                                                               \n");
    ASM7.append("if_lt r2.z, c1.x                                                                                               \n");
    ASM7.append("   add     r2.y,  r2.w, r2.w               // 2*x*y                                                            \n");
    ASM7.append("   add     r2.xy, r2.xy, c3.xy             // x += cx, y += cy                                                 \n");
    ASM7.append("   mad     r2.zw, r1.z, c0.wwyw, c0.wwyw   // increment r1.z (the iteration count), and set w = 0              \n");
    ASM7.append("                                                                                                               \n");
    ASM7.append("   //-- another iteration                                                                                      \n");
    ASM7.append("   mul     r1,    r2.xyxx, r2.xyxy                                                                             \n");
    ASM7.append("   mad     r1.xz, r1.y, c2.ywx, r1.x       // x = x*x - y*y, z = x*x + y*y                                     \n");
    ASM7.append("   if_lt r1.z, c1.x                            // if not escaped                                               \n");
    ASM7.append("       add     r1.y,  r1.w, r1.w               // 2*x*y                                                        \n");
    ASM7.append("       add     r1.xy, r1.xy, c3.xy             // x += cx, y += cy                                             \n");
    ASM7.append("       mad     r1.zw, r2.z, c0.wwyw, c0.wwyw   // increment r1.z (the iteration count), and set w = 0          \n");
    ASM7.append("                   \n");
    ASM7.append("       //-- another iteration          \n");
    ASM7.append("       mul     r2,    r1.xyxx, r1.xyxy             \n");
    ASM7.append("       mad     r2.xz, r2.y, c2.ywx, r2.x       // x = x*x - y*y, z = x*x + y*y             \n");
    ASM7.append("       if_lt r2.z, c1.x                            // if not escaped           \n");
    ASM7.append("           add     r2.y,  r2.w, r2.w               // 2*x*y            \n");
    ASM7.append("           add     r2.xy, r2.xy, c3.xy             // x += cx, y += cy             \n");
    ASM7.append("           mad     r2.zw, r1.z, c0.wwyw, c0.wwyw   // increment r1.z (the iteration count), and set w = 0          \n");
    ASM7.append("                       \n");
    ASM7.append("           //-- another iteration          \n");
    ASM7.append("           mul     r1,    r2.xyxx, r2.xyxy             \n");
    ASM7.append("           mad     r1.xz, r1.y, c2.ywx, r1.x       // x = x*x - y*y, z = x*x + y*y             \n");
    ASM7.append("           if_lt r1.z, c1.x                            // if not escaped           \n");
    ASM7.append("               add     r1.y,  r1.w, r1.w               // 2*x*y            \n");
    ASM7.append("               add     r1.xy, r1.xy, c3.xy             // x += cx, y += cy             \n");
    ASM7.append("               mad     r1.zw, r2.z, c0.wwyw, c0.wwyw   // increment r1.z (the iteration count), and set w = 0          \n");
    ASM7.append("                           \n");
    ASM7.append("               //-- another iteration          \n");
    ASM7.append("               mul     r2,    r1.xyxx, r1.xyxy             \n");
    ASM7.append("               mad     r2.xz, r2.y, c2.ywx, r2.x       // x = x*x - y*y, z = x*x + y*y             \n");
    ASM7.append("               if_lt r2.z, c1.x                            // if not escaped           \n");
    ASM7.append("                   add     r2.y,  r2.w, r2.w               // 2*x*y            \n");
    ASM7.append("                   add     r2.xy, r2.xy, c3.xy             // x += cx, y += cy             \n");
    ASM7.append("                   mad     r2.zw, r1.z, c0.wwyw, c0.wwyw   // increment r1.z (the iteration count), and set w = 0          \n");
    ASM7.append("               else            \n");
    ASM7.append("                   mad     r2, r1, c0.yyyw, c0.wwwz    // If escaped, copy the point coordinate and iteration count, and set .w to 1           \n");
    ASM7.append("               endif                                   \n");
    ASM7.append("           else            \n");
    ASM7.append("               mad     r2, r2, c0.yyyw, c0.wwwz    // If escaped, copy the point coordinate and iteration count, and set .w to 1           \n");
    ASM7.append("           endif           \n");
    ASM7.append("       else            \n");
    ASM7.append("           mad     r2, r1, c0.yyyw, c0.wwwz    // If escaped, copy the point coordinate and iteration count, and set .w to 1           \n");
    ASM7.append("       endif                   \n");
    ASM7.append("   else            \n");
    ASM7.append("       mad     r2, r2, c0.yyyw, c0.wwwz    // If escaped, copy the point coordinate and iteration count, and set .w to 1           \n");
    ASM7.append("   endif           \n");
    ASM7.append("else           \n");
    ASM7.append("   mad     r2, r1, c0.yyyw, c0.wwwz    // If escaped, copy the point coordinate and iteration count, and set .w to 1           \n");
    ASM7.append("endif          \n");
    ASM7.append("           \n");
    ASM7.append("mov oC0, r2            \n");
    ptrSD = new ShaderDescriptor("JuliaIteratorPixelShader", ST_PixelShader, ASM7);
    g0_pFractal->GetShaderManager()->AddShader(ptrSD);




    // ghv: Visualize 2:

    std::string ASM8 =  std::string("ps.2.x             \n"); 
    ASM8.append("def    c0, 0.5f, 0.0f, 0.0f, 0.7f              \n");
    ASM8.append("//                                             \n");
    ASM8.append("dcl        t0.xyzw                             \n");
    ASM8.append("dcl_2d     s0                                  \n");
    ASM8.append("//                                             \n");
    ASM8.append("//                                             \n");
    ASM8.append("texld  r0, t0, s0                              \n");
    ASM8.append("//                                             \n");
    ASM8.append("mul    r0, r0, r0.w                            \n");
    ASM8.append("mul    r1, r0.z, c1                            \n");
    ASM8.append("//                                             \n");
    ASM8.append("mul    r0.x, r0.x, r0.y                        \n");
    ASM8.append("if_lt  r0.x, c0.y                              \n");
    ASM8.append("   mul    r1, r1, c0.w                         \n");
    ASM8.append("endif                                          \n");
    ASM8.append("//                                             \n");
    ASM8.append("mov     oC0, r1                                \n");
    ptrSD = new ShaderDescriptor("OrbitVisualizerPixelShader", ST_PixelShader, ASM8);
    g0_pFractal->GetShaderManager()->AddShader(ptrSD);


    g0_pFractal->GetShaderManager()->ListAll();

    return resPS;
}







