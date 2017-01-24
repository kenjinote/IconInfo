#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#pragma comment(lib, "shlwapi")
#pragma comment(lib, "gdiplus")

#include <windows.h>
#include <shlwapi.h>
#include <gdiplus.h>

TCHAR szClassName[] = TEXT("Window");

typedef struct
{
	BYTE		bWidth;			// Width, in pixels, of the image
	BYTE		bHeight;		// Height, in pixels, of the image
	BYTE		bColorCount;	// Number of colors in image (0 if >=8bpp)
	BYTE		bReserved;		// Reserved ( must be 0)
	WORD		wPlanes;		// Color Planes
	WORD		wBitCount;		// Bits per pixel
	DWORD		dwBytesInRes;	// How many bytes in this resource?
	DWORD		dwImageOffset;	// Where in the file is this image?
} ICONDIRENTRY, *LPICONDIRENTRY;

typedef struct
{
	WORD			idReserved;   // Reserved (must be 0)
	WORD			idType;       // Resource Type (1 for icons)
	WORD			idCount;      // How many images?
	ICONDIRENTRY	idEntries[1]; // An entry for each image (idCount of 'em)
} ICONDIR, *LPICONDIR;

typedef struct
{
	BITMAPINFOHEADER	icHeader;		// DIB header
	RGBQUAD				icColors[1];	// Color table
	BYTE				icXOR[1];		// DIB bits for XOR mask
	BYTE				icAND[1];		// DIB bits for AND mask
} ICONIMAGE, *LPICONIMAGE;

Gdiplus::Image* LoadImageFromMemory(const void* pData, const int nSize)
{
	IStream*pIStream;
	ULARGE_INTEGER LargeUInt;
	LARGE_INTEGER LargeInt;
	CreateStreamOnHGlobal(0, 1, &pIStream);
	LargeUInt.QuadPart = nSize;
	pIStream->SetSize(LargeUInt);
	LargeInt.QuadPart = 0;
	pIStream->Seek(LargeInt, STREAM_SEEK_SET, NULL);
	pIStream->Write(pData, nSize, NULL);
	Gdiplus::Image*pImage = Gdiplus::Image::FromStream(pIStream);
	pIStream->Release();
	if (pImage)
	{
		if (pImage->GetLastStatus() == Gdiplus::Ok)
		{
			return pImage;
		}
		delete pImage;
	}
	return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static HWND hList;
	switch (msg)
	{
	case WM_CREATE:
		hList = CreateWindow(TEXT("LISTBOX"), 0, WS_VISIBLE | WS_CHILD | WS_VSCROLL | LBS_NOINTEGRALHEIGHT, 0, 0, 0, 0, hWnd, 0, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		DragAcceptFiles(hWnd, TRUE);
		break;
	case WM_DROPFILES:
		{
			TCHAR szFilePath[MAX_PATH];
			const UINT iFileNum = DragQueryFile((HDROP)wParam, -1, NULL, 0);
			if (iFileNum == 1)
			{
				SendMessage(hList, LB_RESETCONTENT, 0, 0);
				DragQueryFile((HDROP)wParam, 0, szFilePath, MAX_PATH);
				if (PathMatchSpec(szFilePath, TEXT("*.ico")))
				{
					const HANDLE hFile = CreateFile(szFilePath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
					const LPICONDIR pIconDir = (LPICONDIR)malloc(sizeof(ICONDIR));
					DWORD dwBytesRead;
					ReadFile(hFile, &(pIconDir->idReserved), sizeof(WORD), &dwBytesRead, NULL);
					ReadFile(hFile, &(pIconDir->idType), sizeof(WORD), &dwBytesRead, NULL);
					ReadFile(hFile, &(pIconDir->idCount), sizeof(WORD), &dwBytesRead, NULL);
					const LPICONDIRENTRY pIconDirEntries = (LPICONDIRENTRY)malloc(sizeof(ICONDIRENTRY) * pIconDir->idCount);
					ReadFile(hFile, pIconDirEntries, sizeof(ICONDIRENTRY) * pIconDir->idCount, &dwBytesRead, NULL);
					for (int i = 0; i < pIconDir->idCount; ++i)
					{
						const LPBYTE pIconImage = (LPBYTE)malloc(pIconDirEntries[i].dwBytesInRes);
						SetFilePointer(hFile, pIconDirEntries[i].dwImageOffset, NULL, FILE_BEGIN);
						ReadFile(hFile, pIconImage, pIconDirEntries[i].dwBytesInRes, &dwBytesRead, NULL);
						TCHAR szText[1024] = { 0 };
						if (pIconImage[0] == 0x89 && pIconImage[1] == 0x50 && pIconImage[2] == 0x4e && pIconImage[3] == 0x47 &&
							pIconImage[4] == 0x0d && pIconImage[5] == 0x0a && pIconImage[6] == 0x1a && pIconImage[7] == 0x0a)
						{
							Gdiplus::Image *image = LoadImageFromMemory(pIconImage, pIconDirEntries[i].dwBytesInRes);
							if (image)
							{
								Gdiplus::PixelFormat pixelFormat = image->GetPixelFormat();
								int nBitCount;
								switch (pixelFormat)
								{
								case PixelFormat1bppIndexed:
									nBitCount = 1;
									break;
								case PixelFormat4bppIndexed:
									nBitCount = 4;
									break;
								case PixelFormat8bppIndexed:
									nBitCount = 8;
									break;
								case PixelFormat16bppGrayScale:
								case PixelFormat16bppRGB555:
								case PixelFormat16bppRGB565:
								case PixelFormat16bppARGB1555:
									nBitCount = 16;
									break;
								case PixelFormat24bppRGB:
									nBitCount = 24;
									break;
								case PixelFormat32bppRGB:
								case PixelFormat32bppARGB:
								case PixelFormat32bppPARGB:
								case PixelFormat32bppCMYK:
									nBitCount = 32;
									break;
								case PixelFormat48bppRGB:
									nBitCount = 48;
									break;
								case PixelFormat64bppARGB:
								case PixelFormat64bppPARGB:
									nBitCount = 64;
									break;
								default:
									nBitCount = 0;
									break;
								}
								wsprintf(szText, TEXT("%dx%d, %dビット(PNG)"), image->GetWidth(), image->GetHeight(), nBitCount);
								delete image;
							}
						}
						else
						{
							wsprintf(szText, TEXT("%dx%d, %dビット(BMP)"),
								((LPICONIMAGE)pIconImage)->icHeader.biWidth,
								((LPICONIMAGE)pIconImage)->icHeader.biHeight >> 1, // biHeightには本体イメージとマスクを合計した高さが記録されているので半分にする
								((LPICONIMAGE)pIconImage)->icHeader.biBitCount);
						}
						SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)szText);
						free(pIconImage);
					}
					free(pIconDirEntries);
					free(pIconDir);
					CloseHandle(hFile);
				}
			}
			DragFinish((HDROP)wParam);
		}
		break;
	case WM_SIZE:
		MoveWindow(hList, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPreInst, LPSTR pCmdLine, int nCmdShow)
{
	ULONG_PTR gdiToken;
	Gdiplus::GdiplusStartupInput gdiSI;
	Gdiplus::GdiplusStartup(&gdiToken, &gdiSI, 0);
	MSG msg;
	WNDCLASS wndclass = {
		0,
		WndProc,
		0,
		0,
		hInstance,
		0,
		LoadCursor(0,IDC_ARROW),
		0,
		0,
		szClassName
	};
	RegisterClass(&wndclass);
	HWND hWnd = CreateWindow(
		szClassName,
		TEXT("ドロップされたアイコンファイルの情報を取得"),
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
		CW_USEDEFAULT,
		0,
		CW_USEDEFAULT,
		0,
		0,
		0,
		hInstance,
		0
	);
	ShowWindow(hWnd, SW_SHOWDEFAULT);
	UpdateWindow(hWnd);
	while (GetMessage(&msg, 0, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	Gdiplus::GdiplusShutdown(gdiToken);
	return (int)msg.wParam;
}
