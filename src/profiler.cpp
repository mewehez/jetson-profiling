#include "profiler.h"
#include <strings.h>
#include <jetson-utils/logging.h>

using namespace profiling;

// set default profiling options
FILE* Profiler::mFile = stdout;
std::string Profiler::mFilename = "stdout";


void Profiler::setFile(const char* filename)
{
    if(!filename)  // filename is null
        return;
    
    if(strcasecmp(filename, "stdout") == 0)
        setFile(stdout);
    
    else if(strcasecmp(filename, "stderr") == 0)
        setFile(stderr);
    
    else
    {
        if(strcasecmp(filename, mFilename.c_str()) == 0)
            return;
        
        FILE* file = fopen(filename, "w");

        if(file != NULL)
        {
            setFile(file);
            mFilename = filename;
        }
        else
        {
            LogError("failed to open '%s' for logging\n", filename);
            return;
        }
    }
}

void Profiler::setFile(FILE* file)
{
    if(!file || mFile == file)  // the file is already set
        return;
    
    mFile = file;

    if(mFile == stdout)
        mFilename = "stdout";
    else if(mFile == stderr)
        mFilename = "stderr";
}

void Profiler::writeInferenceTime(double startTimestamp, double duration)
{
    fprintf(getFile(), "model_total; %f; %f\n", duration, startTimestamp);
}

void Profiler::writeLayerTime(const char* layerName, float duration)
{
    fprintf(getFile(), "%s; %f;\n", layerName, duration);
}

