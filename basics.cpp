#pragma once
#define WIN32_LEAN_AND_MEAN
#include <cstdio>
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <exception>
#include <iostream>
#include <wrl.h>
#include <shellapi.h>
#include <algorithm>
#include <cassert>
#include <chrono>
#include <d3dx12/d3dx12.h>

#if defined(CreateWindow)
#undef CreateWindow
#endif

#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif

extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 711;}

extern "C" { __declspec(dllexport) extern const char8_t* D3D12SDKPath = u8".\\D3D12\\"; }

using namespace DirectX;
using namespace Microsoft::WRL;



namespace DxDebug
{
	class com_exception : public std::exception
	{
	public:
		com_exception(HRESULT hr) : result(hr){};

		virtual const char* what() const override
		{
			static char s_str[64] = {};
			sprintf_s(s_str, "Failure with HRESULT of %08X", static_cast<unsigned int>(result));
			return s_str;
		}
	private:
		HRESULT result;
	};

	inline void ThrowIfFailed(HRESULT hr)
	{
		if (FAILED(hr))
		{
			throw com_exception(hr);
		}
	}
}

namespace DxHelper
{
	// 为fence创建事件句柄
	HANDLE CreateEventHandle()
	{
		HANDLE fence_event;
		fence_event = CreateEvent(nullptr, false, false, nullptr);
		assert(fence_event && "failed to create fencde event.");

		return fence_event;
	}
	// 让GPU向fence发送信号
	uint64_t Signal(ComPtr<ID3D12CommandQueue> command_queue, ComPtr<ID3D12Fence1> fence, uint64_t& fence_value)
	{
		uint64_t fence_value_for_signal = ++ fence_value;
		DxDebug::ThrowIfFailed(command_queue->Signal(fence.Get(), fence_value_for_signal));

		return fence_value_for_signal;
	}
	// 直到fence信号越过fence数值或超过设置的等待时间，一直挂起CPU线程
	void WaitForTheFrame(ComPtr<ID3D12Fence1> fence, uint64_t fence_value, HANDLE fence_event, std::chrono::milliseconds duration = std::chrono::milliseconds::max())
	{
		if (fence->GetCompletedValue() < fence_value)
		{
			DxDebug::ThrowIfFailed(fence->SetEventOnCompletion(fence_value, fence_event));
			WaitForSingleObject(fence_event, static_cast<DWORD>(duration.count()));
		}
	}
	// 调用signal和waitfortheframe，确保GPU执行完所有命令
	void FlushGPU(ComPtr<ID3D12CommandQueue> command_queue, ComPtr<ID3D12Fence1> fence, uint64_t& fence_value, HANDLE fence_event)
	{
		uint64_t fence_value_for_signal = Signal(command_queue, fence, fence_value);
		std::chrono::milliseconds duration = std::chrono::milliseconds::max();
		WaitForTheFrame(fence, fence_value_for_signal, fence_event, duration);
	}
}

bool m_use_warp = false;

uint32_t m_client_width = 1280;
uint32_t m_client_height = 720;

bool m_initialized = false;
static const uint8_t m_back_buffer_count = 3;

float m_fov;

// 用于构建MVP矩阵
XMMATRIX m_model_matrix;
XMMATRIX m_view_matrix;
XMMATRIX m_projection_matrix;

bool m_content_loaded;

// 管线对象
D3D12_VIEWPORT m_viewport;
D3D12_RECT m_scissor_rect;
ComPtr<IDXGISwapChain4> m_swap_chain;
ComPtr<ID3D12Device10> m_device;
ComPtr<ID3D12Resource2> m_back_buffers[m_back_buffer_count];
ComPtr<ID3D12CommandAllocator> m_command_allocators[m_back_buffer_count];
ComPtr<ID3D12CommandQueue> m_command_queue;
ComPtr<ID3D12RootSignature> m_root_signature;
ComPtr<ID3D12DescriptorHeap> m_rtv_heap;
ComPtr<ID3D12PipelineState> m_pipeline_state;
ComPtr<ID3D12GraphicsCommandList9> m_command_list;
UINT m_rtv_descriptor_size;

// 应用资源
ComPtr<ID3D12Resource2> m_vertex_buffer;
D3D12_VERTEX_BUFFER_VIEW m_vertex_buffer_view;
ComPtr<ID3D12Resource2> m_index_buffer;
D3D12_INDEX_BUFFER_VIEW m_index_buffer_view;
ComPtr<ID3D12Resource2> m_depth_buffer;
ComPtr<ID3D12DescriptorHeap> m_dsv_heap;


// 同步对象
UINT m_current_back_buffer_index;
HANDLE m_fence_event;
ComPtr<ID3D12Fence1> m_fence;
UINT64 m_fence_value;
UINT64 m_frame_fence_values[m_back_buffer_count]{};

// 交换链控制
bool m_vsync = true;
bool m_tearing_supported = false;
bool m_fullscreen = false;
BOOL m_allow_tearing = false;

// 窗口资源
HWND m_hwnd;
RECT m_window_rect;


// 顶点结构
struct Vertex
{
	XMFLOAT3 position;
	XMFLOAT3 color;
};

// 方体顶点列表
static Vertex g_Vertices[8] = {
    { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f) }, // 0
    { XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) }, // 1
    { XMFLOAT3(1.0f,  1.0f, -1.0f), XMFLOAT3(1.0f, 1.0f, 0.0f) }, // 2
    { XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) }, // 3
    { XMFLOAT3(-1.0f, -1.0f,  1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) }, // 4
    { XMFLOAT3(-1.0f,  1.0f,  1.0f), XMFLOAT3(0.0f, 1.0f, 1.0f) }, // 5
    { XMFLOAT3(1.0f,  1.0f,  1.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) }, // 6
    { XMFLOAT3(1.0f, -1.0f,  1.0f), XMFLOAT3(1.0f, 0.0f, 1.0f) }  // 7
};

// 方体顶点顺序
static WORD g_Indicies[36] =
{
    0, 1, 2, 0, 2, 3,
    4, 6, 5, 4, 7, 6,
    4, 5, 1, 4, 1, 0,
    3, 2, 6, 3, 6, 7,
    1, 5, 6, 1, 6, 2,
    4, 0, 3, 4, 3, 7
};

const XMVECTOR rotation_axis = XMVectorSet(0, 1, 1, 0);
const XMVECTOR eye_position = XMVectorSet(0, 0, -10, 1);
const XMVECTOR focus_point = XMVectorSet(0, 0, 0, 1);
const XMVECTOR up_direction = XMVectorSet(0, 1, 0, 0);


namespace BufferHelper
{
	// 用于上传缓冲区资源
	void UpdateBufferResource(ComPtr<ID3D12GraphicsCommandList9> command_list,
		ID3D12Resource2** p_destination_resource,
		ID3D12Resource2** p_intermediate_resource,
		size_t num_elements, size_t element_size, const void* buffer_data)
	{
		// 缓冲区总大小
		size_t buffer_size = num_elements * element_size;

		// 填写堆属性
		D3D12_HEAP_PROPERTIES default_heap_prop = {
		D3D12_HEAP_TYPE_DEFAULT,
		D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		D3D12_MEMORY_POOL_UNKNOWN, 1u, 1u};
		// 填写资源描述
		D3D12_RESOURCE_DESC default_buffer_desc = {
		D3D12_RESOURCE_DIMENSION_BUFFER,
		0,
		buffer_size,
		1,
		1,
		1,
		DXGI_FORMAT_UNKNOWN,
		{1u,0u},
		D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE};
		// 创建默认缓冲区(以后会优化成CreateCommittedResource3)
		DxDebug::ThrowIfFailed(m_device->CreateCommittedResource(&default_heap_prop, D3D12_HEAP_FLAG_NONE, &default_buffer_desc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(p_destination_resource)));

		if (buffer_data)
		{
			// 填写堆属性
			D3D12_HEAP_PROPERTIES upload_heap_prop = {
			D3D12_HEAP_TYPE_UPLOAD,
			D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
			D3D12_MEMORY_POOL_UNKNOWN, 1u, 1u};
			// 填写资源描述
			D3D12_RESOURCE_DESC upload_buffer_desc = {
			D3D12_RESOURCE_DIMENSION_BUFFER,
			0,
			buffer_size,
			1,
			1,
			1,
			DXGI_FORMAT_UNKNOWN,
			{1u,0u},
			D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
			D3D12_RESOURCE_FLAG_NONE};
			// 创建上传缓冲区(以后会优化成CreateCommittedResource3)
			DxDebug::ThrowIfFailed(m_device->CreateCommittedResource(&upload_heap_prop, D3D12_HEAP_FLAG_NONE, &upload_buffer_desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(p_intermediate_resource)));

			// 描述要上传到GPU资源的子资源数据
			D3D12_SUBRESOURCE_DATA subresource_data{};
			subresource_data.pData = buffer_data;
			subresource_data.RowPitch = buffer_size;
			subresource_data.SlicePitch = subresource_data.RowPitch;

			// 调用d3dx12辅助库，使用上传堆中的中间缓冲区将CPU缓冲区数据上传到默认堆中的GPU资源
			UpdateSubresources(command_list.Get(), *p_destination_resource, *p_intermediate_resource, 0, 0, 1, &subresource_data);
		}
	}

	// 导入所有用于渲染的资源
	bool LoadContent()
	{
		// 上传顶点缓冲区资源
		ComPtr<ID3D12Resource2> intermediate_vertex_buffer;
		UpdateBufferResource(m_command_list.Get(), &m_vertex_buffer, &intermediate_vertex_buffer, _countof(g_Vertices), sizeof(Vertex), g_Vertices);
		// 创建顶点缓冲视图
		m_vertex_buffer_view.BufferLocation = m_vertex_buffer->GetGPUVirtualAddress();
		m_vertex_buffer_view.SizeInBytes = sizeof(g_Vertices);
		m_vertex_buffer_view.StrideInBytes = sizeof(Vertex);

		// 上传索引缓冲区资源
		ComPtr<ID3D12Resource2> intermediate_index_buffer;
		UpdateBufferResource(m_command_list.Get(), &m_index_buffer, &intermediate_index_buffer, _countof(g_Indicies), sizeof(WORD), g_Indicies);
		// 创建索引缓冲视图
		m_index_buffer_view.BufferLocation = m_index_buffer->GetGPUVirtualAddress();
		m_index_buffer_view.Format = DXGI_FORMAT_R16_UINT;
		m_index_buffer_view.SizeInBytes = sizeof(g_Indicies);

		// 填写dsv描述并创建dsv描述符堆
		D3D12_DESCRIPTOR_HEAP_DESC dsv_heap_desc{};
		dsv_heap_desc.NumDescriptors = 1;
		dsv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		DxDebug::ThrowIfFailed(m_device->CreateDescriptorHeap(&dsv_heap_desc, IID_PPV_ARGS(m_dsv_heap.GetAddressOf())));

		// 导入编译好的shader
		ComPtr<ID3DBlob> vertex_blob;
		DxDebug::ThrowIfFailed(D3DReadFileToBlob(L"VertexShader.cso", &vertex_blob));
		ComPtr<ID3DBlob> pixel_blob;
		DxDebug::ThrowIfFailed(D3DReadFileToBlob(L"PixelShader.cso", &pixel_blob));
		
		// 创建顶点输入布局
		D3D12_INPUT_ELEMENT_DESC input_layout[] = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		};

		// 创建根签名
		D3D12_FEATURE_DATA_ROOT_SIGNATURE feature_data{};
		feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
		if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &feature_data, sizeof(feature_data))))
		{
			feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
		}

		// 允许输入布局并拒绝不必要的访问
		D3D12_ROOT_SIGNATURE_FLAGS root_signature_flags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

		// 根常量描述
		D3D12_ROOT_CONSTANTS root_constants{};
		root_constants.Num32BitValues = sizeof(XMMATRIX) / 4;
		root_constants.ShaderRegister = 0;
		root_constants.RegisterSpace = 0;
		// 将根参数初始化为32比特常量根参数
		D3D12_ROOT_PARAMETER1 root_parameters[1];
		root_parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		root_parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		root_parameters[0].DescriptorTable.NumDescriptorRanges = 1;
		root_parameters[0].Constants = root_constants;

		// 填写根签名描述
		D3D12_ROOT_SIGNATURE_DESC1 root_signature_desc{};
		root_signature_desc.Flags = root_signature_flags;
		root_signature_desc.NumParameters = _countof(root_parameters);
		root_signature_desc.pParameters = root_parameters;
		root_signature_desc.NumStaticSamplers = 0;
		root_signature_desc.pStaticSamplers = nullptr;
		// 确定描述版本
		D3D12_VERSIONED_ROOT_SIGNATURE_DESC versioned_desc{};
		versioned_desc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
		versioned_desc.Desc_1_1 = root_signature_desc;

		// 序列化描述
		ComPtr<ID3DBlob> root_signature_blob;
		ComPtr<ID3DBlob> error_blob;
		try
		{
			DxDebug::ThrowIfFailed(D3D12SerializeVersionedRootSignature(&versioned_desc, root_signature_blob.GetAddressOf(), error_blob.GetAddressOf()));
		// 创建根签名
			DxDebug::ThrowIfFailed(m_device->CreateRootSignature(0, root_signature_blob->GetBufferPointer(), root_signature_blob->GetBufferSize(), IID_PPV_ARGS(m_root_signature.GetAddressOf())));
		}
		catch (std::exception e)
		{
			const char* err_str = (const char*)error_blob->GetBufferPointer();
			std::cout << err_str;
		}

		// 创建管线流令牌用于定义管线，d3dx12库
		struct PipelineStateStream
		{
		    CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE p_root_signature;
		    CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT input_layout;
		    CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY primitive_topology_type;
		    CD3DX12_PIPELINE_STATE_STREAM_VS VS;
		    CD3DX12_PIPELINE_STATE_STREAM_PS PS;
		    CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT dsv_format;
		    CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS rtv_formats;
		} pipeline_state_stream;

		// 定义rtv呈现目标的数量和格式
		D3D12_RT_FORMAT_ARRAY rtv_format{};
		rtv_format.NumRenderTargets = 1;
		rtv_format.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		// 填写管线流
		pipeline_state_stream.p_root_signature = m_root_signature.Get();
		pipeline_state_stream.input_layout = {input_layout, _countof(input_layout)};
		pipeline_state_stream.primitive_topology_type = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		pipeline_state_stream.VS = CD3DX12_SHADER_BYTECODE(vertex_blob.Get());
		pipeline_state_stream.PS = CD3DX12_SHADER_BYTECODE(pixel_blob.Get());
		pipeline_state_stream.dsv_format = DXGI_FORMAT_D32_FLOAT;
		pipeline_state_stream.rtv_formats = rtv_format;
		// 创建管线对象
		D3D12_PIPELINE_STATE_STREAM_DESC pipeline_state_stream_desc{sizeof(PipelineStateStream), &pipeline_state_stream};
		DxDebug::ThrowIfFailed(m_device->CreatePipelineState(&pipeline_state_stream_desc, IID_PPV_ARGS(m_pipeline_state.GetAddressOf())));

		DxDebug::ThrowIfFailed(m_command_list->Close());
		// 创建只有一个命令列表的常量数组
		ID3D12CommandList* const pp_command_lists[]{m_command_list.Get()};
		// 执行命令列表并获得隔栏值
		m_command_queue->ExecuteCommandLists(1, pp_command_lists);
		uint64_t fence_value = DxHelper::Signal(m_command_queue, m_fence, m_fence_value);

		DxHelper::WaitForTheFrame(m_fence, fence_value, m_fence_event);

		m_content_loaded = true;

		// 创建用于清除的颜色
		D3D12_CLEAR_VALUE optimized_clear_value{};
		optimized_clear_value.Format = DXGI_FORMAT_D32_FLOAT;
		optimized_clear_value.DepthStencil = {1.0f, 0};

		// 填写堆属性
		D3D12_HEAP_PROPERTIES depth_heap_prop = {
			D3D12_HEAP_TYPE_DEFAULT,
			D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
			D3D12_MEMORY_POOL_UNKNOWN, 1u, 1u};
		// 填写资源描述
		D3D12_RESOURCE_DESC depth_buffer_desc = {
			D3D12_RESOURCE_DIMENSION_TEXTURE2D,
			0,
			m_client_width,
			m_client_height,
			1,
			0,
			DXGI_FORMAT_D32_FLOAT,
			{1u,0u},
			D3D12_TEXTURE_LAYOUT_UNKNOWN,
			D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL};
		// 创建深度缓冲区(以后会优化成CreateCommittedResource3)
		DxDebug::ThrowIfFailed(m_device->CreateCommittedResource(&depth_heap_prop, D3D12_HEAP_FLAG_NONE, &depth_buffer_desc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &optimized_clear_value, IID_PPV_ARGS(m_depth_buffer.GetAddressOf())));

		// 填写深度视图描述
		D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc{};
		dsv_desc.Format = DXGI_FORMAT_D32_FLOAT;
		dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsv_desc.Texture2D.MipSlice = 0;
		dsv_desc.Flags = D3D12_DSV_FLAG_NONE;
		// 创建深度视图
		m_device->CreateDepthStencilView(m_depth_buffer.Get(), &dsv_desc, m_dsv_heap->GetCPUDescriptorHandleForHeapStart());

		return true;
	}
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// 通过提供命令行参数来覆盖一些全局定义的变量
void ParseCommandLineArguments()
{
	int argc;
	wchar_t** argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);

	for (size_t i = 0; i < argc; ++i)
	{
		// 指定窗口宽度
		if (::wcscmp(argv[i], L"-w") == 0 || ::wcscmp(argv[i], L"--width") == 0)
		{
			m_client_width = ::wcstol(argv[++i], nullptr, 10);
		}
		// 指定窗口高度
		if (::wcscmp(argv[i], L"-h") == 0 || ::wcscmp(argv[i], L"--height") == 0)
		{
			m_client_height = ::wcstol(argv[++i], nullptr, 10);
		}
		// 使用虚拟渲染设备warp
		if (::wcscmp(argv[i], L"-warp") == 0 || ::wcscmp(argv[i], L"--warp") == 0)
		{
			m_use_warp = true;
		}
	}
	// 释放CommandLineToArgvW的内存
	::LocalFree(argv);
}
void RegisterWindowClass(HINSTANCE hInstance, const wchar_t* windowClassName)
{
	WNDCLASSEXW wcs = {};

    wcs.cbSize = sizeof(WNDCLASSEX); // WNDCLASSEX必须
    wcs.style = CS_HREDRAW | CS_VREDRAW;;
    wcs.lpfnWndProc = &WindowProc; // 必须，窗口过程指针
    wcs.cbClsExtra = 0;
    wcs.cbWndExtra = DLGWINDOWEXTRA;
    wcs.hInstance = hInstance; // 必须，应用程序实例句柄
    wcs.hIcon = nullptr;
    wcs.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcs.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcs.lpszMenuName = nullptr;
    wcs.lpszClassName = windowClassName; // 必须，标识窗口类的字符串
    wcs.hIconSm = nullptr;

    static ATOM atom = ::RegisterClassExW(&wcs);
    assert(atom > 0 && "Fail to register window class");
}
HWND CreateWindow(const wchar_t* windowClassName, HINSTANCE hInstance, const wchar_t* windowTitle, uint32_t width, uint32_t height)
{
	// 从系统获得显示器长宽
    int screen_width = ::GetSystemMetrics(SM_CXSCREEN);
    int screen_height = ::GetSystemMetrics(SM_CYSCREEN);

    // 创建窗口矩形
    RECT window_rect = {0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};
    ::AdjustWindowRectEx(&window_rect, WS_OVERLAPPEDWINDOW, false, 0);

    // 通过窗口矩形计算窗口长宽
    int window_width = window_rect.right - window_rect.left;
    int window_height = window_rect.bottom - window_rect.top;

    // 计算窗口位置
    int window_x = std::max<int>(0, (screen_width - window_width) / 2);
    int window_y = std::max<int>(0, (screen_height - window_height) / 2);

	// 创建新窗口实例
    HWND hwnd = CreateWindowExW(
		0, // 扩展窗口样式，无特别样式则设为0
        windowClassName, // 窗口类名称
        windowTitle,
        WS_OVERLAPPEDWINDOW,
        window_x,
        window_y,
        window_width,
        window_height,
		nullptr,
        nullptr,
        hInstance,
        nullptr // 指向void*的任意数据的指针，可以使用此值将数据结构传递到窗口过程
    );

	// 初始化裁切矩形到最大值以确保无论屏幕大小如何都能覆盖整个屏幕
	m_scissor_rect = {0, 0, LONG_MAX, LONG_MAX};
	// 初始化渲染的屏幕的可视部分等于窗口大小
	m_viewport = {0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), D3D12_MIN_DEPTH, D3D12_MAX_DEPTH};
	m_fov = 45.0;
	m_content_loaded = false;

	assert(hwnd && "Failed to create window");
	return hwnd;
}

void Initial(HWND hwnd);
void Update();
void Render();
void Resize(uint32_t width, uint32_t height);
void SetFullScreen(bool fullscreen);




void Initial(HWND hwnd)
{
	// 启用调试层
#if defined(_DEBUG)
	{
		ComPtr<ID3D12Debug6> debug_controller;
		DxDebug::ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(debug_controller.GetAddressOf())));
		debug_controller->EnableDebugLayer();
		//DxDebug::ThrowIfFailed(debug_controller->QueryInterface(IID_PPV_ARGS(debug_controller.GetAddressOf())));
		//debug_controller->SetEnableGPUBasedValidation(true);
	}
#endif

	m_hwnd = hwnd;

	// 创建工厂
	ComPtr<IDXGIFactory7> p_factory;
	UINT create_factory_flags = 0;
#if defined(_DEBUG)
	create_factory_flags = DXGI_CREATE_FACTORY_DEBUG;
#endif
	DxDebug::ThrowIfFailed(CreateDXGIFactory2(create_factory_flags, IID_PPV_ARGS(p_factory.GetAddressOf())));
	// 创建设备适配器
	ComPtr<IDXGIAdapter4> hardware_adapter;
	D3D_FEATURE_LEVEL feature_Level = D3D_FEATURE_LEVEL_12_2;
	if (m_use_warp)
	{
		ComPtr<IDXGIAdapter1> dxgi_adapter1;
		DxDebug::ThrowIfFailed(p_factory->EnumWarpAdapter(IID_PPV_ARGS(dxgi_adapter1.GetAddressOf())));
		DxDebug::ThrowIfFailed(dxgi_adapter1.As(&hardware_adapter));
	}
	else
	{
		// 获得最高性能的适配器
		for (UINT adapter_index = 0;
			p_factory->EnumAdapterByGpuPreference(adapter_index, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(hardware_adapter.GetAddressOf())) != DXGI_ERROR_NOT_FOUND;
			++adapter_index)
		{
			DXGI_ADAPTER_DESC3 dxgiAdapterDesc;
			hardware_adapter->GetDesc3(&dxgiAdapterDesc);
			if ((dxgiAdapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 && SUCCEEDED(D3D12CreateDevice(hardware_adapter.Get(), feature_Level, _uuidof(ID3D12Device), nullptr)))
			{
				break;
			}
		}

	}
	// 创建设备
	DxDebug::ThrowIfFailed(D3D12CreateDevice(hardware_adapter.Get(), feature_Level, IID_PPV_ARGS(m_device.GetAddressOf())));

	// 检查是否支持vrr可变刷新率
	if (SUCCEEDED(p_factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &m_allow_tearing, sizeof(m_allow_tearing))))
	{
		m_allow_tearing = true;
	}

	ComPtr<ID3D12DebugDevice2> debug_device;
	// 启用设备调试层
#if defined(_DEBUG)
	DxDebug::ThrowIfFailed(m_device->QueryInterface(IID_PPV_ARGS(debug_device.GetAddressOf())));
	ComPtr<ID3D12InfoQueue> p_info_queue;
	DxDebug::ThrowIfFailed(m_device.As(&p_info_queue));
	p_info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
	p_info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
	p_info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
	D3D12_MESSAGE_SEVERITY Severities[] = {D3D12_MESSAGE_SEVERITY_INFO};
	D3D12_MESSAGE_ID DenyIds[] = {
		D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,   // I'm really not sure how to avoid this message.
		D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,                         // This warning occurs when using capture frame while graphics debugging.
		D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,                       // This warning occurs when using capture frame while graphics debugging.
	};
	D3D12_INFO_QUEUE_FILTER NewFilter = {};
	NewFilter.DenyList.NumSeverities = _countof(Severities);
	NewFilter.DenyList.pSeverityList = Severities;
	NewFilter.DenyList.NumIDs = _countof(DenyIds);
	NewFilter.DenyList.pIDList = DenyIds;
	DxDebug::ThrowIfFailed(p_info_queue->PushStorageFilter(&NewFilter));
#endif
	// 填写命令列队说明
	D3D12_COMMAND_QUEUE_DESC queue_desc{};
	queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_HIGH;
	queue_desc.NodeMask = 0;
	// 创建命令列队
	DxDebug::ThrowIfFailed(m_device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(m_command_queue.GetAddressOf())));

	// 填写交换链描述
	DXGI_SWAP_CHAIN_DESC1 swap_chain_desc{};
	swap_chain_desc.Width = 0;
	swap_chain_desc.Height = 0;
	swap_chain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swap_chain_desc.Stereo = false;
	swap_chain_desc.SampleDesc = {1, 0};
	swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swap_chain_desc.BufferCount = m_back_buffer_count;
	swap_chain_desc.Scaling = DXGI_SCALING_STRETCH;
	swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swap_chain_desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swap_chain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | (m_allow_tearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0);
	// 创建交换链
	ComPtr<IDXGISwapChain1> swap_chain;
	DxDebug::ThrowIfFailed(p_factory->CreateSwapChainForHwnd(m_command_queue.Get(), m_hwnd, &swap_chain_desc, nullptr, nullptr, swap_chain.GetAddressOf()));
	DxDebug::ThrowIfFailed(p_factory->MakeWindowAssociation(m_hwnd, DXGI_MWA_NO_ALT_ENTER));
	DxDebug::ThrowIfFailed(swap_chain.As(&m_swap_chain));
	// 获取当前后台缓冲区的索引
	m_current_back_buffer_index = m_swap_chain->GetCurrentBackBufferIndex();

	// 填写描述符堆描述
	D3D12_DESCRIPTOR_HEAP_DESC heap_desc{};
	heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	heap_desc.NumDescriptors = m_back_buffer_count;
	// 创建rtv堆
	DxDebug::ThrowIfFailed(m_device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(m_rtv_heap.GetAddressOf())));
	// 获得堆增量大小
	m_rtv_descriptor_size = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// 创建命令分配器
	for (int i = 0; i < m_back_buffer_count; ++i)
	{
		DxDebug::ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_command_allocators[i].GetAddressOf())));
	}

	// 创建围栏
	DxDebug::ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_fence.GetAddressOf())));

	// 为交换链中每个后备缓冲区创建渲染目标视图
	D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = m_rtv_heap->GetCPUDescriptorHandleForHeapStart();
	for (int i = 0; i < m_back_buffer_count; ++i)
	{
		// 获得交换链中的缓冲区
		DxDebug::ThrowIfFailed(m_swap_chain->GetBuffer(i, IID_PPV_ARGS(m_back_buffers[i].GetAddressOf())));
		// 为缓冲区创建rtv
		m_device->CreateRenderTargetView(m_back_buffers[i].Get(), nullptr, rtv_handle);
		// 将handle的指针偏移一个rtv的位置
		rtv_handle.ptr += m_rtv_descriptor_size;
	}

	// 创建关闭的命令列表
	DxDebug::ThrowIfFailed(m_device->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(m_command_list.GetAddressOf())));
	// 初始化并打开命令列表
	DxDebug::ThrowIfFailed(m_command_list->Reset(m_command_allocators[m_current_back_buffer_index].Get(), nullptr));
	// 加载资源
	BufferHelper::LoadContent();

	// 创建围栏
	DxDebug::ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_fence.GetAddressOf())));
	// 创建事件句柄
	m_fence_event = DxHelper::CreateEventHandle();

}

void Update()
{
	static uint64_t frame_counter = 0;
	static double elapsed_seconds = 0.0;
	static std::chrono::high_resolution_clock clock;
	static auto time_begin = clock.now();
	// 每次update增加一帧计数，并更新起始时间和间隔时间
	++frame_counter;
	auto time_now = clock.now();
	auto delta_time = time_now - time_begin;
	time_begin = time_now;
	// 多次update后每当流逝的时间到1s，通过帧计数计算fps
	elapsed_seconds += delta_time.count() * 1e-9;
	if (elapsed_seconds > 1.0)
	{
		wchar_t buffer[256];
		auto fps = frame_counter / elapsed_seconds;
		wsprintfW(buffer, L"256", "FPS: %f/n", fps);
		OutputDebugString(buffer);

		frame_counter = 0;
		elapsed_seconds = 0.0;
	}

	// 计算每帧旋转角度
	float angle = static_cast<float>(elapsed_seconds * 360.0);
	m_model_matrix = XMMatrixRotationAxis(rotation_axis, XMConvertToRadians(angle));
	m_view_matrix = XMMatrixLookAtLH(eye_position, focus_point, up_direction);
	// 计算投影矩阵
	float aspect_ratio = m_client_width / static_cast<float>(m_client_height);
	m_projection_matrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(m_fov), aspect_ratio, 0.1f, 100.0f);
}

void Render()
{
	// 根据当前帧索引（后缓冲区索引）获得当前命令分配器和后缓冲区
	ComPtr<ID3D12CommandAllocator> command_allocator = m_command_allocators[m_current_back_buffer_index];
	ComPtr<ID3D12Resource2> back_buffer = m_back_buffers[m_current_back_buffer_index];
	// 重置命令配置器和命令列表
	command_allocator->Reset();
	m_command_list->Reset(command_allocator.Get(), nullptr);

	// 通过资源屏障将当前缓冲区转换成渲染目标阶段，在此填写转换说明
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = back_buffer.Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	// 通过命令列表发出转换命令
	m_command_list->ResourceBarrier(1, &barrier);

	// 将后缓冲区变成clear color
	FLOAT clear_color[] = {0.4f, 0.6f, 0.9f, 1.0f};
	// 获得CPU描述符句柄并通过计算获得指针偏移
	D3D12_CPU_DESCRIPTOR_HANDLE rtv = m_rtv_heap->GetCPUDescriptorHandleForHeapStart();
	rtv.ptr = SIZE_T(INT64(rtv.ptr) + INT64(m_rtv_descriptor_size) * INT64(m_current_back_buffer_index));
	// 通过命令列表发出清理rtv指令
	m_command_list->ClearRenderTargetView(rtv, clear_color, 0, nullptr);

	// 清除dsv视图
	D3D12_CPU_DESCRIPTOR_HANDLE dsv = m_dsv_heap->GetCPUDescriptorHandleForHeapStart();
	m_command_list->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1, 0, 0, nullptr);
	// 设置管线状态
	m_command_list->SetPipelineState(m_pipeline_state.Get());
	// 设置图形根签名
	m_command_list->SetGraphicsRootSignature(m_root_signature.Get());
	// 设置片元格式，顶点和索引缓冲区
	m_command_list->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_command_list->IASetVertexBuffers(0, 1, &m_vertex_buffer_view);
	m_command_list->IASetIndexBuffer(&m_index_buffer_view);
	// 设置视口和裁切矩形
	m_command_list->RSSetViewports(1, &m_viewport);
	m_command_list->RSSetScissorRects(1, &m_scissor_rect);
	// 设置渲染目标
	m_command_list->OMSetRenderTargets(1, &rtv, false, &dsv);
	// 计算MVP矩阵并保定到和设置根常量
	XMMATRIX mvp_matrix = XMMatrixMultiply(m_model_matrix, m_view_matrix);
	mvp_matrix = XMMatrixMultiply(mvp_matrix, m_projection_matrix);
	m_command_list->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, &mvp_matrix, 0);
	// 画出内容
	m_command_list->DrawIndexedInstanced(_countof(g_Indicies), 1, 0, 0, 0);


	// 再将当前缓冲区转换回present呈现阶段
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

	m_command_list->ResourceBarrier(1, &barrier);
	// 关闭命令列表，在命令列队执行列表之前
	DxDebug::ThrowIfFailed(m_command_list->Close());
	// 用命令列队执行命令列表
	ID3D12CommandList* const command_list[] = {m_command_list.Get()};
	m_command_queue->ExecuteCommandLists(_countof(command_list), command_list);
	// 确认是否支持动态分辨率和撕裂，并创建flags
	UINT sync_interval = m_vsync ? 1 : 0;
	UINT present_flags = m_tearing_supported && !m_vsync ? DXGI_PRESENT_ALLOW_TEARING : 0;
	// 将后缓冲区呈现到屏幕
	DXGI_PRESENT_PARAMETERS present_parameter{0, nullptr, nullptr, nullptr};
	DxDebug::ThrowIfFailed(m_swap_chain->Present1(sync_interval, present_flags, &present_parameter));
	// 更新当前命令列队的fence数组
	m_frame_fence_values[m_current_back_buffer_index] = DxHelper::Signal(m_command_queue, m_fence, m_fence_value);
	// 更新帧索引
	m_current_back_buffer_index = m_swap_chain->GetCurrentBackBufferIndex();
	// CPU等待GPU完成
	DxHelper::WaitForTheFrame(m_fence, m_frame_fence_values[m_current_back_buffer_index], m_fence_event);
}

void Resize(uint32_t width, uint32_t height)
{
	// 如果新的长宽和以前的不一样
	if (m_client_width != width || m_client_height != height)
	{
		// 将长宽更新为至少为1的新长宽
		m_client_width = 1u > width ? 1u : width;
		m_client_height = 1u > height ? 1u : height;

		// 更新视口大小
		m_viewport = {0.0f, 0.0f, static_cast<float>(m_client_width), static_cast<float>(m_client_height), D3D12_MIN_DEPTH, D3D12_MAX_DEPTH};

		// 确保GPU执行完后刷新
		DxHelper::FlushGPU(m_command_queue, m_fence, m_fence_value, m_fence_event);
		// 重置每个后缓冲区
		for (int i = 0; i < m_back_buffer_count; ++i)
		{
			m_back_buffers[i].Reset();
			m_frame_fence_values[i] = m_frame_fence_values[m_current_back_buffer_index];
		}
		// 更新交换链描述
		DXGI_SWAP_CHAIN_DESC1 swap_chain_desc{};
		DxDebug::ThrowIfFailed(m_swap_chain->GetDesc1(&swap_chain_desc));
		DxDebug::ThrowIfFailed(m_swap_chain->ResizeBuffers(m_back_buffer_count, m_client_width, m_client_height, DXGI_FORMAT_UNKNOWN, swap_chain_desc.Flags));
		// 更新帧所引，很重要，因为可能会不同
		m_current_back_buffer_index = m_swap_chain->GetCurrentBackBufferIndex();

		// 获得堆增量大小
		m_rtv_descriptor_size = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		// 为交换链中每个后备缓冲区创建渲染目标视图
		D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = m_rtv_heap->GetCPUDescriptorHandleForHeapStart();
		for (int i = 0; i < m_back_buffer_count; ++i)
		{
			// 获得交换链中的缓冲区
			DxDebug::ThrowIfFailed(m_swap_chain->GetBuffer(i, IID_PPV_ARGS(m_back_buffers[i].GetAddressOf())));
			// 为缓冲区创建rtv
			m_device->CreateRenderTargetView(m_back_buffers[i].Get(), nullptr, rtv_handle);
			// 将handle的指针偏移一个rtv的位置
			rtv_handle.ptr += m_rtv_descriptor_size;
		}

		// 创建用于清除的颜色
		D3D12_CLEAR_VALUE optimized_clear_value{};
		optimized_clear_value.Format = DXGI_FORMAT_D32_FLOAT;
		optimized_clear_value.DepthStencil = {1.0f, 0};

		// 填写堆属性
		D3D12_HEAP_PROPERTIES depth_heap_prop = {
			D3D12_HEAP_TYPE_DEFAULT,
			D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
			D3D12_MEMORY_POOL_UNKNOWN, 1u, 1u};
		// 填写资源描述
		D3D12_RESOURCE_DESC depth_buffer_desc = {
			D3D12_RESOURCE_DIMENSION_TEXTURE2D,
			0,
			m_client_width,
			m_client_height,
			1,
			0,
			DXGI_FORMAT_D32_FLOAT,
			{1u,0u},
			D3D12_TEXTURE_LAYOUT_UNKNOWN,
			D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL};
		// 创建深度缓冲区(以后会优化成CreateCommittedResource3)
		DxDebug::ThrowIfFailed(m_device->CreateCommittedResource(&depth_heap_prop, D3D12_HEAP_FLAG_NONE, &depth_buffer_desc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &optimized_clear_value, IID_PPV_ARGS(m_depth_buffer.GetAddressOf())));

		// 填写深度视图描述
		D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc{};
		dsv_desc.Format = DXGI_FORMAT_D32_FLOAT;
		dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsv_desc.Texture2D.MipSlice = 0;
		dsv_desc.Flags = D3D12_DSV_FLAG_NONE;
		// 创建深度视图
		m_device->CreateDepthStencilView(m_depth_buffer.Get(), &dsv_desc, m_dsv_heap->GetCPUDescriptorHandleForHeapStart());
	}
}

void SetFullScreen(bool fullscreen)
{
	if (m_fullscreen != fullscreen)
	{
		// 更改全屏设置
		m_fullscreen = fullscreen;

		if (m_fullscreen)
		{
			// 储存窗口矩形，为了之后重新便会窗口模式
			GetWindowRect(m_hwnd, &m_window_rect);
			// 显式将窗口样式变为0，删除所有窗口组件，然后应用新窗口样式设置
			UINT window_style = WS_OVERLAPPEDWINDOW & ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MAXIMIZEBOX);
			SetWindowLongPtrW(m_hwnd, GWL_STYLE, window_style);
			// 获得窗口占据最多或离窗口最近的显示器
			HMONITOR h_monitor = MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTONEAREST);
			// 获得该显示器的信息
			MONITORINFOEXW monitor_info{};
			monitor_info.cbSize = sizeof(MONITORINFOEXW);
			GetMonitorInfoW(h_monitor, &monitor_info);
			// 设置窗口位置
			SetWindowPos(m_hwnd, HWND_TOP, 
				monitor_info.rcMonitor.left, 
				monitor_info.rcMonitor.top, 
				monitor_info.rcMonitor.right - monitor_info.rcMonitor.left, 
				monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top, 
				SWP_FRAMECHANGED | SWP_NOACTIVATE);
			// 显示窗口
			ShowWindow(m_hwnd, SW_MAXIMIZE);
		}
		else
		{
			SetWindowLongPtrW(m_hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);

			SetWindowPos(m_hwnd, HWND_NOTOPMOST, 
				m_window_rect.left, 
				m_window_rect.top, 
				m_window_rect.right - m_window_rect.left, 
				m_window_rect.bottom - m_window_rect.top, 
				SWP_FRAMECHANGED | SWP_NOACTIVATE);

			ShowWindow(m_hwnd, SW_NORMAL);
		}
	}
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (m_initialized)
	{
		switch (uMsg)
		{
        // 绘制
		case WM_PAINT:
            Update();
            Render();
            break;
        // 按下各按键
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
			{
				bool alt = (::GetAsyncKeyState(VK_MENU) & 0x8000) != 0;

				switch (wParam)
				{
                // 开关V-Sync
				case 'V':
                    m_vsync = !m_vsync;
                    break;
                // 关闭窗口
				case VK_ESCAPE:
                    ::PostQuitMessage(0);
                    break;
                // 切换是否全屏
				case VK_RETURN:
					if (alt)
					{
                        [[fallthrough]];
						case VK_F11:
                            SetFullScreen(!m_fullscreen);
					}
                    break;
				}
			}
            break;
        // 通过不处理该事件防止电脑播放声音
		case WM_SYSCHAR:
            break;
        // 调整窗口大小
		case WM_SIZE:
			{
				RECT client_rect = {};
                ::GetClientRect(m_hwnd, &client_rect);

                int width = client_rect.right - client_rect.left;
                int height = client_rect.bottom - client_rect.top;

                Resize(width, height);
			}
        break;
        // 响应关闭窗口
		case WM_DESTROY:
            ::PostQuitMessage(0);
            break;
		default:
            return ::DefWindowProcW(hWnd, uMsg, wParam, lParam);
		}
	}
    else
    {
	    return ::DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }
	
    return 0;
}

int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	const wchar_t* windowClassName = L"DX12WindowClass";
	ParseCommandLineArguments();
	RegisterWindowClass(hInstance, windowClassName);
	m_hwnd = CreateWindow(windowClassName, hInstance, L"Learning DirectX 12", m_client_width, m_client_height);
	::GetWindowRect(m_hwnd, &m_window_rect);
	Initial(m_hwnd);
	m_initialized = true;

    ShowWindow(m_hwnd, nCmdShow); // 显示窗口

    // 消息循环
    MSG msg = {};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0)
    {
	    TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    DxHelper::FlushGPU(m_command_queue, m_fence, m_fence_value, m_fence_event);

    CloseHandle(m_fence_event);

    return 0;
}