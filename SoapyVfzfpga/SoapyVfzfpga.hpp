//
//  SoapyVfzfpga2.hpp
//  SoapyVfzfpga
//
//  Created by Albin Stigö on 21/05/2018.
//  Copyright © 2018 Albin Stigo. All rights reserved.
//

#ifndef SoapyVfzfpga_hpp
#define SoapyVfzfpga_hpp

#include <SoapySDR/Device.hpp>
#include <SoapySDR/Registry.hpp>

#include <cstdint>
#include <iostream>     // std::cout
#include <fstream>      // std::ifstream
#include <istream>

#include "alsa.h"

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

typedef enum stream_format_t
{
    STREAM_FORMAT_FLOAT32,
    STREAM_FORMAT_INT32,
    STREAM_FORMAT_INT16,
    STREAM_FORMAT_INT8,
} stream_format_t;

class SoapyVfzfgpa : public SoapySDR::Device
{
private:
    snd_pcm_t* d_pcm_handle;
    uint16_t d_period_size;
    stream_format_t d_stream_format;
    std::vector<int32_t> d_buff;
    bool d_agc_mode;
    double d_frequency;
    double d_sample_rate;
    
    // sysfs file handles
    std::fstream d_freq_f;
    
public:
    SoapyVfzfgpa();
    ~SoapyVfzfgpa();
    
    //Implement all applicable virtual methods from SoapySDR::Device
    
    // Identification API
    std::string getDriverKey() const;
    std::string getHardwareKey() const;
    
    // Channels API
    size_t getNumChannels(const int dir) const;
    bool getFullDuplex(const int direction, const size_t channel) const;
    
    // Stream API
    std::vector<std::string> getStreamFormats(const int direction, const size_t channel) const;
    std::string getNativeStreamFormat(const int direction, const size_t channel, double &fullScale) const;
    SoapySDR::ArgInfoList getStreamArgsInfo(const int direction, const size_t channel) const;
    
    SoapySDR::Stream* setupStream(const int direction, const std::string &format, const std::vector<size_t> &channels = std::vector<size_t>(), const SoapySDR::Kwargs &args = SoapySDR::Kwargs());
    
    void closeStream(SoapySDR::Stream *stream);
    size_t getStreamMTU(SoapySDR::Stream *stream) const;
    int activateStream(SoapySDR::Stream *stream,
                       const int flags = 0,
                       const long long timeNs = 0,
                       const size_t numElems = 0);
    int deactivateStream(SoapySDR::Stream *stream, const int flags = 0, const long long timeNs = 0);
    
    int readStream(SoapySDR::Stream *stream,
                   void * const *buffs,
                   const size_t numElems,
                   int &flags,
                   long long &timeNs,
                   const long timeoutUs = 100000);

    // Antennas
    std::vector<std::string> listAntennas(const int direction, const size_t channel) const;
    void setAntenna(const int direction, const size_t channel, const std::string &name);
    std::string getAntenna(const int direction, const size_t channel) const;
    
    // DC offset
    bool hasDCOffsetMode(const int direction, const size_t channel) const;
    
    // Gain
    std::vector<std::string> listGains(const int direction, const size_t channel) const;
    bool hasGainMode(const int direction, const size_t channel) const;
    void setGainMode(const int direction, const size_t channel, const bool automatic);
    bool getGainMode(const int direction, const size_t channel) const;
    void setGain(const int direction, const size_t channel, const double value);
    void setGain(const int direction, const size_t channel, const std::string &name, const double value);
    double getGain(const int direction, const size_t channel, const std::string &name) const;
    SoapySDR::Range getGainRange(const int direction, const size_t channel, const std::string &name) const;
    
    // Frequency
    void setFrequency(const int direction,
                      const size_t channel,
                      const std::string &name,
                      const double frequency,
                      const SoapySDR::Kwargs &args = SoapySDR::Kwargs());
    double getFrequency(const int direction, const size_t channel, const std::string &name) const;
    std::vector<std::string> listFrequencies(const int direction, const size_t channel) const;
    SoapySDR::RangeList getFrequencyRange(const int direction, const size_t channel, const std::string &name) const;
    SoapySDR::ArgInfoList getFrequencyArgsInfo(const int direction, const size_t channel) const;
    
    // Sample Rate API
    void setSampleRate(const int direction, const size_t channel, const double rate);
    double getSampleRate(const int direction, const size_t channel) const;
    std::vector<double> listSampleRates(const int direction, const size_t channel) const;

    // Bandwidth API
    void setBandwidth(const int direction, const size_t channel, const double bw);
    double getBandwidth(const int direction, const size_t channel) const;
    std::vector<double> listBandwidths(const int direction, const size_t channel) const;
    
    // Setting API?
    SoapySDR::ArgInfoList getSettingInfo(void) const;
    void writeSetting(const std::string &key, const std::string &value);
    std::string readSetting(const std::string &key) const;
};

#endif /* SoapyVfzfpga_hpp */
