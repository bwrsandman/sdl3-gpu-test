#include "LNDFile.h"
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
#include <cstdint>
#include <fstream>
#include <ranges>
#include <string>

#include <SDL3/SDL.h>

#if SDL_PLATFORM_APPLE
#include "testgpu_vert_metallib.h"
#include "testgpu_frag_metallib.h"
#else
#include "triangle_vert_spirv.h"
#include "triangle_frag_spirv.h"
#endif
#if SDL_PLATFORM_WINDOWS
#include "triangle_vert_dxil.h"
#include "triangle_frag_dxil.h"
#include "triangle_vert_dxbc.h"
#include "triangle_frag_dxbc.h"
#endif

static SDL_GPUShader *load_shader(SDL_GPUDevice *gpu_device,
                                  SDL_bool is_vertex) {
  SDL_GPUShaderCreateInfo createinfo;
  createinfo.samplerCount = is_vertex ? 0 : 1;
  createinfo.storageBufferCount = 0;
  createinfo.storageTextureCount = 0;
  createinfo.uniformBufferCount = 0;
  createinfo.props = 0;

  SDL_GPUDriver backend = SDL_GetGPUDriver(gpu_device);
#if SDL_PLATFORM_WINDOWS
  if (backend == SDL_GPU_DRIVER_D3D11) {
    createinfo.format = SDL_GPU_SHADERFORMAT_DXBC;
    createinfo.code = is_vertex ? D3D11_TriangleVert : D3D11_TriangleFrag;
    createinfo.codeSize = is_vertex ? SDL_arraysize(D3D11_TriangleVert)
                                    : SDL_arraysize(D3D11_TriangleFrag);
    createinfo.entryPointName = is_vertex ? "VSMain" : "PSMain";
  } else
  if (false && backend == SDL_GPU_DRIVER_D3D12) {
    createinfo.format = SDL_GPU_SHADERFORMAT_DXIL;
    createinfo.code = is_vertex ? triangle_vert_dxil : triangle_frag_dxil;
    createinfo.codeSize = is_vertex ? triangle_vert_dxil_len : triangle_frag_dxil_len;
    createinfo.entryPointName = is_vertex ? "VSMain" : "PSMain";
  } else
#endif
#if SDL_PLATFORM_APPLE
  if (backend == SDL_GPU_DRIVER_METAL) {
    createinfo.format = SDL_GPU_SHADERFORMAT_METALLIB;
    createinfo.code =
        is_vertex ? triangle_vert_metallib : triangle_frag_metallib;
    createinfo.codeSize =
        is_vertex ? triangle_vert_metallib_len : triangle_frag_metallib_len;
    createinfo.entryPointName = is_vertex ? "vs_main" : "fs_main";
  } else
#else
  if (backend == SDL_GPU_DRIVER_VULKAN) {
    createinfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
    createinfo.code = is_vertex ? triangle_vert_spirv : triangle_frag_spirv;
    createinfo.codeSize =
        is_vertex ? triangle_vert_spirv_len : triangle_frag_spirv_len;
    createinfo.entryPointName = "main";
  }
#endif
  else
  {
    return nullptr;
  }

  createinfo.stage =
      is_vertex ? SDL_GPU_SHADERSTAGE_VERTEX : SDL_GPU_SHADERSTAGE_FRAGMENT;
  return SDL_CreateGPUShader(gpu_device, &createinfo);
}

int main(int argc, char *argv[]) {
  openblack::lnd::LNDFile lnd;
  {
    std::fstream f("Land1.lnd");
    auto lnd_result = lnd.ReadFile(f);
    if (lnd_result != openblack::lnd::LNDResult::Success)
    {
      SDL_Log("Failed to read openblack lnd\n");
      return 1;
    }
  }
  {
    const auto n = SDL_GetNumVideoDrivers();
    if (n == 0) {
      SDL_Log("No built-in video drivers\n");
    } else {
      std::string text = "Built-in video drivers:";
      for (int i = 0; i < n; ++i) {
        if (i > 0) {
          text += ",";
        }
        text += std::string(" ") + SDL_GetVideoDriver(i);
      }
      SDL_Log("%s\n", text.c_str());
    }
  }

  if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
    SDL_Log("Couldn't initialize video driver: %s\n", SDL_GetError());
    return 1;
  }
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
  {
    int n = SDL_GetNumRenderDrivers();
    if (n == 0) {
      SDL_Log("No built-in render drivers\n");
    } else {
      SDL_Log("Built-in render drivers:\n");
      for (int i = 0; i < n; ++i) {
        SDL_Log("  %s\n", SDL_GetRenderDriver(i));
      }
    }
  }

  SDL_Window *window = nullptr;
  {
    auto props = SDL_CreateProperties();
    SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING,
                          "SDL3 GPU Test");
    // SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X_NUMBER, r.x);
    // SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, r.y);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, 500);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, 500);
    // SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_FLAGS_NUMBER,
    //                       state->window_flags);
    window = SDL_CreateWindowWithProperties(props);
    SDL_DestroyProperties(props);
    if (!window) {
      SDL_Log("Couldn't create window: %s\n", SDL_GetError());
      SDL_Quit();
      return 1;
    }
  }

  auto gpu_device = SDL_CreateGPUDevice(
      SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXBC |
          SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_METALLIB,
      SDL_TRUE, nullptr);
  if (!gpu_device) {
    SDL_Log("Failed to create GPU device: %s\n", SDL_GetError());

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  if (!SDL_ClaimWindowForGPUDevice(gpu_device, window)) {
    SDL_Log("Failed to claim window with gpu device\n");
    SDL_DestroyGPUDevice(gpu_device);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  const auto& lnd_materials = lnd.GetMaterials();
  SDL_GPUTextureCreateInfo texture_desc;
  SDL_zero(texture_desc);
  texture_desc.type = SDL_GPU_TEXTURETYPE_2D_ARRAY;
  texture_desc.format = SDL_GPU_TEXTUREFORMAT_B5G5R5A1_UNORM;
  texture_desc.usageFlags = SDL_GPU_TEXTUREUSAGE_SAMPLER_BIT;
  texture_desc.width = openblack::lnd::LNDMaterial::k_Width;
  texture_desc.height = openblack::lnd::LNDMaterial::k_Height;
  texture_desc.layerCountOrDepth = lnd.GetMaterials().size();
  texture_desc.levelCount = 1;
  SDL_GPUTexture* texture = SDL_CreateGPUTexture(gpu_device, &texture_desc);
  if (!texture) {
    SDL_Log("Failed to create GPU Texture: %s\n", SDL_GetError());

    SDL_DestroyGPUDevice(gpu_device);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }
  SDL_SetGPUTextureName(gpu_device, texture, "lnd material");

  SDL_GPUTransferBufferCreateInfo tbci;
  SDL_zero(tbci);
  tbci.sizeInBytes = (Uint32)(openblack::lnd::LNDMaterial::k_Width * openblack::lnd::LNDMaterial::k_Height * sizeof(openblack::lnd::LNDMaterial::B5G5R5A1));
  tbci.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;

  SDL_GPUTransferBuffer *tbuf = SDL_CreateGPUTransferBuffer(gpu_device, &tbci);

  if (tbuf == NULL) {
      return 1;
  }

  auto* cmd = SDL_AcquireGPUCommandBuffer(gpu_device);
  SDL_GPUCopyPass *cpass = SDL_BeginGPUCopyPass(cmd);
  for (const auto& [i, lnd_material]: std::ranges::enumerate_view(lnd_materials))
  {
    uint8_t *output = reinterpret_cast<uint8_t *>(SDL_MapGPUTransferBuffer(gpu_device, tbuf, true));
    memcpy(output, lnd_material.texels.data(), lnd_material.texels.size() * sizeof(lnd_material.texels[0]));

    SDL_UnmapGPUTransferBuffer(gpu_device, tbuf);

    SDL_GPUTextureTransferInfo transfer_src_desc;
    SDL_zero(transfer_src_desc);
    transfer_src_desc.transferBuffer = tbuf;
    transfer_src_desc.imagePitch = openblack::lnd::LNDMaterial::k_Width;
    transfer_src_desc.imageHeight = openblack::lnd::LNDMaterial::k_Height;

    SDL_GPUTextureRegion transfer_dest_desc;
    SDL_zero(transfer_dest_desc);
    transfer_dest_desc.texture = texture;
    transfer_dest_desc.layer = i;
    transfer_dest_desc.w = openblack::lnd::LNDMaterial::k_Width;
    transfer_dest_desc.h = openblack::lnd::LNDMaterial::k_Height;

    SDL_UploadToGPUTexture(cpass, &transfer_src_desc, &transfer_dest_desc, false);
  }
  SDL_EndGPUCopyPass(cpass);
  SDL_SubmitGPUCommandBuffer(cmd);

  SDL_GPUSamplerCreateInfo sampler_desc;
  SDL_zero(sampler_desc);

  SDL_GPUSampler* sampler = SDL_CreateGPUSampler(gpu_device, &sampler_desc);
  if (!sampler) {
    SDL_Log("Failed to create GPU Sampler: %s\n", SDL_GetError());

    SDL_ReleaseGPUTexture(gpu_device, texture);
    SDL_DestroyGPUDevice(gpu_device);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  SDL_GPUShader *vertex_shader = load_shader(gpu_device, true);
  SDL_GPUShader *fragment_shader = load_shader(gpu_device, false);

  SDL_GPUColorAttachmentDescription color_attachment_desc;
  SDL_zero(color_attachment_desc);
  color_attachment_desc.format = SDL_GetGPUSwapchainTextureFormat(gpu_device, window);
  color_attachment_desc.blendState.colorWriteMask = 0xF;

  SDL_GPUGraphicsPipelineCreateInfo pipelinedesc;
  SDL_zero(pipelinedesc);
  pipelinedesc.attachmentInfo.colorAttachmentCount = 1;
  pipelinedesc.attachmentInfo.colorAttachmentDescriptions = &color_attachment_desc;
  // TODO: Report upstream that having a null shader causes a seg fault instead
  // of a warning
  pipelinedesc.vertexShader = vertex_shader;
  pipelinedesc.fragmentShader = fragment_shader;
  pipelinedesc.primitiveType = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
  pipelinedesc.multisampleState.sampleMask = 0xFFFFFFFF;

  auto pipeline = SDL_CreateGPUGraphicsPipeline(gpu_device, &pipelinedesc);

  SDL_ReleaseGPUShader(gpu_device, vertex_shader);
  SDL_ReleaseGPUShader(gpu_device, fragment_shader);

  if (!pipeline) {
    SDL_Log("Failed to create GPU pipeline: %s\n", SDL_GetError());

    SDL_ReleaseGPUSampler(gpu_device, sampler);
    SDL_ReleaseGPUTexture(gpu_device, texture);
    SDL_DestroyGPUDevice(gpu_device);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  SDL_Event event;
  bool done = false;
  while (!done) {
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_EVENT_QUIT:
        done = true;
        break;
      }
    }

    /* Acquire the swapchain texture */

    auto *cmd = SDL_AcquireGPUCommandBuffer(gpu_device);
    uint32_t drawablew, drawableh;
    auto swapchain =
        SDL_AcquireGPUSwapchainTexture(cmd, window, &drawablew, &drawableh);

    if (!swapchain) {
      /* No swapchain was acquired, probably too many frames in flight */
      SDL_SubmitGPUCommandBuffer(cmd);
      continue;
    }

    SDL_GPUColorAttachmentInfo color_attachment;

    SDL_zero(color_attachment);
    color_attachment.clearColor.r = 0x35 / 255.0f;
    color_attachment.clearColor.g = 0x3F / 255.0f;
    color_attachment.clearColor.b = 0x3E / 255.0f;
    color_attachment.clearColor.a = 1.0f;
    color_attachment.loadOp = SDL_GPU_LOADOP_CLEAR;
    color_attachment.storeOp = SDL_GPU_STOREOP_STORE;
    color_attachment.texture = swapchain;

    auto pass = SDL_BeginGPURenderPass(cmd, &color_attachment, 1, nullptr);
    SDL_BindGPUGraphicsPipeline(pass, pipeline);
    SDL_GPUTextureSamplerBinding sampler_binding;
    sampler_binding.sampler = sampler;
    sampler_binding.texture = texture;
    SDL_BindGPUFragmentSamplers(pass, 0, &sampler_binding, 1);
    SDL_DrawGPUPrimitives(pass, 6, 1, 0, 0);
    SDL_EndGPURenderPass(pass);

    /* Submit the command buffer! */
    SDL_SubmitGPUCommandBuffer(cmd);
  }

  SDL_ReleaseGPUSampler(gpu_device, sampler);
  SDL_ReleaseGPUTexture(gpu_device, texture);
  SDL_ReleaseGPUGraphicsPipeline(gpu_device, pipeline);
  SDL_DestroyGPUDevice(gpu_device);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
