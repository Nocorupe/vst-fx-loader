
#include <iostream>

#include "../../vst_fx_loader.h"

int main(int argc, const char* argv[])
{
	if (argc < 2) {
		std::cout << "USAGE: " << argv[0] << " VstPluginFilePath" << std::endl;
		return 0;
	}

	vstfx::Vst2x vst2x;
	vst2x.open(argv[1]);
	if (!vst2x) {
		std::cout << vst2x.logger().toString() << std::endl;
	}

	std::cout << "name:" << vst2x.getEffectName() << std::endl;
	std::cout << "vendor:" << vst2x.getVendorString() << std::endl;
	std::cout << "parameters:" << std::endl;
	for (int i = 0; i < vst2x.numParams(); i++) {
		std::cout << "  " << i << ":" << vst2x.getParamName(i) << std::endl;
	}
	
	std::cout << "programs:" << std::endl;
	for (int i = 0; i < vst2x.numPrograms(); i++) {
		std::cout << "  " << i << ":" << vst2x.getProgramName(i) << std::endl;
	}

	// float audio_in[2][1024];
	// float audio_out[2][1024];
	// vst2x.resume();
	// vst2x.processReplacing(audio_in, audio_out, 1024);

	vst2x.close();
	return 0;
}
