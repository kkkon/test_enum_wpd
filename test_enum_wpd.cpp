/*
 * The MIT License
 *
 * Copyright 2015 Kiyofumi Kondoh
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "stdafx.h"

#include <windows.h>

#include <objbase.h>
#pragma comment(lib,"ole32.lib")

#include <PortableDevice.h>
#include <PortableDeviceApi.h>
#pragma comment(lib,"PortableDeviceGUIDs.lib")

static
bool s_optVerbose = false;

void
LOGV( LPCWSTR format, ... )
{
    if ( false == s_optVerbose )
    {
        return;
    }

    wchar_t buff[512];

    va_list argPtr;
    va_start( argPtr, format );

    ::_vsnwprintf_s( buff, sizeof(buff)/sizeof(buff[0]), _TRUNCATE, format, argPtr );

    ::fwprintf( stdout, buff );
    ::OutputDebugStringW( buff );
}

void
LOGI( LPCWSTR format, ... )
{
    wchar_t buff[512];

    va_list argPtr;
    va_start( argPtr, format );

    ::_vsnwprintf_s( buff, sizeof(buff)/sizeof(buff[0]), _TRUNCATE, format, argPtr );

    ::fwprintf( stdout, buff );
    ::OutputDebugStringW( buff );
}

void
DumpPropertyKey( const PROPERTYKEY* pKey )
{
    if ( NULL == pKey )
    {
        return;
    }

    LOGV( L"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x %08x\n"
          , pKey->fmtid.Data1
          , pKey->fmtid.Data2
          , pKey->fmtid.Data3
          , pKey->fmtid.Data4[0]
          , pKey->fmtid.Data4[1]
          , pKey->fmtid.Data4[2]
          , pKey->fmtid.Data4[3]
          , pKey->fmtid.Data4[4]
          , pKey->fmtid.Data4[5]
          , pKey->fmtid.Data4[6]
          , pKey->fmtid.Data4[7]
          , pKey->pid
          );
}

#define MY_WPDENUM_NUM_OBJECTS_PER_ONE_REQUEST (10)
static
DWORD   s_dwCountContent = 0;

void
wpdEnumContent_RecursiveEnumerate(
    LPCWSTR pszObjectId
    , IPortableDeviceContent* pPortableDeviceContent
)
{
    if ( NULL == pszObjectId )
    {
        LOGV( L"wpdEnumContent_RecursiveEnumerate: pszObjectId is NULL" );
        return;
    }
    if ( NULL == pPortableDeviceContent )
    {
        LOGV( L"wpdEnumContent_RecursiveEnumerate: pPortableDeviceContent is NULL" );
        return;
    }


    LOGV( L"enum content: %s\n", pszObjectId );
    {
        IPortableDeviceProperties* pPortableDeviceProperties = NULL;
        IPortableDeviceValues* pAttributes = NULL;

        {
            const HRESULT hr = pPortableDeviceContent->Properties( &pPortableDeviceProperties );
            if ( FAILED(hr) )
            {
                LOGV( L"! Failed. pPortableDeviceContent Properties, hr=0x%08x\n", hr );
            }
        }

        if ( NULL != pPortableDeviceProperties )
        {
            const HRESULT hr = pPortableDeviceProperties->GetValues(
                pszObjectId
                , NULL
                , &pAttributes
                );
            if ( FAILED(hr) )
            {
                LOGV( L"! Failed. pPortableDeviceProperties GetValues, hr=0x%08x\n", hr );
            }
        }

        if ( NULL != pAttributes )
        {
            DWORD dwCount = 0;
            {
                const HRESULT hr = pAttributes->GetCount( &dwCount );
                if ( FAILED(hr) )
                {
                    LOGV( L"! Failed. pPortableDeviceProperties GetCount, hr=0x%08x\n", hr );
                }
                else
                {
                    LOGV( L"pPortableDeviceProperties GetCount, count=%u\n", dwCount );
                }
            }
            for ( DWORD dwIndex = 0; dwIndex < dwCount; ++dwIndex )
            {
                PROPERTYKEY propKey;
                const HRESULT hr = pAttributes->GetAt( dwIndex, &propKey, NULL );
                if ( FAILED(hr) )
                {
                    LOGV( L"! Failed. pPortableDeviceProperties GetAt, hr=0x%08x\n", hr );
                }

                DumpPropertyKey( &propKey );
            }
            {
                LPWSTR pValue = NULL;
                const HRESULT hr = pAttributes->GetStringValue(
                    WPD_OBJECT_ORIGINAL_FILE_NAME
                    , &pValue
                    );
                if ( SUCCEEDED(hr) )
                {
                    LOGV( L" %s\n", pValue );
                }

                if ( NULL != pValue )
                {
                    ::CoTaskMemFree( pValue );
                    pValue = NULL;
                }
            }
            {
                GUID guid;
                const HRESULT hr = pAttributes->GetGuidValue(
                    WPD_OBJECT_CONTENT_TYPE
                    , &guid
                    );
                if ( SUCCEEDED(hr) )
                {
                    if ( ::IsEqualGUID( guid, WPD_CONTENT_TYPE_FOLDER ) )
                    {
                        LOGV( L" Content type: Folder\n" );
                    }
                    else
                    if ( ::IsEqualGUID( guid, WPD_CONTENT_TYPE_IMAGE ) )
                    {
                        LOGV( L" Content type: Image\n" );
                    }
                }
            }
        }

        if ( NULL != pAttributes )
        {
            const DWORD dwCount = pAttributes->Release();
            LOGV( L"pAttributes::Release, count=%u\n", dwCount );
            pAttributes = NULL;
        }

        if ( NULL != pPortableDeviceProperties )
        {
            const DWORD dwCount = pPortableDeviceProperties->Release();
            LOGV( L"pPortableDeviceProperties::Release, count=%u\n", dwCount );
            pPortableDeviceProperties = NULL;
        }
    }

    IEnumPortableDeviceObjectIDs* pEnumPortableDeviceObjectIDs = NULL;
    {
        const DWORD dwFlags = 0;
        IPortableDeviceValues* pFilter = NULL;

        const HRESULT hr = pPortableDeviceContent->EnumObjects(
            dwFlags
            , pszObjectId
            , pFilter
            , &pEnumPortableDeviceObjectIDs
            );
        if ( FAILED(hr) )
        {
            LOGV( L"! Failed. pPortableDeviceContent EnumObjects, hr=0x%08x\n", hr );
        }
    }

    {
        const size_t MY_FETCH_COUNT = MY_WPDENUM_NUM_OBJECTS_PER_ONE_REQUEST;
        LPWSTR* pszObjectIdArray = new LPWSTR[MY_FETCH_COUNT];

        if ( NULL != pszObjectIdArray )
        {
            HRESULT hr = S_OK;
            while ( S_OK == hr ) // not SUCCEEDED(hr)
            {
                DWORD nFetched = 0;

                for ( size_t index = 0; index < MY_FETCH_COUNT; ++index )
                {
                    pszObjectIdArray[index] = NULL;
                }

                hr = pEnumPortableDeviceObjectIDs->Next(
                    MY_FETCH_COUNT
                    , pszObjectIdArray
                    , &nFetched
                    );
                if ( FAILED(hr) )
                {
                    LOGV( L"! Failed. pEnumPortableDeviceObjectIDs Next, hr=0x%08x\n", hr );
                }
                else
                {
                    s_dwCountContent += nFetched;
                    for ( DWORD dwIndex = 0; dwIndex < nFetched; ++dwIndex )
                    {
                        wpdEnumContent_RecursiveEnumerate( pszObjectIdArray[dwIndex], pPortableDeviceContent );
                    }

                    //FreePortableDevicePnPIDs( pszObjectIdArray, MY_FETCH_COUNT );
                    if ( NULL != pszObjectIdArray )
                    {
                        for ( size_t index = 0; index < MY_FETCH_COUNT; ++index )
                        {
                            if ( NULL != pszObjectIdArray[index] )
                            {
                                ::CoTaskMemFree( pszObjectIdArray[index] );
                                pszObjectIdArray[index] = NULL;
                            }
                        }
                    }
                }
            }
        }

        //FreePortableDevicePnPIDs( pszObjectIdArray, MY_FETCH_COUNT );
        if ( NULL != pszObjectIdArray )
        {
            for ( size_t index = 0; index < MY_FETCH_COUNT; ++index )
            {
                if ( NULL != pszObjectIdArray[index] )
                {
                    ::CoTaskMemFree( pszObjectIdArray[index] );
                    pszObjectIdArray[index] = NULL;
                }
            }
        }
        if ( NULL != pszObjectIdArray )
        {
            delete [] pszObjectIdArray;
            pszObjectIdArray = NULL;
        }
    }

    if ( NULL != pEnumPortableDeviceObjectIDs )
    {
        const DWORD dwCount = pEnumPortableDeviceObjectIDs->Release();
        LOGV( L"IEnumPortableDeviceObjectIDs::Release, count=%u\n", dwCount );
        pEnumPortableDeviceObjectIDs = NULL;
    }
}

int _tmain(int argc, _TCHAR* argv[])
{
    if ( NULL != argv )
    {
        for ( int index = 1; index < argc; ++index )
        {
            if ( NULL == argv[index] )
            {
                continue;
            }

            if ( 0 == _tcscmp( argv[index], L"--verbose" ) )
            {
                s_optVerbose = true;
            }
        }
    }

    bool needCoUninitialize = false;
    {
        const DWORD dwCoInit = COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE;
        const HRESULT hr = ::CoInitializeEx( NULL, dwCoInit );
        if ( FAILED(hr) )
        {
            ::DebugBreak();
        }
        else
        {
            needCoUninitialize = true;
        }
    }

    IPortableDeviceManager* pPortableDeviceManager = NULL;
    {
        const HRESULT hr = ::CoCreateInstance(
            CLSID_PortableDeviceManager
            , NULL
            , CLSCTX_INPROC_SERVER
            , IID_PPV_ARGS(&pPortableDeviceManager)
            );
        if ( FAILED(hr) )
        {
            LOGV( L"! Failed. CoCreateInstance CLSID_PortableDeviceManager, hr=0x%08x\n", hr );
        }
    }

    DWORD dwCountDeviceId = 0;
    if ( NULL != pPortableDeviceManager )
    {
        for ( size_t retry = 0; retry < 5; ++retry )
        {
            {
                const HRESULT hr = pPortableDeviceManager->GetDevices( NULL, &dwCountDeviceId );
                if ( FAILED(hr) )
                {
                    LOGV( L"! Failed. IPortableDeviceManager::GetDevices get count, hr=0x%08x\n", hr );
                }
                else
                {
                    LOGI( L"IPortableDeviceManager::GetDevices get count, device count=%u\n", dwCountDeviceId );
                    if ( 0 < dwCountDeviceId )
                    {
                        break;
                    }
                }
            }

            ::Sleep( 5 * 1000 );
            {
                const HRESULT hr = pPortableDeviceManager->RefreshDeviceList();
                if ( FAILED(hr) )
                {
                    LOGV( L"! Failed. IPortableDeviceManager::RefreshDeviceList, hr=0x%08x\n", hr );
                }
            }
        }
    }

    LPWSTR* pDeviceIdArray = NULL;
    if ( 0 < dwCountDeviceId )
    {
        pDeviceIdArray = new PWSTR[dwCountDeviceId];
        for ( size_t index = 0; index < dwCountDeviceId; ++index )
        {
            pDeviceIdArray[index] = NULL;
        }

        if ( NULL != pDeviceIdArray )
        {
            {
                const HRESULT hr = pPortableDeviceManager->GetDevices( pDeviceIdArray, &dwCountDeviceId );
                if ( FAILED(hr) )
                {
                    LOGV( L"! Failed. IPortableDeviceManager::GetDevice get id and count, hr=0x%08x\n", hr );
                }
                else
                {
                    /*
                    for ( size_t index = 0; index < dwCountDeviceId; ++index )
                    {
                        LOGV( L"%3u: %s\n", index, pDeviceIdArray[index] );
                    }
                    */
                }
            }
        }
    }

    if ( NULL != pDeviceIdArray )
    {
        for ( size_t index = 0; index < dwCountDeviceId; ++index )
        {
            if ( NULL == pDeviceIdArray[index] )
            {
                continue;
            }

            LOGV( L"%3u: %s\n", index, pDeviceIdArray[index] );
            {
                DWORD dwSize = 0;
                {
                    const HRESULT hr = pPortableDeviceManager->GetDeviceFriendlyName(
                        pDeviceIdArray[index]
                        , NULL
                        , &dwSize
                        );
                    if ( FAILED(hr) )
                    {
                        LOGV( L"! Failed. IPortableDeviceManager::GetDeviceFriendlyName get count, hr=0x%08x\n", hr );
                        continue;
                    }
                }

                LPWSTR pDispBuff = NULL;
                pDispBuff = new WCHAR[dwSize+1];
                if ( NULL != pDispBuff )
                {
                    pDispBuff[0] = L'\0';

                    {
                        const HRESULT hr = pPortableDeviceManager->GetDeviceFriendlyName(
                            pDeviceIdArray[index]
                            , pDispBuff
                            , &dwSize
                            );
                        if ( FAILED(hr) )
                        {
                            LOGV( L"! Failed. IPortableDeviceManager::GetDeviceFriendlyName get name and count, hr=0x%08x\n", hr );
                        }
                        else
                        {
                            LOGI( L"    FriendlyName: %s\n", pDispBuff );
                        }
                    }
                }

                if ( NULL != pDispBuff )
                {
                    delete [] pDispBuff;
                    pDispBuff = NULL;
                }
            }

            {
                DWORD dwSize = 0;
                {
                    const HRESULT hr = pPortableDeviceManager->GetDeviceManufacturer(
                        pDeviceIdArray[index]
                        , NULL
                        , &dwSize
                        );
                    if ( FAILED(hr) )
                    {
                        LOGV( L"! Failed. IPortableDeviceManager::GetDeviceManufacturer get count, hr=0x%08x\n", hr );
                        continue;
                    }
                }

                LPWSTR pDispBuff = NULL;
                pDispBuff = new WCHAR[dwSize+1];
                if ( NULL != pDispBuff )
                {
                    pDispBuff[0] = L'\0';

                    {
                        const HRESULT hr = pPortableDeviceManager->GetDeviceManufacturer(
                            pDeviceIdArray[index]
                            , pDispBuff
                            , &dwSize
                            );
                        if ( FAILED(hr) )
                        {
                            LOGV( L"! Failed. IPortableDeviceManager::GetDeviceManufacturer get name and count, hr=0x%08x\n", hr );
                        }
                        else
                        {
                            LOGI( L"    Manufacturer: %s\n", pDispBuff );
                        }
                    }
                }

                if ( NULL != pDispBuff )
                {
                    delete [] pDispBuff;
                    pDispBuff = NULL;
                }
            }

            {
                DWORD dwSize = 0;
                {
                    const HRESULT hr = pPortableDeviceManager->GetDeviceDescription(
                        pDeviceIdArray[index]
                        , NULL
                        , &dwSize
                        );
                    if ( FAILED(hr) )
                    {
                        LOGV( L"! Failed. IPortableDeviceManager::GetDeviceDescription get count, hr=0x%08x\n", hr );
                        continue;
                    }
                }

                LPWSTR pDispBuff = NULL;
                pDispBuff = new WCHAR[dwSize+1];
                if ( NULL != pDispBuff )
                {
                    pDispBuff[0] = L'\0';

                    {
                        const HRESULT hr = pPortableDeviceManager->GetDeviceDescription(
                            pDeviceIdArray[index]
                            , pDispBuff
                            , &dwSize
                            );
                        if ( FAILED(hr) )
                        {
                            LOGV( L"! Failed. IPortableDeviceManager::GetDeviceDescription get name and count, hr=0x%08x\n", hr );
                        }
                        else
                        {
                            LOGI( L"    Description : %s\n", pDispBuff );
                        }
                    }
                }

                if ( NULL != pDispBuff )
                {
                    delete [] pDispBuff;
                    pDispBuff = NULL;
                }
            }
        }
    }



    IPortableDeviceValues* pPortableDeviceValues = NULL;
    {
        const HRESULT hr = ::CoCreateInstance(
            CLSID_PortableDeviceValues
            , NULL
            , CLSCTX_INPROC_SERVER
            , IID_PPV_ARGS(&pPortableDeviceValues)
            );
        if ( FAILED(hr) )
        {
            LOGV( L"! Failed. CoCreateInstance CLSID_PortableDeviceValues, hr=0x%08x\n", hr );
        }
    }

    if ( NULL != pPortableDeviceValues )
    {
        const WCHAR szClientName[] = L"WPD client";
        const DWORD dwClientVersionMajor = 1;
        const DWORD dwClientVersionMinor = 0;
        const DWORD dwClientRevision = 1;
        {
            const HRESULT hr =
                pPortableDeviceValues->SetStringValue(
                    WPD_CLIENT_NAME, szClientName
                );
            if ( FAILED(hr) )
            {
                LOGV( L"! Failed. IPortableDeviceValues::SetStringValue WPD_CLIENT_NAME, hr=0x%08x\n", hr );
            }
        }
        {
            const HRESULT hr =
                pPortableDeviceValues->SetUnsignedIntegerValue(
                    WPD_CLIENT_MAJOR_VERSION, dwClientVersionMajor
                );
            if ( FAILED(hr) )
            {
                LOGV( L"! Failed. IPortableDeviceValues::SetUnsignedIntegerValue WPD_CLIENT_MAJOR_VERSION, hr=0x%08x\n", hr );
            }
        }
        {
            const HRESULT hr =
                pPortableDeviceValues->SetUnsignedIntegerValue(
                    WPD_CLIENT_MINOR_VERSION, dwClientVersionMinor
                );
            if ( FAILED(hr) )
            {
                LOGV( L"! Failed. IPortableDeviceValues::SetUnsignedIntegerValue WPD_CLIENT_MINOR_VERSION, hr=0x%08x\n", hr );
            }
        }
        {
            const HRESULT hr =
                pPortableDeviceValues->SetUnsignedIntegerValue(
                    WPD_CLIENT_REVISION, dwClientRevision
                );
            if ( FAILED(hr) )
            {
                LOGV( L"! Failed. IPortableDeviceValues::SetUnsignedIntegerValue WPD_CLIENT_REVISION, hr=0x%08x\n", hr );
            }
        }

        {
            const HRESULT hr =
                pPortableDeviceValues->SetUnsignedIntegerValue(
                    WPD_CLIENT_SECURITY_QUALITY_OF_SERVICE, SECURITY_IMPERSONATION
                );
            if ( FAILED(hr) )
            {
                LOGV( L"! Failed. IPortableDeviceValues::SetUnsignedIntegerValue WPD_CLIENT_SECURITY_QUALITY_OF_SERVICE, hr=0x%08x\n", hr );
            }
        }

        {
            const HRESULT hr =
                pPortableDeviceValues->SetUnsignedIntegerValue(
                    WPD_CLIENT_DESIRED_ACCESS, GENERIC_READ
                );
            if ( FAILED(hr) )
            {
                LOGV( L"! Failed. IPortableDeviceValues::SetUnsignedIntegerValue WPD_CLIENT_DESIRED_ACCESS, hr=0x%08x\n", hr );
            }
        }

        {
            const HRESULT hr =
                pPortableDeviceValues->SetUnsignedIntegerValue(
                    WPD_CLIENT_SHARE_MODE, FILE_SHARE_READ|FILE_SHARE_WRITE
                );
            if ( FAILED(hr) )
            {
                LOGV( L"! Failed. IPortableDeviceValues::SetUnsignedIntegerValue WPD_CLIENT_DESIRED_ACCESS, hr=0x%08x\n", hr );
            }
        }

    }

    {
        for ( size_t index = 0; index < dwCountDeviceId; ++index )
        {
            if ( NULL == pDeviceIdArray[index] )
            {
                continue;
            }

            bool readyPortableDevice = false;
            IPortableDevice* pPortableDevice = NULL;
            {
                const HRESULT hr = ::CoCreateInstance(
                    CLSID_PortableDevice
                    , NULL
                    , CLSCTX_INPROC_SERVER
                    , IID_PPV_ARGS(&pPortableDevice)
                    );
                if ( FAILED(hr) )
                {
                    LOGV( L"! Failed. CoCreateInstance CLSID_PortableDevice, hr=0x%08x\n", hr );
                }
            }

            if ( NULL != pPortableDevice )
            {
                {
                    const HRESULT hr = pPortableDevice->Open( pDeviceIdArray[index], pPortableDeviceValues );
                    if ( FAILED(hr) )
                    {
                        LOGV( L"! Failed. IPortableDevice::Open, hr=0x%08x\n", hr );
                    }
                    else
                    {
                        readyPortableDevice = true;
                    }
                }
            }

            if ( NULL != pPortableDevice )
            {
                if ( readyPortableDevice )
                {
                    IPortableDeviceContent* pPortableDeviceContent = NULL;
                    {
                        const HRESULT hr = pPortableDevice->Content( &pPortableDeviceContent );
                        if ( FAILED(hr) )
                        {
                            LOGV( L"! Failed. IPortableDevice::Content, hr=0x%08x\n", hr );
                        }
                    }
                    if ( NULL != pPortableDeviceContent )
                    {
                        s_dwCountContent = 0;
                        wpdEnumContent_RecursiveEnumerate( WPD_DEVICE_OBJECT_ID, pPortableDeviceContent );
                        LOGI( L"    Content count=%u\n", s_dwCountContent );
                    }

                    if ( NULL != pPortableDeviceContent )
                    {
                        const DWORD dwCount = pPortableDeviceContent->Release();
                        LOGV( L"IPortableDeviceContent::Release, count=%u\n", dwCount );
                        pPortableDeviceContent = NULL;
                    }
                }
            }

            if ( NULL != pPortableDevice )
            {
                const DWORD dwCount = pPortableDevice->Release();
                LOGV( L"IPortableDevice::Release, count=%u\n", dwCount );
                pPortableDevice = NULL;
            }
        }
    }


    if ( NULL != pPortableDeviceValues )
    {
        const DWORD dwCount = pPortableDeviceValues->Release();
        LOGV( L"IPortableDeviceValues::Release, count=%u\n", dwCount );
        pPortableDeviceValues = NULL;
    }

    //FreePortableDevicePnPIDs( pDeviceIdArray, dwCountDeviceId );
    if ( NULL != pDeviceIdArray )
    {
        for ( size_t index = 0; index < dwCountDeviceId; ++index )
        {
            if ( NULL != pDeviceIdArray[index] )
            {
                ::CoTaskMemFree( pDeviceIdArray[index] );
                pDeviceIdArray[index] = NULL;
            }
        }
    }
    if ( NULL != pDeviceIdArray )
    {
        delete [] pDeviceIdArray;
        pDeviceIdArray = NULL;
    }

    if ( NULL != pPortableDeviceManager )
    {
        const DWORD dwCount = pPortableDeviceManager->Release();
        LOGV( L"IPortableDeviceManager::Release, count=%u\n", dwCount );
        pPortableDeviceManager = NULL;
    }

    if ( needCoUninitialize )
    {
        ::CoUninitialize();
    }

#if 0
    if ( ::IsDebuggerPresent() )
    {
        ::DebugBreak();
    }
#endif

	return 0;
}

