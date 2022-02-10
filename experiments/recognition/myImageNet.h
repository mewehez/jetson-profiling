#ifndef __MY_IMAGE_NET_H__
#define __MY_IMAGE_NET_H__

#include <jetson-inference/imageNet.h>
#include <profiling/profiler.h>

using file_profiler_t = profiling::Profiler;

namespace profiling 
{
    class ImageNet : private imageNet
    {
        public:
            // load network instance
            static ImageNet* Create( NetworkType networkType=GOOGLENET, uint32_t maxBatchSize=DEFAULT_MAX_BATCH_SIZE, 
                precisionType precision=TYPE_FASTEST,
                deviceType device=DEVICE_GPU, bool allowGPUFallback=true );

            // create network from commandline
            static ImageNet* Create(const commandLine& cmdLine);

            // enable layer time profiling
            void enableLayerProfiler();

            template<typename T> 
            int classify( T* image, uint32_t width, uint32_t height, float* confidence=NULL )
            {
                return classify((void*)image, width, height, imageFormatFromType<T>(), confidence); 
            }

            int classify( void* image, uint32_t width, uint32_t height, imageFormat format, float* confidence=NULL );

            // destructor
            virtual ~ImageNet();

            // write inference start and duration
            void inferenceStat()
            {
                profilerQuery query = PROFILER_NETWORK;
                if( PROFILER_QUERY(query) )
                {
                    timespec start = mEventsCPU[query*2];
                    float duration = mProfilerTimes[query].y;
                    file_profiler_t::writeInferenceTime(timeDouble(start), duration);
                }
                else{
                    LogInfo("Couldn't read query");
                }
            }

            // Retrieve the description of a particular class.
            inline const char* GetClassDesc( uint32_t index ) const 
            { 
                return mClassDesc[index].c_str(); 
            }

            // Print the profiler times (in millseconds).
            inline void printProfilerTimes()
            {
                LogInfo("\n");
                LogInfo(LOG_TRT "------------------------------------------------\n");
                LogInfo(LOG_TRT "Timing Report %s\n", GetModelPath());
                LogInfo(LOG_TRT "------------------------------------------------\n");

                for( uint32_t n=0; n <= PROFILER_TOTAL; n++ )
                {
                    const profilerQuery query = (profilerQuery)n;

                    if( PROFILER_QUERY(query) )
                        LogInfo(LOG_TRT "%-12s  CPU %9.5fms  CUDA %9.5fms\n", profilerQueryToStr(query), mProfilerTimes[n].x, mProfilerTimes[n].y);
                }

                LogInfo(LOG_TRT "------------------------------------------------\n\n");

                static bool first_run=true;

                if( first_run )
                {
                    LogWarning(LOG_TRT "note -- when processing a single image, run 'sudo jetson_clocks' before\n"
                            "                to disable DVFS for more accurate profiling/timing measurements\n\n");
                    
                    first_run = false;
                }
            }
        
        protected:
            ImageNet();

            // static char buffer[20];

            int classify(float* confidence);

            class Profiler : public nvinfer1::IProfiler
            {
            public:
                Profiler() : timingAccumulator(0.0f)	{ }
                
                virtual void reportLayerTime(const char* layerName, float ms) NOEXCEPT
                {
                    file_profiler_t::writeLayerTime(layerName, ms);
                    // printf(LOG_TRT "-- layer %s - %f ms\n", layerName, ms);
                    timingAccumulator += ms;
                }
                
                float timingAccumulator;
            } gProfiler;
    };

    // TODO create custom layer profiler
}

#endif