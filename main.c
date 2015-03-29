/*
	Hello World example made by Aurelio Mannara for ctrulib
	This code was modified for the last time on: 12/12/2014 21:00 UTC+1

	This wouldn't be possible without the amazing work done by:
	-Smealum
	-fincs
	-WinterMute
	-yellows8
	-plutoo
	-mtheall
	-Many others who worked on 3DS and I'm surely forgetting about
*/

#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "gs.h"
#include "test_vsh_shbin.h"

#define RGBA8(r,g,b,a) ((((r)&0xFF)<<24) | (((g)&0xFF)<<16) | (((b)&0xFF)<<8) | (((a)&0xFF)<<0))

//shader structure
DVLB_s* dvlb;
shaderProgram_s shader;
//texture data pointer
u32* texData;
//vbo structure
gsVbo_s vbo;

//GPU framebuffer address
u32* gpuOut=(u32*)0x1F119400;
//GPU depth buffer address
u32* gpuDOut=(u32*)0x1F370800;

//angle for the vertex lighting (cf test.vsh)
float lightAngle;
//object position and rotation angle
vect3Df_s position, angle;

//vertex structure
typedef struct
{
   vect3Df_s position;
   float texcoord[2];
   vect3Df_s normal;
}vertex_s;

const vertex_s modelVboData[]=
{
   //first face (PZ)
      //first triangle
      {{-0.5f,  0.5f, -1.5f}, {0.0f, 0.0f}, {0.0f, 0.0f, +1.0f}},
      {{ 0.5f,  0.5f, -1.5f}, {1.0f, 0.0f}, {0.0f, 0.0f, +1.0f}},
      {{ 0.5f, -0.5f, -1.5f}, {1.0f, 1.0f}, {0.0f, 0.0f, +1.0f}},
      //second triangle
      {{-0.5f,  0.5f, -1.5f}, {0.0f, 0.0f}, {0.0f, 0.0f, +1.0f}},
      {{ 0.5f, -0.5f, -1.5f}, {1.0f, 1.0f}, {0.0f, 0.0f, +1.0f}},
      {{-0.5f, -0.5f, -1.5f}, {0.0f, 1.0f}, {0.0f, 0.0f, +1.0f}}
};

//stolen from staplebutt
void GPU_SetDummyTexEnv(u8 num)
{
   GPU_SetTexEnv(num,
      GPU_TEVSOURCES(GPU_PREVIOUS, 0, 0),
      GPU_TEVSOURCES(GPU_PREVIOUS, 0, 0),
      GPU_TEVOPERANDS(0,0,0),
      GPU_TEVOPERANDS(0,0,0),
      GPU_REPLACE,
      GPU_REPLACE,
      0xFFFFFFFF);
}

// topscreen
void renderFrame()
{
   GPU_SetViewport((u32*)osConvertVirtToPhys((u32)gpuDOut),(u32*)osConvertVirtToPhys((u32)gpuOut),0,0,240*2,400);

   GPU_DepthMap(-1.0f, 0.0f);
   GPU_SetFaceCulling(GPU_CULL_NONE);
   GPU_SetStencilTest(false, GPU_ALWAYS, 0x00, 0xFF, 0x00);
   GPU_SetStencilOp(GPU_KEEP, GPU_KEEP, GPU_KEEP);
   GPU_SetBlendingColor(0,0,0,0);
   GPU_SetDepthTestAndWriteMask(true, GPU_GREATER, GPU_WRITE_ALL);

   GPUCMD_AddMaskedWrite(GPUREG_0062, 0x1, 0);
   GPUCMD_AddWrite(GPUREG_0118, 0);

   GPU_SetAlphaBlending(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA);
   GPU_SetAlphaTest(false, GPU_ALWAYS, 0x00);

   GPU_SetTextureEnable(GPU_TEXUNIT0);

   GPU_SetTexEnv(0,
      GPU_TEVSOURCES(GPU_TEXTURE0, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR),
      GPU_TEVSOURCES(GPU_TEXTURE0, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR),
      GPU_TEVOPERANDS(0,0,0),
      GPU_TEVOPERANDS(0,0,0),
      GPU_MODULATE, GPU_MODULATE,
      0xFFFFFFFF);
   GPU_SetDummyTexEnv(1);
   GPU_SetDummyTexEnv(2);
   GPU_SetDummyTexEnv(3);
   GPU_SetDummyTexEnv(4);
   GPU_SetDummyTexEnv(5);

   //texturing stuff
   GPU_SetTexture(GPU_TEXUNIT0, (u32*)osConvertVirtToPhys((u32)texData),128,128,GPU_TEXTURE_MAG_FILTER(GPU_NEAREST)|GPU_TEXTURE_MIN_FILTER(GPU_NEAREST),GPU_RGBA8);
   GPU_SetAttributeBuffers(3, (u32*)osConvertVirtToPhys((u32)texData),
      GPU_ATTRIBFMT(0, 3, GPU_FLOAT)|GPU_ATTRIBFMT(1, 2, GPU_FLOAT)|GPU_ATTRIBFMT(2, 3, GPU_FLOAT),
      0xFFC, 0x210, 1, (u32[]){0x00000000}, (u64[]){0x210}, (u8[]){3});

   GPU_SetFloatUniform(GPU_VERTEX_SHADER, shaderInstanceGetUniformLocation(shader.vertexShader, "lightAmbient"), (u32*)(float[]){1.0f, 1.0f, 1.0f, 1.0f}, 1);

   //initialize projection matrix to standard perspective stuff
   gsMatrixMode(GS_PROJECTION);
   gsProjectionMatrix(M_PI/2.0f, 240.0f/400.0f, 1.0f, -1.0f);
   gsRotateZ(M_PI/2); //because framebuffer is sideways...

   gsVboDraw(&vbo);

   GPU_FinishDrawing();
}

u32 gpuCmdSize;
u32* gpuCmd;
u32* gpuCmdRight;


u32* texture_bin;

#define tex_w 168
#define tex_h 152
#define texture_bin_size (tex_w * tex_h * sizeof(*texture_bin))


void gpu_init_calls (void)
{
   GPU_Init(NULL);
   gpuCmdSize=0x40000;
   gpuCmd=(u32*)linearAlloc(gpuCmdSize*4);
   gpuCmdRight=(u32*)linearAlloc(gpuCmdSize*4);
   GPU_Reset(NULL, gpuCmd, gpuCmdSize);
   dvlb=DVLB_ParseFile((u32*)test_vsh_shbin, test_vsh_shbin_size);
   shaderProgramInit(&shader);
   shaderProgramSetVsh(&shader, &dvlb->DVLE[0]);

   //initialize GS
   gsInit(&shader);
   gsInit(NULL);

   // Flush the command buffer so that the shader upload gets executed
   GPUCMD_Finalize();
   GPUCMD_FlushAndRun(NULL);
   gspWaitForP3D();

   //create texture
   texData=(u32*)linearMemAlign(texture_bin_size, 0x80); //textures need to be 0x80-byte aligned
   memcpy(texData, texture_bin, texture_bin_size);

   //create VBO
   gsVboInit(&vbo);
   gsVboCreate(&vbo, sizeof(modelVboData));
   gsVboAddData(&vbo, (void*)modelVboData, sizeof(modelVboData), sizeof(modelVboData)/sizeof(vertex_s));
   gsVboFlushData(&vbo);

   //initialize object position and angle
   position=vect3Df(0.0f, 0.0f, -2.0f);
   angle=vect3Df(M_PI/4, M_PI/4, 0.0f);

}

void gpu_frame_calls(void)
{

}

int main(int argc, char **argv)
{
   gfxInitDefault();
//   gfxInit(GSP_RGBA8_OES,GSP_RGBA8_OES,false);

   gfxSet3D(false);

	consoleInit(GFX_BOTTOM, NULL);



   printf("\x1b[5;19H3ds_test");
   printf("\x1b[10;15H0x%08X",(unsigned int)osGetKernelVersion());
	printf("\x1b[19;15HPress Start to exit.");

   int frames = 0;

//   gfxSwapBuffers();


   texture_bin =  (uint32_t*)malloc (tex_w * tex_h * sizeof(*texture_bin));
   int i,j;
   for (j=0; j < tex_h ; j++)
   {
      for (i=0; i < tex_w ; i++)
      {
         if ((i&0x8) || (j&0x8))
            texture_bin[i + j * tex_w] =  0x888888FF;
         else
            texture_bin[i + j * tex_w] =  0xFFFFFFFF;

      }

   }

//   u32* dst = ((u32*)gfxTopLeftFramebuffers[1]) + (240-tex_h)/2 + 240*(400-tex_w)/2;//  - 240 * 10;
//   for (i=0;i<tex_w;i++)
//      for (j=0;j<tex_h;j++)
//      {
//         dst[240*i + (tex_h-j)]=texture_bin[i+j*tex_w];
//      }

//    gfxFlushBuffers();

   gpu_init_calls();

   u32 backgroundColor=RGBA8(0x68, 0xB0, 0xD8, 0xFF);

	while (aptMainLoop())
	{
		//Scan all the inputs. This should be done once for each frame
		hidScanInput();

		//hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
		u32 kDown = hidKeysDown();

		if (kDown & KEY_START) break; // break in order to return to hbmenu


//      printf("\x1b[25;15Hframes : %8i",frames);
//      printf("\x1b[26;0HlinearSpaceFree() : %iMB",(int)linearSpaceFree()/(1024*1024));
//      frames++;

      gsStartFrame();
      renderFrame();
      GPUCMD_Finalize();


      //draw the frame
      GPUCMD_FlushAndRun(NULL);
      gspWaitForP3D();

      //clear the screen
      GX_SetDisplayTransfer(NULL, (u32*)gpuOut, 0x019001E0, (u32*)gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL), 0x019001E0, 0x01001000);
      gspWaitForPPF();

      //clear the screen
      GX_SetMemoryFill(NULL, (u32*)gpuOut, backgroundColor, (u32*)&gpuOut[0x2EE00], 0x201, (u32*)gpuDOut, 0x00000000, (u32*)&gpuDOut[0x2EE00], 0x201);
      gspWaitForPSC0();
      gfxSwapBuffersGpu();

      gspWaitForEvent(GSPEVENT_VBlank0, true);


//      gfxFlushBuffers();
//      gfxSwapBuffers();

	}

//	gfxExit();

   gsExit();
   shaderProgramFree(&shader);
   DVLB_Free(dvlb);
   gfxExit();
	return 0;
}
