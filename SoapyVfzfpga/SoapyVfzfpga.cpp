#include "SoapyVfzfpga.hpp"
#include <SoapySDR/Logger.hpp>

#include <iostream>     // std::cout
#include <fstream>      // std::ifstream
#include <istream>

SoapyVfzfgpa::SoapyVfzfgpa() :
d_agc_mode(false),
d_period_size(4096),
d_frequency(0),
d_sample_rate(89286)
{
    d_freq_f.open("/sys/class/sdr/vfzsdr/frequency");
    // Sample buffer
    d_buff.resize(2 * d_period_size);
}

SoapyVfzfgpa::~SoapyVfzfgpa()
{
    d_freq_f.close();
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
    formats.push_back("CS16");
    formats.push_back("CS32");
    formats.push_back("CF32");
    return formats;
}

std::string SoapyVfzfgpa::getNativeStreamFormat(const int direction, const size_t channel, double &fullScale) const
{
    fullScale = (1 << 24);
    return "CS32";
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
    
    SoapySDR_logf(SOAPY_SDR_INFO, "Wants format %s", format.c_str());
    
    if (format == "CS16") {
        d_stream_format = STREAM_FORMAT_INT16;
        SoapySDR_log(SOAPY_SDR_INFO, "Using format CS16");
    }
    else if (format == "CS32") {
        d_stream_format = STREAM_FORMAT_INT32;
        SoapySDR_log(SOAPY_SDR_INFO, "Using format CS32");
    }
    else if (format == "CF32")
    {
        d_stream_format = STREAM_FORMAT_FLOAT32;
        SoapySDR_log(SOAPY_SDR_INFO, "Using format CF32");
    } else {
        throw std::runtime_error("setupStream invalid format");
    }
    
    d_pcm_handle = alsa_pcm_handle("vfzsdr", d_period_size, SND_PCM_STREAM_CAPTURE);
    assert(d_pcm_handle != NULL);
    
    return (SoapySDR::Stream *) this;
}

void SoapyVfzfgpa::closeStream(SoapySDR::Stream *stream)
{
    SoapySDR_log(SOAPY_SDR_INFO, "close stream");
    if (d_pcm_handle != nullptr) {
        snd_pcm_close(d_pcm_handle);
    }
    
    
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
    
    // int err = 0;
    
    snd_pcm_drop(d_pcm_handle);
    snd_pcm_prepare(d_pcm_handle);
    
    return 0;
}

int SoapyVfzfgpa::readStream(SoapySDR::Stream *stream,
                             void * const *buffs,
                             const size_t numElems,
                             int &flags,
                             long long &timeNs,
                             const long timeoutUs)
{
    // This function has to be well defined at all times
    if (d_pcm_handle == NULL) {
        return 0;
    }
    
    // Are we running?
    if (snd_pcm_state(d_pcm_handle) != SND_PCM_STATE_RUNNING) {
        return 0;
    }
    
    // Timeout if not ready
    if(snd_pcm_wait(d_pcm_handle, int(timeoutUs / 1000)) == 0) {
        return SOAPY_SDR_TIMEOUT;
    }
    
    // Read from ALSA
    snd_pcm_sframes_t frames = 0;
    int err = 0;
    // no program is complete without a goto
again:
    // read numElems or d_period_size
    frames = snd_pcm_readi(d_pcm_handle, &d_buff[0], MIN(d_period_size, numElems));
    // try to handle xruns
    if(frames < 0) {
        err = (int) frames;
        if(snd_pcm_recover(d_pcm_handle, err, 0) == 0) {
            SoapySDR_logf(SOAPY_SDR_ERROR, "readStream recoverd from %s", snd_strerror(err));
            goto again;
        } else {
            SoapySDR_logf(SOAPY_SDR_ERROR, "readStream error: %s", snd_strerror(err));
            return SOAPY_SDR_STREAM_ERROR;
        }
    }
    
    // Convert to appropriate format
    switch (d_stream_format) {
        case STREAM_FORMAT_FLOAT32:
        {
            float scale = 1./INT32_MAX;
            float *out = (float*) buffs[0];
            int32_t *in = &d_buff[0];
            for (int i = 0; i < 2 * frames; i++) {
                out[i] = in[i] * scale;
            }
        }
            break;
        case STREAM_FORMAT_INT32:
        {
            int32_t *out = (int32_t*) buffs[0];
            int32_t *in = &d_buff[0];
            for (int i = 0; i < 2 * frames; i++) {
                out[i] = in[i];
            }
        }
            break;
        case STREAM_FORMAT_INT16:
        case STREAM_FORMAT_INT8:
        default:
            throw std::runtime_error("readStream invalid format");
            break;
    }
    
    // const float* data = (const float*) buff0;
    // SoapySDR_logf(SOAPY_SDR_INFO, "readStream %d %d %f", numElems, elements, data[10]);
    
    return (int)frames;
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
    SoapySDR_log(SOAPY_SDR_INFO, "setFrequency");
    
    if (name == "RF")
    {
        d_freq_f << int(frequency);
        d_freq_f.clear();
        d_freq_f.seekg(0, std::ios::beg);
        
        d_frequency = frequency;
    }
}

double SoapyVfzfgpa::getFrequency(const int direction, const size_t channel, const std::string &name) const
{
    SoapySDR_logf(SOAPY_SDR_INFO, "getFrequency");
    if (name == "RF")
    {
        return d_frequency;
    } else {
        throw std::runtime_error("getFrequency for nonexisting tuner");
    }
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
    rates.push_back(89286.);
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
    return (SoapySDR::Device*) new SoapyVfzfgpa();
}

static SoapySDR::Registry registerVfzfgpa("vfzfpga", &findVfzfgpa, &makeVfzfgpa, SOAPY_SDR_ABI_VERSION);
