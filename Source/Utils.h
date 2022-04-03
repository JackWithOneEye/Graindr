#pragma once

constexpr float SMOOTHED_VAL_RAMP_LEN_SEC = 0.1f;

using SmoothedVal = SmoothedValue<float, ValueSmoothingTypes::Linear>;
using SmoothedValM = SmoothedValue<float, ValueSmoothingTypes::Multiplicative>;

static float cubicInterpolation(float y0, float y1, float y2, float y3, float fraction)
{
    float fraction2 = fraction * fraction;
    float a0 = -0.5f * y0 + 1.5f * y1 - 1.5f * y2 + 0.5f * y3;
    float a1 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    float a2 = -0.5f * y0 + 0.5f * y2;
    float a3 = y1;
    return a0 * fraction * fraction2 + a1 * fraction2 + a2 * fraction + a3;
}

class CircularBuffer
{
public:
    CircularBuffer() {}
    
    void flushBuffer() { memset(&buffer[0], 0.0f, bufferLength * sizeof(float)); }
    
    void createCircularBuffer(unsigned int _bufferLength)
    {
        writeIndex = 0;
        bufferLength = nextPowerOfTwo(_bufferLength);
        wrapMask = bufferLength - 1;
        buffer.reset(new float[bufferLength]);
        flushBuffer();
    }
    
    void writeBuffer(float input)
    {
        buffer[writeIndex++] = input;
        writeIndex &= wrapMask;
    }
    
    float readBuffer(int delayInSamples)
    {
        int readIndex = writeIndex - delayInSamples;
        readIndex &= wrapMask;
        return buffer[readIndex];
    }
    
    float readBuffer(float delayInFractionalSamples)
    {
        float fraction = delayInFractionalSamples - (int) delayInFractionalSamples;
        
        float y0 = readBuffer((int) delayInFractionalSamples - 1);
        float y1 = readBuffer((int) delayInFractionalSamples);
        float y2 = readBuffer((int) delayInFractionalSamples + 1);
        float y3 = readBuffer((int) delayInFractionalSamples + 2);

        return cubicInterpolation(y0, y1, y2, y3, fraction);
    }

    int getWriteIndex() { return writeIndex; }
    
private:
    std::unique_ptr<float[]> buffer = nullptr;
    unsigned int writeIndex = 0;
    unsigned int bufferLength = 1024;
    unsigned int wrapMask = 1023;
    // bool interpolate = false;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CircularBuffer)
};

constexpr float oneOverTwoPi = 1.0f / MathConstants<float>::twoPi;

struct NormalisedPhase
{
    void reset() noexcept { phase = 0.0f; }
    
    float advance (float increment, float phaseShift = 0.0f) noexcept
    {
        jassert(increment >= 0);
        
        auto offset = phaseShift * oneOverTwoPi;

        auto last = phase;
        auto next = last + increment;

        while ((next + offset) >= 1.0f)
            next -= 1.0f;

        phase = next;
        return last + offset;
    }
    
    float phase = 0.0f;
};

using LFOWaveFunc = std::function<float(NormalisedPhase& /*phase*/, float /*increment*/, float /*phaseShift*/)>;

static inline float polyBlep(float arg, float increment, float modFactor = 1.0f)
{
    auto incr = modFactor * increment;
    if (arg < incr)
    {
        auto t = arg / (incr);
        return t + t - t * t - 1.0f;
    }
    
    if (arg > 1.0f - incr)
    {
        auto t = (arg - 1.0f) / (incr);
        return t + t + t * t + 1.0f;
    }
    
    return 0.0f;
}

class FastMathLFO
{
public:
    FastMathLFO() {}
    
    enum LFOWave
    {
        SIN,
        TRI,
        SAW,
        RMP,
        SQR,
        RSH
    };
    
    enum LFOPolarity
    {
        UNIPOLAR,
        BIPOLAR
    };
    
    LFOPolarity polarity = LFOPolarity::UNIPOLAR;
    
    void reset(float sampleRate)
    {
        depth.reset(sampleRate, 0.05f);
        rshState.reset(sampleRate, 0.001f);
        resetPhase();
    }
    
    void resetPhase()
    {
        phase.reset();
        rshCounter = 0;
        rshState = 0.0f;
        phaseIncrement.setCurrentAndTargetValue(phaseIncrement.getTargetValue());
    }
    
    float scaleValue(float lfoOut, float min, float max)
    {
        if (polarity == LFOPolarity::UNIPOLAR)
        {
            return lfoOut * (max - min) + min;
        }
        // bipolar
        return lfoOut * (max - min);
    }
    
    void setParams(float freq, float depth_pct, LFOWave _waveform, LFOPolarity _polarity)
    {
        phaseIncrement.setTargetValue(freq / fs);
        depth.setTargetValue(jlimit(0.0f, 1.0f, depth_pct * 0.01f));
        waveform = _waveform;
        waveFunc = getWaveFunc(waveform);
        polarity = _polarity;
    }
        
    float getNextSample(float phaseShift)
    {
        float bipolarSample = 0.0f;
        auto nextIncr = phaseIncrement.getNextValue();
        if (waveform == LFOWave::RSH)
        {
            int thresh = (int)(1.0f / (2.0f * nextIncr));
            if (rshCounter >= thresh)
            {
                rshState.setTargetValue(noiseGenerator.nextFloat() * 2.0f - 1.0f);
                rshCounter = 0;
            }
            else
            {
                rshCounter++;
            }
            bipolarSample = rshState.getNextValue();
            phase.advance(nextIncr);
        }
        else
        {
            bipolarSample = waveFunc(phase, nextIncr, phaseShift);
        }
        
        auto halfDepth = 0.5f * depth.getNextValue();
                
        if (polarity == LFOPolarity::UNIPOLAR)
        {
            return halfDepth * (bipolarSample + 1.0f);
        }
        
        // bipolar
        return halfDepth * bipolarSample;
    }
    
private:
    float fs = 44100.0f;
    SmoothedVal phaseIncrement = 0.0f;
    SmoothedVal depth = 0.0f;
    LFOWave waveform = LFOWave::SIN;
    LFOWaveFunc waveFunc = getWaveFunc(LFOWave::SIN);
    
    NormalisedPhase phase;
    
    int rshCounter = 0;
    Random noiseGenerator;
    SmoothedVal rshState = 0.0f;
        
    inline LFOWaveFunc getWaveFunc(LFOWave wave)
    {
        if (wave == LFOWave::SIN)
            return [](NormalisedPhase& phase, float increment, float phaseShift)
            {
                auto arg = phase.advance(increment, 0.25f + phaseShift);
                
                if (arg < 0.5f)
                    return dsp::FastMathApproximations::sin(MathConstants<float>::twoPi * (arg - 0.25f));
                
                return 0.0f -  dsp::FastMathApproximations::sin(MathConstants<float>::twoPi * (arg - 0.75f));
            };

        if (wave == LFOWave::TRI)
            return [](NormalisedPhase& phase, float increment, float phaseShift)
            {
                auto arg = phase.advance(increment, 0.25f + phaseShift);
                return 1.0f - 2.0f * std::fabs(2.0f * arg - 1.0f);
            };
        
        if (wave == LFOWave::SAW)
            return [](NormalisedPhase& phase, float increment, float phaseShift)
            {
                auto arg = phase.advance(increment, phaseShift);
                return  1.0f - 2.0f * arg + polyBlep(arg, increment);
            };

        if (wave == LFOWave::RMP)
            return [](NormalisedPhase& phase, float increment, float phaseShift)
            {
                auto arg = phase.advance(increment, phaseShift);
                return  2.0f * arg - 1.0f - polyBlep(arg, increment);
            };
        
        if (wave == LFOWave::SQR)
            return [](NormalisedPhase& phase, float increment, float phaseShift)
            {
                auto arg = phase.advance(increment, phaseShift);
                float val = arg < 0.5f ? 1.0f : -1.0f;
                val += polyBlep(arg, increment);
                auto argModulo = (arg - 0.5f) - floor(arg - 0.5f);
                val -= polyBlep(argModulo, increment);
                return val;
            };
        
        return [](NormalisedPhase, float, float) { return 0.0f; };
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FastMathLFO)
};
