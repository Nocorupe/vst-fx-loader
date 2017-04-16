# Vst Fx Loader
---------------------------------
* VST2.xのエフェクトプラグインローダ  
* シングルファイルライブラリ

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

## 使い方
1 "VST3 SDK"を [Steinberg Website](http://www.steinberg.net/en/company/developer.html) からダウンロードし適宜解凍.
2 "VST3 SDK\pluginterfaces\vst2.x" をインクルードディレクトリに追加.
3 "vst_fx_loader.h" をコピーし適宜プロジェクトに追加.
4 どれか一つの .cc/.cpp ファイルで以下のフラグを定義する.
```cpp
#define VSTFXLOADER_IMPLEMENTATION
#include "vst_fx_loader.h"
```

## vs2015-example
"vendor\vstsdk2.x\README.txt"を参照してVSTSDKを配置し、ビルド

## License
[MIT](https://github.com/Nocorupe/vst-fx-loader/blob/master/LICENSE)
