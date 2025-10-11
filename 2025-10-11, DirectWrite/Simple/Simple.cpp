// from https://learn.microsoft.com/en-us/windows/win32/direct2d/direct2d-quickstart

#include "framework.h"

#ifndef HINST_THISCOMPONENT
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)
#endif

class DemoApp
{
    HWND hwnd;
    winrt::com_ptr<ID2D1Factory> d2dFactory;
    winrt::com_ptr<ID2D1HwndRenderTarget> renderTarget;
    winrt::com_ptr<ID2D1SolidColorBrush> lightSlateGrayBrush;
    winrt::com_ptr<ID2D1SolidColorBrush> cornflowerBlueBrush;

    winrt::com_ptr<ID2D1SolidColorBrush> blackBrush;

    // for direct write
    winrt::com_ptr<IDWriteFactory> dwriteFactory;
    winrt::com_ptr<IDWriteTextFormat> textFormat;

    std::wstring text;
    float dpi;

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

    void OnDpiChanged(UINT dpi);

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};

DemoApp::DemoApp()
    : hwnd(nullptr)
    , d2dFactory(nullptr)
    , renderTarget(nullptr)
    , lightSlateGrayBrush(nullptr)
    , cornflowerBlueBrush(nullptr)
    , text(L"안녕하세요")
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

        hwnd = CreateWindowW(L"D2DDemoApp", L"Direct2D demo application",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            0, 0,
            nullptr, nullptr,
            HINST_THISCOMPONENT,
            this);

        if (hwnd)
        {
            // Beacuse the SetWindowPos function takes its size in pixels, we
            // optain window's DPI, and use it to scale the window size.
            dpi = (float)GetDpiForWindow(hwnd);

            SetWindowPos(hwnd, nullptr, 0, 0, 
                static_cast<int>(ceil(640.f * dpi / 96.f)),
                static_cast<int>(ceil(480.f * dpi / 96.f)),
                SWP_NOMOVE);

            ShowWindow(hwnd, SW_SHOWNORMAL);
            UpdateWindow(hwnd);
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
    HRESULT hr = S_OK;

    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, d2dFactory.put());
    if (FAILED(hr)) return hr;

    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)dwriteFactory.put());
    if (FAILED(hr)) return hr;
    
    return hr;
}

HRESULT DemoApp::CreateDeviceResources()
{
    HRESULT hr = S_OK;

    if (renderTarget) return hr;
    
    RECT rc;
    GetClientRect(hwnd, &rc);

    D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

    // create a Direct2D render light
    hr = d2dFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(hwnd, size), renderTarget.put());
    if (FAILED(hr)) return hr;

    // Create a gray brush
    hr = renderTarget->CreateSolidColorBrush(
        D2D1::ColorF(D2D1::ColorF::LightSlateGray),
        lightSlateGrayBrush.put());
    if (FAILED(hr)) return hr;
    
    // Create a blue brush
    hr = renderTarget->CreateSolidColorBrush(
        D2D1::ColorF(D2D1::ColorF::CornflowerBlue),
        cornflowerBlueBrush.put()
    );
    if (FAILED(hr)) return hr;

    hr = renderTarget->CreateSolidColorBrush(
        D2D1::ColorF(D2D1::ColorF::Black),
        blackBrush.put());
    if (FAILED(hr)) return hr;

    float dpi = (float)GetDpiForWindow(hwnd);
    hr = dwriteFactory->CreateTextFormat(
        L"맑은 고딕", // font name
        nullptr,     // font collection (nullptr sets it to use the system font collection)
        DWRITE_FONT_WEIGHT_REGULAR,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        72.0f * dpi / USER_DEFAULT_SCREEN_DPI, // dpi가 아니라 dip이다
        L"",
        textFormat.put()
    );
    if (FAILED(hr)) return hr;

    hr = textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    if (FAILED(hr)) return hr;

    hr = textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    if (FAILED(hr)) return hr;


    return hr;
}

void DemoApp::DiscardDeviceResources()
{
    renderTarget = nullptr;
    lightSlateGrayBrush = nullptr;
    cornflowerBlueBrush = nullptr;
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

            case WM_DPICHANGED:
            {
                UINT dpi = HIWORD(wParam);
                RECT* suggested = reinterpret_cast<RECT*>(lParam);
                SetWindowPos(hwnd, nullptr,
                    suggested->left, suggested->top,
                    suggested->right - suggested->left,
                    suggested->bottom - suggested->top,
                    SWP_NOZORDER | SWP_NOACTIVATE);
                pDemoApp->OnDpiChanged(dpi);

                break;
            }

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
    if (FAILED(hr)) return hr;

    renderTarget->BeginDraw();
    renderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
    renderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

    D2D1_SIZE_F rtSize = renderTarget->GetSize();

    int width = (int)rtSize.width;
    int height = (int)rtSize.height;

    for (int x = 0; x < width; x+=10)
        renderTarget->DrawLine(
            D2D1::Point2F((float)x, 0.0f),
            D2D1::Point2F((float)x, rtSize.height),
            lightSlateGrayBrush.get(),
            0.5f
        );

    for( int y = 0; y < height; y+=10)
    {
        renderTarget->DrawLine(
            D2D1::Point2F(0.0f, (float)y),
            D2D1::Point2F(rtSize.width, (float)y),
            lightSlateGrayBrush.get(),
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

    renderTarget->FillRectangle(&rectangle1, lightSlateGrayBrush.get());
    renderTarget->DrawRectangle(&rectangle2, cornflowerBlueBrush.get());
    
    D2D1_RECT_F layoutRect = D2D1::RectF(0.0f, 0.0f, rtSize.width, rtSize.height);

    renderTarget->DrawText(text.c_str(), (UINT)text.length(), textFormat.get(), layoutRect, blackBrush.get());


    hr = renderTarget->EndDraw();

    if (hr == D2DERR_RECREATE_TARGET)
    {
        hr = S_OK;
        DiscardDeviceResources();
    }

    return hr;
}

void DemoApp::OnResize(UINT width, UINT height)
{
    if (renderTarget)
    {
        // Note: This method can fail, but it's okay to ignore the 
        // error here, because the error will be returned again the next time EndDraw is called
        renderTarget->Resize(D2D1::SizeU(width, height));
    }
}

void DemoApp::OnDpiChanged(UINT dpi)
{   
    HRESULT hr = dwriteFactory->CreateTextFormat(
        L"맑은 고딕", // font name
        nullptr,     // font collection (nullptr sets it to use the system font collection)
        DWRITE_FONT_WEIGHT_REGULAR,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        72.0f * dpi / USER_DEFAULT_SCREEN_DPI, // dpi가 아니라 dip이다
        L"",
        textFormat.put()
    );
    if (FAILED(hr)) return;

    hr = textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    if (FAILED(hr)) return;

    hr = textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    if (FAILED(hr)) return;
}