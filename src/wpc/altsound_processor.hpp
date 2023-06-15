#include <string>
#include <array>

#include "altsound_data.h"
#include "snd_alt.h"
#include "..\ext\bass\bass.h"

typedef std::array<AltsoundStreamInfo, ALT_MAX_CHANNELS> StreamArray;
typedef std::array<float, ALT_MAX_CHANNELS> DuckingArray;

class AltsoundProcessor {
public:

	// Standard constructor
	AltsoundProcessor(const char* gname_in,
		              const float global_vol_in,
		              const float master_vol_in);

	// Destructor
	~AltsoundProcessor();

	bool process_cmd(const unsigned int cmd_in);

	bool stop_music_stream();

	void setGlobalVolume(float vol_in);
	float getGlobalMusicVol();
	void setGlobalMusicVol(float vol_in);
	StreamArray& getStreams();
	DuckingArray& getStreamDucking();
	AltsoundStreamInfo& getMusicStream();
	AltsoundStreamInfo& getJingleStream();
	void setMusicStream(const AltsoundStreamInfo& stream_info_in);
	void setJingleStream(const AltsoundStreamInfo& stream_info_in);

protected:

	// Default constructor
	AltsoundProcessor();

private: // functions
	
	// set_volume
	int set_volume(HSTREAM stream_in, const float vol_in);

	//// clear out sample data and memory
	//void initialize_sample_data();
	
	// parse CSV file and populate sample data
	bool load_samples();
	
	// find sample matching provided command
	int get_sample(const unsigned int cmd_combined_in);
	
	// process music commands
	bool process_music(PinSamples* psd_in, int sample_idx_in, AltsoundStreamInfo& stream_out);
	
	// process jingle commands
	bool process_jingle(PinSamples* psd_in, int sample_idx_in, AltsoundStreamInfo& stream_out);
	
	// process sfx commands
	bool process_sfx(PinSamples* psd_in, int sample_idx_in, AltsoundStreamInfo& stream_out);

	// BASS SYNCPROC callback when jingle samples end
	static void CALLBACK jingle_callback(HSYNC handle, DWORD channel, DWORD data, void *user);
	
	// BASS SYNCPROC callback when sfx samples end
	static void CALLBACK sfx_callback(HSYNC handle, DWORD channel, DWORD data, void *user);
	
	// BASS SYNCPROC callback when music samples end
	static void CALLBACK music_callback(HSYNC handle, DWORD channel, DWORD data, void *user);

	bool create_stream(const char* fname_in, const int loop_count_in, SYNCPROC* syncproc_in, \
		               AltsoundStreamInfo& stream_out);

	HSYNC create_sync(HSTREAM stream_in, SYNCPROC* syncproc_in, unsigned int stream_idx_in);
	// get lowest ducking value of all active streams
	float get_min_ducking();
	
	// find available sound channel for sample playback
	int find_free_channel(void);
	
	// get short path of current game <gamename>/subpath/filename
	const char* get_short_path(const char* path_in);
	
	// stop playback of provided stream handle
	bool stop_stream(HSTREAM stream_in);
	
	// free BASS resources of provided stream handle
	bool free_stream(HSTREAM stream_in);

private: // data
	
	std::string game_name;
	bool is_initialized;
	bool is_stable; // future use
	float global_vol;
	float master_vol;
	float music_vol;
	CmdData cmds;
	PinSamples psd;
	StreamArray channel_stream;
	DuckingArray channel_ducking;
	AltsoundStreamInfo cur_mus_stream;
	AltsoundStreamInfo cur_jin_stream;
};

// ---------------------------------------------------------------------------
// Inline functions
// ---------------------------------------------------------------------------

inline void AltsoundProcessor::setGlobalVolume(float vol_in) {
	global_vol = vol_in;
}

inline StreamArray& AltsoundProcessor::getStreams() {
	return channel_stream;
}

inline DuckingArray& AltsoundProcessor::getStreamDucking() {
	return channel_ducking;
}

inline AltsoundStreamInfo& AltsoundProcessor::getMusicStream() {
	return cur_mus_stream;
}

inline void AltsoundProcessor::setMusicStream(const AltsoundStreamInfo& stream_info_in) {
	cur_mus_stream = stream_info_in;
}

inline float AltsoundProcessor::getGlobalMusicVol() {
	return music_vol;
}

inline void  AltsoundProcessor::setGlobalMusicVol(float vol_in) {
	music_vol = vol_in;
}

inline AltsoundStreamInfo& AltsoundProcessor::getJingleStream() {
	return cur_jin_stream;
}

inline void AltsoundProcessor::setJingleStream(const AltsoundStreamInfo& stream_info_in) {
	cur_jin_stream = stream_info_in;
}