#pragma once
#include <vector>
#include <memory>
#include <d3d9.h>
#include <d3dx9.h>
#include <detourxs.h>
#include "vmt.h"

#pragma comment(lib, "d3d9")
#pragma comment(lib, "d3dx9")

typedef HRESULT(WINAPI* FnDrawIndexedPrimitive)(IDirect3DDevice9*, D3DPRIMITIVETYPE, INT, UINT, UINT, UINT, UINT);
typedef HRESULT(WINAPI* FnEndScene)(IDirect3DDevice9*);
typedef HRESULT(WINAPI* FnCreateQuery)(IDirect3DDevice9*, D3DQUERYTYPE, IDirect3DQuery9**);
typedef HRESULT(WINAPI* FnReset)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);
typedef HRESULT(WINAPI* FnPresent)(IDirect3DDevice9*, const RECT*, const RECT*, HWND, const RGNDATA*);

class CDirectX9Hook
{
public:
	CDirectX9Hook();
	// CDirectX9Hook(IDirect3DDevice9* device);
	~CDirectX9Hook();
	
	void Init();
	IDirect3DDevice9* GetDevice();

	template<typename Fn>
	bool HookFunction(Fn function);

	template<typename Fn>
	bool UnhookFunction(Fn function);

protected:
	bool CreateDevice();
	bool ReleaseDevice();
	bool SetupFirstHook();
	bool SetupSecondHook(IDirect3DDevice9* device);

public:
	bool CheckHookStatus(IDirect3DDevice9* device);

	std::vector<FnDrawIndexedPrimitive>& GetHookList(FnDrawIndexedPrimitive);
	std::vector<FnEndScene>& GetHookList(FnEndScene);
	std::vector<FnCreateQuery>& GetHookList(FnCreateQuery);
	std::vector<FnReset>& GetHookList(FnReset);
	std::vector<FnPresent>& GetHookList(FnPresent);

	FnDrawIndexedPrimitive m_pfnDrawIndexedPrimitive;
	FnEndScene m_pfnEndScene;
	FnCreateQuery m_pfnCreateQuery;
	FnReset m_pfnReset;
	FnPresent m_pfnPresent;

	std::vector<FnDrawIndexedPrimitive> m_vfnDrawIndexedPrimitive;
	std::vector<FnEndScene> m_vfnEndScene;
	std::vector<FnCreateQuery> m_vfnCreateQuery;
	std::vector<FnReset> m_vfnReset;
	std::vector<FnPresent> m_vfnPresent;

private:
	std::vector<DetourXS*> m_vpDetourList;
	CVmtHook* m_pVmtHook;
	PDWORD m_pVMT;

private:
	IDirect3D9* m_pD3D;
	IDirect3DDevice9* m_pDevice;
	IDirect3DDevice9* m_pOriginDevice;
	bool m_bSuccessCreated;
	bool m_bIsFirstHooked;
	bool m_bIsSecondHooked;
};

extern std::unique_ptr<CDirectX9Hook> g_pDirextXHook;
