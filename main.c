#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <psxetc.h>
#include <psxgte.h>
#include <psxgpu.h>
#include <psxcd.h>

#define PAL		// For switching between PAL and NTSC mode
#define OTLEN 8 // The length of the ordering table


// Define the DISPlay and DRAW environment pairs
// along with a buffer counter
DISPENV dispenv[2];
DRAWENV drawenv[2];
int db;

// A simply ordering table of 8 elements. The reason why
// two tables are defined is for double buffered rendering reasons
u_long ot[2][OTLEN];

// A primitive buffer with 32kb for primitves
// along with a variable (nextpri) to keep track where the next
// primitive should be written to
char primbuff[2][32768];
char *nextpri;

// Primitives
TILE* tile;				// Pointer for TILE
SPRT* sprt;				// Pointer for SPRT
int prac_x, prac_y;		// x and y for the primitive structure

typedef struct
{
	int mode, u, v;
	int x, y, w, h;
	int r, g, b;
	RECT prect, crect;
} TIM;

TIM nakke_tim;


// Function declarations
void init();
void display();
void draw();
char* loadFile(const char* filename);
void loadTexture(u_int* tim, TIM_IMAGE* tparam);


// Main function
int main()
{
	init();

	int counter = 0;
	prac_x = 64;
	prac_y = 128;

	TIM_IMAGE _nakke;
	loadTexture((u_int*)loadFile("\\NAKKE.TIM;1"), &_nakke);
	nakke_tim.mode = _nakke.mode;
	nakke_tim.prect = *_nakke.prect;
	nakke_tim.crect = *_nakke.crect;
	nakke_tim.w = nakke_tim.h = 64;
	nakke_tim.x = 192;
	nakke_tim.y = 64;
	nakke_tim.r = nakke_tim.g = nakke_tim.b = 128;
	nakke_tim.u = (nakke_tim.prect.x%64)<<(2-(nakke_tim.mode&0x3));
	nakke_tim.v = (nakke_tim.prect.y&0xff);

	// Main loop
	for(;;)
	{
		FntPrint(-1, "SKAL DU SPILLE SMART?");
		FntPrint(-1, "\nCOUNTER: %d", counter);
		FntFlush(-1);
		counter++;

		draw();
		display();
	}

	return 0;
}

// Function initializations
void init()
{
	// Resets the GPU and enables interrupts
	ResetGraph(0);

	// Initialize the CD-ROM library
	CdInit();

	#ifdef PAL	// PAL
		// Configures the pair of DISPlay environments for PAL mode (320x256)
		SetDefDispEnv(&dispenv[0], 0, 0, 320, 256);
		SetDefDispEnv(&dispenv[1], 0, 256, 320, 256);

		// Screen offset to center the picture vertically
		dispenv[0].screen.y = dispenv[1].screen.y = 24;

		// Forces PAL video standard
		SetVideoMode(MODE_PAL);

		// Configures the pair of practice environments
		SetDefDrawEnv(&drawenv[0], 0, 256, 320, 256);
		SetDefDrawEnv(&drawenv[1], 0, 0, 320, 256);
	#else		// NTSC
		// Configures the pair of DISPlay environments for NTSC mode (320x240)
		SetDefDispEnv(&dispenv[0], 0, 0, 320, 240);
		SetDefDispEnv(&dispenv[1], 0, 240, 320, 240);

		// Configures the pair of DRAW environments
		SetDefDrawEnv(&drawenv[0], 0, 240, 320, 240);
		SetDefDrawEnv(&drawenv[1], 0, 0, 320, 240);
	#endif

	// Specifies the clear color of the DRAW environment
	setRGB0(&drawenv[0], 113, 1, 147);
	setRGB0(&drawenv[1], 113, 1, 147);

	// Enable background clear
	drawenv[0].isbg = 1;
	drawenv[1].isbg = 1;

	// Apply the environments
	PutDispEnv(&dispenv[0]);
	PutDrawEnv(&drawenv[0]);

	// the db variable must be set to 0
	db = 0;
	// Reset the next primitive buffer
	nextpri = primbuff[db];

	// Load the internal font texture and create the text stream
	FntLoad(960, 0);
	FntOpen(0, 8, 320, 256, 0, 100);
}

void display()
{
	// Wait for the GPU to finish drawing and V-Blank
	DrawSync(0);
	VSync(0);

	PutDispEnv(&dispenv[db]);
	PutDrawEnv(&drawenv[db]);

	// Enable display, as ResetGraph() masks the it
	SetDispMask(1);
	
	// Draw the ordering table
	DrawOTag(ot[db]+OTLEN-1);

	// Flip the buffer counter (db)
	db = !db;
	// Reset the next primitive pointer
	nextpri = primbuff[db];
}

void draw()
{
	ClearOTagR(ot[db], OTLEN);

	tile = (TILE*)nextpri;
	setTile(tile);					// Initialize the primitive (very important)
	setXY0(tile, prac_x, prac_y);	// Set primitive (x,y) position
	setWH(tile, 64, 64);			// Set primitive (w,h) size
	setRGB0(tile, 255, 255, 0);		// Set primitive color to yellow
	addPrim(ot[db], tile);			// Add the primitive to the ordering table

	nextpri += sizeof(TILE);		// Move on to the next primitive pointer

	sprt = (SPRT*)nextpri;
	setSprt(sprt);
	setXY0(sprt, 192, 64);
	setWH(sprt, 64, 64);
	setUV0(sprt, nakke_tim.u, nakke_tim.v);
	setClut(sprt, nakke_tim.crect.x, nakke_tim.crect.y);
	setRGB0(sprt, nakke_tim.r, nakke_tim.g, nakke_tim.b);
	addPrim(ot[db], sprt);

	nextpri += sizeof(SPRT);
}

char* loadFile(const char* filename)
{
	CdlFILE filePos;
	int numsecs;
	char *buff;

	buff = NULL;

	// Try to locate the file on the CD
	if (CdSearchFile(&filePos, (char*)filename) == NULL)
	{
		// Print an error message if the file wasn't found
		printf("%s not found", filename);
	}
	else
	{
		// Print a success message if it was found
		printf("%s was found", filename);

		// Calculate the number of sectors to read for the file
		numsecs = (filePos.size+2047)/2048;

		// Allocate the buffer for the file
		buff = (char*)malloc(2048*numsecs);

		// Set the read target to the file
		CdControl(CdlSetloc, (u_char*)&filePos.pos, 0);

		// Start the read operation
		CdRead(numsecs, (u_long*)buff, CdlModeSpeed);

		// Wait until the read operation is complete
		CdReadSync(0, 0);
	}

	// Return either a blank buffer, or the content of the file
	return buff;
}


void loadTexture(u_int* tim, TIM_IMAGE* tparam)
{
	// Read TIM information
	GetTimInfo((uint32_t*) tim, tparam);

	// Upload pixel data to framebuffer
	LoadImage(tparam->prect, (u_long*)tparam->paddr);
	DrawSync(0);

	// Upload CLUT to framebuffer if present
	if (tparam->mode & 0x8)
	{
		LoadImage(tparam->crect, (u_long*)tparam->caddr);
		DrawSync(0);
	}
}