package codec

type CodecID int

const (
    CODECID_VIDEO_H264 CodecID = iota
    CODECID_VIDEO_H265
    CODECID_VIDEO_VP8

    CODECID_AUDIO_AAC CodecID = iota + 98
    CODECID_AUDIO_G711A
    CODECID_AUDIO_G711U
    CODECID_AUDIO_OPUS
    CODECID_AUDIO_MP3

    CODECID_UNRECOGNIZED = 999
)

type H264_NAL_TYPE int

const (
    H264_NAL_RESERVED H264_NAL_TYPE = iota
    H264_NAL_P_SLICE
    H264_NAL_SLICE_A
    H264_NAL_SLICE_B
    H264_NAL_SLICE_C
    H264_NAL_I_SLICE
    H264_NAL_SEI
    H264_NAL_SPS
    H264_NAL_PPS
    H264_NAL_AUD
)

type H265_NAL_TYPE int

const (
    H265_NAL_Slice_TRAIL_N H265_NAL_TYPE = iota
    H265_NAL_LICE_TRAIL_R
    H265_NAL_SLICE_TSA_N
    H265_NAL_SLICE_TSA_R
    H265_NAL_SLICE_STSA_N
    H265_NAL_SLICE_STSA_R
    H265_NAL_SLICE_RADL_N
    H265_NAL_SLICE_RADL_R
    H265_NAL_SLICE_RASL_N
    H265_NAL_SLICE_RASL_R

    //IDR
    H265_NAL_SLICE_BLA_W_LP H265_NAL_TYPE = iota + 6
    H265_NAL_SLICE_BLA_W_RADL
    H265_NAL_SLICE_BLA_N_LP
    H265_NAL_SLICE_IDR_W_RADL
    H265_NAL_SLICE_IDR_N_LP
    H265_NAL_SLICE_CRA

    //vps pps sps
    H265_NAL_VPS H265_NAL_TYPE = iota + 16
    H265_NAL_SPS
    H265_NAL_PPS
    H265_NAL_AUD

    //SEI
    H265_NAL_SEI H265_NAL_TYPE = iota + 19
    H265_NAL_SEI_SUFFIX
)

func CodecString(codecid CodecID) string {
    switch codecid {
    case CODECID_VIDEO_H264:
        return "H264"
    case CODECID_VIDEO_H265:
        return "H265"
    case CODECID_VIDEO_VP8:
        return "VP8"
    case CODECID_AUDIO_AAC:
        return "AAC"
    case CODECID_AUDIO_G711A:
        return "G711A"
    case CODECID_AUDIO_G711U:
        return "G711U"
    case CODECID_AUDIO_OPUS:
        return "OPUS"
    case CODECID_AUDIO_MP3:
        return "MP3"
    default:
        return "UNRECOGNIZED"
   }
}
