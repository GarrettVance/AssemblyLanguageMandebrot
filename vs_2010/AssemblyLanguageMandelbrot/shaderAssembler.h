#pragma once

enum ShaderType 
{
    ST_VertexShader = 0, 
    ST_PixelShader = 1,
    ST_Unknown = 2
};    



class ShaderDescriptor
{
public:
    ShaderDescriptor();
    ~ShaderDescriptor();
	ShaderDescriptor(std::string pName, ShaderType pType, std::string pASMSource); 
    std::string                     GetName();
	IDirect3DVertexShader9 *        GetVertexShader();
	IDirect3DPixelShader9 *         GetPixelShader();
private:
    HRESULT                         gvAssembleVertexShader();
    HRESULT                         gvAssemblePixelShader();
    std::string                     sName;
    ShaderType                      sType;
    std::string                     sASMSource;
    IDirect3DVertexShader9 *        sVertexShader;
    IDirect3DPixelShader9 *         sPixelShader;
};    




class ShaderManager
{
public:
    ShaderManager();
    ~ShaderManager();
    void                            AddShader(ShaderDescriptor * pSD);
    void                            ListAll();
    IDirect3DVertexShader9 *        GetVertexShader(std::string pName);
    IDirect3DPixelShader9 *         GetPixelShader(std::string pName);
private:
    std::vector<ShaderDescriptor *> *   sShaders;
};


