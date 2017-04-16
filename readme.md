# Vst Fx Loader
---------------------------------
Simple single file VST2.x effect plugin loader.  
  
日本語->[readmejp.md](https://github.com/Nocorupe/vst-fx-loader/blob/master/readmejp.md)

## Example code
```cpp
vstfx::Vst2x vst2x;
vst2x.open("VSTPlugin.dll");

std::cout << "name:" << vst2x.getEffectName() << std::endl;
std::cout << "parameters:" << std::endl;
for (int i = 0; i < vst2x.numParams(); i++) {
    std::cout << "  " << vst2x.getParamName(i) << std::endl;
}

// get/set parameter value
float v = vst2x.getParameter(i);
vst2x.setParameter(i,0.5f);

// main processing  
float * in_heads[2] = { new float[2048],new float[2048] };
float * out_heads[2] = { new float[2048],new float[2048] };

vst2x.resume();
vst2x.processReplacing( in_heads, out_heads, 2048 );
```

## Requirements
* VST3 SDK


## How to use
1 Download "VST3 SDK" from [Steinberg Website](http://www.steinberg.net/en/company/developer.html) and unzip.
2 Add "VST3 SDK\pluginterfaces\vst2.x" to Additional Include Directory in your project.
3 Copy "vst_fx_loader.h" and add to your project.
4 Define implementation flag in only **one** .cc/.cpp .
```cpp
#define VSTFXLOADER_IMPLEMENTATION
#include "vst_fx_loader.h"
```

## vs2015-example
Deploy VST SDK and build. (read "vendor\vstsdk2.x\README.txt" )

## License
[MIT](https://github.com/Nocorupe/vst-fx-loader/blob/master/LICENSE)
