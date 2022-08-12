#include <iostream>
#include <string>
#include "vrmx.h"
#include "vrmx-io.h"

int
main (int argc, char **argv)
{
  if (argc < 3) {
    std::cerr << argv[0] << " in.vrm out.json" << std::endl;
    return 1;
  }

  std::string vrmPath = std::string (argv[1]);
  std::string jsonPath = std::string (argv[2]);

  try {
    vrmx::VRMContext ctx = vrmx::VRMContext::LoadBinaryFromFile (vrmPath);
    ctx.ToJSONFile (jsonPath);
  } catch (const std::string ex) {
    return 1;
  }

  return 0;
}
