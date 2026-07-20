#include "Pch.h"
#include "ShaderHelper.h"
#include <fstream>

#pragma comment(lib, "dxcompiler.lib")

using namespace Microsoft::WRL;

namespace ShaderHelper {

    bool CompileShader(const std::wstring& path, const std::wstring& entryPoint,
        const std::wstring& profile, IDxcBlob** outBlob) {
        using Microsoft::WRL::ComPtr;

        ComPtr<IDxcUtils> pUtils;
        ComPtr<IDxcCompiler3> pCompiler;
        DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&pUtils));
        DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&pCompiler));

        ComPtr<IDxcIncludeHandler> pIncludeHandler;
        pUtils->CreateDefaultIncludeHandler(&pIncludeHandler);

        // 셰이더 파일 로드
        ComPtr<IDxcBlobEncoding> pSourceBlob;
        HRESULT hr = pUtils->LoadFile(path.c_str(), nullptr, &pSourceBlob);
        if (FAILED(hr)) {
            std::string pathA(path.begin(), path.end());
            spdlog::error("Shader file not found: {}", pathA);
            MessageBoxW(nullptr, L"셰이더 파일을 찾을 수 없습니다.", path.c_str(), MB_OK | MB_ICONERROR);
            return false;
        }

        DxcBuffer sourceBuffer;
        sourceBuffer.Ptr = pSourceBlob->GetBufferPointer();
        sourceBuffer.Size = pSourceBlob->GetBufferSize();
        sourceBuffer.Encoding = DXC_CP_ACP;

        // 컴파일 인자 구성
        std::vector<LPCWSTR> arguments = {
            path.c_str(),
            L"-E", entryPoint.c_str(),
            L"-T", profile.c_str()
        };

#if defined(_DEBUG)
        arguments.push_back(L"-Zi");
        arguments.push_back(L"-Qembed_debug");
        arguments.push_back(L"-Od");
#endif

        ComPtr<IDxcResult> pResults;
        pCompiler->Compile(
            &sourceBuffer,
            arguments.data(), (UINT32)arguments.size(),
            pIncludeHandler.Get(),
            IID_PPV_ARGS(&pResults)
        );

        // 에러 확인
        ComPtr<IDxcBlobUtf8> pErrors;
        IDxcBlobUtf16* pErrorName = nullptr;
        pResults->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), &pErrorName);
        if (pErrors != nullptr && pErrors->GetStringLength() > 0) {
            std::string errorLog = pErrors->GetStringPointer();
            spdlog::error("Shader Compile Failed!\n{}", errorLog);

            // 파일에 에러 로그 출력
            std::ofstream fout("shader-error.txt");
            fout << errorLog;
            fout.close();

            MessageBoxW(nullptr, L"Shader 컴파일 에러 발생. shader-error.txt를 확인하세요.", path.c_str(), MB_OK | MB_ICONERROR);
            return false;
        }

        pResults->GetStatus(&hr);
        if (FAILED(hr)) return false;

        pResults->GetResult(outBlob);
        return true;
    } // CompileShader

    bool InitVertexShader(const std::wstring& path, IDxcBlob** outBlob) {
        return CompileShader(path, entryPoint, vsProfile, outBlob);
    } // InitVertexShader

    bool InitPixelShader(const std::wstring& path, IDxcBlob** outBlob) {
        return CompileShader(path, entryPoint, psProfile, outBlob);
    } // InitPixelShader

    bool InitComputeShader(const std::wstring& path, IDxcBlob** outBlob) {
        return CompileShader(path, entryPoint, csProfile, outBlob);
	} // InitComputeShader

    bool InitGeometryShader(const std::wstring& path, IDxcBlob** outBlob) {
        return CompileShader(path, entryPoint, gsProfile, outBlob);
	} // InitGeometryShader

    bool InitHullShader(const std::wstring& path, IDxcBlob** outBlob) {
        return CompileShader(path, entryPoint, hsProfile, outBlob);
	} // InitHullShader

    bool InitDomainShader(const std::wstring& path, IDxcBlob** outBlob) {
        return CompileShader(path, entryPoint, dsProfile, outBlob);
    } // InitDomainShader

} // namespace ShaderHelper