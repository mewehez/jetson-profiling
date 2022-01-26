#ifndef ___PROFILER_H__
#define ___PROFILER_H__

#include <stdio.h>
#include <string>

namespace profiling
{
    /*
    * Writes the layer times to an output stream.
    */
    class Profiler
    {
    private:
        static FILE* mFile;
        static std::string mFilename;
    
    public:
        // Get the current profiler output
        static inline FILE*  getFile() { return mFile; }
        // Get the output name
        static inline const char* getFileName() { return mFilename.c_str(); }
        // Set the profiling output. can be "stdout", "times.txt", etc.
        static void setFile(const char* filename);
        // Set the profiling file. It can be a builtin file (stdout, stderr) or a file that has been opened by the user.
        static void setFile(FILE* file);
        static void writeInferenceTime(double startTimestamp, double duration);
        static void writeLayerTime(const char* layerName, float duration);
    };
}

#endif