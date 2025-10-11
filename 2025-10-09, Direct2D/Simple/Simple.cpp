// from https://learn.microsoft.com/en-us/windows/win32/direct2d/direct2d-quickstart

#include "framework.h"

#ifndef HINST_THISCOMPONENT
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)
#endif

class DemoApp
{
    HWND m_hwnd;
    winrt::com_ptr<ID2D1Factory> m_pDirect2DFactory;
    winrt::com_ptr<ID2D1HwndRenderTarget> m_pRenderTarget;
    winrt::com_ptr<ID2D1SolidColorBrush> m_pLightSlateGrayBrush;
    winrt::com_ptr<ID2D1SolidColorBrush> m_pCornflowerBlueBrush;

public:
    DemoApp();
    ~DemoApp();

    // Register the window class and call methods for instantiating drawing resources
    HRESULT Initialize();

    // Process and dispatch messages
    void RunMessageLoop();

private:
    // Initialize device-independent resources.
    HRESULT CreateDeviceIndependentResources();

    // Initialize device-dependent resources.
    HRESULT CreateDeviceResources();

    // Release device-dependent resource.
    void DiscardDeviceResources();

    // Draw content
    HRESULT OnRender();

    // Resize the render target
    void OnResize(UINT width, UINT height);

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};

DemoApp::DemoApp()
    : m_hwnd(nullptr)
    , m_pDirect2DFactory(nullptr)
    , m_pRenderTarget(nullptr)
    , m_pLightSlateGrayBrush(nullptr)
    , m_pCornflowerBlueBrush(nullptr)
{
}

DemoApp::~DemoApp()
{
}

void DemoApp::RunMessageLoop()
{
    MSG msg;

    while(GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

HRESULT DemoApp::Initialize()
{
    HRESULT hr = CreateDeviceIndependentResources();

    if (SUCCEEDED(hr))
    {
        WNDCLASSEXW wcex = {sizeof(WNDCLASSEXW)};
        wcex.style = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc = DemoApp::WndProc;
        wcex.cbClsExtra = 0;
        wcex.cbWndExtra = 0;
        wcex.hInstance = HINST_THISCOMPONENT;
        wcex.hbrBackground = nullptr;
        wcex.lpszMenuName = nullptr;
        wcex.hCursor = LoadCursor(nullptr, IDI_APPLICATION);
        wcex.lpszClassName = L"D2DDemoApp";

        RegisterClassExW(&wcex);

        m_hwnd = CreateWindowW(L"D2DDemoApp", L"Direct2D demo application",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            0, 0,
            nullptr, nullptr,
            HINST_THISCOMPONENT,
            this);

        if (m_hwnd)
        {
            // Beacuse the SetWindowPos function takes its size in pixels, we
            // optain window's DPI, and use it to scale the window size.
            float dpi = (float)GetDpiForWindow(m_hwnd);

            SetWindowPos(m_hwnd, nullptr, 0, 0, 
                static_cast<int>(ceil(640.f * dpi / 96.f)),
                static_cast<int>(ceil(480.f * dpi / 96.f)),
                SWP_NOMOVE);

            ShowWindow(m_hwnd, SW_SHOWNORMAL);
            UpdateWindow(m_hwnd);
        }
    }

    return hr;
}

int WINAPI wWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPWSTR /*lpCmdLine*/, int /*nCmdShow*/)
{
    // UseHeapSetInformation to specify that the process should terminate if the heap manager detects an error in any heap used by the process.
    // The return value is ignored, because we want to continue running in the unlikely event that HeapSetInformation fails.
    HeapSetInformation(nullptr, HeapEnableTerminationOnCorruption, nullptr, 0);

    if (SUCCEEDED(CoInitialize(nullptr)))
    {
        {
            DemoApp app;
            if (SUCCEEDED(app.Initialize()))
            {
                app.RunMessageLoop();
            }
        }
        CoUninitialize();
    }

    return 0;
}

HRESULT DemoApp::CreateDeviceIndependentResources()
{
    return D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, m_pDirect2DFactory.put());
}

HRESULT DemoApp::CreateDeviceResources()
{
    HRESULT hr = S_OK;

    if (!m_pRenderTarget)
    {

        RECT rc;
        GetClientRect(m_hwnd, &rc);

        D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

        // create a Direct2D render light
        hr = m_pDirect2DFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(m_hwnd, size), m_pRenderTarget.put());

        if (SUCCEEDED(hr))
        {
            // Create a gray brush
            hr = m_pRenderTarget->CreateSolidColorBrush(
                D2D1::ColorF(D2D1::ColorF::LightSlateGray),
                m_pLightSlateGrayBrush.put());
        }

        if (SUCCEEDED(hr))
        {
            // Create a blue brush
            hr = m_pRenderTarget->CreateSolidColorBrush(
                D2D1::ColorF(D2D1::ColorF::CornflowerBlue),
                m_pCornflowerBlueBrush.put()
            );
        }
    }

    return hr;
}

void DemoApp::DiscardDeviceResources()
{
    m_pRenderTarget = nullptr;
    m_pLightSlateGrayBrush = nullptr;
    m_pCornflowerBlueBrush = nullptr;
}

LRESULT CALLBACK DemoApp::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;

    if (message == WM_CREATE)
    {
        LPCREATESTRUCTW pcs = (LPCREATESTRUCTW)lParam;
        DemoApp* pDemoApp = (DemoApp*)pcs->lpCreateParams;

        ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)pDemoApp);
        result = 1;
    }
    else
    {
        DemoApp* pDemoApp = (DemoApp*)::GetWindowLongPtrW(hwnd, GWLP_USERDATA);

        bool wasHandled = false;

        if (pDemoApp)
        {
            switch(message)
            {
            case WM_SIZE:
                {
                    UINT width = LOWORD(lParam);
                    UINT height = HIWORD(lParam);
                    pDemoApp->OnResize(width, height);
                }
                result = 0;
                wasHandled = true;
                break;

            case WM_DISPLAYCHANGE:
                {
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
                result = 0;
                wasHandled = true;
                break;

            case WM_PAINT:
                {
                    pDemoApp->OnRender();
                    ValidateRect(hwnd, nullptr);
                }

                result = 0;
                wasHandled = true; 
                break;

            case WM_DESTROY:
                {
                    PostQuitMessage(0);
                }
                result = 1;
                wasHandled = true;
                break;
            }
        }

        if (!wasHandled)
        {
            result = DefWindowProc(hwnd, message, wParam, lParam);
        }
    }

    return result;
}

HRESULT DemoApp::OnRender()
{
    HRESULT hr = S_OK;
    hr = CreateDeviceResources();

    if (SUCCEEDED(hr))
    {
        m_pRenderTarget->BeginDraw();
        m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
        m_pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

        D2D1_SIZE_F rtSize = m_pRenderTarget->GetSize();

        int width = (int)rtSize.width;
        int height = (int)rtSize.height;

        for (int x = 0; x < width; x+=10)
            m_pRenderTarget->DrawLine(
                D2D1::Point2F((float)x, 0.0f),
                D2D1::Point2F((float)x, rtSize.height),
                m_pLightSlateGrayBrush.get(),
                0.5f
            );

        for( int y = 0; y < height; y+=10)
        {
            m_pRenderTarget->DrawLine(
                D2D1::Point2F(0.0f, (float)y),
                D2D1::Point2F(rtSize.width, (float)y),
                m_pLightSlateGrayBrush.get(),
                0.5f
            );
        }

        D2D1_RECT_F rectangle1 = D2D1::RectF(
            rtSize.width / 2 - 50.0f,
            rtSize.height / 2 - 50.0f,
            rtSize.width / 2 + 50.0f,
            rtSize.height / 2 + 50.0f
        );

        D2D1_RECT_F rectangle2 = D2D1::RectF(
            rtSize.width / 2 - 100.0f,
            rtSize.height / 2 - 100.0f,
            rtSize.width / 2 + 100.0f,
            rtSize.height / 2 + 100.0f
        );

        m_pRenderTarget->FillRectangle(&rectangle1, m_pLightSlateGrayBrush.get());
        m_pRenderTarget->DrawRectangle(&rectangle2, m_pCornflowerBlueBrush.get());
        hr = m_pRenderTarget->EndDraw();

        if (hr == D2DERR_RECREATE_TARGET)
        {
            hr = S_OK;
            DiscardDeviceResources();
        }
    }

    return hr;
}

void DemoApp::OnResize(UINT width, UINT height)
{
    if (m_pRenderTarget)
    {
        // Note: This method can fail, but it's okay to ignore the 
        // error here, because the error will be returned again the next time EndDraw is called
        m_pRenderTarget->Resize(D2D1::SizeU(width, height));
    }
}