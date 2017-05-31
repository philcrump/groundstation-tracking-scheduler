#include <stdint.h>
#include <time.h>

uint64_t timestamp_s(void)
{
  struct timespec spec;
  clock_gettime(CLOCK_REALTIME, &spec);
  return (uint64_t) spec.tv_sec + (((uint64_t) spec.tv_nsec) / 1000);
}
uint64_t timestamp_ms(void)
{
  struct timespec spec;
  clock_gettime(CLOCK_REALTIME, &spec);
  return ((uint64_t) spec.tv_sec) * 1000 + (((uint64_t) spec.tv_nsec) / 1000000);
}
