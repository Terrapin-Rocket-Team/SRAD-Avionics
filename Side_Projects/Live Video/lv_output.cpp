#include "lv_output.hpp"

LVOutput::LVOutput(VideoOptions const *options) : Output(options)
{
    // arguments lists, must start with name of command and end with NULL
    char *argument_list_local[] = {"ffmpeg", "-s", "1280x720", "-f", "rawvideo", "-i", "-", "-vcodec", "h264_v4l2m2m", "-r", "30", "-b:v", "5M", "-an", "-sn", "-dn", "-y", "~/video/test.mp4", NULL};
    char *argument_list_live[] = {"aomenc", "-w", "1280", "-h", "720", "--profile=0", "--kf-max-dist=240", "--end-usage=cbr", "--min-q=1", "--max-q=63", "--undershoot-pct=100", "--overshoot-pct=50", "--buf-sz=1000", "--buf-initial-sz=500", "--buf-optimal-sz=600", "--max-intra-rate=600", "--passes=1", "--lag-in-frames=0", "--error-resilient=0", "--tile-columns=1", "--tile-rows=3", "--aq-mode=2", "--enable-obmc=0", "--enable-global-motion=1", "--enable-warped-motion=0", "--deltaq-mode=0", "--enable-tpl-model=0", "--mode-cost-upd-freq=2", "--coeff-cost-upd-freq=2", "--enable-ref-frame-mvs=0", "--mv-cost-upd-freq=3", "--enable-order-hint=0", "--cpu-used=10", "--rt", "--usage=1", "--threads=3", "--ivf", "--target-bitrate=250", "-", "-o", "~/video/out.av1", NULL};

    // setup pipes
    pipe(pipeLocal);
    pipe(pipeLive);

    // start the local copy process
    pidLocal = fork();
    if (pidLocal == 0)
    {
        dup2(pipeLocal[0], STDIN_FILENO);
        execvp("ffmpeg", argument_list_local);
    }

    // start the live copy process
    pidLive = fork();
    if (pidLive == 0)
    {
        dup2(pipeLive[0], STDIN_FILENO);
        execvp("aomenc", argument_list_live);
    }
}

LVOutput::~LVOutput()
{
    // close all the pipes
    close(pipeLocal[0]);
    close(pipeLocal[1]);
    close(pipeLive[0]);
    close(pipeLive[1]);
}

void LVOutput::outputBuffer(void *mem, size_t size, int64_t timestamp_us, uint32_t flags)
{
    write(pipeLocal[1], mem, size);
    write(pipeLive[1], mem, size);
}