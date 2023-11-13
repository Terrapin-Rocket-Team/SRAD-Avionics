#include <stdio.h>
#include <exception>

// libcamera
#include "core/libcamera_encoder.hpp"
#include "output/output.hpp"
#include "lv_output.hpp"

using namespace std::placeholders;

static void eventLoop(LibcameraEncoder &app)
{
    VideoOptions const *options = app.GetOptions();
    std::unique_ptr<Output> output = std::unique_ptr<Output>((Output *)(new LVOutput(options)));
    app.SetEncodeOutputReadyCallback(std::bind(&Output::OutputReady, output.get(), _1, _2, _3, _4));
    app.SetMetadataReadyCallback(std::bind(&Output::MetadataReady, output.get(), _1));

    // start camera
    app.OpenCamera();
    app.ConfigureVideo(LibcameraEncoder::FLAG_VIDEO_JPEG_COLOURSPACE);
    app.StartEncoder();
    app.StartCamera();

    // main loop
    for (unsigned int count = 0;; count++)
    {
        LibcameraEncoder::Msg msg = app.Wait();
        if (msg.type == LibcameraApp::MsgType::Timeout)
        {
            printf("%s", "Camera timeout...attempting to restart");
            app.StopCamera();
            app.StartCamera();
            continue;
        }
        if (msg.type == LibcameraApp::MsgType::Quit)
            return;
        else if (msg.type != LibcameraApp::MsgType::RequestComplete)
            throw std::runtime_error("Msg type not recognized!");

        CompletedRequestPtr &completedRequest = std::get<CompletedRequestPtr>(msg.payload);
        app.EncodeBuffer(completedRequest, app.VideoStream());
    }
}

int main(int argc, char *argv[])
{
    try
    {
        LibcameraEncoder app;
        VideoOptions *options = app.GetOptions();
        if (options->Parse(argc, argv))
        {
            eventLoop(app);
        }
    }
    catch (std::exception const &e)
    {
        printf("%s%s", "Error: ", e.what());
        return -1;
    }

    return 0;
}