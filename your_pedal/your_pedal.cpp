#include "daisy_petal.h"
#include "daisysp.h"
#include "flanger.h"
#include "terrarium.h"

using namespace daisy;
using namespace terrarium;

// Declare a global daisy_petal for hardware access
DaisyPetal hw;

bool bypass;
Led led1, led2;

/**
 * Main Layout:
 *
    | Blend  | Depth | Freq  |
    | Drive? | Fdbck | Delay |
 *
 * Alt Layout:
 *
    | Blend     | Attack  | Thresh |
    | Sidechain | Release | Ratio  |
 *
 * Switches:
 *
    | Alt | Duplicate | tbd | tbd |
 */
float flanger_blend, flanger_depth, flanger_freq, flanger_drive, flanger_fdbck,
    flanger_delay;
float comp_blend, comp_attack, comp_thresh, comp_sidechain, comp_release,
    comp_ratio;
Parameter knob1, knob2, knob3, knob4, knob5, knob6;

Flanger flanger;

// Idea: set up a compressor but the sidechain is pretty well scooped so that
// lead lines shine through
daisysp::Compressor compressor;

float setIfChanged(float old_val, float new_val, void (*set)(float)) {
  if (fabs(new_val - old_val) > 0.01) {
    set(new_val);
    return new_val;
  } else {
    return old_val;
  }
}

void dummySet(float in) {}

/*
 * Process terrarium knobs and switches
 */
void processTerrariumControls() {
  // update switch values
  // https://electro-smith.github.io/libDaisy/classdaisy_1_1_switch.html
  if (hw.switches[Terrarium::FOOTSWITCH_1].RisingEdge()) {
    bypass = !bypass;
  }

  bool alt_mode = hw.switches[Terrarium::SWITCH_1].Pressed();
  if (alt_mode) {
    flanger_blend = setIfChanged(flanger_blend, knob1.Process(), dummySet);
    flanger_depth = setIfChanged(flanger_depth, knob2.Process(),
                                 [](float x) { flanger.SetLfoDepth(x); });
    flanger_freq = setIfChanged(flanger_freq, knob3.Process(),
                                [](float x) { flanger.SetLfoFreq(x); });
    flanger_drive = setIfChanged(flanger_drive, knob4.Process(), dummySet);
    flanger_fdbck = setIfChanged(flanger_fdbck, knob5.Process(),
                                 [](float x) { flanger.SetFeedback(x); });
    flanger_delay = setIfChanged(flanger_delay, knob6.Process(),
                                 [](float x) { flanger.SetDelay(x); });
  } else {
    comp_blend = setIfChanged(comp_blend, knob1.Process(), dummySet);
    comp_attack = setIfChanged(comp_attack, knob2.Process(),
                               [](float x) { compressor.SetAttack(x); });
    comp_thresh = setIfChanged(comp_thresh, knob3.Process(),
                               [](float x) { compressor.SetThreshold(x); });
    comp_sidechain = setIfChanged(comp_sidechain, knob4.Process(), dummySet);
    comp_release = setIfChanged(comp_release, knob5.Process(),
                                [](float x) { compressor.SetRelease(x); });
    comp_ratio = setIfChanged(comp_ratio, knob6.Process(),
                              [](float x) { compressor.SetRatio(x); });
  }

  flanger.setDuplicate(hw.switches[Terrarium::SWITCH_2].Pressed());

  // Set led2 to match the flanger lfo
  led2.Set(flanger.getLfoVal());

  led1.Set(bypass ? 0.0f : 1.0f);
}

/*
 * This runs at a fixed rate, to prepare audio samples
 */
void callback(AudioHandle::InterleavingInputBuffer in,
              AudioHandle::InterleavingOutputBuffer out, size_t size) {
  hw.ProcessAllControls();
  processTerrariumControls();
  led1.Update();
  led2.Update();

  for (size_t i = 0; i < size; i += 2) {
    // Process your signal here
    if (bypass) {
      out[i] = in[i];
    } else {
      // processed signal
      // Using knob1 as volume here
      out[i] = flanger.Process(in[i]) * knob1.Value();
    }
  }
}

int main(void) {
  hw.Init();
  float samplerate = hw.AudioSampleRate();

  led1.Init(hw.seed.GetPin(Terrarium::LED_1), false);
  led2.Init(hw.seed.GetPin(Terrarium::LED_2), false);

  knob1.Init(hw.knob[Terrarium::KNOB_1], 0.0f, 1.0f, Parameter::LINEAR);
  knob2.Init(hw.knob[Terrarium::KNOB_2], 0.0f, 1.0f, Parameter::LINEAR);
  knob3.Init(hw.knob[Terrarium::KNOB_3], 0.0f, 1.0f, Parameter::LINEAR);
  knob4.Init(hw.knob[Terrarium::KNOB_4], 0.0f, 1.0f, Parameter::LINEAR);
  knob5.Init(hw.knob[Terrarium::KNOB_5], 0.0f, 1.0f, Parameter::LINEAR);
  knob6.Init(hw.knob[Terrarium::KNOB_6], 0.0f, 1.0f, Parameter::LINEAR);

  flanger.Init(samplerate);

  bypass = true;

  hw.StartAdc();
  hw.StartAudio(callback);

  while (1) {
    // Do lower priority stuff infinitely here
    System::Delay(10);
  }
}
