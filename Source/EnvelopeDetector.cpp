#include "EnvelopeDetector.h"

EnvelopeDetectorOutput EnvelopeDetector::processSample(float x)
{
    bool onAttack = false;
    x = std::fabs(x);
    if (dMode == DetectionMode::MS || dMode == DetectionMode::RMS)
        x *= x;
    
    float currentEnvelope = 0.0f;
    
    if (x > lastEnvelope)
    {
        currentEnvelope = attackTime * (lastEnvelope - x) + x;
        onAttack = lastState == State::REL;
        lastState = State::ATK;
    }
    else
    {
        currentEnvelope = releaseTime * (lastEnvelope - x) + x;
        lastState = State::REL;
    }
    
    if (clampToOne)
        currentEnvelope = jmin(currentEnvelope, 1.0f);
    
    currentEnvelope = jmax(currentEnvelope, 0.0f);
    
    lastEnvelope = currentEnvelope;
    
    if (dMode == DetectionMode::RMS)
        currentEnvelope = std::sqrt(currentEnvelope);
    
    EnvelopeDetectorOutput output(currentEnvelope, onAttack);
    return output;
}
