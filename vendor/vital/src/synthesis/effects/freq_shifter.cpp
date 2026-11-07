// VialFX - Frequency Shifter effect. Licensed under GPL-3.0-or-later.
#include "freq_shifter.h"

#include "synth_constants.h"

#include <algorithm>
#include <cmath>

namespace vital {

  FreqShifter::Hilbert::Hilbert() {
    static const float a[4] = { 0.6923878f, 0.9360654322959f, 0.9882295226860f, 0.9987488452737f };
    static const float b[4] = { 0.4021921162426f, 0.8561710882420f, 0.9722909545651f, 0.9952884791278f };
    for (int i = 0; i < 4; ++i) {
      slow[i].a2 = a[i] * a[i];
      fast[i].a2 = b[i] * b[i];
    }
  }

  void FreqShifter::Hilbert::tick(float x, float& real, float& imag) {
    float u = delayed;
    delayed = x;
    for (auto& stage : slow)
      u = stage.tick(u);
    float v = x;
    for (auto& stage : fast)
      v = stage.tick(v);
    real = v;
    imag = u;
  }

  void FreqShifter::Hilbert::reset() {
    delayed = 0.0f;
    for (auto& s : slow) { s.x1 = s.x2 = s.y1 = s.y2 = 0.0f; }
    for (auto& s : fast) { s.x1 = s.x2 = s.y1 = s.y2 = 0.0f; }
  }

  FreqShifter::FreqShifter() : Processor(kNumInputs, kNumOutputs), wet_(0.5f), feedback_(0.0f), stereo_(0.0f),
                               shift_(0.0f), fb_dir_(0.0f), range_hz_(100.0f) {
    hardReset();
  }

  void FreqShifter::setSampleRate(int sample_rate) {
    Processor::setSampleRate(sample_rate);
  }

  void FreqShifter::hardReset() {
    for (int c = 0; c < 2; ++c) {
      main_[c].reset();
      feedback_shifter_[c].reset();
      for (auto& s : smear_[c]) s = Allpass1();
      feedback_state_[c] = 0.0f;
      phase_[c] = fb_phase_[c] = 0.0;
      shift_hz_[c] = fb_hz_[c] = 0.0f;
    }
  }

  float FreqShifter::rangeHz() const {
    const int unit = static_cast<int>(input(kUnit)->at(0)[0] + 0.5f);
    float hz = 100.0f;
    switch (unit) {
      case 0:
        hz = input(kFrequency)->at(0)[0];   // exponential controls already deliver Hz
        break;
      case 1:
        hz = 1.0f / std::max(input(kPeriod)->at(0)[0], 1.0e-5f);   // seconds
        break;
      case 2: case 3: case 4: {
        float index = std::min(std::max(input(kTempo)->at(0)[0], 0.0f),
                               static_cast<float>(constants::kNumSyncedFrequencyRatios - 1));
        hz = constants::kSyncedFrequencyRatios[static_cast<int>(index + 0.3f)] * input(kBeatsPerSecond)->at(0)[0];
        if (unit == 3) hz *= 2.0f / 3.0f;
        if (unit == 4) hz *= 3.0f / 2.0f;
        break;
      }
      default: {
        float note = input(kMidi)->at(0)[0] + input(kKeytrackTranspose)->at(0)[0];
        hz = 440.0f * std::pow(2.0f, (note - 69.0f) / 12.0f);
        break;
      }
    }
    return std::min(std::max(hz, 0.0f), 0.45f * getSampleRate());
  }

  void FreqShifter::process(int num_samples) {
    processWithInput(input(kAudio)->source->buffer, num_samples);
  }

  void FreqShifter::processWithInput(const poly_float* audio_in, int num_samples) {
    poly_float* audio_out = output(kAudioOut)->buffer;
    const float sample_rate = static_cast<float>(getSampleRate());
    const float nyquist_guard = 0.45f * sample_rate;
    const float two_pi = 6.28318530717959f;

    const float target_range = rangeHz();
    const float target_shift = std::min(std::max(input(kShift)->at(0)[0], -1.0f), 1.0f);
    const float target_stereo = std::min(std::max(input(kStereo)->at(0)[0], -1.0f), 1.0f);
    const float target_fb_dir = std::min(std::max(input(kFbDirection)->at(0)[0], -1.0f), 1.0f);
    const float target_feedback = std::min(std::max(input(kFeedback)->at(0)[0], -1.0f), 1.0f) * 0.95f;
    const float target_wet = std::min(std::max(input(kDryWet)->at(0)[0], 0.0f), 1.0f);
    const bool use_allpass = input(kAllpass)->at(0)[0] >= 0.5f;
    const float smooth = 1.0f - std::exp(-1.0f / (0.02f * sample_rate));

    for (int i = 0; i < num_samples; ++i) {
      range_hz_ += (target_range - range_hz_) * smooth;
      shift_ += (target_shift - shift_) * smooth;
      stereo_ += (target_stereo - stereo_) * smooth;
      fb_dir_ += (target_fb_dir - fb_dir_) * smooth;
      feedback_ += (target_feedback - feedback_) * smooth;
      wet_ += (target_wet - wet_) * smooth;

      const float base = shift_ * range_hz_, offset = stereo_ * range_hz_ * 0.25f;
      const float hz[2] = { std::min(std::max(base + offset, -nyquist_guard), nyquist_guard),
                            std::min(std::max(base - offset, -nyquist_guard), nyquist_guard) };
      const float fb_hz = std::min(std::max(fb_dir_ * range_hz_, -nyquist_guard), nyquist_guard);

      float result[2];
      for (int c = 0; c < 2; ++c) {
        const float dry = audio_in[i][c];
        float re, im;
        main_[c].tick(dry + feedback_state_[c], re, im);
        phase_[c] += two_pi * hz[c] / sample_rate;
        if (phase_[c] > two_pi) phase_[c] -= two_pi;
        if (phase_[c] < -two_pi) phase_[c] += two_pi;
        const float shifted = re * static_cast<float>(std::cos(phase_[c])) - im * static_cast<float>(std::sin(phase_[c]));

        float fb = std::tanh(shifted);
        if (use_allpass)
          for (auto& stage : smear_[c])
            fb = stage.tick(fb, 0.7f);
        if (std::fabs(fb_dir_) > 1.0e-3f) {
          float fre, fim;
          feedback_shifter_[c].tick(fb, fre, fim);
          fb_phase_[c] += two_pi * fb_hz / sample_rate;
          if (fb_phase_[c] > two_pi) fb_phase_[c] -= two_pi;
          if (fb_phase_[c] < -two_pi) fb_phase_[c] += two_pi;
          fb = fre * static_cast<float>(std::cos(fb_phase_[c])) - fim * static_cast<float>(std::sin(fb_phase_[c]));
        }
        feedback_state_[c] = fb * feedback_;
        result[c] = dry + (shifted - dry) * wet_;
      }
      audio_out[i] = poly_float(result[0], result[1], 0.0f, 0.0f);
    }
  }
} // namespace vital
