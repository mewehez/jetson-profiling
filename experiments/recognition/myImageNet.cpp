#include "myImageNet.h"

using namespace profiling;

// static ImageNet::buffer;

// constructor
ImageNet::ImageNet() : imageNet() {}

// destructor
ImageNet::~ImageNet() {}

// Create
ImageNet* ImageNet::Create( const commandLine& cmdLine )
{
	ImageNet* net = NULL;

	// obtain the network name
	const char* modelName = cmdLine.GetString("network");
	
	if( !modelName )
		modelName = cmdLine.GetString("model", "googlenet");
	
	// parse the network type
	const ImageNet::NetworkType type = NetworkTypeFromStr(modelName);

	if( type == ImageNet::CUSTOM )
	{
        // Custom network not supported yet
        LogError(LOG_TRT "myImageNet -- custom models not supperted.");
		net = NULL;
	}
	else
	{
		// create from pretrained model
		net = ImageNet::Create(type);
	}

	if( !net )
		return NULL;

	// enable layer profiling if desired
	if( cmdLine.GetFlag("profile") )
		net->enableLayerProfiler();

	return net;
}

void ImageNet::enableLayerProfiler()
{
    mEnableProfiler = true;

    if(mContext != NULL)
        mContext->setProfiler(&gProfiler);
}

ImageNet* ImageNet::Create(ImageNet::NetworkType networkType, uint32_t maxBatchSize, 
                precisionType precision, deviceType device, bool allowGPUFallback)
{
    ImageNet* net = new ImageNet();
	
	if( !net )
		return NULL;
	
	if( !net->init(networkType, maxBatchSize, precision, device, allowGPUFallback) )
	{
		LogError(LOG_TRT "myImageNet -- failed to initialize.\n");
		return NULL;
	}
	
	net->mNetworkType = networkType;
	return net;
}

// Classify
int ImageNet::classify( void* image, uint32_t width, uint32_t height, imageFormat format, float* confidence )
{
	// verify parameters
	if( !image || width == 0 || height == 0 )
	{
		LogError(LOG_TRT "imageNet::Classify( 0x%p, %u, %u ) -> invalid parameters\n", image, width, height);
		return -1;
	}
	
	// downsample and convert to band-sequential BGR
	if( !PreProcess(image, width, height, format) )
	{
		LogError(LOG_TRT "imageNet::Classify() -- tensor pre-processing failed\n");
		return -1;
	}
	
	return classify(confidence);
}

// Classify
int ImageNet::classify( float* confidence )
{	
	// process with TRT
	if( !Process() )
	{
		LogError(LOG_TRT "myImageNet::Process() failed\n");
		return -1;
	}
	
	PROFILER_BEGIN(PROFILER_POSTPROCESS);

	// determine the maximum class
	int classIndex = -1;
	float classMax = -1.0f;
	
	//const float valueScale = IsModelType(MODEL_ONNX) ? 0.01f : 1.0f;

	for( size_t n=0; n < mOutputClasses; n++ )
	{
		const float value = mOutputs[0].CPU[n] /** valueScale*/;
		
		if( value >= 0.01f )
			LogVerbose("class %04zu - %f  (%s)\n", n, value, mClassDesc[n].c_str());
	
		if( value > classMax )
		{
			classIndex = n;
			classMax   = value;
		}
	}
	
	if( confidence != NULL )
		*confidence = classMax;
	
	//printf("\nmaximum class:  #%i  (%f) (%s)\n", classIndex, classMax, mClassDesc[classIndex].c_str());
	PROFILER_END(PROFILER_POSTPROCESS);	
	return classIndex;
}

