#pragma once

#include <Windows.h>
#include <MMSystem.h>

class SimpleMIDIPlayer
{
public:
	SimpleMIDIPlayer();
	~SimpleMIDIPlayer();

	typedef enum { C, Cs, D, Ds, E, F, Fs, G, Gs, A, As, B } NotesEnum;
	typedef struct {
		SimpleMIDIPlayer* player;
		NotesEnum note;
		bool major;
		BYTE intensity;
	} SimpleMIDIPlayerThreadStruct;

	/**
	 * These are used to play music...
	 */
	void playNote( NotesEnum note, int octave );
	void stopNote( NotesEnum note, int octave );
	void stopAll();
	void selectInstrument( BYTE i );

private:
	UINT sendMIDIEvent(HMIDIOUT hmo, BYTE bStatus, BYTE bData1, BYTE bData2);
	HMIDIOUT	outHandle;
	BYTE getNote( int note, int octave );
};

DWORD WINAPI _SimpleMIDIPlayerThread(
  LPVOID lpParameter   // thread data
);