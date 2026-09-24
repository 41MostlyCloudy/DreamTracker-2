#pragma once


#include "GlobalVariables.h"






void SetUpAudioEngine();

void StartNote(int channel, int sampleNumber, float pitch);

void PlayChannels(float* pOutputF32, ma_uint32 frameCount, ma_uint32 frameOffset);

void updateSongOnBeat();

void updateChannelOnBeat(int ch);

void RecordSong();

void StartOrStopSong();

void DrawSampleDisplay();

void DrawEnvelopeDisplay();

void DrawOscilloscope(int channel);





////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// New Audio

void readModulator(float* pOutputF32, ma_uint64 frameCount, int channel, int waveForm, float* mod);

void applySubtractiveFilters(float* pOutputF32, ma_uint64 frameCount, int channel, float* input, ma_uint32 frameOffset);

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);

void readWithFMAlgorithm(float* pOutputF32, ma_uint64 frameCount, int channel, ma_uint32 frameOffset);









// Decoders
ma_decoder_config decoderConfig;


// Device
ma_device_config deviceConfig;
ma_device device;


// Encoder
ma_encoder_config encoderConfig;
ma_encoder encoder;









void readModulator(float* pOutputF32, ma_uint64 frameCount, int channel, int op, float* mod)
{
    // For a mapped wave, all instrument properties are used.
    // The channel waveform state uses the direct operator waveform.


    float notePitch = channels[channel].pitch * channels[channel].arpPitch;


    



    int lfo = loadedInstruments[channels[channel].instrument].waveforms[op].octave - 3;
    float lfoMultiplier = pow(0.5f, lfo);

    notePitch *= lfoMultiplier;

    if (notePitch > 12.0f) notePitch = 12.0f;





    


    for (int i = 0; i < frameCount; i++)
    {
        


        //////////////////////////////////////////////////////////////////////////////////////////////////////// Envelope

        channels[channel].waveforms[op].ampEnvelope.envelopePos += 0.001f * loadedInstruments[channels[channel].instrument].envelopeScale;

        while (channels[channel].waveforms[op].ampEnvelope.envelopePos >= 1.0f)
        {
            channels[channel].waveforms[op].ampEnvelope.currentEnvelopeAmp = channels[channel].waveforms[op].ampEnvelope.nextEnvelopeAmp;
            channels[channel].waveforms[op].ampEnvelope.currentEnvelopeIndex++;

            if (channels[channel].waveforms[op].ampEnvelope.currentEnvelopeIndex < 32)
            {
                channels[channel].waveforms[op].ampEnvelope.nextEnvelopeAmp = float(loadedInstruments[channels[channel].instrument].waveforms[op].envelope[channels[channel].waveforms[op].ampEnvelope.currentEnvelopeIndex]) / 255.0f;
            }

            channels[channel].waveforms[op].ampEnvelope.envelopePos -= 1.0f;
        }


        float envInterp = channels[channel].waveforms[op].ampEnvelope.envelopePos;
        float envAmp = channels[channel].waveforms[op].ampEnvelope.currentEnvelopeAmp * (1.0f - envInterp) + channels[channel].waveforms[op].ampEnvelope.nextEnvelopeAmp * envInterp;


        // Apply release
        if (channels[channel].noteStopped)
        {
            if (loadedInstruments[channels[channel].instrument].waveforms[op].noSustain)
            {
                envAmp = 0.0f;
            }
            else
            {
                channels[channel].waveforms[op].releaseTimer += (1.0f - loadedInstruments[channels[channel].instrument].waveforms[op].release) * 0.0002;
                envAmp *= 1.0f - channels[channel].waveforms[op].releaseTimer;
                if (envAmp < 0.0f)
                    envAmp = 0.0f;
            }
        }
        

        //////////////////////////////////////////////////////////////////////////////////////////////////////// Pitch envelope

        channels[channel].pitchEnvelope.envelopePos += 0.001f * loadedInstruments[channels[channel].instrument].envelopeScale;

        while (channels[channel].pitchEnvelope.envelopePos >= 1.0f)
        {
            channels[channel].pitchEnvelope.currentEnvelopeAmp = channels[channel].pitchEnvelope.nextEnvelopeAmp;
            channels[channel].pitchEnvelope.currentEnvelopeIndex++;

            if (channels[channel].pitchEnvelope.currentEnvelopeIndex < 32)
            {
                channels[channel].pitchEnvelope.nextEnvelopeAmp = float(loadedInstruments[channels[channel].instrument].pitchEnvelope[channels[channel].pitchEnvelope.currentEnvelopeIndex]) / 127.0f;
            }

            channels[channel].pitchEnvelope.envelopePos -= 1.0f;
        }


        float pitchEnvInterp = channels[channel].pitchEnvelope.envelopePos;
        float pitchEnvAmp = channels[channel].pitchEnvelope.currentEnvelopeAmp * (1.0f - pitchEnvInterp) + channels[channel].pitchEnvelope.nextEnvelopeAmp * pitchEnvInterp;


        ////////////////////////////////////////////////////////////////////////////////////////////////////////


        notePitch = channels[channel].pitch * pitchEnvAmp;


        

        // Step arp timer.
        float arpStep = (loadedSong.bpm * loadedInstruments[channels[channel].instrument].arpSpeed) / (30.0f * 48000.0f);
        channels[channel].arpTimer += arpStep;

        while (channels[channel].arpTimer >= loadedInstruments[channels[channel].instrument].arpLength)
        {
            channels[channel].arpTimer -= loadedInstruments[channels[channel].instrument].arpLength;
        }

        // Arp
        if (int(channels[channel].arpTimer) != int(channels[channel].arpTimer - arpStep))
        {
            float arpNote = float(loadedInstruments[channels[channel].instrument].arpPitches[int(channels[channel].arpTimer)]);
            arpNote /= loadedSong.edo;
            arpNote = pow(2, arpNote);
            channels[channel].arpPitch = arpNote;
        }

        notePitch *= channels[channel].arpPitch;




        notePitch *= lfoMultiplier;

        if (notePitch > 12.0f) notePitch = 12.0f;
        if (notePitch < 0.0f) notePitch = 0.0f;


        ////////////////////////////////////////////////////////////////////////////////////////////////////////// Modulate

        float frameVol = 0.0f;

        if (mod != nullptr)
        {
            notePitch += mod[i] * 4.0f;
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////////// Read frame data.
        // Make sure that the frame reading position is inside the sample.
        while (channels[channel].waveforms[op].sampleReadPos >= waveformLength)
            channels[channel].waveforms[op].sampleReadPos -= waveformLength;
        while (channels[channel].waveforms[op].sampleReadPos < 0)
            channels[channel].waveforms[op].sampleReadPos += waveformLength;


        


        for (int freq = 0; freq < 8; freq++)
        {
            float readIndex = channels[channel].waveforms[op].sampleReadPos * waveformSpeed * float(freq + 1);
            int index1 = int(readIndex);
            int index2 = int(readIndex) + 1;

            float t = readIndex - index1;  // Fractional part


            while (index1 >= waveformLength)
                index1 -= waveformLength;
            while (index2 >= waveformLength)
                index2 -= waveformLength;


            float freqVol = waveForms[loadedInstruments[channels[channel].instrument].waveforms[op].waveType].pcmFrames[index1] * (1.0f - t)
                + waveForms[loadedInstruments[channels[channel].instrument].waveforms[op].waveType].pcmFrames[index2] * t;

            freqVol *= float(loadedInstruments[channels[channel].instrument].waveforms[op].frequencies[freq]) / 64.0f;

            frameVol += freqVol;
        }

        

        channels[channel].waveforms[op].sampleReadPos += notePitch;

        // Add noise to carrier.
        if (mod != nullptr)
        {

            if (notePitch > 0.01f)
                channels[channel].waveforms[op].noiseReadPos += notePitch * 0.5f;
            else
                channels[channel].waveforms[op].noiseReadPos += 0.01f;

            while (channels[channel].waveforms[op].noiseReadPos >= 1.0f)
            {
                channels[channel].waveforms[op].noiseReadPos--;

                channels[channel].waveforms[op].noiseVal = channels[channel].waveforms[op].nextNoiseVal;
                channels[channel].waveforms[op].nextNoiseVal = float((rand() % 256) - 127) / 128.0f;
            }

            float noiseT = channels[channel].waveforms[op].noiseReadPos;
            float noiseAmp = channels[channel].waveforms[op].noiseVal * (1.0f - noiseT) + channels[channel].waveforms[op].nextNoiseVal * noiseT;

            frameVol += loadedInstruments[channels[channel].instrument].waveforms[op].noiseVolume * noiseAmp;
        }


        // Add the LFO.
        // Make sure that the frame reading position is inside the sample.
        while (channels[channel].waveforms[op].lfoReadPos >= waveformLength)
            channels[channel].waveforms[op].lfoReadPos -= waveformLength;
        while (channels[channel].waveforms[op].lfoReadPos < 0)
            channels[channel].waveforms[op].lfoReadPos += waveformLength;


        float lfoReadIndex = channels[channel].waveforms[op].lfoReadPos * waveformSpeed;
        int lfoIndex1 = int(lfoReadIndex);
        int lfoIndex2 = int(lfoReadIndex) + 1;

        float lfoT = lfoReadIndex - lfoIndex1;  // Fractional part


        while (lfoIndex1 >= waveformLength)
            lfoIndex1 -= waveformLength;
        while (lfoIndex2 >= waveformLength)
            lfoIndex2 -= waveformLength;


        float lfoVol = ((waveForms[0].pcmFrames[lfoIndex1] * (1.0f - lfoT) + waveForms[0].pcmFrames[lfoIndex2] * lfoT) + 1.0f) * 0.5f;

        float lfoDepth = loadedInstruments[channels[channel].instrument].waveforms[op].lfoDepth;

        frameVol *= (1.0f - (lfoDepth * 0.5f)) + (lfoVol * lfoDepth);

        channels[channel].waveforms[op].lfoReadPos += 0.05f * loadedInstruments[channels[channel].instrument].waveforms[op].lfoSpeed;


        //////////////////////////////////////////////////////////////////////////////////////////////////////////

        
        



        // Glide to volume.
        float interp = 0.01f;
        channels[channel].waveforms[op].smoothVolume = channels[channel].waveforms[op].smoothVolume * (1.0f - interp) + envAmp * interp;
        
        frameVol *= channels[channel].waveforms[op].smoothVolume;


        pOutputF32[i] += frameVol;



        ////////////////////////////////////////////

        

        // Pitch Slide
        if (channels[channel].pitchSlide != 0.0f)
        {
            channels[channel].pitch += channels[channel].pitchSlide * 0.00001f * 120.0f;

            if (channels[channel].pitch > 8.0f)
                channels[channel].pitch = 8.0f;
            else if (channels[channel].pitch < 0.0f)
                channels[channel].pitch = 0.0f;
        }

        
    }


    




    return;
}



void applySubtractiveFilters(float* pOutputF32, ma_uint64 frameCount, int channel, float* input, ma_uint32 frameOffset)
{


    float volumeL = 0;
    float volumeR = 0;

    // Panning
    float pan = channels[channel].stereo;
    volumeL = (1.0f - pan) * 2.0f;
    volumeR = (pan) * 2.0f;

    if (volumeL > 1.0f) volumeL = 1.0f;
    else if (volumeL < 0.0f) volumeL = 0.0f;
    if (volumeR > 1.0f) volumeR = 1.0f;
    else if (volumeR < 0.0f) volumeR = 0.0f;


    

    for (int i = 0; i < frameCount; i++)
    {
        float noteVolume = channels[channel].volume * loadedInstruments[channels[channel].instrument].volume * 2.0f;
        if (noteVolume > 2.0f) noteVolume = 2.0f;
        else if (noteVolume < 0.0f) noteVolume = 0.0f;

        

        // Wave Volume Slide
        if (channels[channel].volumeSlide != 0.0f)
        {
            channels[channel].volume += channels[channel].volumeSlide * 0.0001f * 120.0f;
            if (channels[channel].volume > 1.0f) channels[channel].volume = 1.0f;
            else if (channels[channel].volume < 0.0f) channels[channel].volume = 0.0f;
        }




        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        float frameL = input[i];


        frameL *= 0.5f;

        float frameR = frameL;

        frameL *= noteVolume * volumeL;
        frameR *= noteVolume * volumeR;

        
        

        // Write the frames.
        pOutputF32[(i + frameOffset) * 2] += frameL;
        pOutputF32[(i + frameOffset) * 2 + 1] += frameR;


        // Add the info to the oscilloscope.
        float combinedVol = frameL * noteVolume * volumeL + frameR * noteVolume * volumeR;

        channels[channel].oscilloscope.pcmFrames[int(channels[channel].oscilloscope.readPos)] = combinedVol;

        // Step the reading position.
        channels[channel].oscilloscope.readPos += channels[channel].pitch;



        while (channels[channel].oscilloscope.readPos >= waveformLength)
            channels[channel].oscilloscope.readPos -= waveformLength;


        /////////////////////////////////////////////// Update note parameters.
    }


    return;
}



void readWithFMAlgorithm(float* pOutputF32, ma_uint64 frameCount, int channel, ma_uint32 frameOffset)
{
    if (!channels[channel].playing)
        return;

    

    float frames[480] = { 0 };
    float mod2[480] = { 0 };

    float oldPitch = channels[channel].pitch;
    float oldArp = channels[channel].arpTimer;

    float oldPitchEnvAmp = channels[channel].pitchEnvelope.currentEnvelopeAmp;
    float oldPitchEnvInd = channels[channel].pitchEnvelope.currentEnvelopeIndex;
    float oldPitchEnvPos = channels[channel].pitchEnvelope.envelopePos;
    float oldPitchEnvAmp2 = channels[channel].pitchEnvelope.nextEnvelopeAmp;

    readModulator(mod2, frameCount, channel, 1, NULL); // Read the modulator

    channels[channel].pitch = oldPitch;
    channels[channel].arpTimer = oldArp;

    channels[channel].pitchEnvelope.currentEnvelopeAmp = oldPitchEnvAmp;
    channels[channel].pitchEnvelope.currentEnvelopeIndex = oldPitchEnvInd;
    channels[channel].pitchEnvelope.envelopePos = oldPitchEnvPos;
    channels[channel].pitchEnvelope.nextEnvelopeAmp = oldPitchEnvAmp2;

    // Set the pitch of the arp if the note has changed.
    float arpNote = float(loadedInstruments[channels[channel].instrument].arpPitches[int(oldArp)]);
    arpNote /= loadedSong.edo;
    arpNote = pow(2, arpNote);
    channels[channel].arpPitch = arpNote;

    

    readModulator(frames, frameCount, channel, 0, mod2); // Read the carrier


    

    applySubtractiveFilters(pOutputF32, frameCount, channel, frames, frameOffset);
    

    


    return;
}



void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    std::lock_guard<std::shared_mutex> lock(mtx);

    
    float* pOutputF32 = (float*)pOutput;
    std::fill(pOutputF32, pOutputF32 + 960, 0.0f);

    

    if (editor.playingSong)
    {
        ma_uint32 frameOffset = 0;
        ma_uint32 framesToRead = 480;

        ma_uint32 fInOldBeat = loadedSong.timeInNote * 48.0f;

        float elapsedMS = float(frameCount) / 48000.0f * 1000.0f;

        loadedSong.timeInNote += elapsedMS;
        loadedSong.timeInSong += elapsedMS;


        ma_uint32 fInNote = loadedSong.timeInNote * 48.0f;

        ma_uint32 fInBeat = (60000.0f / (loadedSong.bpm * 4.0f)) * 48.0f;

        while (fInNote >= fInBeat)
        {
            float fInThisBeat = fInBeat - fInOldBeat;
            PlayChannels(pOutputF32, fInThisBeat, frameOffset);
            frameOffset += fInThisBeat;
            fInOldBeat = 0.0f;
            fInNote -= fInBeat;
            loadedSong.timeInNote -= fInBeat / 48.0f;
            framesToRead -= fInThisBeat;
            updateSongOnBeat();
        }

        PlayChannels(pOutputF32, framesToRead, frameOffset);
        
    }
    else
    {
        ma_uint32 frameOffset = 0;
        // For each channel, read out frames of data in 4000 frame groups.
        PlayChannels(pOutputF32, frameCount, frameOffset);
    }


    
    
    (void)pInput;
    (void)pDevice;
}



void PlayChannels(float* pOutputF32, ma_uint32 frameCount, ma_uint32 frameOffset)
{
    for (int channel = 0; channel < 8; channel++)
    {
        if (!(channels[channel].muted || (editor.playSolo && !channels[channel].solo)))
        {

            if (channels[channel].patternOffset > 0) // Start offset note.
            {
                channels[channel].patternOffset -= frameCount;

                if (channels[channel].patternOffset <= 0) // Start the new note.
                {
                    int framesOver = -channels[channel].patternOffset;

                    readWithFMAlgorithm(pOutputF32, ma_uint64(frameCount - framesOver), channel, frameOffset);
                    ma_uint64 delayedFrameOffset = ma_uint64(frameOffset + frameCount - framesOver);
                    channels[channel].patternOffset = 0;
                    StartNote(channel, channels[channel].offsetInstrument, channels[channel].offsetNote);
                    readWithFMAlgorithm(pOutputF32, framesOver, channel, delayedFrameOffset);
                }
                else
                    readWithFMAlgorithm(pOutputF32, frameCount, channel, frameOffset);
            }
            else if (channels[channel].retrigger > 0) // Retrigger note.
            {
                channels[channel].retriggerTimer += frameCount;

                float retrigAmount = 48000.0f * (120.0f / float(loadedSong.bpm)) * ((channels[channel].retrigger - 1.0f) / 256.0f);

                if (channels[channel].retriggerTimer > retrigAmount)
                {
                    channels[channel].retriggerTimer -= retrigAmount;
                    StartNote(channel, channels[channel].offsetInstrument, channels[channel].offsetNote);
                    
                }
                readWithFMAlgorithm(pOutputF32, frameCount, channel, frameOffset);
            }
            else
                readWithFMAlgorithm(pOutputF32, frameCount, channel, frameOffset);
        }
    }

    return;
}








void SetUpAudioEngine()
{
    // In this example, all decoders need to have the same output format.
    decoderConfig = ma_decoder_config_init(ma_format_f32, 2, 48000);

    /* Create only a single device. The decoders will be mixed together in the callback. In this example the data format needs to be the same as the decoders. */
    deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format = ma_format_f32;
    deviceConfig.playback.channels = 2;
    deviceConfig.sampleRate = 48000;
    deviceConfig.dataCallback = data_callback;
    deviceConfig.pUserData = NULL;

    ma_device_init(NULL, &deviceConfig, &device);

    // Now we start playback and wait for the audio thread to tell us to stop.
    ma_device_start(&device);
    

    // Set up the encoder
    encoderConfig = ma_encoder_config_init(ma_encoding_format_wav, ma_format_f32, 2, 48000);
    



    return;
}




void StartNote(int channel, int sampleNumber, float pitch)
{
    channels[channel].offsetNote = pitch;


    if (!editor.playingSong)
        for (int ch = 0; ch < 8; ch++)
            channels[ch].playing = false;

    if (!loadedInstruments[sampleNumber].enabled) // Don't play samples that are not loaded.
        return;

    if (sampleNumber < 0) // Don't play notes with samples below zero. (-1 is used to stop notes.)
        return;

    

    channels[channel].playing = true;






    // If changing instruments, reset frame reading to make sure that phase is consistent.
    if (sampleNumber != channels[channel].instrument)
    {
        for (int wave = 0; wave < 2; wave++)
            channels[channel].waveforms[wave].sampleReadPos = 0.0f;
    }

    channels[channel].instrument = sampleNumber;


    // Set pitch.
    pitch -= loadedSong.edo * 4;
    

    // Reset arpeggiation.
    channels[channel].arpTimer = 0.0f;



    float wavePitch = pitch;
    wavePitch = pow(2, wavePitch / loadedSong.edo);

    channels[channel].pitch = wavePitch;
    

    for (int wave = 0; wave < 2; wave++)
    {
        // Set envelope position.
        channels[channel].waveforms[wave].ampEnvelope.currentEnvelopeIndex = 0;
        channels[channel].waveforms[wave].ampEnvelope.envelopePos = 0.0f;
        
        channels[channel].waveforms[wave].ampEnvelope.currentEnvelopeAmp = float(loadedInstruments[sampleNumber].waveforms[wave].envelope[0]) / 255.0f;
        channels[channel].waveforms[wave].ampEnvelope.nextEnvelopeAmp = float(loadedInstruments[sampleNumber].waveforms[wave].envelope[0]) / 255.0f;

        

        // Restart release timers.
        channels[channel].waveforms[wave].releaseTimer = 0.0f;
    }


    // Reset pitch envelope.
    channels[channel].pitchEnvelope.currentEnvelopeIndex = 0;
    channels[channel].pitchEnvelope.envelopePos = 0.0f;

    channels[channel].pitchEnvelope.currentEnvelopeAmp = float(loadedInstruments[sampleNumber].pitchEnvelope[0]) / 127.0f;
    channels[channel].pitchEnvelope.nextEnvelopeAmp = float(loadedInstruments[sampleNumber].pitchEnvelope[0]) / 127.0f;




    channels[channel].noteStopped = false;

    

    if (channels[channel].volume > 1)
        channels[channel].volume = 1;
    else if (channels[channel].volume < 0)
        channels[channel].volume = 0;




    return;
}







void updateSongOnBeat()
{
    // The interface has changed, and must be redrawn.
    gui.drawFrameThisFrame = true;

    loadedSong.currentNote++;



    

    for (int ch = 0; ch < 8;ch++)
    {
        if (channels[ch].loopAtEnd) // Loop channel pattern
        {
            int rowCount = loadedSong.channelPatterns[ch].patterns[loadedSong.patterns[loadedSong.patternSequence[loadedSong.currentPattern]].channelPatterns[ch]].rows;

            if (loadedSong.currentNote % rowCount == 0)
            {
                loadedSong.noteChannelIndex[ch] = 0;
                loadedSong.volumeChannelIndex[ch] = 0;
                loadedSong.effectChannelIndex[ch] = 0;
            }
        }
    }


    if (loadedSong.currentNote >= loadedSong.patterns[loadedSong.patternSequence[loadedSong.currentPattern]].rows) // Start new frame.
    {

        loadedSong.currentPattern++;
        loadedSong.currentNote = 0;

        if (loadedSong.currentPattern >= loadedSong.patternSequence.size()) // Restart song.
        {
            loadedSong.currentPattern = 0;
            loadedSong.timeInSong = 0.0f;
            if (editor.recordingSong) // Stop recording.
            {
                editor.recordingSong = false;
            }
        }


        loadCurrentPattern();



        for (int ch = 0; ch < 8; ch++)
        {
            loadedSong.noteChannelIndex[ch] = 0;
            loadedSong.volumeChannelIndex[ch] = 0;
            loadedSong.effectChannelIndex[ch] = 0;
            loadedSong.toNextChannelNote[ch] = 0;
            loadedSong.toNextChannelVolume[ch] = 0;
            loadedSong.toNextChannelEffect[ch] = 0;


            int patternNumber = loadedSong.patternSequence[loadedSong.currentPattern];
            int channelPatternNum = loadedSong.patterns[patternNumber].channelPatterns[ch];
            channels[ch].stereo = loadedSong.channelPatterns[ch].patterns[channelPatternNum].stereo;
        }
    }


    


    // Read note data an display notes.
    for (int ch = 0; ch < 8; ch++)
    {
        updateChannelOnBeat(ch);
    }

    

    return;
}



void updateChannelOnBeat(int ch)
{

    // Find distance to first note, volume and effect.
    int noteIndex = loadedSong.noteChannelIndex[ch];
    int volumeIndex = loadedSong.volumeChannelIndex[ch];
    int effectIndex = loadedSong.effectChannelIndex[ch];




    if (loadedSong.noteChannelIndex[ch] == 0) // Initial "to next note" at the start of each frame.
    {
        loadedSong.toNextChannelNote[ch] = loadedSong.channelPatterns[ch].patterns[loadedSong.patterns[loadedSong.patternSequence[loadedSong.currentPattern]].channelPatterns[ch]].notes[0];
        loadedSong.noteChannelIndex[ch]++;
        noteIndex++;
        loadedSong.toNextChannelVolume[ch] = loadedSong.channelPatterns[ch].patterns[loadedSong.patterns[loadedSong.patternSequence[loadedSong.currentPattern]].channelPatterns[ch]].volumes[0];
        loadedSong.volumeChannelIndex[ch]++;
        volumeIndex++;
        loadedSong.toNextChannelEffect[ch] = loadedSong.channelPatterns[ch].patterns[loadedSong.patterns[loadedSong.patternSequence[loadedSong.currentPattern]].channelPatterns[ch]].effects[0];
        loadedSong.effectChannelIndex[ch]++;
        effectIndex++;
    }


    loadedSong.toNextChannelNote[ch]--;
    loadedSong.toNextChannelVolume[ch]--;
    loadedSong.toNextChannelEffect[ch]--;


    



    if (loadedSong.toNextChannelVolume[ch] < 0) // Read next volume.
    {
        if (volumeIndex < loadedSong.channelPatterns[ch].patterns[loadedSong.patterns[loadedSong.patternSequence[loadedSong.currentPattern]].channelPatterns[ch]].volumes.size())
        {
            int volume = loadedSong.channelPatterns[ch].patterns[loadedSong.patterns[loadedSong.patternSequence[loadedSong.currentPattern]].channelPatterns[ch]].volumes[volumeIndex];
            loadedSong.volumeChannelIndex[ch]++;
            volumeIndex++;

            channels[ch].volume = float(volume) / 255.0f;


            // Set distance to next note.
            if (volumeIndex < loadedSong.channelPatterns[ch].patterns[loadedSong.patterns[loadedSong.patternSequence[loadedSong.currentPattern]].channelPatterns[ch]].volumes.size())
            {
                loadedSong.toNextChannelVolume[ch] = loadedSong.channelPatterns[ch].patterns[loadedSong.patterns[loadedSong.patternSequence[loadedSong.currentPattern]].channelPatterns[ch]].volumes[volumeIndex];
                loadedSong.volumeChannelIndex[ch]++;
            }
            else
                loadedSong.toNextChannelVolume[ch] = 255; // No more notes in this channel in the frame.
        }
    }
    
    

    if (loadedSong.toNextChannelNote[ch] < 0)
    {
        if (noteIndex < loadedSong.channelPatterns[ch].patterns[loadedSong.patterns[loadedSong.patternSequence[loadedSong.currentPattern]].channelPatterns[ch]].notes.size()) // Stop notes do not reset notes.
        {
            int note = loadedSong.channelPatterns[ch].patterns[loadedSong.patterns[loadedSong.patternSequence[loadedSong.currentPattern]].channelPatterns[ch]].notes[noteIndex];
            if (note != 255)
                channels[ch].resetChannelEffects(false);
        }
    }



    

    if (loadedSong.toNextChannelEffect[ch] < 0) // Read next effect.
    {
        if (effectIndex < loadedSong.channelPatterns[ch].patterns[loadedSong.patterns[loadedSong.patternSequence[loadedSong.currentPattern]].channelPatterns[ch]].effects.size())
        {
            int effect = loadedSong.channelPatterns[ch].patterns[loadedSong.patterns[loadedSong.patternSequence[loadedSong.currentPattern]].channelPatterns[ch]].effects[effectIndex];
            loadedSong.effectChannelIndex[ch]++;
            effectIndex++;


            int effectType = effect / 16;
            int effectVal = effect % 16;



            if (effectType == 1) // Increase pitch.
                channels[ch].pitchSlide = float(effectVal) / 100.0f;
            else if (effectType == 2) // Decrease pitch.
                channels[ch].pitchSlide = float(effectVal) / -100.0f;
            else if (effectType == 3) // Increase volume.
                channels[ch].volumeSlide = float(effectVal) / 2000.0f;
            else if (effectType == 4) // Decrease volume.
                channels[ch].volumeSlide = float(effectVal) / -2000.0f;
            else if (effectType == 13) // Delay note.
            {
                float fInBeat = (60000.0f / (loadedSong.bpm * 4.0f)) * 48.0f;
                channels[ch].patternOffset = fInBeat * (float(effectVal) / 16.0f);
            }
            else if (effectType == 14) // Retrigger.
            {
                channels[ch].retrigger = float(effectVal * 2);
            }




            // Set distance to next note.
            if (effectIndex < loadedSong.channelPatterns[ch].patterns[loadedSong.patterns[loadedSong.patternSequence[loadedSong.currentPattern]].channelPatterns[ch]].effects.size())
            {
                loadedSong.toNextChannelEffect[ch] = loadedSong.channelPatterns[ch].patterns[loadedSong.patterns[loadedSong.patternSequence[loadedSong.currentPattern]].channelPatterns[ch]].effects[effectIndex];
                loadedSong.effectChannelIndex[ch]++;
            }
            else
                loadedSong.toNextChannelEffect[ch] = 255; // No more notes in this channel in the frame.
        }
    }




    

    if (loadedSong.toNextChannelNote[ch] < 0) // Read next note.
    {
        if (noteIndex < loadedSong.channelPatterns[ch].patterns[loadedSong.patterns[loadedSong.patternSequence[loadedSong.currentPattern]].channelPatterns[ch]].notes.size())
        {
            int note = loadedSong.channelPatterns[ch].patterns[loadedSong.patterns[loadedSong.patternSequence[loadedSong.currentPattern]].channelPatterns[ch]].notes[noteIndex];
            loadedSong.noteChannelIndex[ch]++;
            noteIndex++;
            

            int instrument = 0;
            if (note != 255) // No instruments for stop notes.
            {
                instrument = loadedSong.channelPatterns[ch].patterns[loadedSong.patterns[loadedSong.patternSequence[loadedSong.currentPattern]].channelPatterns[ch]].notes[noteIndex];
                loadedSong.noteChannelIndex[ch]++;
                noteIndex++;
            }

            // Set distance to next note.
            if (noteIndex < loadedSong.channelPatterns[ch].patterns[loadedSong.patterns[loadedSong.patternSequence[loadedSong.currentPattern]].channelPatterns[ch]].notes.size())
            {
                float pastNote = loadedSong.toNextChannelNote[ch];
                loadedSong.toNextChannelNote[ch] = loadedSong.channelPatterns[ch].patterns[loadedSong.patterns[loadedSong.patternSequence[loadedSong.currentPattern]].channelPatterns[ch]].notes[noteIndex];
                loadedSong.toNextChannelNote[ch] += pastNote + 1;
                loadedSong.noteChannelIndex[ch]++;
            }
            else
            {
                loadedSong.toNextChannelNote[ch] = 255; // No more notes in this channel in the frame.
            }

            if (note == 255) // Stop note.
            {
                channels[ch].noteStopped = true;
            }
            else
            {
                channels[ch].offsetNote = note;
                channels[ch].offsetInstrument = instrument;
                channels[ch].retriggerTimer = 0;



                if (channels[ch].patternOffset == 0) // Start note.
                    StartNote(ch, instrument, note);
            }
        }
    }




    return;
}



void RecordSong()
{
    if (editor.playingSong)
        StartOrStopSong();
    saveCurrentPattern();
    while (loadedSong.patternSequence[loadedSong.currentPattern] >= loadedSong.patterns.size()) // Create a new frame when changed to one not yet used.
    {
        PatternIndexTable newFrame;
        loadedSong.patterns.emplace_back(newFrame);
    }
    loadedSong.currentPattern = 0;
    loadCurrentPattern();


    StartOrStopSong();


    std::string fileName = fileNavigator.currentFilePath.std::filesystem::path::string() + "/" + loadedSong.songName + ".wav";
    const char* name = &fileName[0];
    ma_encoder_init_file(name, &encoderConfig, &encoder);

    

    editor.recordingSong = true;

    while (editor.recordingSong)
    {
        float frameCount = 480;

        float pOut[960] = { 0.0f };
        float* pOutputF32 = (float*)pOut;
        std::fill(pOutputF32, pOutputF32 + 960, 0.0f);


        ma_uint32 frameOffset = 0;
        ma_uint32 framesToRead = 480;

        float fInOldBeat = loadedSong.timeInNote * 48.0f;

        float elapsedMS = float(frameCount) / 48000.0f * 1000.0f;

        loadedSong.timeInNote += elapsedMS;
        loadedSong.timeInSong += elapsedMS;


        float fInNote = loadedSong.timeInNote * 48.0f;

        float fInBeat = (60000.0f / (loadedSong.bpm * 4.0f)) * 48.0f;

        while (fInNote >= fInBeat)
        {
            float fInThisBeat = fInBeat - fInOldBeat;
            PlayChannels(pOutputF32, fInThisBeat, frameOffset);
            frameOffset += fInThisBeat;
            fInOldBeat = 0.0f;
            fInNote -= fInBeat;
            loadedSong.timeInNote -= fInBeat / 48.0f;
            framesToRead -= fInThisBeat;
            updateSongOnBeat();
        }

        PlayChannels(pOutputF32, framesToRead, frameOffset);


        ma_uint64 framesWritten;
        ma_encoder_write_pcm_frames(&encoder, pOutputF32, 480, &framesWritten); // Write frames to file if recording.


    }

    
    
    StartOrStopSong();


    ma_encoder_uninit(&encoder);

    

    return;
}




void StartOrStopSong()
{


    editor.playingSong = !editor.playingSong;
    loadedSong.bpm = loadedSong.startingBPM;


    if (!editor.playingSong)
    {
        // Draw the audio oscilloscopes.
        if (gui.uiDisplayMenuOption == 3)
        {
            for (int ch = 0; ch < 8; ch++)
            {
                for (int fr = 0; fr < waveformLength; fr++)
                    channels[ch].oscilloscope.pcmFrames[fr] = 0.0f;
                DrawOscilloscope(ch);
            }
        }
    }


    saveCurrentPattern();

    for (int ch = 0; ch < 8; ch++)
    {
        channels[ch].playing = false;
    }

    if (editor.playingSong)
        loadedSong.currentNote = -1;
    else
        loadedSong.currentNote = 0;

    loadedSong.timeInNote = (60000.0f / (loadedSong.bpm * 4.0f));
    loadedSong.timeInSong = 0.0f;
    for (int ch = 0; ch < 8; ch++)
    {
        loadedSong.noteChannelIndex[ch] = 0;
        loadedSong.volumeChannelIndex[ch] = 0;
        loadedSong.effectChannelIndex[ch] = 0;
        loadedSong.toNextChannelNote[ch] = 0;
        loadedSong.toNextChannelVolume[ch] = 0;
        loadedSong.toNextChannelEffect[ch] = 0;



        int patternNumber = loadedSong.patternSequence[loadedSong.currentPattern];
        int channelPatternNum = loadedSong.patterns[patternNumber].channelPatterns[ch];
        channels[ch].stereo = loadedSong.channelPatterns[ch].patterns[channelPatternNum].stereo;



        // Set pitch.
            // If the pitch is 255, it is a stop note and the next note's pitch is taken.

        float pitch = 0;
        int firstNotePos = 1;

        while (pitch == 0 && loadedSong.channelPatterns[ch].patterns[loadedSong.patterns[loadedSong.patternSequence[loadedSong.currentPattern]].channelPatterns[ch]].notes.size() > firstNotePos)
        {
            pitch = loadedSong.channelPatterns[ch].patterns[loadedSong.patterns[loadedSong.patternSequence[loadedSong.currentPattern]].channelPatterns[ch]].notes[firstNotePos];

            if (pitch == 255)
            {
                pitch = 0;
                firstNotePos += 2;
            }
            else
            {
                pitch -= loadedSong.edo * 4;
                pitch = pow(2, pitch / loadedSong.edo);
            }
        }


        for (int wave = 0; wave < 2; wave++)
        {
            channels[ch].waveforms[wave].smoothVolume = 0.0f;


            channels[ch].waveforms[wave].sampleReadPos = 0.0f; // Restart phase to avoid phase cancelation.
        }

        channels[ch].pitch = pitch;


        channels[ch].resetChannelEffects(true);

    }
}




void DrawSampleDisplay()
{

    if (editor.selectedInstrument < 0 || !loadedInstruments[editor.selectedInstrument].enabled)
        return;




    for (int x = 0; x < 264; x++)
    {
        float brightness = 1.0f;


        int subdivision = int((float(x) / 264.0f) * 32.0f);
        bool lighten = subdivision % 2 == 0;

        for (int y = 0; y < 80; y++)
        {
            if (lighten)
            {
                sampleDisplay.pixelData[x + 264 * y].r = gui.uiColors[3] * 255.0f;
                sampleDisplay.pixelData[x + 264 * y].g = gui.uiColors[4] * 255.0f;
                sampleDisplay.pixelData[x + 264 * y].b = gui.uiColors[5] * 255.0f;
            }
            else
            {
                sampleDisplay.pixelData[x + 264 * y].r = gui.uiColors[0] * 255.0f;
                sampleDisplay.pixelData[x + 264 * y].g = gui.uiColors[1] * 255.0f;
                sampleDisplay.pixelData[x + 264 * y].b = gui.uiColors[2] * 255.0f;
            }
        }
    }


    DrawEnvelopeDisplay();

    

    return;
}




void DrawEnvelopeDisplay()
{

    int envNumber = sampleDisplay.selectedEnvelope;


    int lastEnvAmp = 0.0;

    if (envNumber < 2)
        lastEnvAmp = int((float(loadedInstruments[editor.selectedInstrument].waveforms[envNumber].envelope[0]) / 255.0) * 80.0);
    else
        lastEnvAmp = int((float(loadedInstruments[editor.selectedInstrument].pitchEnvelope[0]) / 255.0) * 80.0);

    for (int x = 0; x < 264; x++)
    {
        float brightness = 1.0f;


        int index = int((float(x) / 264.0) * 32.0);
        int nextIndex = index + 1;
        if (nextIndex >= 32)
            nextIndex = 31;
        bool lighten = index % 2 == 0;

        float interp = ((float(x) / 264.0) * 32.0) - index;


        int envVal = 0.0f;
        int envValNext = 0.0f;

        if (envNumber < 2)
        {
            envVal = int((float(loadedInstruments[editor.selectedInstrument].waveforms[envNumber].envelope[index]) / 255.0) * 80.0);
            envValNext = int((float(loadedInstruments[editor.selectedInstrument].waveforms[envNumber].envelope[nextIndex]) / 255.0) * 80.0);
        }
        else
        {
            envVal = int((float(loadedInstruments[editor.selectedInstrument].pitchEnvelope[index]) / 255.0) * 80.0);
            envValNext = int((float(loadedInstruments[editor.selectedInstrument].pitchEnvelope[nextIndex]) / 255.0) * 80.0);
        }



        float envAmp = envVal * (1.0f - interp) + envValNext * interp;


        for (int y = 0; y < 80; y++)
        {
            if ((y >= envAmp - 1 && y <= lastEnvAmp) || (y <= envAmp && y >= lastEnvAmp - 1))
            {
                sampleDisplay.pixelData[x + 264 * y].r = gui.uiColors[48] * 255.0f;
                sampleDisplay.pixelData[x + 264 * y].g = gui.uiColors[49] * 255.0f;
                sampleDisplay.pixelData[x + 264 * y].b = gui.uiColors[50] * 255.0f;
            }
        }

        if (envNumber == 2)
        {
            sampleDisplay.pixelData[x + 264 * 37].r = gui.uiColors[9] * 255.0f;
            sampleDisplay.pixelData[x + 264 * 37].g = gui.uiColors[10] * 255.0f;
            sampleDisplay.pixelData[x + 264 * 37].b = gui.uiColors[11] * 255.0f;

            sampleDisplay.pixelData[x + 264 * 40].r = gui.uiColors[9] * 255.0f;
            sampleDisplay.pixelData[x + 264 * 40].g = gui.uiColors[10] * 255.0f;
            sampleDisplay.pixelData[x + 264 * 40].b = gui.uiColors[11] * 255.0f;
        }

        lastEnvAmp = envAmp;
    }

    return;

}



void DrawOscilloscope(int channel)
{
    float pos = 0.0f;

    float step = (float(waveformLength) / waveformSpeed) / 48.0f;


    

    float initialVol = channels[channel].oscilloscope.pcmFrames[0] * 2.0f;
    int lastVol = int((initialVol + 1.0f) * 20.0f);

    if (lastVol >= 40)
        lastVol = 39;
    else if (lastVol < 0)
        lastVol = 0;

    
    for (int x = 1; x < 47; x++)
    {
        for (int y = 1; y < 39; y++)
        {
            channels[channel].oscilloscope.pixelData[x + 48 * y].r = gui.uiColors[0] * 255.0f;
            channels[channel].oscilloscope.pixelData[x + 48 * y].g = gui.uiColors[1] * 255.0f;
            channels[channel].oscilloscope.pixelData[x + 48 * y].b = gui.uiColors[2] * 255.0f;
        }

        pos += step;

        if (pos >= waveformLength)
            pos -= waveformLength;

        float vol = channels[channel].oscilloscope.pcmFrames[int(pos)] * 2.0f;

        

        int volPos = int((vol + 1.0f) * 20.0f);

        if (volPos >= 40)
            volPos = 39;
        else if (volPos < 0)
            volPos = 0;


        if (lastVol < volPos)
        {
            for (int y = lastVol; y <= volPos; y++)
            {
                channels[channel].oscilloscope.pixelData[x + 48 * y].r = gui.uiColors[48] * 255.0f;
                channels[channel].oscilloscope.pixelData[x + 48 * y].g = gui.uiColors[49] * 255.0f;
                channels[channel].oscilloscope.pixelData[x + 48 * y].b = gui.uiColors[50] * 255.0f;
            }
        }
        else
        {
            for (int y = volPos; y <= lastVol; y++)
            {
                channels[channel].oscilloscope.pixelData[x + 48 * y].r = gui.uiColors[48] * 255.0f;
                channels[channel].oscilloscope.pixelData[x + 48 * y].g = gui.uiColors[49] * 255.0f;
                channels[channel].oscilloscope.pixelData[x + 48 * y].b = gui.uiColors[50] * 255.0f;
            }
        }


        lastVol = volPos;
        
    }


    // Draw channel number
    for (int y = 0; y < 8; y++)
    {
        for (int x = 0; x < 8; x++)
        {
            if (channels[channel].oscilloscopeNumber.px[x][y])
            {
                channels[channel].oscilloscope.pixelData[x + 48 * (38 - y)].r = gui.uiColors[9] * 255.0f;
                channels[channel].oscilloscope.pixelData[x + 48 * (38 - y)].g = gui.uiColors[10] * 255.0f;
                channels[channel].oscilloscope.pixelData[x + 48 * (38 - y)].b = gui.uiColors[11] * 255.0f;
            }
        }
    }


    return;
}



