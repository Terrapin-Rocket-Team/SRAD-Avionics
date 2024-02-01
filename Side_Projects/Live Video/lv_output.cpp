#include "lv_output.hpp"

LVOutput::LVOutput(VideoOptions const *options) : Output(options)
{
    pipeLocal = popen("ffmpeg -s 1280x720 -f rawvideo -i - -vcodec h264_v4l2m2m -r 30 -b:v 5M -an -sn -dn -y ~/video/out.mp4", "w");
    pipeLive = popen("aomenc -w 1280 -h 720 --profile=0 --kf-max-dist=240 --end-usage=cbr --min-q=1 --max-q=63 --undershoot-pct=100 --overshoot-pct=50 --buf-sz=1000 --buf-initial-sz=500 --buf-optimal-sz=600 --max-intra-rate=600 --passes=1 --lag-in-frames=0 --error-resilient=0 --tile-columns=1 --tile-rows=3 --aq-mode=2 --enable-obmc=0 --enable-global-motion=1 --enable-warped-motion=0 --deltaq-mode=0 --enable-tpl-model=0 --mode-cost-upd-freq=2 --coeff-cost-upd-freq=2 --enable-ref-frame-mvs=0 --mv-cost-upd-freq=3 --enable-order-hint=0 --cpu-used=10 --rt --usage=1 --threads=3 --ivf --target-bitrate=250 --fps=30 - -o ~/video/out.av1", "w");
}

LVOutput::~LVOutput()
{
    pclose(pipeLocal);
    pclose(pipeLive);
}

void LVOutput::outputBuffer(void *mem, size_t size, int64_t timestamp_us, uint32_t flags)
{
    // assume the buffer is one frame
    fwrite(mem, size, 1, pipeLocal);
    fwrite(mem, size, 1, pipeLive);
}