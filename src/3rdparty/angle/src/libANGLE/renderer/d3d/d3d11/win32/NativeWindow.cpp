//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// NativeWindow.cpp: Handler for managing HWND native window types.

#include "libANGLE/renderer/d3d/d3d11/NativeWindow.h"
#include "libANGLE/renderer/d3d/d3d11/renderer11_utils.h"

#include "common/debug.h"

#include <initguid.h>
#if !defined(__MINGW32__)
#include <dcomp.h>
#endif

namespace rx
{

NativeWindow::NativeWindow(EGLNativeWindowType window,
                           const egl::Config *config,
                           bool directComposition)
    : mWindow(window),
      mDirectComposition(directComposition),
      mDevice(nullptr),
      mCompositionTarget(nullptr),
      mVisual(nullptr),
      mConfig(config)
{
}

NativeWindow::~NativeWindow()
{
#if !defined(__MINGW32__)
    SafeRelease(mCompositionTarget);
    SafeRelease(mDevice);
    SafeRelease(mVisual);
#endif
}

bool NativeWindow::initialize()
{
    return true;
}

bool NativeWindow::getClientRect(LPRECT rect)
{
    return GetClientRect(mWindow, rect) == TRUE;
}

bool NativeWindow::isIconic()
{
    return IsIconic(mWindow) == TRUE;
}

bool NativeWindow::isValidNativeWindow(EGLNativeWindowType window)
{
    return IsWindow(window) == TRUE;
}

#if defined(ANGLE_ENABLE_D3D11)
HRESULT NativeWindow::createSwapChain(ID3D11Device* device, DXGIFactory* factory,
                                      DXGI_FORMAT format, unsigned int width, unsigned int height,
                                      DXGISwapChain** swapChain)
{
    if (device == NULL || factory == NULL || swapChain == NULL || width == 0 || height == 0)
    {
        return E_INVALIDARG;
    }

#if !defined(__MINGW32__)
    if (mDirectComposition)
    {
        HMODULE dcomp = ::GetModuleHandle(TEXT("dcomp.dll"));
        if (!dcomp)
        {
            return E_INVALIDARG;
        }

        typedef HRESULT(WINAPI * PFN_DCOMPOSITION_CREATE_DEVICE)(
            IDXGIDevice * dxgiDevice, REFIID iid, void **dcompositionDevice);
        PFN_DCOMPOSITION_CREATE_DEVICE createDComp =
            reinterpret_cast<PFN_DCOMPOSITION_CREATE_DEVICE>(
                GetProcAddress(dcomp, "DCompositionCreateDevice"));
        if (!createDComp)
        {
            return E_INVALIDARG;
        }

        if (!mDevice)
        {
            IDXGIDevice *dxgiDevice = d3d11::DynamicCastComObject<IDXGIDevice>(device);
            HRESULT result = createDComp(dxgiDevice, __uuidof(IDCompositionDevice),
                                         reinterpret_cast<void **>(&mDevice));
            SafeRelease(dxgiDevice);

            if (FAILED(result))
            {
                return result;
            }
        }

        if (!mCompositionTarget)
        {
            HRESULT result = mDevice->CreateTargetForHwnd(mWindow, TRUE, &mCompositionTarget);
            if (FAILED(result))
            {
                return result;
            }
        }

        if (!mVisual)
        {
            HRESULT result = mDevice->CreateVisual(&mVisual);
            if (FAILED(result))
            {
                return result;
            }
        }

        IDXGIFactory2 *factory2             = d3d11::DynamicCastComObject<IDXGIFactory2>(factory);
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {0};
        swapChainDesc.Width                 = width;
        swapChainDesc.Height                = height;
        swapChainDesc.Format                = format;
        swapChainDesc.Stereo                = FALSE;
        swapChainDesc.SampleDesc.Count      = 1;
        swapChainDesc.SampleDesc.Quality    = 0;
        swapChainDesc.BufferUsage =
            DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_SHADER_INPUT;
        swapChainDesc.BufferCount           = 2;
        swapChainDesc.Scaling               = DXGI_SCALING_STRETCH;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        swapChainDesc.AlphaMode =
            mConfig->alphaSize == 0 ? DXGI_ALPHA_MODE_IGNORE : DXGI_ALPHA_MODE_PREMULTIPLIED;
        swapChainDesc.Flags         = 0;
        IDXGISwapChain1 *swapChain1 = nullptr;
        HRESULT result =
            factory2->CreateSwapChainForComposition(device, &swapChainDesc, nullptr, &swapChain1);
        if (SUCCEEDED(result))
        {
            *swapChain = static_cast<DXGISwapChain *>(swapChain1);
        }
        mVisual->SetContent(swapChain1);
        mCompositionTarget->SetRoot(mVisual);
        SafeRelease(factory2);
        return result;
    }

    // Use IDXGIFactory2::CreateSwapChainForHwnd if DXGI 1.2 is available to create a DXGI_SWAP_EFFECT_SEQUENTIAL swap chain.
    IDXGIFactory2 *factory2 = d3d11::DynamicCastComObject<IDXGIFactory2>(factory);
    if (factory2 != nullptr)
    {
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };
        swapChainDesc.Width = width;
        swapChainDesc.Height = height;
        swapChainDesc.Format = format;
        swapChainDesc.Stereo = FALSE;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage =
            DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT | DXGI_USAGE_BACK_BUFFER;
        swapChainDesc.BufferCount = 1;
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_SEQUENTIAL;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        swapChainDesc.Flags = 0;
        IDXGISwapChain1 *swapChain1 = nullptr;
        HRESULT result = factory2->CreateSwapChainForHwnd(device, mWindow, &swapChainDesc, nullptr, nullptr, &swapChain1);
        if (SUCCEEDED(result))
        {
            const HRESULT makeWindowAssociationResult = factory->MakeWindowAssociation(mWindow, DXGI_MWA_NO_ALT_ENTER);
            UNUSED_VARIABLE(makeWindowAssociationResult);
            *swapChain = static_cast<DXGISwapChain*>(swapChain1);
        }
        SafeRelease(factory2);
        return result;
    }
#endif // !__MINGW32__

    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferCount = 1;
    swapChainDesc.BufferDesc.Format = format;
    swapChainDesc.BufferDesc.Width = width;
    swapChainDesc.BufferDesc.Height = height;
    swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferUsage =
        DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT | DXGI_USAGE_BACK_BUFFER;
    swapChainDesc.Flags = 0;
    swapChainDesc.OutputWindow = mWindow;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    const HRESULT result = factory->CreateSwapChain(device, &swapChainDesc, swapChain);
    if (SUCCEEDED(result))
    {
        const HRESULT makeWindowAssociationResult = factory->MakeWindowAssociation(mWindow, DXGI_MWA_NO_ALT_ENTER);
        UNUSED_VARIABLE(makeWindowAssociationResult);
    }
    return result;
}
#endif

void NativeWindow::commitChange()
{
#if !defined(__MINGW32__)
    if (mDevice)
    {
        mDevice->Commit();
    }
#endif
}
}
