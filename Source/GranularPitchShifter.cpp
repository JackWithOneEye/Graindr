#include "GranularPitchShifter.h"

void GranularPitchShifter::processBlock(float *buffer, const int numSamples)
{
    for (int n = 0; n < numSamples; ++n)
    {
        buffer[n] = processSample(buffer[n]);
    }
}

float GranularPitchShifter::processOutputBuffers(float modulation, bool triggerStrech)
{
    auto pFactor = pitchShiftFactor.getNextValue();
    auto ftFactor = fineTuneFactor.getNextValue();
    auto totalPitchFactor = pFactor * ftFactor;
        
    currentOutputSize = (int) (grainSize_smpls.getNextValue());
    currentInputSize = static_cast<float>(currentOutputSize) * totalPitchFactor;
    
    auto txt = jlimit(0.0f, 1.0f, texture.getNextValue() + modulation);
    int overlapWidth = (int) (currentOutputSize * txt * 0.5f);
    int stride = currentOutputSize - overlapWidth - 1;
    
    if (triggerStrech)
    {
        output1TriggerStretchOnNext = true;
        output2TriggerStretchOnNext = true;
    }
    
    if (writtenSmplsCtr > stride) {
        writtenSmplsCtr = 0;
        
        int multiplierIncr = 0;
        if (++stretchCtr >= stretch)
        {
            stretchCtr = 0;
        }
        else
        {
            multiplierIncr = 1;
        }
        stretch = nextStretch;
        
        resample(currentInputSize, currentOutputSize, totalPitchFactor);
        if (output1ReadIdx >= currentOutputSize)
        {
            output1ReadIdx = 0;
            output1ReadOffset = 0;
            output2ReadOffset = currentOutputSize;
            auto writeIdx = outputBuffer1.getWriteIndex();
            if (output1TriggerStretchOnNext || (writeIdx > 0 && writeIdx < currentOutputSize))
            {
                output1StretchMultiplier = 1;
            }
            else
            {
                output1StretchMultiplier = output2StretchMultiplier + multiplierIncr;
            }
            
            output1TriggerStretchOnNext = false;
        }
        else if (output2ReadIdx >= currentOutputSize)
        {
            output2ReadIdx = 0;
            output2ReadOffset = 0;
            output1ReadOffset = currentOutputSize;
            auto writeIdx = outputBuffer2.getWriteIndex();
            if (output2TriggerStretchOnNext || (writeIdx > 0 && writeIdx < currentOutputSize))
            {
                output2StretchMultiplier = 1;
            }
            else
            {
                output2StretchMultiplier = output1StretchMultiplier + multiplierIncr;
            }
            
            output2TriggerStretchOnNext = false;
        }
    }
    
    auto o1OutRev = 0.0f;
    auto o1OutFwd = 0.0f;
    if (output1ReadIdx < currentOutputSize)
    {
        auto win = windowFunc(output1ReadIdx, currentOutputSize, overlapWidth);
        auto grainStart = output1StretchMultiplier * currentOutputSize + output1ReadOffset;
        o1OutRev = outputBuffer1.readBuffer(grainStart - currentOutputSize + output1ReadIdx + 1) * win;
        o1OutFwd = outputBuffer1.readBuffer(grainStart - output1ReadIdx) * win;
        output1ReadIdx++;
    }
    auto o1Out = (playbackDir == PlaybackDirection::REVERSE ? o1OutRev : o1OutFwd);
    
    auto o2OutRev = 0.0f;
    auto o2OutFwd = 0.0f;
    if (output2ReadIdx < currentOutputSize)
    {
        auto win = windowFunc(output2ReadIdx, currentOutputSize, overlapWidth);
        auto grainStart = output2StretchMultiplier * currentOutputSize + output2ReadOffset;
        o2OutRev = outputBuffer2.readBuffer(grainStart - currentOutputSize + output2ReadIdx + 1) * win;
        o2OutFwd = outputBuffer2.readBuffer(grainStart - output2ReadIdx) * win;
        output2ReadIdx++;
    }
    auto o2Out = (playbackDir == PlaybackDirection::REVERSE || playbackDir == PlaybackDirection::ALTERNATE ? o2OutRev : o2OutFwd);
    
    auto sumOut = o1Out + o2Out;
    
    sumOut = fdbkLpf.processSample(sumOut);
    
    if (toneType == ToneType::ANALOG)
    {
        sumOut = softClipper(sumOut);
        sumOut = ANALOG_POST_DEL_LOOP_GAIN * psBandpass.processSample(sumOut);
    }
    sumOut = fdbkHpf.processSample(sumOut);

    return sumOut;
}

float GranularPitchShifter::processPostDelay(float x)
{
    auto delayLineOut = postDelayBuffer.readBuffer(currentInputSize);
    auto fb = feedback.getNextValue();
    
    if (toneType == ToneType::ANALOG)
    {
        delayLineOut = softClipper(delayLineOut);
        delayLineOut = ANALOG_POST_DEL_LOOP_GAIN * postDelayBandpass.processSample(delayLineOut);
    }
    delayLineOut = postDelayHpf.processSample(delayLineOut);
    
    auto y = x + fb * delayLineOut;
    postDelayBuffer.writeBuffer(y);
    
    return y;
}

float GranularPitchShifter::processSample(float x, float modulation, bool triggerStrech)
{
    auto outputBuffersOut = processOutputBuffers(modulation, triggerStrech);
    auto y = processPostDelay(outputBuffersOut);
    
    writeInputBuffer(x, outputBuffersOut);
    
    return y;
}

inline void GranularPitchShifter::resample(float inputSize, int outputSize, float factor)
{
    float phasor = 0.0f;
    int currPos = 0;
    for (int i = 0; i < outputSize; i++)
    {
        float delay = inputSize - static_cast<float>(currPos);
        float y = inputBuffer.readBuffer(delay);
        
        if (phasor != 0.0f)
        {
            float y0 = inputBuffer.readBuffer(delay + 1.0f);
            float y1 = y; // inputBuffer.readBuffer(delay)
            float y2 = inputBuffer.readBuffer(delay - 1.0f);
            float y3 = inputBuffer.readBuffer(delay - 2.0f);
            
            y = cubicInterpolation(y0, y1, y2, y3, phasor);
        }
        
        outputBuffer1.writeBuffer(y);
        outputBuffer2.writeBuffer(y);
        phasor += factor;
        while (phasor >= 1.0f)
        {
            phasor -= 1.0f;
            currPos++;
        }
    }
}

inline float GranularPitchShifter::windowFunc(int n, int outputSize, int overlapWidth)
{
    float w = 1.0f;
    if (n < overlapWidth)
    {
        w = static_cast<float>(n) / static_cast<float>(overlapWidth);
    }
    else if (n >= outputSize - overlapWidth)
    {
        w = static_cast<float>(outputSize - n - 1) / static_cast<float>(overlapWidth);
    }
    return w;
}

void GranularPitchShifter::writeInputBuffer(float x, float feedbackIn)
{
    inputBuffer.writeBuffer(x + shimmer.getNextValue() * feedbackIn);
    writtenSmplsCtr++;
}
