// SPDX-License-Identifier: GPL-3.0-or-later
// Compile each product's real shader and verify premultiplied native planes.
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <simd/simd.h>
#include <stdint.h>

typedef struct {
    simd_float2 origin, size, viewport;
    uint32_t offset, stride, width, height, alpha, transform;
    simd_float4 source;
} SceneImage;

int main(int argc, char **argv) {
    @autoreleasepool {
        if (argc < 2) return 2;
        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        id<MTLCommandQueue> queue = [device newCommandQueue];
        if (!device || !queue) return 3;
        for (int path = 1; path < argc; ++path) {
            NSError *error = nil;
            NSString *source = [NSString stringWithContentsOfFile:@(argv[path])
                encoding:NSUTF8StringEncoding error:&error];
            id<MTLLibrary> library = [device newLibraryWithSource:source options:nil error:&error];
            MTLRenderPipelineDescriptor *pipeline = [MTLRenderPipelineDescriptor new];
            pipeline.vertexFunction = [library newFunctionWithName:@"waylandVertex"];
            pipeline.fragmentFunction = [library newFunctionWithName:@"waylandTexture"];
            MTLRenderPipelineColorAttachmentDescriptor *color = pipeline.colorAttachments[0];
            color.pixelFormat = MTLPixelFormatRGBA8Unorm;
            color.blendingEnabled = YES;
            color.sourceRGBBlendFactor = color.sourceAlphaBlendFactor = MTLBlendFactorOne;
            color.destinationRGBBlendFactor = color.destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
            id<MTLRenderPipelineState> state = [device newRenderPipelineStateWithDescriptor:pipeline error:&error];
            if (!state) { fprintf(stderr, "%s\n", error.description.UTF8String); return 4; }
            MTLTextureDescriptor *desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                width:2 height:1 mipmapped:NO];
            desc.storageMode = MTLStorageModeShared;
            desc.usage = MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget;
            id<MTLTexture> input = [device newTextureWithDescriptor:desc];
            id<MTLTexture> output = [device newTextureWithDescriptor:desc];
            const uint8_t pixels[] = {0, 0, 0, 0, 128, 0, 0, 128};
            [input replaceRegion:MTLRegionMake2D(0, 0, 2, 1) mipmapLevel:0 withBytes:pixels bytesPerRow:8];
            for (uint32_t alpha = 0; alpha < 2; ++alpha) {
                SceneImage image = {.size={2,1}, .viewport={2,1}, .width=2, .height=1,
                    .alpha=alpha, .source={0,0,1,1}};
                MTLRenderPassDescriptor *pass = [MTLRenderPassDescriptor renderPassDescriptor];
                pass.colorAttachments[0].texture = output;
                pass.colorAttachments[0].loadAction = MTLLoadActionClear;
                pass.colorAttachments[0].storeAction = MTLStoreActionStore;
                pass.colorAttachments[0].clearColor = MTLClearColorMake(0,0,1,1);
                id<MTLCommandBuffer> command = [queue commandBuffer];
                id<MTLRenderCommandEncoder> encoder = [command renderCommandEncoderWithDescriptor:pass];
                [encoder setRenderPipelineState:state];
                [encoder setVertexBytes:&image length:sizeof(image) atIndex:0];
                [encoder setFragmentBytes:&image length:sizeof(image) atIndex:1];
                [encoder setFragmentTexture:input atIndex:0];
                [encoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
                [encoder endEncoding]; [command commit]; [command waitUntilCompleted];
                if (command.status != MTLCommandBufferStatusCompleted) return 5;
                uint8_t actual[8];
                [output getBytes:actual bytesPerRow:8 fromRegion:MTLRegionMake2D(0,0,2,1) mipmapLevel:0];
                uint8_t expected[] = {0,0,alpha?255:0,255,128,0,alpha?127:0,255};
                if (memcmp(actual, expected, sizeof(actual))) {
                    fprintf(stderr,"alpha=%u pixel mismatch: %s\n",alpha,argv[path]); return 6;
                }
            }
            printf("PASS native texture transparency, premultiplied blending and RGBX: %s\n", argv[path]);
        }
    }
    return 0;
}
