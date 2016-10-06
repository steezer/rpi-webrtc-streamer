/*
Copyright (c) 2016, rpi-webrtc-streamer Lyu,KeunChang

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

    * Neither the name of the copyright holder nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef RASPI_ENCODER_H_
#define RASPI_ENCODER_H_

#include <memory>
#include <vector>

#include "webrtc/system_wrappers/include/clock.h"
#include "webrtc/system_wrappers/include/critical_section_wrapper.h"
#include "webrtc/base/platform_thread.h"
#include "webrtc/modules/video_coding/include/video_codec_interface.h"
#include "webrtc/media/engine/webrtcvideoencoderfactory.h"

#include "h264bitstream_parser.h"


#include "mmal_wrapper.h"

namespace webrtc {

class RaspiEncoder : public VideoEncoder {
public:
    static RaspiEncoder* Create();
    // If H.264 is supported (any implementation).
    static bool IsSupported();

    ~RaspiEncoder() override {}
};

class RaspiEncoderImpl : public RaspiEncoder {
public:
    RaspiEncoderImpl();
    ~RaspiEncoderImpl() override;

    // |max_payload_size| is ignored.
    // The following members of |codec_settings| are used. The rest are ignored.
    // - codecType (must be kVideoCodecH264)
    // - targetBitrate
    // - maxFramerate
    // - width
    // - height
    int32_t InitEncode(const VideoCodec* codec_settings,
                       int32_t number_of_cores,
                       size_t /*max_payload_size*/) override;
    int32_t Release() override;

    int32_t RegisterEncodeCompleteCallback(
        EncodedImageCallback* callback) override;
    int32_t SetRates(uint32_t bitrate, uint32_t framerate) override;

    // The result of encoding - an EncodedImage and RTPFragmentationHeader - are
    // passed to the encode complete callback.
    int32_t Encode(const VideoFrame& frame,
                   const CodecSpecificInfo* codec_specific_info,
                   const std::vector<FrameType>* frame_types) override;

    // Unsupported / Do nothing.
    int32_t SetChannelParameters(uint32_t packet_loss, int64_t rtt) override;
    int32_t SetPeriodicKeyFrames(bool enable) override;
    void OnDroppedFrame() override;

private:
    bool IsInitialized() const;

    MMALEncoderWrapper *mmal_encoder_;

    //
    // Encoded frame process thread
    CriticalSectionWrapper* drainCritSect_;
    std::unique_ptr<rtc::PlatformThread> drainThread_;
    static bool DrainThread(void*);
    bool drainStarted_;
    bool DrainProcess();

    // Reports statistics with histograms.
    void ReportInit();
    void ReportError();

    //
    VideoCodec codec_settings_;
    EncodedImageCallback* encoded_image_callback_;
    EncodedImage encoded_image_;
    bool drop_next_frame_;

    // H264 bitstream parser, used to extract QP from encoded bitstreams.
    H264StreamParser h264_stream_parser_;

#ifdef RASPI_QUALITY
    RaspiQualityScaler quality_scaler_;
#endif // RASPI_QUALITY
    int	res_width_, res_height_;
    bool resolution_scale_;

    bool has_reported_init_;
    bool has_reported_error_;
    Clock* const clock_;
};

//
// Implementation of Raspberry MMAL based encoder factory.
class MMALVideoEncoderFactory
        : public cricket::WebRtcVideoEncoderFactory {
public:
    MMALVideoEncoderFactory();
    virtual ~MMALVideoEncoderFactory();

    // WebRtcVideoEncoderFactory implementation.
    VideoEncoder* CreateVideoEncoder(webrtc::VideoCodecType type)
    override;
    const std::vector<VideoCodec>& codecs() const override;
    void DestroyVideoEncoder(webrtc::VideoEncoder* encoder) override;
    bool EncoderTypeHasInternalSource(VideoCodecType type) const override;

private:

    // Empty if platform support is lacking, const after ctor returns.
    std::vector<VideoCodec> supported_codecs_;
};


}  // namespace webrtc

#endif  // RASPI_ENCODER_H_