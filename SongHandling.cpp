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

void GenerateAdditiveWave(Instrument* instrument, int op);

void GenerateAllInstrumentWaves(Instrument* instrument);

void ConstructWave(Instrument* instrument, int op, int waveType, int frequencies[16], float framesToWrite, float periodLength, int frequency, float* inputWave);






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

    


    float notePitch = 1.0f;
    if (loadedInstruments[channels[channel].instrument].waveforms[op].pitchToNote)
        notePitch = channels[channel].pitch * channels[channel].arpPitch;


    



    int lfo = loadedInstruments[channels[channel].instrument].waveforms[op].octave - 3;
    float lfoMultiplier = pow(0.5f, lfo);

    notePitch *= lfoMultiplier;

    if (notePitch > 12.0f) notePitch = 12.0f;





    


    for (int i = 0; i < frameCount; i++)
    {
        


        //////////////////////////////////////////////////////////////////////////////////////////////////////// Envelope

        channels[channel].waveforms[op].envelopePos += 0.001f * loadedInstruments[channels[channel].instrument].envelopeScale;

        while (channels[channel].waveforms[op].envelopePos >= 1.0f)
        {
            channels[channel].waveforms[op].currentEnvelopeAmp = channels[channel].waveforms[op].nextEnvelopeAmp;
            channels[channel].waveforms[op].currentEnvelopeIndex++;

            if (channels[channel].waveforms[op].currentEnvelopeIndex < 32)
            {
                channels[channel].waveforms[op].nextEnvelopeAmp = float(loadedInstruments[channels[channel].instrument].waveforms[op].envelope[channels[channel].waveforms[op].currentEnvelopeIndex]) / 255.0f;
            }

            channels[channel].waveforms[op].envelopePos -= 1.0f;
        }


        float envInterp = channels[channel].waveforms[op].envelopePos;
        float envAmp = channels[channel].waveforms[op].currentEnvelopeAmp * (1.0f - envInterp) + channels[channel].waveforms[op].nextEnvelopeAmp * envInterp;


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
        

        ////////////////////////////////////////////////////////////////////////////////////////////////////////





        

        if (loadedInstruments[channels[channel].instrument].waveforms[op].pitchToNote)
            notePitch = channels[channel].pitch;
        else
            notePitch = 1.0;


        

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

        if (loadedInstruments[channels[channel].instrument].waveforms[op].pitchToNote)
            notePitch *= channels[channel].arpPitch;




        notePitch *= lfoMultiplier;

        if (notePitch > 12.0f) notePitch = 12.0f;
        if (notePitch < 0.0f) notePitch = 0.0f;


        ////////////////////////////////////////////////////////////////////////////////////////////////////////// Modulate

        float frameVol = 0.0f;

        if (mod != nullptr)
        {
            float modStrength = loadedInstruments[channels[channel].instrument].modScale * channels[channel].modMultiply;

            if (loadedInstruments[channels[channel].instrument].modulationType == 1) // FM
            {
                notePitch += mod[i] * modStrength * 8.0f;
            }
            else if (loadedInstruments[channels[channel].instrument].modulationType == 2) // AM
            {
                envAmp *= mod[i] * modStrength * 8.0f;
            }
            else if (loadedInstruments[channels[channel].instrument].modulationType == 3) // Apply PM.
            {
                float mapPos = ((mod[i] * mod[i] * mod[i] * modStrength) + 0.5f) * 4.0f;
                while (mapPos > 1.0f) mapPos--;
                while (mapPos < 0.0f) mapPos++;
                mapPos *= 182;
                channels[channel].waveforms[op].sampleReadPos = mapPos;
                notePitch = 0.0f;
            }
            else if (loadedInstruments[channels[channel].instrument].modulationType == 4) // Apply delay.
            {
                float delayIndex = channels[channel].waveforms[op].sampleReadPos + mod[i] * 100.0f * modStrength * 4.0f;

                while (delayIndex < 0) delayIndex += (183);
                while (delayIndex >= 183) delayIndex -= (183);

                ma_uint32 dIndex1 = delayIndex;
                ma_uint32 dIndex2 = delayIndex + 1;

                float t2 = delayIndex - dIndex1;  // Fractional part
                if (dIndex2 >= 183)
                    dIndex2 = 0;

                frameVol += loadedInstruments[channels[channel].instrument].waveforms[op].pcmFrames[dIndex1] * (1.0f - t2) + loadedInstruments[channels[channel].instrument].waveforms[op].pcmFrames[dIndex2] * t2;
            }
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////////// Read frame data.
        // Make sure that the frame reading position is inside the sample.
        while (channels[channel].waveforms[op].sampleReadPos >= 183)
            channels[channel].waveforms[op].sampleReadPos -= 183;
        while (channels[channel].waveforms[op].sampleReadPos < 0)
            channels[channel].waveforms[op].sampleReadPos += 183;


        float readIndex = int(channels[channel].waveforms[op].sampleReadPos);
        int index1 = int(readIndex);
        int index2 = int(readIndex) + 1;
        

        float t = channels[channel].waveforms[op].sampleReadPos - index1;  // Fractional part


        if (index2 >= 183)
            index2 = 0;

        frameVol += loadedInstruments[channels[channel].instrument].waveforms[op].pcmFrames[index1] * (1.0f - t) + loadedInstruments[channels[channel].instrument].waveforms[op].pcmFrames[index2] * t;

        

        channels[channel].waveforms[op].sampleReadPos += notePitch;

        // Add noise.
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

        
        // Modulator Slide
        if (channels[channel].modSlide != 0.0f)
        {
            channels[channel].modMultiply += channels[channel].modSlide * 0.00001f * 120.0f;

            if (channels[channel].modMultiply > 1.0f)
                channels[channel].modMultiply = 1.0f;
            else if (channels[channel].modMultiply < 0.0f)
                channels[channel].modMultiply = 0.0f;
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



        if (channels[channel].oscilloscope.readPos >= 183.0f)
            channels[channel].oscilloscope.readPos -= 183.0f;


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
    float oldMod = channels[channel].modMultiply;
    float oldArp = channels[channel].arpTimer;

    readModulator(mod2, frameCount, channel, 1, NULL); // Read the modulator

    channels[channel].pitch = oldPitch;
    channels[channel].modMultiply = oldMod;
    channels[channel].arpTimer = oldArp;

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
            /*
            if (loadedInstruments[channels[channel].instrument].arpLength >= 0) // Arpeggiate note.
            {
                channels[channel].arpTimer += frameCount;

                float arpAmount = 48000.0f * (120.0f / loadedSong.bpm) * ((loadedInstruments[channels[channel].instrument].arpSpeed + 0.0625f)) / 8.0f;


                if (channels[channel].arpTimer > arpAmount)
                {
                    channels[channel].arpTimer += arpAmount;
                    //channels[channel].arpIndex++;
                    if (channels[channel].arpTimer > loadedInstruments[channels[channel].instrument].arpLength)
                        channels[channel].arpTimer = 0.0f;
                    //if (channels[channel].arpIndex > loadedInstruments[channels[channel].instrument].arpLength)
                    //    channels[channel].arpIndex = -1;
                }
            }*/


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






    // If changing instruments, reset frame reading to make sure that phase is consistant.
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
        channels[channel].waveforms[wave].currentEnvelopeIndex = 0;
        channels[channel].waveforms[wave].envelopePos = 0.0f;

        
        channels[channel].waveforms[wave].currentEnvelopeAmp = float(loadedInstruments[sampleNumber].waveforms[wave].envelope[0]) / 255.0f;
        channels[channel].waveforms[wave].nextEnvelopeAmp = float(loadedInstruments[sampleNumber].waveforms[wave].envelope[0]) / 255.0f;

        

        // Restart release timers.
        channels[channel].waveforms[wave].releaseTimer = 0.0f;
    }

    
    

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
            else if (effectType == 5) // Increase modulator.
                channels[ch].modSlide = float(effectVal) / 1000.0f;
            else if (effectType == 6) // Decrease modulator.
                channels[ch].modSlide = float(effectVal) / -1000.0f;
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
                for (int fr = 0; fr < 183; fr++)
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


    DrawEnvelopeDisplay();

    


    for (int x = 0; x < 264; x++)
    {
        float brightness = 1.0f;


        int subdivision = int((float(x) / 264.0f) * 16.0f);
        bool lighten = subdivision % 2 == 0;

        for (int y = 40; y < 80; y++)
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




    int otherOp = 0;
    if (sampleDisplay.selectedOperator == 0)
        otherOp = 1;
    
    // Draw the unselected operator.
    float lastFrameVal = int(loadedInstruments[editor.selectedInstrument].waveforms[otherOp].pcmFrames[0] * 20.0f) + 60.0f;


    for (int x = 0; x < 264; x++)
    {
        int frameIndex = (float(x) / 264.0f) * (183.0f);


        if (frameIndex >= 183)
            break;

        int frameVal = int(loadedInstruments[editor.selectedInstrument].waveforms[otherOp].pcmFrames[frameIndex] * 20.0f) + 60.0f;



        if (frameVal > 79)
            frameVal = 79;
        else if (frameVal < 40)
            frameVal = 40;



        float brightness = 127.0f;



        if (lastFrameVal < frameVal)
        {
            for (int y = lastFrameVal; y <= frameVal; y++)
            {
                sampleDisplay.pixelData[x + 264 * y].r = gui.uiColors[6] * 255.0f;
                sampleDisplay.pixelData[x + 264 * y].g = gui.uiColors[7] * 255.0f;
                sampleDisplay.pixelData[x + 264 * y].b = gui.uiColors[8] * 255.0f;
            }
        }
        else
        {
            for (int y = frameVal; y <= lastFrameVal; y++)
            {
                sampleDisplay.pixelData[x + 264 * y].r = gui.uiColors[6] * 255.0f;
                sampleDisplay.pixelData[x + 264 * y].g = gui.uiColors[7] * 255.0f;
                sampleDisplay.pixelData[x + 264 * y].b = gui.uiColors[8] * 255.0f;
            }
        }



        lastFrameVal = frameVal;
    }


    

    // Draw the selected operator.

    lastFrameVal = int(loadedInstruments[editor.selectedInstrument].waveforms[sampleDisplay.selectedOperator].pcmFrames[0] * 20.0f) + 60.0f;

    for (int x = 0; x < 528; x++)
    {
        int frameIndex = (float(x) / 264.0f) * (183.0f);


        if (frameIndex >= 183)
            break;

        int frameVal = int(loadedInstruments[editor.selectedInstrument].waveforms[sampleDisplay.selectedOperator].pcmFrames[frameIndex] * 20.0f) + 60.0f;

        if (frameVal > 79)
            frameVal = 79;
        else if (frameVal < 40)
            frameVal = 40;



        float brightness = 127.0f;


        if (lastFrameVal < frameVal)
        {
            for (int y = lastFrameVal; y <= frameVal; y++)
            {
                sampleDisplay.pixelData[x + 264 * y].r = gui.uiColors[48] * 255.0f;
                sampleDisplay.pixelData[x + 264 * y].g = gui.uiColors[49] * 255.0f;
                sampleDisplay.pixelData[x + 264 * y].b = gui.uiColors[50] * 255.0f;
            }
        }
        else
        {
            for (int y = frameVal; y <= lastFrameVal; y++)
            {
                sampleDisplay.pixelData[x + 264 * y].r = gui.uiColors[48] * 255.0f;
                sampleDisplay.pixelData[x + 264 * y].g = gui.uiColors[49] * 255.0f;
                sampleDisplay.pixelData[x + 264 * y].b = gui.uiColors[50] * 255.0f;
            }
        }




        lastFrameVal = frameVal;


    }

    

    return;
}



void DrawEnvelopeDisplay()
{

    int otherOp = 0;
    if (sampleDisplay.selectedOperator == 0)
        otherOp = 1;

    
    int lastEnvAmp1 = int((float(loadedInstruments[editor.selectedInstrument].waveforms[sampleDisplay.selectedOperator].envelope[0]) / 255.0) * 40.0);
    int lastEnvAmp2 = int((float(loadedInstruments[editor.selectedInstrument].waveforms[otherOp].envelope[0]) / 255.0) * 40.0);

    for (int x = 0; x < 264; x++)
    {
        float brightness = 1.0f;


        int index = int((float(x) / 264.0) * 32.0);
        int nextIndex = index + 1;
        if (nextIndex >= 32)
            nextIndex = 31;
        bool lighten = index % 2 == 0;

        float interp = ((float(x) / 264.0) * 32.0) - index;

        /////////////////////////////////////////////////////////////

        int envVal = int((float(loadedInstruments[editor.selectedInstrument].waveforms[otherOp].envelope[index]) / 255.0) * 40.0);
        int envValNext = int((float(loadedInstruments[editor.selectedInstrument].waveforms[otherOp].envelope[nextIndex]) / 255.0) * 40.0);

        int envAmp = envVal * (1.0f - interp) + envValNext * interp;


        for (int y = 0; y < 40; y++)
        {
            if ((y >= envAmp - 1 && y <= lastEnvAmp2) || (y <= envAmp && y >= lastEnvAmp2 - 1))
            {
                sampleDisplay.pixelData[x + 264 * y].r = gui.uiColors[6] * 255.0f;
                sampleDisplay.pixelData[x + 264 * y].g = gui.uiColors[7] * 255.0f;
                sampleDisplay.pixelData[x + 264 * y].b = gui.uiColors[8] * 255.0f;
            }
            else if (lighten)
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

        lastEnvAmp2 = envAmp;


        ///////////////////////////////////////////////

        envVal = int((float(loadedInstruments[editor.selectedInstrument].waveforms[sampleDisplay.selectedOperator].envelope[index]) / 255.0) * 40.0);
        envValNext = int((float(loadedInstruments[editor.selectedInstrument].waveforms[sampleDisplay.selectedOperator].envelope[nextIndex]) / 255.0) * 40.0);

        envAmp = envVal * (1.0f - interp) + envValNext * interp;


        for (int y = 0; y < 40; y++)
        {
            if ((y >= envAmp - 1 && y <= lastEnvAmp1) || (y <= envAmp && y >= lastEnvAmp1 - 1))
            {
                sampleDisplay.pixelData[x + 264 * y].r = gui.uiColors[48] * 255.0f;
                sampleDisplay.pixelData[x + 264 * y].g = gui.uiColors[49] * 255.0f;
                sampleDisplay.pixelData[x + 264 * y].b = gui.uiColors[50] * 255.0f;
            }
        }

        lastEnvAmp1 = envAmp;
    }

    return;

}



void DrawOscilloscope(int channel)
{
    float pos = 0.0f;

    float step = 183.0f / 48.0f;


    

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

        if (pos >= 183.0f)
            pos -= 183.0f;

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



void GenerateAdditiveWave(Instrument* instrument, int op)
{
    
    if (!instrument->enabled) // Create a new sample.
    {
        instrument->enabled = true;
    }


    if (instrument->waveforms[op].waveType == 4) // Generate noise
    {

        float scale = instrument->waveforms[op].numOfSineWaves;


        // Create base noise frequency.
        srand(instrument->waveforms[op].noiseSeed);

        float noiseVol1 = float((rand() % 256) - 127) / 128.0f;
        float noiseVol2 = float((rand() % 256) - 127) / 128.0f;
        float index = 0.0f;


        float newFrames[183];


        for (int x = 0; x < 183; x++)
        {

            float toAdd = (1.0f / 183.0f) * (scale * 4.0f);

            if (int(index + toAdd) != int(index))
            {
                noiseVol1 = noiseVol2;
                noiseVol2 = float((rand() % 256) - 127) / 128.0f;

                // Move back to the first frame if at the end of the sample.

                if (x + (1.0f / toAdd) * 2.0f >= 183)
                {
                    noiseVol2 = newFrames[0];
                }
            }

            index += toAdd;
            float interp = index - int(index);

            float interpVol = noiseVol1 * (1.0f - interp) + noiseVol2 * interp;


            newFrames[x] = interpVol;
        }



        // Copy it to create overtones.
        for (int fr = 0; fr < 183; fr++)
        {
            instrument->waveforms[op].pcmFrames[fr] = 0.0f;

            for (int wave = 0; wave < 8; wave++)
            {
                if (instrument->waveforms[op].frequencies[wave] > 0)
                {
                    int interpFrame = fr * (wave + 1);
                    while (interpFrame >= 183)
                        interpFrame -= 183;

                    instrument->waveforms[op].pcmFrames[fr] += newFrames[interpFrame] * instrument->waveforms[op].frequencies[wave] * 0.07 * 0.25f;
                }
            }
        }



    }
    else
    {


        for (int fr = 0; fr < 183; fr++)
            instrument->waveforms[op].pcmFrames[fr] = 0.0f;

        for (int freq = 0; freq < 8; freq++)
        {
            float waveSize = ((1.0f / (freq + 1.0f)) * 48000.0f) / 261.625f;
            waveSize *= 2.0f;


            if (instrument->waveforms[op].frequencies[freq] != 0)
                ConstructWave(instrument, op, instrument->waveforms[op].waveType, instrument->waveforms[op].frequencies, 183, waveSize, freq, instrument->waveforms[op].pcmFrames);
        }
    }

    for (int x = 0; x < 183; x++)
    {
        instrument->waveforms[op].pcmFrames[x] += (instrument->waveforms[op].offset - 0.5f) * 2.0f;
        if (instrument->waveforms[op].pcmFrames[x] > 0.85f) instrument->waveforms[op].pcmFrames[x] = 0.85f;
        else if (instrument->waveforms[op].pcmFrames[x] < -0.85f) instrument->waveforms[op].pcmFrames[x] = -0.85f;
    }


    
    

    
    







    int len = 183;

    

    // Duty cycle
    float duty = instrument->waveforms[op].dutyCycle; // Length of left size.

    if (duty != 0.5f)
    {
        // Stretch left side.
        std::vector <float> newFr;

        for (int fr = 0; fr < len * duty; fr++)
        {
            int pos = (fr / duty) / 2.0f;
            float posF = (float(fr) / duty) / 2.0f;
            float interp = posF - pos;

            float amp;
            if (fr == len - 1)
                amp = instrument->waveforms[op].pcmFrames[pos] * (1.0f - interp) + instrument->waveforms[op].pcmFrames[0] * interp;
            else
                amp = instrument->waveforms[op].pcmFrames[pos] * (1.0f - interp) + instrument->waveforms[op].pcmFrames[pos + 1] * interp;

            newFr.emplace_back(amp);
        }

        // Stretch right side.
        for (int fr = 0; fr < len * (1.0f - duty); fr++)
        {
            int pos = (len / 2.0f) + (fr / (1.0f - duty)) / 2.0f;
            float posF = (float(len) / 2.0f) + (float(fr) / (1.0f - duty)) / 2.0f;
            float interp = posF - pos;

            float amp;
            if (pos == len - 1)
                amp = instrument->waveforms[op].pcmFrames[pos] * (1.0f - interp) + instrument->waveforms[op].pcmFrames[0] * interp;
            else
                amp = instrument->waveforms[op].pcmFrames[pos] * (1.0f - interp) + instrument->waveforms[op].pcmFrames[pos + 1] * interp;

            newFr.emplace_back(amp);
        }



        for (int fr = 0; fr < len; fr++)
        {
            instrument->waveforms[op].pcmFrames[fr] = newFr[fr];
        }
    }
    

    // Apply mirror.
    if (instrument->waveforms[op].mirror)
    {
        for (int fr = 0; fr < 91; fr++)
        {
            instrument->waveforms[op].pcmFrames[fr] = instrument->waveforms[op].pcmFrames[fr * 2];
        }
        instrument->waveforms[op].pcmFrames[91] = instrument->waveforms[op].pcmFrames[90];
        for (int fr = 0; fr < 91; fr++)
        {
            instrument->waveforms[op].pcmFrames[182 - fr] = instrument->waveforms[op].pcmFrames[fr];
        }
    }
    



    return;
}



void ConstructWave(Instrument* instrument, int op, int waveType, int frequencies[16], float framesToWrite, float periodLength, int frequency, float* inputWave)
{
    float periodLen = periodLength;


    


    for (int x = 0; x < framesToWrite; x++)
    {
        if (waveType == -1) // Empty wave
        {
            inputWave[x] += 1.0f;
        }
        else if (waveType == 0) // Sine wave
        {
            float vol = 0;

            float waveLen = float(periodLen * 0.5f);
            float periodPos = float(x);
            while (periodPos > waveLen)
                periodPos -= waveLen;
            periodPos /= waveLen;

            vol += sin(float(x) * 2.0f * 6.28312 / periodLen) * frequencies[frequency] * 0.07f * 0.25f;


            inputWave[x] += vol;
        }
        else if (waveType == 1) // Square wave
        {
            float vol = 0;

            float duty = 0.5f;

            if (instrument->waveforms[op].generateFromSines)
            {
                float waveLen = float(periodLen * 0.5f);
                float periodPos = float(x);
                while (periodPos > waveLen)
                    periodPos -= waveLen;
                periodPos /= waveLen;

                bool addSign = true;
                for (int w = 1; w < instrument->waveforms[op].numOfSineWaves * 2; w += 2)
                {
                    vol += (sin(periodPos * float(w) * 6.283f) * frequencies[frequency] * 0.07f * 0.25f) / float(w);
                }
            }
            else
            {
                float round = instrument->waveforms[op].smoothness;

                float waveLen = float(periodLen * 0.5f);

                float periodPos = float(x);

                while (periodPos > waveLen)
                    periodPos -= waveLen;

                periodPos /= waveLen;


                float volume = 1.0f * frequencies[frequency] * 0.07f * 0.25f;


                float c1 = round * (duty * 0.5f);
                float c2 = duty - (round * (duty * 0.5f));
                float c3 = duty;
                float c4 = (round * ((1.0f - duty) * 0.5f)) + duty;
                float c5 = 1.0f - (round * ((1.0f - duty) * 0.5f));

                // Rounded corners
                if (periodPos < c1) // corner
                {
                    float x1 = periodPos - c1;
                    float r1 = c1;
                    float yVal = sqrt((r1 * r1) - (x1 * x1));
                    yVal *= 1.0f * (round * volume) / r1;
                    yVal += volume - (round * volume);
                    vol += yVal;
                }
                else if (periodPos <= c2) // flat
                {
                    vol += volume;
                }
                else if (periodPos < c3) // corner
                {
                    float x1 = periodPos - c2;
                    float r1 = c1;
                    float yVal = sqrt((r1 * r1) - (x1 * x1));
                    yVal *= 1.0f * (round * volume) / r1;
                    yVal += volume - (round * volume);
                    vol += yVal;
                }
                else if (periodPos < c4) // corner
                {
                    float x1 = periodPos - c4;
                    float r1 = c4 - duty;
                    float yVal = sqrt((r1 * r1) - (x1 * x1));
                    yVal *= 1.0f * (round * volume) / r1;
                    yVal += volume - (round * volume);
                    vol -= yVal;
                }
                else if (periodPos <= c5) // flat
                {
                    vol -= volume;
                }
                else // corner
                {
                    float x1 = periodPos - c5;
                    float r1 = 1.0f - c5;
                    float yVal = sqrt((r1 * r1) - (x1 * x1));
                    yVal *= 1.0f * (round * volume) / r1;
                    yVal += volume - (round * volume);
                    vol -= yVal;
                }
            }


            inputWave[x] += vol;
        }
        else if (waveType == 2) // Triangle wave
        {
            float vol = 0;

            float waveLen = float(periodLen * 0.5f);
            float periodPos = float(x);
            while (periodPos > waveLen)
                periodPos -= waveLen;
            periodPos /= waveLen;

            if (periodPos > 0.5)
                vol -= float(periodPos - 0.5f - 0.25f) * frequencies[frequency] * 0.07f;
            else
                vol -= (0.25f - float(periodPos)) * frequencies[frequency] * 0.07f;

            inputWave[x] += vol;
        }
        else if (waveType == 3) // Saw wave
        {
            float vol = 0;

            
            if (instrument->waveforms[op].generateFromSines)
            {
                float waveLen = float(periodLen * 0.5f);
                float periodPos = float(x);
                while (periodPos > waveLen)
                    periodPos -= waveLen;
                periodPos /= waveLen;


                bool addSign = true;
                for (int w = 1; w < instrument->waveforms[op].numOfSineWaves + 1; w++)
                {
                    if (addSign)
                        vol += (sin(periodPos * float(w) * 6.283f) * frequencies[frequency] * 0.05f * 0.25f) / float(w);
                    else
                        vol -= (sin(periodPos * float(w) * 6.283f) * frequencies[frequency] * 0.05f * 0.25f) / float(w);
                    addSign = !addSign;
                }
            }
            else
            {
                float round = instrument->waveforms[op].smoothness * 0.5f;

                float waveLen = float(periodLen * 0.5f);
                float periodPos = float(x);
                while (periodPos > waveLen)
                    periodPos -= waveLen;
                periodPos /= waveLen;

                float amplitude = frequencies[frequency] * 0.15f * 0.25f;

                // Approximate y in a Bezier curve.
                if (periodPos < round)
                {
                    float t = 0.5f;
                    float interval = 0.5f;
                    for (int j = 0; j < 32; j++)
                    {
                        float estimate = (t * t) * round;
                        interval *= 0.5f;

                        float posInCurve = periodPos;

                        if (estimate > posInCurve)
                            t -= interval;
                        else
                            t += interval;
                    }
                    float roundedVol = -(t * t) * (amplitude * round + amplitude * 0.5f) + t * amplitude + amplitude * 0.5f;


                    vol -= (roundedVol)-amplitude * 0.5f;
                }
                else if (periodPos > 1.0f - round)
                {
                    float t = 0.5f;
                    float interval = 0.5f;
                    for (int j = 0; j < 32; j++)
                    {
                        float estimate = 1.0f - round + 2 * round * t - (t * t) * round;
                        interval *= 0.5f;

                        float posInCurve = periodPos;

                        if (estimate > posInCurve)
                            t -= interval;
                        else
                            t += interval;
                    }
                    float roundedVol = -(t * t) * (amplitude * round + amplitude * 0.5f) + 2 * t * amplitude * round + amplitude - (amplitude * round);

                    vol += (roundedVol)-amplitude * 0.5f;
                }
                else
                {
                    vol += (periodPos * amplitude) - amplitude * 0.5f;
                }
            }

            inputWave[x] += vol;
        }
        else if (waveType == 5) // Wave A
        {
            float vol = 0;

            
            float waveLen = float(periodLen * 1.0f);
            float periodPos = float(x);
            while (periodPos > waveLen)
                periodPos -= waveLen;
            periodPos /= waveLen;

            
            bool addSign = true;
            for (int w = 1; w < instrument->waveforms[op].numOfSineWaves * 4; w += 4)
            {
                if (addSign)
                    vol += (sin(periodPos * float(w) * 6.283f) * frequencies[frequency] * 0.07f * 0.25f) / float(w);
                else
                    vol -= (sin(periodPos * float(w) * 6.283f) * frequencies[frequency] * 0.07f * 0.25f) / float(w);
                addSign = !addSign;
            }

            inputWave[x] += vol;
        }
        else if (waveType == 6) // Wave B
        {
            float vol = 0;

            float waveLen = float(periodLen * 0.5f);
            float periodPos = float(x);
            while (periodPos > waveLen)
                periodPos -= waveLen;
            periodPos /= waveLen;

            


            vol = sin(float(x) * 2.0f * 4.0f * 6.28312 / periodLen) * frequencies[frequency] * 0.02f * 0.25f;

            if (periodPos < 0.25f)
            {
                float t = periodPos / 0.25f;
                vol = -(frequencies[frequency] * 0.08f * 0.25f) * (1.0f - t);
            }
            else if (periodPos >= 0.5f && periodPos < 0.75f)
            {
                float t = (periodPos - 0.5f) / 0.25f;
                vol = -(frequencies[frequency] * 0.08f * 0.25f) * (1.0f - t);
            }



            if (periodPos > 0.5f)
                vol *= -1.0f;


            inputWave[x] += vol;

        }
    }

    





    return;
}



void GenerateAllInstrumentWaves(Instrument *instrument)
{
    for (int wave = 0; wave < 4; wave++)
    {
        GenerateAdditiveWave(instrument, wave);
    }
    sampleDisplay.selectedOperator = 0;

    return;
}

