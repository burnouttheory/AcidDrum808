// SPDX-License-Identifier: MIT
// Burnout Theory Acid Drum 808 -- simple three-band dynamics for the stereo master.
// This does not replace or alter the TR-808 voice synthesis. All DSP operations
// run in-place with no audio-thread allocations or locks.
#pragma once
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>

namespace acid808 {
class ThreeBandCompressor final {
  struct State {
    double lowLP = 0.0, highLP = 0.0;
    double env[3] = {0.0,0.0,0.0};
  };
  std::array<State,2> channels{};
  std::array<std::atomic<double>,3> intensity{};
public:
  ThreeBandCompressor() {for (auto& x : intensity) x.store(0.0, std::memory_order_relaxed);}
  void Set(int band, double amount) noexcept {
    if(band>=0 && band<3 && std::isfinite(amount))
      intensity[band].store(std::clamp(amount,0.0,100.0),std::memory_order_relaxed);
  }
  double Get(int band) const noexcept {
    return (band>=0 && band<3)?intensity[band].load(std::memory_order_relaxed):0.;
  }
  void Reset() noexcept {channels={};}
  template <typename Sample>
  void Process(Sample* samples, int frames, double sampleRate, int ch) noexcept {
    if(!samples || frames<=0 || ch<0 || ch>1 || sampleRate<=0.)return;
    const double amount[3]={Get(0)*0.01,Get(1)*0.01,Get(2)*0.01};
    if(amount[0]<=0. && amount[1]<=0. && amount[2]<=0.)return;
    constexpr double pi=3.14159265358979323846;
    const double aLow=std::exp(-2.*pi*160./sampleRate);
    const double aMid=std::exp(-2.*pi*2500./sampleRate);
    const double attack=std::exp(-1./(0.008*sampleRate));
    const double release=std::exp(-1./(0.140*sampleRate));
    State& s=channels[ch];
    for(int i=0;i<frames;i++) {
      const double x=static_cast<double>(samples[i]);
      s.lowLP=(1.-aLow)*x+aLow*s.lowLP;
      const double rest=x-s.lowLP;
      s.highLP=(1.-aMid)*rest+aMid*s.highLP;
      const double bands[3]={s.lowLP,s.highLP,rest-s.highLP};
      double y=0.;
      for(int b=0;b<3;b++) {
        const double v=bands[b];
        const double level=std::abs(v);
        s.env[b]=(level>s.env[b]?attack:release)*s.env[b]+(1.-(level>s.env[b]?attack:release))*level;
        double gain=1.;
        if(amount[b]>0.) {
          const double thresholdDb=-12.0-20.0*amount[b];
          const double ratio=1.0+7.0*amount[b];
          const double envDb=20.0*std::log10(std::max(1e-8,s.env[b]));
          if(envDb>thresholdDb) {
            const double gainDb=(thresholdDb+(envDb-thresholdDb)/ratio)-envDb;
            gain=std::pow(10.0,gainDb/20.0);
          }
        }
        y+=v*gain;
      }
      samples[i]=static_cast<Sample>(y);
    }
  }
};
} // namespace acid808
