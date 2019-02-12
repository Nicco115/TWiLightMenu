#include <nds.h>
#include <fat.h>
#include <stdio.h>
#include <maxmod9.h>

#include "common/dsimenusettings.h"
#include "common/systemdetails.h"

#include "soundbank.h"
#include "soundbank_bin.h"

extern u16 bmpImageBuffer[256*192];
extern u16 videoImageBuffer[39][256*144];

extern void* dsiSplashLocation;

extern bool fadeType;

bool cartInserted;

static char videoFrameFilename[256];

static FILE* videoFrameFile;

extern bool rocketVideo_playVideo;
extern int rocketVideo_videoYpos;
extern int rocketVideo_videoFrames;
extern int rocketVideo_videoFps;
extern int rocketVideo_currentFrame;

#include "bootsplash.h"

#define CONSOLE_SCREEN_WIDTH 32
#define CONSOLE_SCREEN_HEIGHT 24

void BootJingleDSi() {
	
	mmInitDefaultMem((mm_addr)soundbank_bin);

	mmLoadEffect( SFX_DSIBOOT );

	mm_sound_effect dsiboot = {
		{ SFX_DSIBOOT } ,			// id
		(int)(1.0f * (1<<10)),	// rate
		0,		// handle
		255,	// volume
		128,	// panning
	};
	
	mmEffectEx(&dsiboot);
}

void drawNintendoLogoToVram(void) {
	if (!cartInserted) return;

	// Draw first half of Nintendo logo
	int x = 66;
	int y = 155;
	for (int i=122*14; i<122*28; i++) {
		if (x >= 66+122) {
			x = 66;
			y--;
		}
		if (BG_GFX[(256*192)+i] != 0xFFFF) {
			BG_GFX[y*256+x] = BG_GFX[(256*192)+i];
		}
		x++;
	}
}

void BootSplashDSi(void) {

	u16 whiteCol = 0xFFFF;
	for (int i = 0; i < 256*256; i++) {
		BG_GFX[i] = ((whiteCol>>10)&0x1f) | ((whiteCol)&(0x1f<<5)) | (whiteCol&0x1f)<<10 | BIT(15);
		BG_GFX_SUB[i] = ((whiteCol>>10)&0x1f) | ((whiteCol)&(0x1f<<5)) | (whiteCol&0x1f)<<10 | BIT(15);
	}

	if (ms().hsMsg) {
		// Load H&S image
		FILE* file = fopen("nitro:/graphics/hsmsg.bmp", "rb");

		if (file) {
			// Start loading
			fseek(file, 0xe, SEEK_SET);
			u8 pixelStart = (u8)fgetc(file) + 0xe;
			fseek(file, pixelStart, SEEK_SET);
			fread(bmpImageBuffer, 2, 0x18000, file);
			u16* src = bmpImageBuffer;
			int x = 0;
			int y = 191;
			for (int i=0; i<256*192; i++) {
				if (x >= 256) {
					x = 0;
					y--;
				}
				u16 val = *(src++);
				BG_GFX_SUB[y*256+x] = ((val>>10)&0x1f) | ((val)&(0x1f<<5)) | (val&0x1f)<<10 | BIT(15);
				x++;
			}
		}

		fclose(file);
	}

	bool sixtyFps = true;

	cartInserted = (REG_SCFG_MC != 0x11);

	fadeType = true;
	
	if (cartInserted) {
		videoFrameFile = fopen("nitro:/video/dsisplash/nintendo.bmp", "rb");

		if (videoFrameFile) {
			// Start loading
			fseek(videoFrameFile, 0xe, SEEK_SET);
			u8 pixelStart = (u8)fgetc(videoFrameFile) + 0xe;
			fseek(videoFrameFile, pixelStart, SEEK_SET);
			fread(bmpImageBuffer, 2, 0x1B00, videoFrameFile);
			u16* src = bmpImageBuffer;
			for (int i=0; i<122*28; i++) {
				u16 val = *(src++);
				if (val != 0x7C1F) {
					BG_GFX[(256*192)+i] = ((val>>10)&0x1f) | ((val)&(0x1f<<5)) | (val&0x1f)<<10 | BIT(15);
				}
			}
		}
		fclose(videoFrameFile);
	}

	if (sixtyFps) {
		rocketVideo_videoFrames = 108;
		rocketVideo_videoFps = 60;

		videoFrameFile = fopen("nitro:/video/dsisplash_60fps.rvid", "rb");

		/*for (u8 selectedFrame = 0; selectedFrame <= rocketVideo_videoFrames; selectedFrame++) {
			if (selectedFrame < 0x10) {
				snprintf(videoFrameFilename, sizeof(videoFrameFilename), "nitro:/video/dsisplash_60fps/0x0%x.bmp", (int)selectedFrame);
			} else {
				snprintf(videoFrameFilename, sizeof(videoFrameFilename), "nitro:/video/dsisplash_60fps/0x%x.bmp", (int)selectedFrame);
			}
			videoFrameFile = fopen(videoFrameFilename, "rb");

			if (videoFrameFile) {
				// Start loading
				fseek(videoFrameFile, 0xe, SEEK_SET);
				u8 pixelStart = (u8)fgetc(videoFrameFile) + 0xe;
				fseek(videoFrameFile, pixelStart, SEEK_SET);
				fread(bmpImageBuffer, 2, 0x12000, videoFrameFile);
				u16* src = bmpImageBuffer;
				int x = 0;
				int y = 143;
				for (int i=0; i<256*144; i++) {
					if (x >= 256) {
						x = 0;
						y--;
					}
					u16 val = *(src++);
					videoImageBuffer[0][y*256+x] = ((val>>10)&0x1f) | ((val)&(0x1f<<5)) | (val&0x1f)<<10 | BIT(15);
					x++;
				}
			}
			fclose(videoFrameFile);
			memcpy(dsiSplashLocation+(selectedFrame*0x12000), videoImageBuffer[0], 0x12000);
		}*/

		if (videoFrameFile) {
			bool doRead = false;

			if (isDSiMode()) {
				doRead = true;
			} else if (sys().isRegularDS()) {
				sysSetCartOwner (BUS_OWNER_ARM9);	// Allow arm9 to access GBA ROM (or in this case, the DS Memory Expansion Pak)
				*(vu32*)(0x09000000) = 0x53524C41;
				if (*(vu32*)(0x09000000) == 0x53524C41) {
					// Set to load video into DS Memory Expansion Pak
					dsiSplashLocation = (void*)0x09000000;
					doRead = true;
				}
			}
			if (doRead) {
				fread(dsiSplashLocation, 1, 0x7B0000, videoFrameFile);
			} else {
				sixtyFps = false;
			}
		} else {
			sixtyFps = false;
		}
	}
	if (!sixtyFps) {
		rocketVideo_videoFrames = 42;
		rocketVideo_videoFps = 24;

		for (int selectedFrame = 0; selectedFrame < 39; selectedFrame++) {
			if (selectedFrame < 10) {
				snprintf(videoFrameFilename, sizeof(videoFrameFilename), "nitro:/video/dsisplash/frame0%i.bmp", selectedFrame);
			} else {
				snprintf(videoFrameFilename, sizeof(videoFrameFilename), "nitro:/video/dsisplash/frame%i.bmp", selectedFrame);
			}
			videoFrameFile = fopen(videoFrameFilename, "rb");

			if (videoFrameFile) {
				// Start loading
				fseek(videoFrameFile, 0xe, SEEK_SET);
				u8 pixelStart = (u8)fgetc(videoFrameFile) + 0xe;
				fseek(videoFrameFile, pixelStart, SEEK_SET);
				fread(bmpImageBuffer, 2, 0x12000, videoFrameFile);
				u16* src = bmpImageBuffer;
				int x = 0;
				int y = 143;
				for (int i=0; i<256*144; i++) {
					if (x >= 256) {
						x = 0;
						y--;
					}
					u16 val = *(src++);
					videoImageBuffer[selectedFrame][y*256+x] = ((val>>10)&0x1f) | ((val)&(0x1f<<5)) | (val&0x1f)<<10 | BIT(15);
					x++;
				}
			}
			fclose(videoFrameFile);

			if (cartInserted && selectedFrame > 5) {
				// Draw first half of Nintendo logo
				int x = 66;
				int y = 130+13;
				for (int i=122*14; i<122*28; i++) {
					if (x >= 66+122) {
						x = 66;
						y--;
					}
					if (BG_GFX[(256*192)+i] != 0xFFFF) {
						videoImageBuffer[selectedFrame][y*256+x] = BG_GFX[(256*192)+i];
					}
					x++;
				}
			}
		}
	}

	rocketVideo_videoYpos = 12;
	rocketVideo_playVideo = true;

	while (rocketVideo_playVideo) {
		if (sixtyFps && rocketVideo_currentFrame >= 16) {
			drawNintendoLogoToVram();
		}
		if (cartInserted && rocketVideo_currentFrame == (sixtyFps ? 16 : 6)) {
			// Draw last half of Nintendo logo
			int x = 66;
			int y = 144+13;
			for (int i=0; i<122*14; i++) {
				if (x >= 66+122) {
					x = 66;
					y--;
				}
				BG_GFX[(y+12)*256+x] = BG_GFX[(256*192)+i];
				x++;
			}
		}
		if (rocketVideo_currentFrame == (sixtyFps ? 24 : 10)) {
			BootJingleDSi();
			break;
		}
		swiWaitForVBlank();
	}

	if (!sixtyFps) {
		for (int selectedFrame = 39; selectedFrame <= 42; selectedFrame++) {
			snprintf(videoFrameFilename, sizeof(videoFrameFilename), "nitro:/video/dsisplash/frame%i.bmp", selectedFrame);
			videoFrameFile = fopen(videoFrameFilename, "rb");

			if (videoFrameFile) {
				// Start loading
				fseek(videoFrameFile, 0xe, SEEK_SET);
				u8 pixelStart = (u8)fgetc(videoFrameFile) + 0xe;
				fseek(videoFrameFile, pixelStart, SEEK_SET);
				fread(bmpImageBuffer, 2, 0x12000, videoFrameFile);
				u16* src = bmpImageBuffer;
				int x = 0;
				int y = 143;
				for (int i=0; i<256*144; i++) {
					if (x >= 256) {
						x = 0;
						y--;
					}
					u16 val = *(src++);
					videoImageBuffer[selectedFrame-39][y*256+x] = ((val>>10)&0x1f) | ((val)&(0x1f<<5)) | (val&0x1f)<<10 | BIT(15);
					x++;
				}

				if (cartInserted) {
					// Draw first half of Nintendo logo
					int x = 66;
					int y = 130+13;
					for (int i=122*14; i<122*28; i++) {
						if (x >= 66+122) {
							x = 66;
							y--;
						}
						if (BG_GFX[(256*192)+i] != 0xFFFF) {
							videoImageBuffer[selectedFrame-39][y*256+x] = BG_GFX[(256*192)+i];
						}
						x++;
					}
				}
				//dmaCopy((void*)videoImageBuffer[0], (u16*)BG_GFX+(256*12), 0x12000);
			}
			fclose(videoFrameFile);
		}
	}

	while (rocketVideo_playVideo) {
		if (sixtyFps && rocketVideo_currentFrame >= 16) {
			drawNintendoLogoToVram();
		}
		swiWaitForVBlank();
	}
	if (!sixtyFps) swiWaitForVBlank();

	for (int i = 0; i < 256*60; i++) {
		BG_GFX[i] = ((whiteCol>>10)&0x1f) | ((whiteCol)&(0x1f<<5)) | (whiteCol&0x1f)<<10 | BIT(15);
	}

	// Pause on frame 31 for a second		
	for (int i = 0; i < 80; i++) { swiWaitForVBlank(); }

	/*FILE* destinationFile = fopen("sd:/_nds/TWiLightMenu/extractedvideo.rvid", "wb");
	fwrite(dsiSplashLocation, 1, 0x7C0000, destinationFile);
	fclose(destinationFile);*/

	// Fade out
	fadeType = false;
	for (int i = 0; i < 30; i++) { swiWaitForVBlank(); }
}

void BootSplashInit(void) {

	videoSetMode(MODE_3_2D | DISPLAY_BG3_ACTIVE);
	videoSetModeSub(MODE_3_2D | DISPLAY_BG3_ACTIVE);
	vramSetBankD(VRAM_D_MAIN_BG_0x06000000);
	vramSetBankC(VRAM_C_SUB_BG_0x06200000);
	REG_BG3CNT = BG_MAP_BASE(0) | BG_BMP16_256x256 | BG_PRIORITY(0);
	REG_BG3X = 0;
	REG_BG3Y = 0;
	REG_BG3PA = 1<<8;
	REG_BG3PB = 0;
	REG_BG3PC = 0;
	REG_BG3PD = 1<<8;

	REG_BG3CNT_SUB = BG_MAP_BASE(0) | BG_BMP16_256x256 | BG_PRIORITY(0);
	REG_BG3X_SUB = 0;
	REG_BG3Y_SUB = 0;
	REG_BG3PA_SUB = 1<<8;
	REG_BG3PB_SUB = 0;
	REG_BG3PC_SUB = 0;
	REG_BG3PD_SUB = 1<<8;

	BootSplashDSi();
}

