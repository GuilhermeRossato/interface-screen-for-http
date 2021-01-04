// DirectX method to capture screen
#include <D3D9.h>

#ifdef D3DADAPTER_DEFAULT

HRESULT Direct3D9TakeScreenshot(uint32_t adapter, LPBYTE *pBuffer, uint32_t *pStride, const RECT *pInputRc);

#define GetBGRAPixel(b,s,x,y)       (((LPDWORD)(((LPBYTE)b) + y * s))[x])
#define GetBGRAPixelBlue(p)         (LOBYTE(p))
#define GetBGRAPixelGreen(p)        (HIBYTE(p))
#define GetBGRAPixelRed(p)          (LOBYTE(HIWORD(p)))
#define GetBGRAPixelAlpha(p)        (HIBYTE(HIWORD(p)))
#ifndef HRCHECK
	#define HRCHECK(__expr) {hr=(__expr);if(FAILED(hr)){wprintf(L"FAILURE 0x%08X (%i)\n\tline: %u file\n", hr, hr, __LINE__);goto cleanup;}}
#endif

HRESULT Direct3D9TakeScreenshot(uint32_t adapter, LPBYTE *pBuffer, uint32_t *pStride, const RECT *pInputRc);

BYTE * pixel_buffer = NULL;

uint8_t * capture_screen_buffer = NULL;
int64_t capture_screen_buffer_size = 0;

uint8_t * CaptureScreenBuffer(int x, int y, int w, int h) {
	if (x < 0 || y < 0 || w <= 0 || h <= 0) {
		return NULL;
	}
	LONG left = x;
	LONG top = y;
	LONG width = w;
	LONG height = h;
	uint32_t stride;
	RECT rc = { left, top, left + width, top + height };
	Direct3D9TakeScreenshot((uint32_t) D3DADAPTER_DEFAULT, &pixel_buffer, &stride, &rc);

	int64_t needed_buffer_size = w * h * 3;

	if (capture_screen_buffer_size < needed_buffer_size || capture_screen_buffer == NULL) {
		if (capture_screen_buffer != NULL) {
			free(capture_screen_buffer);
		}
		capture_screen_buffer = (uint8_t *) calloc(needed_buffer_size + 200, sizeof(uint8_t));
		capture_screen_buffer_size = needed_buffer_size + 200;
	}

	for (int j = 0; j < h; j++) {
		for (int i = 0; i < w; i++) {
			DWORD pixel = GetBGRAPixel(pixel_buffer, stride, i, j);
			int index = (j * w + i) * 3;
			capture_screen_buffer[index + 0] = GetBGRAPixelRed(pixel);
			capture_screen_buffer[index + 1] = GetBGRAPixelGreen(pixel);
			capture_screen_buffer[index + 2] = GetBGRAPixelBlue(pixel);
		}
	}

	return capture_screen_buffer;
}

HRESULT hr = S_OK;
IDirect3D9 *d3d = nullptr;
IDirect3DDevice9 *device = nullptr;
IDirect3DSurface9 *surface = nullptr;
D3DPRESENT_PARAMETERS parameters = { 0 };
D3DDISPLAYMODE mode;
D3DLOCKED_RECT rc;

HRESULT Direct3D9TakeScreenshot(uint32_t adapter, LPBYTE *pBuffer, uint32_t *pStride, const RECT *pInputRc) {
	static int64_t times_called = 0;
	if (!pBuffer || !pStride) return E_INVALIDARG;

	*pStride = 0;

	if (times_called == 0) {
		// init D3D and get screen size
		d3d = Direct3DCreate9(D3D_SDK_VERSION);
		HRCHECK(d3d->GetAdapterDisplayMode(adapter, &mode));
	}

	LONG width = pInputRc ? (pInputRc->right - pInputRc->left) : mode.Width;
	LONG height = pInputRc ? (pInputRc->bottom - pInputRc->top) : mode.Height;

	if (times_called == 0) {
		parameters.Windowed = TRUE;
		parameters.BackBufferCount = 1;
		parameters.BackBufferHeight = height;
		parameters.BackBufferWidth = width;
		parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
		parameters.hDeviceWindow = NULL;
	}

	if (times_called == 0) {
		// Create device & capture surface (note it needs desktop size, not our capture size)
		HRCHECK(d3d->CreateDevice(adapter, D3DDEVTYPE_HAL, NULL, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &parameters, &device));
		HRCHECK(device->CreateOffscreenPlainSurface(mode.Width, mode.Height, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &surface, nullptr));
	}

	// Get pitch/stride to compute the required buffer size
	HRCHECK(surface->LockRect(&rc, pInputRc, 0));
	*pStride = rc.Pitch;
	HRCHECK(surface->UnlockRect());

	if (!*pBuffer) {
		// allocate buffer
		*pBuffer = (LPBYTE)LocalAlloc(0, *pStride * height);
	}
	if (!*pBuffer) {
		hr = E_OUTOFMEMORY;
		goto cleanup;
	}

	// get the data
	HRCHECK(device->GetFrontBufferData(0, surface));

	// copy it into our buffer
	HRCHECK(surface->LockRect(&rc, pInputRc, 0));
	CopyMemory(*pBuffer, rc.pBits, rc.Pitch * height);
	HRCHECK(surface->UnlockRect());

	cleanup:
	if (FAILED(hr)) {
		if (*pBuffer) {
			LocalFree(*pBuffer);
			*pBuffer = NULL;
		}
		*pStride = 0;
		if (surface) {
			surface->Release();
			surface = nullptr;
		}
		if (device) {
			device->Release();
			device = nullptr;
		}
		if (d3d) {
			d3d->Release();
			d3d = nullptr;
		}
	}

	times_called++;
	return hr;
}

#endif
