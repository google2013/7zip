// RandGen.cpp

#include "StdAfx.h"

#include "Windows/Synchronization.h"

#include "RandGen.h"

#ifndef _WIN32
#include <unistd.h>
#define USE_POSIX_TIME
#define USE_POSIX_TIME2
#endif

#ifdef USE_POSIX_TIME
#include <time.h>
#ifdef USE_POSIX_TIME2
#include <sys/time.h>
#endif
#endif

// This is not very good random number generator.
// Please use it only for salt.
// First generated data block depends from timer and processID.
// Other generated data blocks depend from previous state
// Maybe it's possible to restore original timer value from generated value.

#define HASH_UPD(x) Sha256_Update(&hash, (const Byte *)&x, sizeof(x));

void CRandomGenerator::Init()
{
  CSha256 hash;
  Sha256_Init(&hash);

  #ifdef _WIN32
  DWORD w = ::GetCurrentProcessId();
  HASH_UPD(w);
  w = ::GetCurrentThreadId();
  HASH_UPD(w);
  #else
  pid_t pid = getpid();
  HASH_UPD(pid);
  pid = getppid();
  HASH_UPD(pid);
  #endif

  for (unsigned i = 0; i < 1000; i++)
  {
    #ifdef _WIN32
    LARGE_INTEGER v;
    if (::QueryPerformanceCounter(&v))
      HASH_UPD(v.QuadPart);
    #endif

    #ifdef USE_POSIX_TIME
    #ifdef USE_POSIX_TIME2
    timeval v;
    if (gettimeofday(&v, 0) == 0)
    {
      HASH_UPD(v.tv_sec);
      HASH_UPD(v.tv_usec);
    }
    #endif
    time_t v2 = time(NULL);
    HASH_UPD(v2);
    #endif

    DWORD tickCount = ::GetTickCount();
    HASH_UPD(tickCount);
    
    for (unsigned j = 0; j < 100; j++)
    {
      Sha256_Final(&hash, _buff);
      Sha256_Init(&hash);
      Sha256_Update(&hash, _buff, SHA256_DIGEST_SIZE);
    }
  }
  Sha256_Final(&hash, _buff);
  _needInit = false;
}

static NWindows::NSynchronization::CCriticalSection g_CriticalSection;

void CRandomGenerator::Generate(Byte *data, unsigned size)
{
  g_CriticalSection.Enter();
  if (_needInit)
    Init();
  while (size > 0)
  {
    CSha256 hash;
    
    Sha256_Init(&hash);
    Sha256_Update(&hash, _buff, SHA256_DIGEST_SIZE);
    Sha256_Final(&hash, _buff);
    
    Sha256_Init(&hash);
    UInt32 salt = 0xF672ABD1;
    HASH_UPD(salt);
    Sha256_Update(&hash, _buff, SHA256_DIGEST_SIZE);
    Byte buff[SHA256_DIGEST_SIZE];
    Sha256_Final(&hash, buff);
    for (unsigned i = 0; i < SHA256_DIGEST_SIZE && size > 0; i++, size--)
      *data++ = buff[i];
  }
  g_CriticalSection.Leave();
}

CRandomGenerator g_RandomGenerator;
