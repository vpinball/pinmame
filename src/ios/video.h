#define MAX_VIDEO_WIDTH			1600
#define MAX_VIDEO_HEIGHT		1200

extern int			frameskip;
extern int          g_low_latency_throttle;

extern UINT32 		palette_16bit_lookup[];
extern UINT32 		palette_32bit_lookup[];

void update_palette(struct mame_display* display);
static void update_visible_area(struct mame_display *display);
void check_inputs(void);
void update_autoframeskip(void);
static void throttle_speed(void);
static void render_frame(struct mame_bitmap *bitmap, const struct rectangle *bounds, void *vector_dirty_pixels);
void throttle_speed_part(int part, int totalparts);
void SetThrottleAdj(int adj);
