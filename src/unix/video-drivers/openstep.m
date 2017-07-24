/*
 * OpenStep specific file for XMAME. Here we define OpenStep specific
 * versions of all the MAME functions necessary to get it running under
 * OpenStep. The lastest version of this code now draws in a more conventional
 * manner to try and aid portability between this and Mac OS X. Thus all the
 * DisplayPostScript has been removed and the drawing embedded within a
 * custom NSView.
 *
 * -bat. 12/11/2000
 */

#import <stdio.h> 
#import <stdlib.h>
#import <unistd.h> 
#import <AppKit/AppKit.h>

#import "xmame.h"
#import "osdepend.h"
#import "driver.h"
#import "keyboard.h"
#import "devices.h"
#import "effect.h"

/*
 * There are two flavours of OpenStep - the original Display PostScript
 * based one and the new Mac OS X version. Sadly there is no good way of
 * distinguishing the two, thus we make a few intelligent guesses here.
 * If we have 'Apple' we are probably an OSX machine, and if we have 'BSD43'
 * then we are probably an old NeXT system. GNUstep is neither (and has not
 * been tried with this driver) so the default is old style.
 */

#ifdef __APPLE__
#define COCOA 1
#endif

#ifdef BSD43
#undef COCOA
#endif

/* display options */

struct rc_option display_opts[] = {
	{
	"OpenStep related", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL
	}, {
	NULL, NULL, rc_end, NULL, NULL, 0, 0, NULL, NULL
	}
};

/*
 * Size of the black border
 */

#define BORDER 12

/*
 * Variables used by command-line window.
 */

NSWindow *theWindow = nil;		/* needed by keyboard code */
static NSBitmapImageRep *thisBitmap = nil;
static int bitmap_width, bitmap_height;
static double gameAspect;

static NSDate *thePast = nil;		/* avoid memory leak on pause */

int isMinaturised = 0;			/* used to disable sound */

/*
 * Screen bitmap variable
 */

static unsigned short *screen12bit = NULL;

/*
 * Autorelease pool variables. We have an outer pool which is never freed
 * due to lessons learnt from EOF about always having at least *one* pool
 * active ! We also have a pool that exists across display open and closes
 * to make sure that everthing we make in a display open vanishes properly.
 */

static NSAutoreleasePool *outer_pool = nil;
static NSAutoreleasePool *display_pool = nil;

/*
 * Intialise the two pools. Create the application object.
 */

int
sysdep_init(void)
{
	outer_pool = [NSAutoreleasePool new];
	NSApp = [[NSApplication sharedApplication] retain];
	thePast = [NSDate distantPast];
	return OSD_OK;
}

/*
 * Free up the autorelease pools before exitting.
 */

void
sysdep_close(void)
{
	[NSApp release];
	[outer_pool release];
}

/*
 * Shut down the display. We close the dps window and exit.
 */

void
sysdep_display_close(void)
{
	[theWindow close];
	theWindow = nil;
	[display_pool release];
}

extern void openstep_keyboard_init(void);

/*
 * This is the custom view we use to do all the drawing into the window, and
 * also to handle the resizing methods to enable us to maintain the windows
 * aspect ratio to allow resizing. The view is opaque and draws eveything
 * on top of a black background which extends just beyound the frame being
 * displayed to provide a black border for games such as PacMan.
 */

@interface MameView : NSView

- (void)drawRect:(NSRect)aRect;
- (BOOL)isOpaque;
- (NSSize)windowWillResize:(NSWindow*)sender toSize:(NSSize)proposedFrameSize;

@end

@implementation MameView : NSView

/*
 * Maintain the aspect ratio of the game area when resizing.
 */

- (NSSize)windowWillResize:(NSWindow*)sender toSize:(NSSize)proposedFrameSize
{
	NSSize originalFrameSize = [sender frame].size;
	NSSize originalViewSize = [self frame].size;
	float extraWidth = originalFrameSize.width - originalViewSize.width;
	float extraHeight = originalFrameSize.height - originalViewSize.height;

	/* remove extras to make new view size */
	proposedFrameSize.width -= (extraWidth + (BORDER*2));
	proposedFrameSize.height -= (extraHeight + (BORDER*2));

	/* make new view have correct aspect ratio */
	if((proposedFrameSize.width / proposedFrameSize.height) < gameAspect)
		proposedFrameSize.width =
				proposedFrameSize.height * gameAspect;
	else
		proposedFrameSize.height =
				proposedFrameSize.width / gameAspect;

	/* add extras and return */
	proposedFrameSize.width += (extraWidth + (BORDER*2));
	proposedFrameSize.height += (extraHeight + (BORDER*2));
	return proposedFrameSize;
}

/*
 * Get the view bounds, draw the borders and then the bitmap into the
 * centre of it. We probably do not need to redraw the borders each time,
 * but we get strange effects onm OS X if we do not.
 */

-(void)drawRect:(NSRect)aRect
{
	NSRect blackRect, theRect = [self bounds];

	/* draw the borders */
	[[NSColor blackColor] set];
	blackRect = theRect;
	blackRect.size.width = BORDER;
	NSRectFill(blackRect);
	blackRect.origin.x = theRect.size.width - BORDER;
	NSRectFill(blackRect);
	blackRect = theRect;
	blackRect.size.height = BORDER;
	NSRectFill(blackRect);
	blackRect.origin.y = theRect.size.height - BORDER;
	NSRectFill(blackRect);

	/* draw the bitmap */
	theRect.size.width -= (BORDER*2);
	theRect.size.height -= (BORDER*2);
	theRect.origin.x += BORDER;
	theRect.origin.y += BORDER;

	[thisBitmap drawInRect:theRect];
}

/*
 * This view is always opaque
 */

- (BOOL)isOpaque
{
	return YES;
}

@end

/*
 * Create the display. We create a window of the appropriate size, then
 * make it display on the screen. Keyboard initialisation is also called
 * from this function. The view contains a custom content view with it's
 * own graphics state (for speed) that does the actual drawing of the bitmap.
 */

int
sysdep_create_display(int depth)
{
	MameView *theView = nil;
	NSRect content_rect = { {100,100}, {0,0} };

	/* make the display pool */
	display_pool = [NSAutoreleasePool new];

	bitmap_width = visual_width * widthscale;
	bitmap_height = visual_height * heightscale;

	/* set the size of the view */
	gameAspect = (double)bitmap_width / (double)bitmap_height;
	content_rect.size.width = bitmap_width + (BORDER*2);
	content_rect.size.height = bitmap_height + (BORDER*2);

	/* allocate memory for 12 bit colour version */
	screen12bit = [[NSMutableData dataWithLength:
			(2*bitmap_width*bitmap_height)] mutableBytes];
	if(!screen12bit) {
		fprintf(stderr,"12 bit memory allocate failed\n");
		[display_pool release];
		display_pool = nil;
		return OSD_NOT_OK;
	}

	/* create bitmap object  */
	thisBitmap = [[NSBitmapImageRep alloc]
		initWithBitmapDataPlanes:(void*)&screen12bit
		pixelsWide:bitmap_width pixelsHigh:bitmap_height
		bitsPerSample:4 samplesPerPixel:3
		hasAlpha:NO isPlanar:NO
		colorSpaceName:NSDeviceRGBColorSpace
		bytesPerRow:2*bitmap_width bitsPerPixel:16];
	if(!thisBitmap) {
		fprintf(stderr,"Bitmap creation failed\n");
		[display_pool release];
		display_pool = nil;
		return OSD_NOT_OK;
	}
	[thisBitmap autorelease];

	/* create a window - retained is broken on public beta */
	theWindow = [[NSWindow alloc] initWithContentRect:content_rect
			styleMask:(NSTitledWindowMask |
			NSMiniaturizableWindowMask |
			NSResizableWindowMask)
#ifdef COCOA
			backing:NSBackingStoreBuffered
#else
			backing:NSBackingStoreRetained
#endif
			defer:NO];
	[theWindow setTitle:[NSString
		stringWithCString:Machine->gamedrv->description]];
	[theWindow setReleasedWhenClosed:YES];

	/* create the custom content view */
	theView = [[MameView alloc] initWithFrame:
			[[theWindow contentView] frame]];
	[theView allocateGState];
	[theWindow setContentView:theView];
	[theWindow setMinSize:[theWindow frame].size];
	[theWindow setDelegate:theView];

	/* send it front and display the game name */
	[theWindow makeKeyAndOrderFront:nil];
	puts(Machine->gamedrv->description);

	/* set up the structure for the palette code */
	display_palette_info.writable_colors = 0;
	display_palette_info.depth = 16;

#ifdef LSB_FIRST
	display_palette_info.red_mask = 0x00f0;
	display_palette_info.green_mask = 0x000f;
	display_palette_info.blue_mask = 0xf000;
#else
	/* untested due to lack of big-endian machines at present */
	display_palette_info.red_mask = 0xf000;
	display_palette_info.green_mask = 0x0f00;
	display_palette_info.blue_mask = 0x00f0;
#endif

	/* shifts will be calculated from above settings */
	display_palette_info.red_shift = 0;
	display_palette_info.green_shift = 0;
	display_palette_info.blue_shift = 0;

	/* initialise the keyboard and return */
	openstep_keyboard_init();
	return OSD_OK;
}

/*
 * 8 bit display update. We use dirty unless the palette has been
 * changed, in which case the whole screen is updated.
 */

static void
update_display_8bpp(struct mame_bitmap *bitmap)
{
#define	SRC_PIXEL	unsigned char
#define	DEST_PIXEL	unsigned short
#define	DEST		screen12bit
#define	DEST_WIDTH	bitmap_width
#define	INDIRECT	current_palette->lookup
#include "blit.h"
#undef	SRC_PIXEL
#undef	DEST_PIXEL
#undef	DEST
#undef	DEST_WIDTH
#undef	INDIRECT
}

/*
 * 16 bit display update
 */

static void
update_display_16bpp(struct mame_bitmap *bitmap)
{
#define	SRC_PIXEL	unsigned short
#define	DEST_PIXEL	unsigned short
#define	DEST		screen12bit
#define	DEST_WIDTH	bitmap_width
	if(current_palette->lookup) {
#define	INDIRECT	current_palette->lookup
#include "blit.h"
#undef	INDIRECT
	} else {
#include "blit.h"
	}

#undef	SRC_PIXEL
#undef	DEST_PIXEL
#undef	DEST
#undef	DEST_WIDTH
}

/*
 * Update the display.  We create the bitmapped data for the current frame
 * and draw it into the window. If the window is minaturised however, then
 * we go into a loop catching events and passing them until such a time as it
 * is no longer minaturised. This is to avoid a huge drain on CPU time when
 * not actually playing and effectively acts like a pause.
 */

void
sysdep_update_display(struct mame_bitmap *bitmap)
{
	/* pause if minturised, setting the flag */
	while([theWindow isMiniaturized]) {
		NSEvent *thisEvent;
		isMinaturised = 1;
		thisEvent = [NSApp nextEventMatchingMask:NSAnyEventMask
				untilDate:thePast inMode:NSDefaultRunLoopMode
				dequeue:YES];
		if(thisEvent)
			[NSApp sendEvent:thisEvent];
		else
			usleep(50000);
	}
	isMinaturised = 0;

	/* call appropriate function with dirty */
	if(bitmap->depth == 16)
		update_display_16bpp(bitmap);
	else
		update_display_8bpp(bitmap);

	/* make the view as dirty and redisplay the window */
	[[theWindow contentView] setNeedsDisplay:YES];
	[theWindow displayIfNeeded];

	/* flushing is done differently on the two variants */
#ifdef COCOA
	[[NSGraphicsContext currentContext] flushGraphics];
#else
	PSWait();
#endif
}

/*
 * OpenStep system are always 16bpp capable.
 */

int
sysdep_display_16bpp_capable(void)
{
	return 1;
}

/*
 * The following functions are dummies - we always generate 16 bit
 * colour output on OpenStep systems.
 */

int
sysdep_display_alloc_palette(int writable_colours)
{
	return OSD_OK;
}

int
sysdep_display_set_pen(int pen,
		unsigned char r, unsigned char g, unsigned char b)
{
	return OSD_OK;
}
