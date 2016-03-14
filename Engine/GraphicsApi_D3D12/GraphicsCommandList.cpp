#include "GraphicsCommandList.hpp"

#include "PipelineState.hpp"
#include "NativeCast.hpp"

#include <vector>

namespace inl {
namespace gxapi_dx12 {


GraphicsCommandList::GraphicsCommandList(ID3D12GraphicsCommandList* native) {
	if (native == nullptr) {
		throw std::runtime_error("Null pointer not allowed here.");
	}

	m_native = native;
}


GraphicsCommandList::~GraphicsCommandList() {
	m_native->Release();
}


ID3D12CommandList* GraphicsCommandList::GetNativeGenericList() {
	return m_native;
}


ID3D12GraphicsCommandList * GraphicsCommandList::GetNative() {
	return m_native;
}


void GraphicsCommandList::ResetState(gxapi::IPipelineState* newState) {
	m_native->ClearState(native_cast(newState));
}


void GraphicsCommandList::Close() {
	m_native->Close();
}


void GraphicsCommandList::Reset(gxapi::ICommandAllocator * allocator, gxapi::IPipelineState * newState) {
	m_native->Reset(native_cast(allocator), native_cast(newState));
}


void GraphicsCommandList::ClearDepthStencil(gxapi::DescriptorHandle dsv, float depth, uint8_t stencil, size_t numRects, inl::Rectangle* rects, bool clearDepth, bool clearStencil) {
	D3D12_CPU_DESCRIPTOR_HANDLE depthStencilView;
	depthStencilView.ptr = reinterpret_cast<uintptr_t>(dsv.cpuAddress);

	D3D12_CLEAR_FLAGS flags;
	flags = static_cast<D3D12_CLEAR_FLAGS>(clearDepth ? D3D12_CLEAR_FLAGS::D3D12_CLEAR_FLAG_DEPTH : 0);
	flags = static_cast<D3D12_CLEAR_FLAGS>(flags | (clearStencil ? D3D12_CLEAR_FLAGS::D3D12_CLEAR_FLAG_STENCIL : 0));

	std::vector<D3D12_RECT> castedRects;
	castedRects.reserve(numRects);
	for (int i = 0; i < numRects; i++) {
		castedRects.push_back(native_cast(rects[i]));
	}

	D3D12_RECT* pRects = (castedRects.size() == 0) ? nullptr : castedRects.data();

	m_native->ClearDepthStencilView(depthStencilView, flags, depth, stencil, castedRects.size(), pRects);
}


void GraphicsCommandList::ClearRenderTarget(gxapi::DescriptorHandle rtv, ColorRGBA color, size_t numRects, inl::Rectangle* rects) {
	D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView;
	renderTargetView.ptr = reinterpret_cast<uintptr_t>(rtv.cpuAddress);

	float nativeColor[4] = {color.r, color.g, color.b, color.a};

	std::vector<D3D12_RECT> castedRects;
	castedRects.reserve(numRects);
	for (int i = 0; i < numRects; i++) {
		castedRects.push_back(native_cast(rects[i]));
	}

	D3D12_RECT* pRects = (castedRects.size() == 0) ? nullptr : castedRects.data();

	m_native->ClearRenderTargetView(renderTargetView, nativeColor, castedRects.size(), pRects);
}


void GraphicsCommandList::CopyBuffer(gxapi::IResource* dst, size_t dstOffset, gxapi::IResource* src, size_t srcOffset, size_t numBytes) {
	m_native->CopyBufferRegion(native_cast(dst), dstOffset, native_cast(src), srcOffset, numBytes);
}


void GraphicsCommandList::CopyResource(gxapi::IResource* dst, gxapi::IResource* src) {
	m_native->CopyResource(native_cast(dst), native_cast(src));
}


void GraphicsCommandList::CopyTexture(gxapi::IResource* dst, unsigned dstSubresourceIndex, gxapi::IResource* src, unsigned srcSubresourceIndex) {
	auto nativeDst = CreateTextureCopyLocation(dst, dstSubresourceIndex);
	auto nativeSrc = CreateTextureCopyLocation(src, srcSubresourceIndex);

	m_native->CopyTextureRegion(&nativeDst, 0, 0, 0, &nativeSrc, nullptr);
}


void GraphicsCommandList::CopyTexture(gxapi::IResource* dst, TextureDescription dstDesc, gxapi::IResource* src, TextureDescription srcDesc, int offx, int offy, int offz, Cube region) {
	auto nativeDst = CreateTextureCopyLocation(dst, dstDesc);
	auto nativeSrc = CreateTextureCopyLocation(src, srcDesc);

	D3D12_BOX srcBox;
	srcBox.back = region.back;
	srcBox.front = region.front;
	srcBox.left = region.left;
	srcBox.right = region.right;
	srcBox.bottom = region.bottom;
	srcBox.top = region.top;

	m_native->CopyTextureRegion(&nativeDst, offx, offy, offz, &nativeSrc, &srcBox);
}


void GraphicsCommandList::DrawIndexedInstanced(unsigned numIndices, unsigned startIndex, int vertexOffset, unsigned numInstances, unsigned startInstance) {
	m_native->DrawIndexedInstanced(numIndices, numInstances, startIndex, vertexOffset, startInstance);
}


void GraphicsCommandList::DrawInstanced(unsigned numVertices, unsigned startVertex, unsigned numInstances, unsigned startInstance) {
	m_native->DrawInstanced(numVertices, numInstances, startVertex, startInstance);
}


void GraphicsCommandList::ExecuteBundle(IGraphicsCommandList* bundle) {
	m_native->ExecuteBundle(native_cast(bundle));
}


void GraphicsCommandList::SetIndexBuffer(void * gpuVirtualAddress, size_t sizeInBytes, eFormat format) {
	D3D12_INDEX_BUFFER_VIEW ibv;
	ibv.BufferLocation = reinterpret_cast<std::uintptr_t>(gpuVirtualAddress);
	ibv.Format = native_cast(format);
	ibv.SizeInBytes = sizeInBytes;

	m_native->IASetIndexBuffer(&ibv);
}


void GraphicsCommandList::SetPrimitiveTopology(ePrimitiveTopology topology) {
	m_native->IASetPrimitiveTopology(native_cast(topology));
}


void GraphicsCommandList::SetVertexBuffers(unsigned startSlot, unsigned count, void ** gpuVirtualAddress, unsigned * sizeInBytes, unsigned * strideInBytes) {
	std::vector<D3D12_VERTEX_BUFFER_VIEW> views;
	views.reserve(count);

	for (int i = 0; i < count; i++) {
		D3D12_VERTEX_BUFFER_VIEW tmpView;
		tmpView.BufferLocation = reinterpret_cast<uintptr_t>(gpuVirtualAddress[i]);
		tmpView.SizeInBytes = sizeInBytes[i];
		tmpView.StrideInBytes = strideInBytes[i];

		views.push_back(std::move(tmpView));
	}

	m_native->IASetVertexBuffers(startSlot, views.size(), views.data());
}


void GraphicsCommandList::SetRenderTargets(unsigned numRenderTargets, gxapi::DescriptorHandle* renderTargets, gxapi::DescriptorHandle* depthStencil) {
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> renderTargetDescriptors;
	renderTargetDescriptors.reserve(numRenderTargets);
	for (int i = 0; i < numRenderTargets; i++) {
		D3D12_CPU_DESCRIPTOR_HANDLE tmpDescriptor;
		tmpDescriptor.ptr = reinterpret_cast<uintptr_t>(renderTargets[i].cpuAddress);
		renderTargetDescriptors.push_back(tmpDescriptor);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE* pDepthStencilDescriptor = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE depthStencilDescriptor;
	if (depthStencil != nullptr) {
		depthStencilDescriptor.ptr = reinterpret_cast<uintptr_t>(depthStencil->cpuAddress);
		pDepthStencilDescriptor = &depthStencilDescriptor;
	}

	m_native->OMSetRenderTargets(renderTargetDescriptors.size(), renderTargetDescriptors.data(), false, pDepthStencilDescriptor);
}


void GraphicsCommandList::SetBlendFactor(float r, float g, float b, float a) {
	float native[4] = {r, g, b, a};
	m_native->OMSetBlendFactor(native);
}


void GraphicsCommandList::SetStencilRef(unsigned stencilRef) {
	m_native->OMSetStencilRef(stencilRef);
}


void GraphicsCommandList::SetScissorRects(unsigned numRects, inl::Rectangle* rects) {
	std::vector<D3D12_RECT> nativeRects;
	nativeRects.reserve(numRects);

	for (int i = 0; i < numRects; i++) {
		nativeRects.push_back(native_cast(rects[i]));
	}

	m_native->RSSetScissorRects(nativeRects.size(), nativeRects.data());
}


void GraphicsCommandList::SetViewports(unsigned numViewports, Viewport* viewports) {
	std::vector<D3D12_VIEWPORT> nativeViewports;
	nativeViewports.reserve(numViewports);

	for (int i = 0; i < numViewports; i++) {
		nativeViewports.push_back(native_cast(viewports[i]));
	}

	m_native->RSSetViewports(nativeViewports.size(), nativeViewports.data());
}


void GraphicsCommandList::SetGraphicsRootConstant(unsigned parameterIndex, unsigned destOffset, uint32_t value) {
	m_native->SetGraphicsRoot32BitConstant(parameterIndex, value, destOffset);
}


void GraphicsCommandList::SetGraphicsRootConstants(unsigned parameterIndex, unsigned destOffset, unsigned numValues, uint32_t* value) {
	m_native->SetComputeRoot32BitConstants(parameterIndex, numValues, value, destOffset);
}


void GraphicsCommandList::SetGraphicsRootConstantBuffer(unsigned parameterIndex, void* gpuVirtualAddress) {
	m_native->SetGraphicsRootConstantBufferView(parameterIndex, reinterpret_cast<uintptr_t>(gpuVirtualAddress));
}


void GraphicsCommandList::SetGraphicsRootDescriptorTable(unsigned parameterIndex, gxapi::DescriptorHandle baseHandle) {
	D3D12_GPU_DESCRIPTOR_HANDLE baseDescriptor;
	baseDescriptor.ptr = reinterpret_cast<uintptr_t>(baseHandle.gpuAddress);

	m_native->SetGraphicsRootDescriptorTable(parameterIndex, baseDescriptor);
}


void GraphicsCommandList::SetGraphicsRootShaderResource(unsigned parameterIndex, void* gpuVirtualAddress) {
	m_native->SetGraphicsRootShaderResourceView(parameterIndex, reinterpret_cast<uintptr_t>(gpuVirtualAddress));
}


void GraphicsCommandList::SetGraphicsRootSignature(gxapi::IRootSignature* rootSignature) {
	m_native->SetGraphicsRootSignature(native_cast(rootSignature));
}


void GraphicsCommandList::SetPipelineState(gxapi::IPipelineState * pipelineState) {
	m_native->SetPipelineState(native_cast(pipelineState));
}


D3D12_TEXTURE_COPY_LOCATION GraphicsCommandList::CreateTextureCopyLocation(gxapi::IResource* texture, TextureDescription descrition) {

	D3D12_TEXTURE_COPY_LOCATION result;
	{
		result.pResource = native_cast(texture);
		result.Type = D3D12_TEXTURE_COPY_TYPE::D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT placedFootprint;
		{
			D3D12_SUBRESOURCE_FOOTPRINT footprint;
			{
				footprint.Depth = descrition.depth;
				footprint.Format = native_cast(descrition.format);
				footprint.Height = descrition.height;
				footprint.Width = descrition.width;
				size_t rowSize = GetFormatSizeInBytes(descrition.format)*descrition.width;
				size_t alignement = D3D12_TEXTURE_DATA_PITCH_ALIGNMENT;
				footprint.RowPitch = rowSize + (alignement - rowSize % alignement) % alignement;
			}
			placedFootprint.Footprint = footprint;
			placedFootprint.Offset = 0;
		}
		result.PlacedFootprint = placedFootprint;
	}

	return result;
}


D3D12_TEXTURE_COPY_LOCATION GraphicsCommandList::CreateTextureCopyLocation(gxapi::IResource* texture, unsigned subresourceIndex)
{
	D3D12_TEXTURE_COPY_LOCATION result;
	result.pResource = native_cast(texture);
	result.Type = D3D12_TEXTURE_COPY_TYPE::D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	result.SubresourceIndex = subresourceIndex;

	return result;
}


}
}