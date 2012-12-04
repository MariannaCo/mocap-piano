#include "stdafx.h"
#include "SimpleMIDIPlayer.h"

SimpleMIDIPlayer::SimpleMIDIPlayer(){
	midiOutOpen(&outHandle, (UINT)-1, 0, 0, CALLBACK_WINDOW);

	selectInstrument( 0 );
}

SimpleMIDIPlayer::~SimpleMIDIPlayer(){
	midiOutClose(outHandle);
}

UINT SimpleMIDIPlayer::sendMIDIEvent(HMIDIOUT hmo, BYTE bStatus, BYTE bData1, BYTE bData2) 
{ 
    union { 
        DWORD dwData; 
        BYTE bData[4]; 
    } u; 
 
    // Construct the MIDI message. 
 
    u.bData[0] = bStatus;  // MIDI status byte 
    u.bData[1] = bData1;   // first MIDI data byte 
    u.bData[2] = bData2;   // second MIDI data byte 
    u.bData[3] = 0; 
 
    // Send the message. 
	return midiOutShortMsg(hmo, u.dwData); 
}

BYTE SimpleMIDIPlayer::getNote( int note, int octave ){
	return (BYTE)((( 12 * octave ) + note) % 127);
}


void SimpleMIDIPlayer::playNote( NotesEnum note, int octave ){
	sendMIDIEvent(	outHandle, 0x90, getNote(note, octave) , 127 );
}

void SimpleMIDIPlayer::stopNote( NotesEnum note, int octave ){
	sendMIDIEvent(	outHandle, 0x80, getNote(note, octave) , 127 );
}

void SimpleMIDIPlayer::stopAll(){
	sendMIDIEvent( outHandle, 0xB0, 0x7B, 0x00 );
}

void SimpleMIDIPlayer::selectInstrument( BYTE i ){
	sendMIDIEvent(	outHandle, 0xC0, i , (BYTE)0 );
}
/*
void SimpleMIDIPlayer::_playMajorBarChord( int n, BYTE intensity ){
	//int s = 50 - (40 * (intensity/127));
	//if ( intensity < 70 ) intensity = 70;

	intensity = 127;
	int s = 20;

	sendMIDIEvent(	outHandle, 0x90, getNote(n,3) , intensity );
	Sleep(s);
	sendMIDIEvent(	outHandle, 0x90, getNote(n + 7,3) , intensity );
	Sleep(s);
	sendMIDIEvent(	outHandle, 0x90, getNote(n + 12,3) , intensity );
	Sleep(s);
	sendMIDIEvent(	outHandle, 0x90, getNote(n + 16,3) , intensity );
	Sleep(s);
	sendMIDIEvent(	outHandle, 0x90, getNote(n + 19,3) , intensity );
	Sleep(s);
	sendMIDIEvent(	outHandle, 0x90, getNote(n + 24,3) , intensity );
}
*/