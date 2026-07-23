#pragma once
#ifndef DSY_ENCODER_NO_SW_H
#define DSY_ENCODER_NO_SW_H
#include "daisy_core.h"
#include "per/gpio.h"
#include "sys/system.h"

namespace daisy
{
/** 
    @brief Generic Class for handling Quadrature Encoders \n 
    Inspired/influenced by Mutable Instruments (pichenettes) Encoder classes
    @author Stephen Hensley
    @date December 2019
    @ingroup controls
*/
class EncoderNoSwitch
{
  public:
    EncoderNoSwitch() {}
    ~EncoderNoSwitch() {}

    /** Initializes the encoder with the specified hardware pins.
     * Update rate is to be deprecated in a future release
     */
    void Init(Pin a, Pin b, float update_rate = 0.f);
    /** Called at update_rate to debounce and handle timing for the switch.
     * In order for events not to be missed, its important that the Edge/Pressed checks be made at the same rate as the debounce function is being called.
     */
    void Debounce(int encScale = 1);

    /** Returns +1 if the encoder was turned clockwise, -1 if it was turned counter-clockwise, or 0 if it was not just turned. */
    inline int32_t Increment() const { return updated_ ? inc_ : 0; }


    /** To be removed in breaking update
     * \param update_rate Does nothing
    */
    inline void SetUpdateRate(float update_rate) {}

  private:
    uint32_t last_update_;
    bool     updated_;
    GPIO     hw_a_, hw_b_;
    uint8_t  a_, b_;
    int32_t  inc_;
};
} // namespace daisy
#endif