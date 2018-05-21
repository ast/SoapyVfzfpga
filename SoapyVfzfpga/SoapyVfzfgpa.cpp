#include "SoapyVfzfpga.hpp"
#include <SoapySDR/Logger.hpp>
#include <iostream>

SoapyVfzfgpa::SoapyVfzfgpa()
{
    //d_pcm_handle = alsa_d_pcm_handle("vfzfpga", 1024, SND_PCM_STREAM_CAPTURE);
    // SoapySDR_setLogLevel(SOAPY_SDR_FA);
    d_agc_mode = false;
    d_period_size = 2048;
    d_pcm_handle = alsa_pcm_handle("funcubef", d_period_size, SND_PCM_STREAM_CAPTURE);
    assert(d_pcm_handle != NULL);
}

SoapyVfzfgpa::~SoapyVfzfgpa()
{
    snd_pcm_close(d_pcm_handle);
}

// Identification API
std::string SoapyVfzfgpa::getDriverKey() const
{
    return "Vfzfpga";
}

std::string SoapyVfzfgpa::getHardwareKey() const
{
    return "Vfzfpga";
}

// Channels API
size_t SoapyVfzfgpa::getNumChannels(const int dir) const
{
    return (dir == SOAPY_SDR_RX) ? 1 : 0;
}

bool SoapyVfzfgpa::getFullDuplex(const int direction, const size_t channel) const
{
    SoapySDR_log(SOAPY_SDR_INFO, "getFullDuplex");
    return false;
}

// Stream API
std::vector<std::string> SoapyVfzfgpa::getStreamFormats(const int direction, const size_t channel) const
{
    std::vector<std::string> formats;
    formats.push_back("CF32");
    //formats.push_back("CF32");
    return formats;
}

std::string SoapyVfzfgpa::getNativeStreamFormat(const int direction, const size_t channel, double &fullScale) const
{
    fullScale = (1 << 16);
    return "CS16";
}

SoapySDR::ArgInfoList SoapyVfzfgpa::getStreamArgsInfo(const int direction, const size_t channel) const
{
    SoapySDR::ArgInfoList streamArgs;
    
    SoapySDR::ArgInfo chanArg;
    chanArg.key = "chan";
    chanArg.value = "stereo_iq";
    chanArg.name = "Channel Setup";
    chanArg.description = "Input channel configuration.";
    chanArg.type = SoapySDR::ArgInfo::STRING;
    
    std::vector<std::string> chanOpts;
    std::vector<std::string> chanOptNames;
    
    chanOpts.push_back("stereo_iq");
    chanOptNames.push_back("Complex L/R = I/Q");

    chanArg.options = chanOpts;
    chanArg.optionNames = chanOptNames;
    
    streamArgs.push_back(chanArg);
    
    return streamArgs;
}

SoapySDR::Stream *SoapyVfzfgpa::setupStream(const int direction, const std::string &format, const std::vector<size_t> &channels, const SoapySDR::Kwargs &args)
{
    //check the channel configuration
    if (channels.size() > 1 or (channels.size() > 0 and channels.at(0) != 0))
    {
        throw std::runtime_error("setupStream invalid channel selection");
    }
    if (format != "CF32") {
        throw std::runtime_error("setupStream invalid format");
    }
    
    SoapySDR_log(SOAPY_SDR_INFO, "Using format CF32");
    
    return (SoapySDR::Stream *) this;
}

void SoapyVfzfgpa::closeStream(SoapySDR::Stream *stream)
{
    SoapySDR_log(SOAPY_SDR_INFO, "close stream");
    // clear bufs
}

size_t SoapyVfzfgpa::getStreamMTU(SoapySDR::Stream *stream) const
{
    SoapySDR_log(SOAPY_SDR_INFO, "get mtu");
    
    return d_period_size;
}

int SoapyVfzfgpa::activateStream(SoapySDR::Stream *stream,
                                 const int flags,
                                 const long long timeNs,
                                 const size_t numElems)
{
    SoapySDR_log(SOAPY_SDR_INFO, "activate stream");

    // snd_pcm_prepare(d_pcm_handle);
    snd_pcm_start(d_pcm_handle);
    
    return 0;
}

int SoapyVfzfgpa::deactivateStream(SoapySDR::Stream *stream, const int flags, const long long timeNs)
{
    SoapySDR_log(SOAPY_SDR_INFO, "deactivate stream");
    snd_pcm_pause(d_pcm_handle, 1);
    snd_pcm_reset(d_pcm_handle);
    
    return 0;
}

int SoapyVfzfgpa::readStream(SoapySDR::Stream *stream,
                             void * const *buffs,
                             const size_t numElems,
                             int &flags,
                             long long &timeNs,
                             const long timeoutUs)
{
    if (d_pcm_handle == NULL) {
        return 0;
    }
    // buffs is an array of pointers, we only use the first channel.
    void* buff0 = buffs[0];

    // timestamp
    snd_pcm_uframes_t avail;
    snd_htimestamp_t tstamp;
    long long *longstamp;
    longstamp = (long long*)&tstamp;
    
    int elements;
    
    // Timeout if not ready
    if(snd_pcm_wait(d_pcm_handle, int(timeoutUs / 1000))) {
        // Retreive a timestamp
        snd_pcm_htimestamp(d_pcm_handle, &avail, &tstamp);
        timeNs = *longstamp;
        
        elements = (int) snd_pcm_readi(d_pcm_handle, buff0, MIN(d_period_size, numElems));
        if(elements < 0) {
            throw std::runtime_error("readStream something wrong");
        }

        // const float* data = (const float*) buff0;
        // SoapySDR_logf(SOAPY_SDR_INFO, "readStream %d %d %f", numElems, elements, data[10]);
    } else {
        throw std::runtime_error("timeout");
    }
    
    return elements;
}


std::vector<std::string> SoapyVfzfgpa::listAntennas(const int direction, const size_t channel) const
{
    SoapySDR_log(SOAPY_SDR_INFO, "listAntennas");
    
    std::vector<std::string> antennas;
    antennas.push_back("RX");
    // antennas.push_back("TX");
    return antennas;
}

void SoapyVfzfgpa::setAntenna(const int direction, const size_t channel, const std::string &name)
{
    SoapySDR_log(SOAPY_SDR_INFO, "setAntenna");
    // TODO
}

std::string SoapyVfzfgpa::getAntenna(const int direction, const size_t channel) const
{
    SoapySDR_log(SOAPY_SDR_INFO, "getAntenna");
    return "RX";
    // return "TX";
}

bool SoapyVfzfgpa::hasDCOffsetMode(const int direction, const size_t channel) const
{
    return false;
}

std::vector<std::string> SoapyVfzfgpa::listGains(const int direction, const size_t channel) const
{
    //list available gain elements,
    //the functions below have a "name" parameter
    std::vector<std::string> results;
    // results.push_back("AUDIO");
    return results;
}

bool SoapyVfzfgpa::hasGainMode(const int direction, const size_t channel) const
{
    SoapySDR_log(SOAPY_SDR_INFO, "hasGainMode");
    
    return false;
}

void SoapyVfzfgpa::setGainMode(const int direction, const size_t channel, const bool automatic)
{
    d_agc_mode = automatic;
    SoapySDR_logf(SOAPY_SDR_DEBUG, "Setting Audio AGC: %s", automatic ? "Automatic" : "Manual");
}

bool SoapyVfzfgpa::getGainMode(const int direction, const size_t channel) const
{
    SoapySDR_log(SOAPY_SDR_INFO, "getGainMode");
    
    return d_agc_mode;
}

void SoapyVfzfgpa::setGain(const int direction, const size_t channel, const double value)
{
    SoapySDR_log(SOAPY_SDR_INFO, "setGain");
    
    SoapySDR::Device::setGain(direction, channel, value);
}

void SoapyVfzfgpa::setGain(const int direction, const size_t channel, const std::string &name, const double value)
{
    SoapySDR_logf(SOAPY_SDR_DEBUG, "Setting gain: %f", value);
}

double SoapyVfzfgpa::getGain(const int direction, const size_t channel, const std::string &name) const
{
    SoapySDR_log(SOAPY_SDR_INFO, "getGain");
    return 0;
}

SoapySDR::Range SoapyVfzfgpa::getGainRange(const int direction, const size_t channel, const std::string &name) const
{
    SoapySDR_log(SOAPY_SDR_INFO, "getGainRange");
    return SoapySDR::Range(0, 100);
}

// Frequency
void SoapyVfzfgpa::setFrequency(const int direction,
                                const size_t channel,
                                const std::string &name,
                                const double frequency,
                                const SoapySDR::Kwargs &args)
{
    if (name == "RF")
    {
        d_frequency = frequency;
    }
}

double SoapyVfzfgpa::getFrequency(const int direction, const size_t channel, const std::string &name) const
{
    SoapySDR_log(SOAPY_SDR_INFO, "getFrequency");
    return d_frequency;
}

std::vector<std::string> SoapyVfzfgpa::listFrequencies(const int direction, const size_t channel) const
{
    SoapySDR_log(SOAPY_SDR_INFO, "listFrequencies");
    
    std::vector<std::string> names;
    names.push_back("RF");
    return names;
}

SoapySDR::RangeList SoapyVfzfgpa::getFrequencyRange(const int direction, const size_t channel, const std::string &name) const
{
    SoapySDR_log(SOAPY_SDR_INFO, "getFrequencyRange");
    
    SoapySDR::RangeList results;
    if (name == "RF")
    {
        results.push_back(SoapySDR::Range(0, 45000000));
    }
    return results;
}

SoapySDR::ArgInfoList SoapyVfzfgpa::getFrequencyArgsInfo(const int direction, const size_t channel) const
{
    SoapySDR_log(SOAPY_SDR_INFO, "getFrequencyArgsInfo");
    
    SoapySDR::ArgInfoList freqArgs;
    // TODO: frequency arguments
    return freqArgs;
    
}

void SoapyVfzfgpa::setSampleRate(const int direction, const size_t channel, const double rate)
{
    SoapySDR_logf(SOAPY_SDR_INFO, "setSampleRate %f", rate);
    
    d_sample_rate = rate;
}

double SoapyVfzfgpa::getSampleRate(const int direction, const size_t channel) const
{
    SoapySDR_logf(SOAPY_SDR_INFO, "getSampleRate %f", d_sample_rate);
    
    return d_sample_rate;
}

std::vector<double> SoapyVfzfgpa::listSampleRates(const int direction, const size_t channel) const
{
    SoapySDR_log(SOAPY_SDR_INFO, "listSampleRates");
    
    std::vector<double> rates;
    rates.push_back(192000);
    return rates;
}


void SoapyVfzfgpa::setBandwidth(const int direction, const size_t channel, const double bw)
{
    SoapySDR_log(SOAPY_SDR_INFO, "setBandwidth");
    SoapySDR::Device::setBandwidth(direction, channel, bw);
}

double SoapyVfzfgpa::getBandwidth(const int direction, const size_t channel) const
{
    SoapySDR_log(SOAPY_SDR_INFO, "getBandwidth");
    return SoapySDR::Device::getBandwidth(direction, channel);
}

bool SoapyVfzfgpa::hasHardwareTime(const std::string &what) const
{
    return true;
}

long long SoapyVfzfgpa::getHardwareTime(const std::string &what) const
{
    snd_pcm_uframes_t avail;
    snd_htimestamp_t tstamp;
    long long *longstamp;
    longstamp = (long long*) &tstamp;
    snd_pcm_htimestamp(d_pcm_handle, &avail, &tstamp);
    return *longstamp;
}

SoapySDR::ArgInfoList SoapyVfzfgpa::getSettingInfo(void) const
{
    SoapySDR::ArgInfoList settings;
    
    SoapySDR_log(SOAPY_SDR_INFO, "getSettingInfo");
    
    return settings;
}

void SoapyVfzfgpa::writeSetting(const std::string &key, const std::string &value)
{
    SoapySDR_log(SOAPY_SDR_INFO, "writeSetting");
}

std::string SoapyVfzfgpa::readSetting(const std::string &key) const
{
    SoapySDR_log(SOAPY_SDR_INFO, "readSetting");
    
    return "empty";
}

std::vector<double> SoapyVfzfgpa::listBandwidths(const int direction, const size_t channel) const
{
    SoapySDR_log(SOAPY_SDR_INFO, "listBandwidths");
    std::vector<double> results;
    return results;
}


// Registry
SoapySDR::KwargsList findVfzfgpa(const SoapySDR::Kwargs &args)
{
    SoapySDR_log(SOAPY_SDR_INFO, "findVfzfgpa");
    
    SoapySDR::KwargsList results;
    SoapySDR::Kwargs soapyInfo;
    
    soapyInfo["device_id"] = std::to_string(0);
    soapyInfo["label"] = "vfzfpga";
    soapyInfo["device"] = "vfzfpga";
    // add some more
    
    results.push_back(soapyInfo);
    
    return results;
}

SoapySDR::Device *makeVfzfgpa(const SoapySDR::Kwargs &args)
{
    SoapySDR_log(SOAPY_SDR_INFO, "makeVfzfgpa");
    
    //create an instance of the device object given the args
    //here we will translate args into something used in the constructor
    return new SoapyVfzfgpa();
}

static SoapySDR::Registry registerVfzfgpa("vfzfpga", &findVfzfgpa, &makeVfzfgpa, SOAPY_SDR_ABI_VERSION);