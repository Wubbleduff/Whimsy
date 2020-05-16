
#include "profiling.h"

#include <windows.h>
#include <vector>

struct TimeBlock
{
  LARGE_INTEGER start;
  const char *desc;

  TimeBlock(const char *d, LARGE_INTEGER n) : desc(d), start(n) {}
};

struct ProfilingData
{
    std::vector<TimeBlock> times;
};
static ProfilingData *profiling_data;



void start_timer(const char *desc)
{
    LARGE_INTEGER start;
    QueryPerformanceCounter(&start);

    TimeBlock t = TimeBlock(desc, start);
    profiling_data->times.push_back(t);
}

float end_timer()
{
    TimeBlock &t = profiling_data->times.back();

    LARGE_INTEGER end;
    QueryPerformanceCounter(&end);

    LONGLONG diff = end.QuadPart - t.start.QuadPart;

    LARGE_INTEGER counts_per_second;
    QueryPerformanceFrequency(&counts_per_second);

    double seconds = (double)diff / counts_per_second.QuadPart;

    return (float)seconds;
}



void init_profiling()
{
    //profiling_data = (ProfilingData *)calloc(1, sizeof(ProfilingData));
    profiling_data = new ProfilingData();
}




