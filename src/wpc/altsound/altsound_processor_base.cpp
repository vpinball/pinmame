// ---------------------------------------------------------------------------
// altsound_processor_base.cpp
// 06/23/23 - Dave Roscoe
//
// Base class implementation for AltsoundProcessor, which encapsulates
// command execution, and specialization in support of all AltSound formats
// ---------------------------------------------------------------------------
// license:<TODO>
// ---------------------------------------------------------------------------
#include "altsound_processor_base.hpp"

// ---------------------------------------------------------------------------
// CTOR/DTOR
// ---------------------------------------------------------------------------

AltsoundProcessorBase::AltsoundProcessorBase(const std::string& game_name_in)
: game_name(game_name_in)
{
}

AltsoundProcessorBase::~AltsoundProcessorBase()
{
	// clean up stored steam objects
	for (auto& stream : channel_stream) {
		delete stream;
		stream = nullptr;
	}
}

// ---------------------------------------------------------------------------

bool AltsoundProcessorBase::findFreeChannel(unsigned int& channel_out)
{
	//LOG(("BEGIN: findFreeChannel()\n"));

	auto it = std::find(channel_stream.begin(), channel_stream.end(), nullptr);
	if (it != channel_stream.end()) {
		channel_out = std::distance(channel_stream.begin(), it);
		LOG(("- Found free channel: %02u\n", channel_out));
		//LOG(("END: findFreeChannel()\n"));
		return true;
	}

	LOG(("- ERROR: No free channels available!\n"));
	//LOG(("END: findFreeChannel()\n"));
	return false;
}

// ----------------------------------------------------------------------------

bool AltsoundProcessorBase::setStreamVolume(HSTREAM stream_in, const float vol_in)
{
	//LOG(("BEGIN: AltsoundProcessorBase::setVolume()\n"));

	if (stream_in == BASS_NO_STREAM)
		return true;

	float new_vol = vol_in * global_vol * master_vol;
	bool success = BASS_ChannelSetAttribute(stream_in, BASS_ATTRIB_VOL, new_vol);

	if (!success) {
		LOG(("- FAILED: BASS_ChannelSetAttribute(%u, BASS_ATTRIB_VOL, %.2f): %s\n", \
			stream_in, new_vol, get_bass_err()));
	}
	else {
		LOG(("- Successfully set volume for stream(%u): SAMPLE_VOL:%.02f  GLOBAL_VOL:%.02f  MASTER_VOL:%.02f\n",
			  stream_in, vol_in, global_vol, master_vol));
	}

	//LOG(("END: AltsoundProcessorBase::setVolume()\n"));
	return success;
}

// ----------------------------------------------------------------------------

bool AltsoundProcessorBase::createStream(void* syncproc_in, AltsoundStreamInfo* stream_out)
{
	LOG(("BEGIN: AltsoundProcessorBase::createStream()\n"));

	std::string short_path = getShortPath(stream_out->sample_path); // supports logging
	unsigned int ch_idx;
	
	if (!findFreeChannel(ch_idx)) {
		LOG(("- No free channels available!\n"));
		LOG(("END: AltsoundProcessorBase::createStream()\n"));
		return false;
	}

	stream_out->channel_idx = ch_idx; // store channel assignment
	bool loop = stream_out->loop;

  // Create playback stream
	HSTREAM hstream = BASS_StreamCreateFile(FALSE, stream_out->sample_path.c_str(), 0, 0, loop ? BASS_SAMPLE_LOOP : 0);

	if (hstream == BASS_NO_STREAM) {
		// Failed to create stream
		LOG(("- FAILED: BASS_StreamCreateFile(%s): %s\n", short_path.c_str(), get_bass_err()));
		LOG(("END: AltsoundProcessorBase::createStream()\n"));
		return false;
	}

	// Set callback to execute when sample playback ends
	SYNCPROC* callback = static_cast<SYNCPROC*>(syncproc_in);
	HSYNC hsync = 0;

	if (callback) {
		// Set sync to execute callback when sample playback ends
		hsync = BASS_ChannelSetSync(hstream, BASS_SYNC_END | BASS_SYNC_ONETIME, 0,
			                              callback, stream_out);
		if (!hsync) {
			// Failed to set sync
			LOG(("- FAILED: BASS_ChannelSetSync(): STREAM: %u ERROR: %s\n", hstream, get_bass_err()));
			freeStream(hstream);
			LOG(("END: AltsoundProcessorBase::createStream()\n"));
			return false;
		}
	}
	LOG(("- Successfully created stream(%u) on channel(%02d)\n", hstream, ch_idx));

	stream_out->hstream = hstream; // store hstream
	stream_out->hsync = hsync; // store hsync

	LOG(("END: AltsoundProcessorBase::createStream()\n"));
	return true;
}

// ----------------------------------------------------------------------------

bool AltsoundProcessorBase::freeStream(const HSTREAM hstream_in)
{
	//LOG(("BEGIN: AltsoundProcessorBase::freeStream()\n"));

	bool success = BASS_StreamFree(hstream_in);
	if (!success) {
		LOG(("- FAILED: BASS_StreamFree(%u)\n", hstream_in));
	}
	else {
		LOG(("- Successfully free'd stream(%u)\n", hstream_in));
	}

	//LOG(("END: AltsoundProcessorBase::freeStream()\n"));
	return success;
}

// ----------------------------------------------------------------------------

bool AltsoundProcessorBase::stopStream(HSTREAM hstream_in)
{
	LOG(("BEGIN: AltsoundProcessorBase::stopStream()\n"));

	bool success = false;

	if (hstream_in != BASS_NO_STREAM) {
		if (BASS_ChannelStop(hstream_in)) {
			success = freeStream(hstream_in);
			if (!success) {
				LOG(("- FAILED: freeStream()\n"));
			}
			else {
				LOG(("- Successfully stopped stream(%u)\n", hstream_in));
			}
		}
		else {
			LOG(("- FAILED: BASS_ChannelStop(%u): %s\n", hstream_in, get_bass_err()));
		}
	}

	LOG(("END: AltsoundProcessorBase::stopStream()\n"));
	return success;
}

// ----------------------------------------------------------------------------

bool AltsoundProcessorBase::stopAllStreams()
{
	bool success = true;

	for (auto stream : channel_stream) {
		if (!stream)
			continue;

		if (!stopStream(stream->hstream)) {
			success = false;
			LOG(("FAILED: stopStream(%u)", stream->hstream));
		}
	}
	
	return success;
}

// ---------------------------------------------------------------------------
// Helper function to remove major path from filenames.  Returns just:
// <ROM shortname>/<rest of path>
// ---------------------------------------------------------------------------

std::string AltsoundProcessorBase::getShortPath(const std::string& path_in)
{
	std::string tmp_str = strstr(path_in.c_str(), game_name.c_str());
	return tmp_str.empty() ? path_in : tmp_str;
}

