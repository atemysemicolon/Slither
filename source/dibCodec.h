#pragma once

namespace Slither
{
  void encodeDib_BGR_8u (
    const unsigned char* input,
    int width, 
    int height,
    int rowStepBytes,
    const char* path );

}
