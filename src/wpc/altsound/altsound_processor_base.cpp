// ---------------------------------------------------------------------------
// altsound_processor_base.cpp
//
// Base class implementation for AltsoundProcessor, which encapsulates
// command execution, and specialization in support of all AltSound formats
// ---------------------------------------------------------------------------
// license:BSD-3-Clause
// copyright-holders: Dave Roscoe
// ---------------------------------------------------------------------------
#include "altsound_processor_base.hpp"

#include <chrono>
#include <iomanip>

#include "altsound_logger.hpp"

extern AltsoundLogger alog;
extern StreamArray channel_stream;

// initialize static data members
float AltsoundProcessorBase::global_vol = 1.0f;
float AltsoundProcessorBase::master_vol = 1.0f;

// reference to sound command recording status
extern bool rec_snd_cmds;

// DAR@20230721
// ALTSOUND_STANDALONE is a compile-time flag set only when building the
// altsound processor as an executable.  It prevents the executable from
// trying to record a cmdlog.txt during playback, corrupting the source
// file
//
#ifndef ALTSOUND_STANDALONE
	std::ofstream logFile;
	std::chrono::high_resolution_clock::time_point lastCmdTime;
#endif

// ---------------------------------------------------------------------------
// CTOR/DTOR
// ---------------------------------------------------------------------------

AltsoundProcessorBase::AltsoundProcessorBase(const std::string& _game_name,
	                                         const std::string& _vpm_path)
: game_name(_game_name),
  vpm_path(_vpm_path),
  skip_count(0)
{
}

AltsoundProcessorBase::~AltsoundProcessorBase()
{
#ifndef ALTSOUND_STANDALONE
	// close sound command recording log
	if (logFile.is_open()) {
		logFile.close();
	}
#endif

	// clean up stored steam objects
	for (auto& stream : channel_stream) {
		delete stream;
		stream = nullptr;
	}
}

// ---------------------------------------------------------------------------

bool AltsoundProcessorBase::handleCmd(const unsigned int cmd_in) 
{
#ifndef ALTSOUND_STANDALONE
	if (rec_snd_cmds) {
		// sound command recording is enabled

		// Get the current time.
		const auto currentTime = std::chrono::high_resolution_clock::now();

		// Compute the difference from the last command time.
		const auto deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastCmdTime).count();

		// Log the time and command.
		logFile << std::setw(10) << std::setfill('0') << std::dec << deltaTime;
		logFile << ", 0x" << std::setw(4) << std::setfill('0') << std::hex << cmd_in;
		logFile << ", " << "<user comments here>" << std::endl;

		// Store the current time for the next command.
		lastCmdTime = currentTime;
	}
#endif
	return true;
}

// ---------------------------------------------------------------------------

void AltsoundProcessorBase::init()
{
#ifndef ALTSOUND_STANDALONE
	// If recording sound commands, initialize output file
	if (rec_snd_cmds) {
		if (!ALT_CALL(startLogging(AltsoundProcessorBase::getGameName()))) {
			ALT_ERROR(0, "FAILED startLogging()");
		}
		else {
			ALT_INFO(1, "SUCCESS startLogging()");
		}
	}
#endif
}

// ---------------------------------------------------------------------------

bool AltsoundProcessorBase::startLogging(const std::string& gameName) {
#ifndef ALTSOUND_STANDALONE
	ALT_DEBUG(0, "BEGIN startLogging()");

	// Check if gameName is not empty
	if (gameName.empty()) {
		ALT_ERROR(1, "Game name is empty. Cannot start logging.");
		ALT_DEBUG(0, "END startLogging()");
		return false;
	}

	std::string recording_fname = vpm_path + "/altsound/" + gameName+ "/cmdlog.txt";
	logFile.open(recording_fname);
	if (!logFile.is_open()) {
		ALT_ERROR(1, "Failed to open log file: %s", recording_fname.c_str());
		ALT_DEBUG(0, "END startLogging()");
		return false;
	}

	ALT_INFO(1, "Creating sound command log: %s", recording_fname.c_str());

	std::string game_altsound_path = vpm_path + "/altsound/" + gameName;

	// Check if game_altsound_path is valid
	if (!dir_exists(game_altsound_path)) {
		ALT_ERROR(1, "Altsound path does not exist: %s", game_altsound_path.c_str());
		ALT_DEBUG(0, "END startLogging()");
		return false;
	}

	logFile << "altsound_path: " << game_altsound_path << std::endl;
	lastCmdTime = std::chrono::high_resolution_clock::now();

	ALT_DEBUG(0, "END startLogging()");
#endif
	return true;
}

// ---------------------------------------------------------------------------

bool AltsoundProcessorBase::findFreeChannel(unsigned int& channel_out)
{
	ALT_INFO(0, "BEGIN: AltsoundProcessorBase::findFreeChannel()");
	INDENT;

	const auto it = std::find(channel_stream.begin(), channel_stream.end(), nullptr);
	if (it != channel_stream.end()) {
		channel_out = static_cast<unsigned int>(std::distance(channel_stream.begin(), it));
		ALT_INFO(1, "Found free channel: %02u", channel_out);
		
		OUTDENT;
		ALT_DEBUG(0, "END: AltsoundProcessorBase::findFreeChannel()");
		return true;
	}
	ALT_ERROR(1, "No free channels available!");
	
	OUTDENT;
	ALT_DEBUG(0, "END AltsoundProcessorBase::findFreeChannel()");
	return false;
}

// ----------------------------------------------------------------------------

bool AltsoundProcessorBase::setStreamVolume(HSTREAM stream_in, const float vol_in)
{
	ALT_DEBUG(0, "BEGIN: AltsoundProcessorBase::setVolume()");
	INDENT;

	if (stream_in == BASS_NO_STREAM)
		return true;

	const float new_vol = vol_in * global_vol * master_vol;
	ALT_INFO(1, "Setting volume for stream %u", stream_in);
	ALT_DEBUG(1, "SAMPLE_VOL:%.02f  GLOBAL_VOL:%.02f  MASTER_VOL:%.02f", vol_in,
		      global_vol, master_vol);
	const bool success = BASS_ChannelSetAttribute(stream_in, BASS_ATTRIB_VOL, new_vol) != 0;

	if (!success) {
		ALT_ERROR(1, "FAILED BASS_ChannelSetAttribute(BASS_ATTRIB_VOL)");
	}
	else {
		ALT_INFO(1, "SUCCESS BASS_ChannelSetAttribute(BASS_ATTRIB_VOL)");
	}

	OUTDENT;
	ALT_DEBUG(0, "END: AltsoundProcessorBase::setVolume()");
	return success;
}

// ----------------------------------------------------------------------------

float AltsoundProcessorBase::getStreamVolume(HSTREAM stream_in)
{
	if (stream_in == BASS_NO_STREAM)
		return -FLT_MAX;

	float vol;
	if (!BASS_ChannelGetAttribute(stream_in, BASS_ATTRIB_VOL, &vol))
		return -FLT_MAX;
	else
		return vol/(global_vol * master_vol);
}

// ----------------------------------------------------------------------------

bool AltsoundProcessorBase::createStream(void* syncproc_in, AltsoundStreamInfo* stream_out)
{
	ALT_DEBUG(0, "BEGIN AltsoundProcessorBase::createStream()");
	INDENT;

	const std::string short_path = getShortPath(stream_out->sample_path);
	unsigned int ch_idx;
	
	if (!ALT_CALL(findFreeChannel(ch_idx))) {
		ALT_ERROR(1, "FAILED AltsoundProcessorBase::findFreeChannel()");
		
		OUTDENT;
		ALT_DEBUG(0, "END AltsoundProcessorBase::createStream()");
		return false;
	}

	stream_out->channel_idx = ch_idx; // store channel assignment
	const bool loop = stream_out->loop;

    // Create playback stream
	HSTREAM hstream = BASS_StreamCreateFile(FALSE, stream_out->sample_path.c_str(), 0, 0, loop ? BASS_SAMPLE_LOOP : 0);

	if (hstream == BASS_NO_STREAM) {
		// Failed to create stream
		ALT_ERROR(1, "FAILED BASS_StreamCreateFile(%s): %s", short_path.c_str(), get_bass_err());
		
		OUTDENT;
		ALT_DEBUG(0, "END: AltsoundProcessorBase::createStream()");
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
			ALT_ERROR(1, "FAILED BASS_ChannelSetSync(): STREAM: %u ERROR: %s", hstream, get_bass_err());
			freeStream(hstream);
			
			OUTDENT;
			ALT_DEBUG(0, "END: AltsoundProcessorBase::createStream()");
			return false;
		}
	}
	ALT_INFO(1, "Successfully created stream(%u) on channel(%02d)", hstream, ch_idx);

	stream_out->hstream = hstream; // store hstream
	stream_out->hsync = hsync; // store hsync

	OUTDENT;
	ALT_DEBUG(0, "END: AltsoundProcessorBase::createStream()");
	return true;
}

// ----------------------------------------------------------------------------

bool AltsoundProcessorBase::freeStream(const HSTREAM hstream_in)
{
	ALT_INFO(0, "BEGIN AltsoundProcessorBase::freeStream()");
	INDENT;

	const bool success = BASS_StreamFree(hstream_in) != 0;
	if (!success) {
		ALT_ERROR(1, "FAILED BASS_StreamFree(%u)", hstream_in);
	}
	else {
		ALT_INFO(1, "Successfully free'd stream(%u)", hstream_in);
	}

	OUTDENT;
	ALT_DEBUG(0, "END AltsoundProcessorBase::freeStream()");
	return success;
}

// ----------------------------------------------------------------------------

bool AltsoundProcessorBase::stopStream(HSTREAM hstream_in)
{
	ALT_DEBUG(0, "BEGIN: AltsoundProcessorBase::stopStream()");
	INDENT;

	bool success = false;

	if (hstream_in != BASS_NO_STREAM) {
		if (BASS_ChannelStop(hstream_in)) {
			success = freeStream(hstream_in);
			if (!success) {
				ALT_ERROR(0, "FAILED AltsoundProcessorBase::freeStream()");
			}
			else {
				ALT_INFO(0, "Successfully stopped stream(%u)", hstream_in);
			}
		}
		else {
			ALT_ERROR(0, "FAILED BASS_ChannelStop(%u): %s", hstream_in, get_bass_err());
		}
	}

	OUTDENT;
	ALT_DEBUG(0, "END: AltsoundProcessorBase::stopStream()");
	return success;
}

// ----------------------------------------------------------------------------

bool AltsoundProcessorBase::stopAllStreams()
{
	ALT_DEBUG(0, "BEGIN AltsoundProcessorBase::stopAllStreams()");
	INDENT;

	bool success = true;

	for (auto stream : channel_stream) {
		if (!stream)
			continue;

		if (!stopStream(stream->hstream)) {
			success = false;
			ALT_ERROR(0, "FAILED stopStream(%u)", stream->hstream);
		}
	}
	
	ALT_DEBUG(0, "END AltsoundProcessorBase::stopAllStreams()");
	return success;
}

// ---------------------------------------------------------------------------
// Helper function to remove major path from filenames.  Returns just:
// <ROM shortname>/<rest of path>
// ---------------------------------------------------------------------------

std::string AltsoundProcessorBase::getShortPath(const std::string& path_in)
{
	const std::size_t pos = path_in.find(game_name);
	if (pos != std::string::npos)
	{
		return path_in.substr(pos);
	}
	else
	{
		return path_in;
	}
}
