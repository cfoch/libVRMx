#include <iostream>
#include <string>
#include "vrmx.h"

int
main (int argc, char **argv)
{
  if (argc < 2) {
    std::cerr << argv[0] << " file.vrm" << std::endl;
    return -1;
  }

  std::string path = std::string (argv[1]);
  try {
    vrmx::VRMContext ctx = vrmx::VRMContext::LoadBinaryFromFile (path);
  } catch (const std::string ex) {
    std::cout << ex << std::endl;
  }

  return 0;
}
