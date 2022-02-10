#include <cstdio>
#include <jetson-inference/imageNet.h>
#include <jetson-utils/loadImage.h>

#include "myImageNet.h"

// use jetson libs in headless mode
#define IS_HEADLESS() "headless"  // run without display


// print command line options
int usage()
{
	printf("usage: imagenet input_IMAGE [--help] [--network=NETWORK] ...\n");
	printf("                [--nb-runs=TOTAL_RUNS] [--profile-out=PROFILE_OUT]\n\n");
	printf("Runs inference on image multiple times with an image recognition DNN.\n");
	printf("See below for additional arguments that may not be shown above.\n\n");	
	printf("positional arguments:\n");
	printf("    input_IMAGE     path to image on which we whant to make our prediction.\n");
	printf("    PROFILE_OUT     output method for the profiler values (out.txt, stdout, etc). Defaults to stdout.\n");
    printf("    TOTAL_RUNS      total inferences to run. Defaults to 10.\n\n");
    printf("%s", imageNet::Usage());
	printf("%s", Log::Usage());

	return 0;
}


int main(int argc, char** argv)
{
    // parse command line
    commandLine cmdLine(argc, argv, IS_HEADLESS());

    if(cmdLine.GetFlag("help"))
        return usage();
    
    
    // cmdLine.AddArg("--log-level=silent");  // force silent logging
    // cmdLine.AddFlag("profile");  // force profiling
    // Log::ParseCmdLine(cmdLine);  // update logger

    int maxInfer = cmdLine.GetInt("nb-runs", 10);
    if(cmdLine.GetFlag("profile"))
    {
        file_profiler_t::setFile(cmdLine.GetString("profile-out", "stdout"));
    }

    
    // a command line argument containing the filename is expected
    if(argc < 2)
    {
        printf("recognition: expected image filename as argument\n");
        printf("example usage: ./recognition my_image.jpg\n");
    }

    // retrieve the filename from command line array
    const char*  imgFilename = argv[1];

    // variables to store the image data pointer and dimension
    uchar3* imgPtr  = NULL;
    int imgWidth    = 0;
    int imgHeight   = 0;

    LogInfo("Loading image: %s\n", argv[0]);
    // load the image from disk as uchar3 RGB (24 bits per pixel)
    if(!loadImage(imgFilename, &imgPtr, &imgWidth, &imgHeight))
    {
        // NOTE: loadImage will automaticaly log an error message
        // LogError("failed to load image '%s' \n", imgFilename);
        return 1;
    }

    // loadimage recognition network with TensorRT
    // imageNet* net = imageNet::Create(imageNet::GOOGLENET);
    profiling::ImageNet* net = profiling::ImageNet::Create(cmdLine);

    // check to make sure that the network model loaded properly
    if(!net)
    {
        printf("failed to load image recognition network\n");
        return 1;
    }

    // net->EnableDebug();
    // net->EnableLayerProfiler();
    // net->enableLayerProfiler();

    // variable to store confidence of the classification (between 0 and 1)
    float confidence = 0.0;
    int classIndex = -1;

    // inference multiple times
    int i{0};

    while(i < maxInfer)
    {
        printf("\t--Iteration %d of %d\n", i+1, maxInfer);
        // classify the image, return the object class index (or -1 on error)
        classIndex = net->classify(imgPtr, imgWidth, imgHeight, &confidence);

        // make sure a valid classification result was returned
        if(classIndex >= 0)
        {
            // log inference start and duration
            net->inferenceStat();
            // retrieve the name/description of the object class index
            // const char* classDescription = net->GetClassDesc(classIndex);

            // print out the classification results
            // LogInfo("image is recognized as '%s' (class #%i) with %f%% confidence\n", 
            //         classDescription, classIndex, confidence * 100.0f);
            i++;
        }
        else
        {
            // value is < 0, so an error occured
            LogError("failed to classify image\n");
        }
        // net->printProfilerTimes();
    }

    // print profiler times
    // net->printProfilerTimes();

    // free the network's resources before shutting down
    delete net;
    fclose(file_profiler_t::getFile());

    return 0;
}

// ./recognition $HOME/experiments/profiling/data/images/black_bear.jpg --network=resnet-18 --profile