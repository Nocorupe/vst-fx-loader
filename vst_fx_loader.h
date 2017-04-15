//
// The MIT License (MIT)
// Copyright (c) 2017 nocorupe.
// 

// 2017/04/15 version 0.5 

//
// Use this in *one* .cc
//   #define VSTFXLOADER_IMPLEMENTATION
//   #include "vst_fx_loader.h"
//
#ifndef VSTFXLOADER_H_
#define VSTFXLOADER_H_

#include <string>
#include <memory>
#include <vector>
#include <array>
#include <deque>
#include <map>
#include <mutex>
#include <sstream>


struct AEffect;
struct VstTimeInfo;

namespace vstfx {

#ifdef _WIN32
	typedef short Int16;
	typedef int Int32;
	typedef __int64 Int64;
#else
#include <stdint.h>
	typedef int16_t Int16;
	typedef int32_t Int32;
	typedef int64_t Int64;
#endif

#define VSTFX_64BIT_PLATFORM _WIN64 || __LP64__

#if VSTFX_64BIT_PLATFORM
	typedef Int64 IntPtr;
#else
	typedef Int32 IntPtr;
#endif



class ParamProperties {
public:
	ParamProperties()
		: isSwitch(false)
		, minValue(0.0f)
		, maxValue(1.0f)
	{}
	~ParamProperties()
	{}
	
	bool isSwitch;
	float minValue;
	float maxValue;
};

class EffectInterface {
public:
	// wrap dispatcher 
	virtual std::string getEffectName() = 0;
	virtual ParamProperties getParameterProperties(int aIndex) = 0;

	virtual std::string getParamDisplay(int aIndex) = 0;
	virtual std::string getParamName(int aIndex) = 0;
	virtual std::string getParamLabel(int aIndex) = 0;

	virtual void setProgram(int aIndex) = 0;
	virtual int getProgram() = 0;
	virtual std::string getProgramName(int aIndex) = 0;


	// wrap member
	virtual int numParams() = 0;
	virtual float getParameter(int aIndex) = 0;
	virtual void setParameter(int aIndex, float aValue) = 0;

	virtual std::size_t numPrograms() = 0;


	// process
	virtual void resume() = 0; // on
	virtual void suspend() = 0; // off
	virtual void processReplacing(float ** aInput, float ** aOutput, std::size_t aFrame) = 0;

	// time info util
	virtual void setTempo(double aTempo) = 0;

};


class Logger {
public:
	Logger();

	void push(std::string aMessage);
	std::string toString() const ;
	std::stringstream& toStream();

	void clear();

private:
	static constexpr std::size_t mMaxQueueSize = 16;
	std::deque<std::string> mMessageQueue;
	std::stringstream mStream;
	std::mutex mMutex;
};

class LibLoader {
public:
	LibLoader();
	~LibLoader();
	LibLoader& operator=(LibLoader&& aSrc) noexcept;

	operator bool() const;

public:
	bool load(std::string aFileName);
	void free();
	void* getProcAddress(std::string aProcName);
	std::string getErrorMessage();

private:
	struct Impl;
	std::unique_ptr<Impl> mImpl;

private:
	LibLoader(const LibLoader& src) = default;
};


class Vst2x : public EffectInterface {
public:
	Vst2x();
	virtual ~Vst2x();
	
	virtual Vst2x& operator=(Vst2x&& aOther) noexcept;

	virtual bool open(std::string aFileName);
	virtual void close();
	
	virtual operator bool();
public:
	// wrap dispatcher 
	virtual std::string getEffectName() override;
	virtual ParamProperties getParameterProperties(int aIndex) override;

	virtual std::string getParamDisplay(int aIndex) override;
	virtual std::string getParamName(int aIndex) override;
	virtual std::string getParamLabel(int aIndex) override;

	virtual void setProgram(int aIndex) override;
	virtual int getProgram() override;
	virtual std::string getProgramName(int aIndex) override;

	virtual std::string getVendorString();

	// wrap member
	virtual int numParams() override;
	virtual float getParameter(int aIndex) override;
	virtual void setParameter(int aIndex, float aValue) override;

	virtual std::size_t numPrograms() override;


	// process
	virtual void resume() override; // on
	virtual void suspend() override; // off
	virtual void processReplacing(float ** aInput, float ** aOutput, std::size_t aFrame) override;

	// time info util
	virtual void setTempo(double aTempo) override;

	virtual std::string getFilePath() const;
	virtual Logger& logger();
public:
	IntPtr hostCallback(AEffect *aEffect, Int32 aOpcode, Int32 aIndex, IntPtr aValue, void *aDataPtr, float aOpt);

private:
	IntPtr dispatcher(Int32 aOpcode, Int32 aIndex, IntPtr aValue, void *aPtr, float aOpt);
	bool setAEffect();
	
	std::size_t mSampleRate;
	std::size_t mBlockSize;

	std::unique_ptr<VstTimeInfo> mTimeInfo;
	std::unique_ptr<LibLoader> mLibLoader;
	AEffect *mAEffect;

	std::string mFilePath;
	Logger mLogger;

};

}


#ifdef VSTFXLOADER_IMPLEMENTATION


#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable: 4996)
#pragma warning(disable: 4100)

#include <algorithm>
#include <Windows.h>
#include "aeffectx.h"

namespace vstfx {
	typedef AEffect *(*Vst2xPluginEntryFunc)(audioMasterCallback host);

	Vst2x::Vst2x()
		: mAEffect(NULL)
		, mTimeInfo(new VstTimeInfo)
		, mSampleRate(44100)
		, mBlockSize(2048)
	{
	}

	inline Vst2x::~Vst2x()
	{
		close();
	}

	inline bool Vst2x::open(std::string aFileName)
	{
		close();
		mLibLoader = std::unique_ptr<LibLoader>(new LibLoader);
		if (mLibLoader->load(aFileName) == false) {
			mLogger.push(mLibLoader->getErrorMessage());
			mLibLoader.release();
			return false;
		}
		mFilePath = aFileName;
		return setAEffect();
	}

	inline void Vst2x::close()
	{
	}

	inline Vst2x::operator bool()
	{
		return (mAEffect != NULL);
	}

	

	Vst2x& Vst2x::operator=(Vst2x&& aOther) noexcept 
	{
		if (bool()) {
			close();
		}
		if (this != &aOther) {
			if (aOther) {
				mLibLoader = std::unique_ptr<LibLoader>(std::move(aOther.mLibLoader));
				
			}
		}
		return *this;
	}

	void setSpeakers(struct VstSpeakerArrangement *aDst, int aChannels) {
		using std::min;
		using std::max;
		std::memset(aDst, 0, sizeof(struct VstSpeakerArrangement));
		aDst->numChannels = min(max(aChannels, 0), 8);
		static constexpr std::array<VstSpeakerArrangementType, 9> types = { {
				kSpeakerArrEmpty,
				kSpeakerArrMono,
				kSpeakerArrStereo,
				kSpeakerArr30Music,
				kSpeakerArr40Music,
				kSpeakerArr50,
				kSpeakerArr60Music,
				kSpeakerArr70Music,
				kSpeakerArr80Music
			} };

		aDst->type = types[aDst->numChannels];

		for (int i = 0; i < aDst->numChannels; i++) {
			aDst->speakers[i].azimuth = 0.0f;
			aDst->speakers[i].elevation = 0.0f;
			aDst->speakers[i].radius = 0.0f;
			aDst->speakers[i].reserved = 0.0f;
			aDst->speakers[i].name[0] = '\0';
			aDst->speakers[i].type = kSpeakerUndefined;
		}
	}

	std::string getStringData(AEffect* aAEffect, VstIntPtr aOpcode, int aIndex = 0, int aValue = 0) {
		std::array<char, kVstMaxNameLen + 1> charBuff = {};
		aAEffect->dispatcher(aAEffect, aOpcode, aIndex, aValue, charBuff.data(), 0);

		return std::string(charBuff.data());
	}

	

	inline IntPtr Vst2x::dispatcher(Int32 aOpcode, Int32 aIndex, IntPtr aValue, void * aPtr, float aOpt)
	{
		return mAEffect->dispatcher(mAEffect, aOpcode, aIndex, aValue, aPtr, aOpt);
	}

	bool Vst2x::setAEffect()
	{
		auto entryPoint = mLibLoader->getProcAddress("VSTPluginMain");

		if (entryPoint == NULL) {
			entryPoint = mLibLoader->getProcAddress("VSTPluginMain()");
		}
		if (entryPoint == NULL) {
			entryPoint = mLibLoader->getProcAddress("main");
		}

		if (entryPoint == NULL) {
			mLogger.push("plugin entrypoint is not found.");
			return false;
		}
		Vst2xPluginEntryFunc entryFunc = (Vst2xPluginEntryFunc)entryPoint;
		mAEffect = entryFunc([](AEffect *aEffect, VstInt32 aOpcode, VstInt32 aIndex, VstIntPtr aValue, void *aDataPtr, float aOpt) -> VstIntPtr {
			// @JP: vstpluginmain.cpp main()ではaudioMasterVersionだけがコールされる
			if (aOpcode == audioMasterVersion) {
				return kVstVersion;
			}

			if (!aEffect || !aEffect->user) {
				return 0;
			}

			Vst2x *vst = static_cast<Vst2x *>(aEffect->user);
			return vst->hostCallback(aEffect, aOpcode, aIndex, aValue, aDataPtr, aOpt);
		});

		if (mAEffect == NULL) {
			mLogger.push("plugin entrypoint returns NULL.");
			return false;
		}

		if (mAEffect->magic != kEffectMagic) {
			mLogger.push("plugin has bad magic number.");
			return false;
		}

		int categ = dispatcher(effGetPlugCategory, 0, 0, NULL, 0.0f);
		if (categ > kPlugCategEffect) {
			std::stringstream ss;
			ss << "plugin category is not Effect.";
			ss << categ;
			mLogger.push(ss.str());
			return false;
		}

		mAEffect->user = static_cast<void*>(this);
		//mUniqueID = mAEffect->uniqueID;
		//mUniqueIDStr = uniqueIDtoStr(mUniqueID);

		// @JP: effOpenはhostがロードを終えたタイミングでのコールが推奨されている
		dispatcher(effOpen, 0, 0, NULL, 0.0f);

		dispatcher(effSetSampleRate, 0, 0, NULL, static_cast<float>(mSampleRate));
		dispatcher(effSetBlockSize, 0, (VstIntPtr)mBlockSize, NULL, 0.0f);
		struct VstSpeakerArrangement inSpeakers;
		setSpeakers(&inSpeakers, mAEffect->numInputs);
		struct VstSpeakerArrangement outSpeakers;
		setSpeakers(&outSpeakers, mAEffect->numOutputs);
		dispatcher(effSetSpeakerArrangement, 0, (VstIntPtr)&inSpeakers, &outSpeakers, 0.0f);

		return true;
	}


	inline std::string Vst2x::getEffectName()
	{
		return getStringData(mAEffect, effGetEffectName);
	}

	inline ParamProperties Vst2x::getParameterProperties(int aIndex)
	{
		ParamProperties pp;
		VstParameterProperties vstpp;
		if (dispatcher(effGetParameterProperties, aIndex, 0, &vstpp, 0.0f)) {
			pp.isSwitch = (bool)(vstpp.flags & kVstParameterIsSwitch);
			if (vstpp.flags & kVstParameterUsesIntegerMinMax) {
				pp.maxValue = (float)vstpp.maxInteger;
				pp.minValue = (float)vstpp.minInteger;
			}
		}
		return pp;
	}

	inline std::string Vst2x::getParamDisplay(int aIndex)
	{
		std::array<char, kVstMaxNameLen + 1> charBuff = {};
		mAEffect->dispatcher(mAEffect, effGetParamDisplay, aIndex, 0, (void*)(charBuff.data()), 0);

		return std::string(charBuff.data());
	}

	inline std::string Vst2x::getParamName(int aIndex)
	{
		return getStringData(mAEffect, effGetParamName, aIndex);
	}

	inline std::string Vst2x::getParamLabel(int aIndex)
	{
		return getStringData(mAEffect, effGetParamLabel, aIndex);
	}

	inline void Vst2x::setProgram(int aIndex)
	{
		dispatcher(effSetProgram, aIndex, 0, 0, 0);
	}

	inline int Vst2x::getProgram()
	{
		return dispatcher(effGetProgram, 0,0,0,0.0f);
	}

	inline std::string Vst2x::getProgramName(int aIndex)
	{
		return  getStringData(mAEffect, effGetProgramNameIndexed, aIndex);
	}

	inline std::string Vst2x::getVendorString()
	{
		return getStringData(mAEffect, effGetVendorString);
	}

	inline int Vst2x::numParams()
	{
		return mAEffect->numParams;
	}

	inline float Vst2x::getParameter(int aIndex)
	{
		return mAEffect->getParameter(mAEffect,aIndex);
	}

	inline void Vst2x::setParameter(int aIndex, float aValue)
	{
		mAEffect->setParameter(mAEffect, aIndex, aValue);
	}

	inline std::size_t Vst2x::numPrograms()
	{
		return mAEffect->numPrograms;
	}

	inline void Vst2x::resume()
	{
		dispatcher(effMainsChanged, 0, true, 0, 0);
	}

	inline void Vst2x::suspend()
	{
		dispatcher(effMainsChanged, 0, false, 0, 0);
	}

	inline void Vst2x::processReplacing(float ** aInput, float ** aOutput, std::size_t aFrame)
	{
		mAEffect->processReplacing(mAEffect, aInput, aOutput, aFrame);
	}

	inline void Vst2x::setTempo(double aTempo)
	{
		VstTimeInfo& vstTimeInfo = *mTimeInfo;

		vstTimeInfo.sampleRate = static_cast<double>(mSampleRate);

		vstTimeInfo.flags |= kVstTransportPlaying;

		vstTimeInfo.tempo = aTempo;
		vstTimeInfo.flags |= kVstTempoValid;
	}

	inline std::string Vst2x::getFilePath() const
	{
		return mFilePath;
	}

	inline Logger & Vst2x::logger()
	{
		return mLogger;
	}

	inline IntPtr Vst2x::hostCallback(AEffect * aEffect, Int32 aOpcode, Int32 aIndex, IntPtr aValue, void * aDataPtr, float aOpt)
	{
		if (aEffect == NULL) return 0;

		auto unsupportedOpcode = [&](const char* aOpcode)
		{
			static char buff[2048];
			std::sprintf(buff, "[%s] @callback unsupported opcode (%s)", getEffectName().c_str(), aOpcode);
			mLogger.push(std::string(buff));
		};

		VstIntPtr result = 0;

		switch (aOpcode) {
		case audioMasterAutomate:
			break;

		case audioMasterVersion:
			result = 2400;
			break;

		case audioMasterCurrentId:
			result = aEffect->uniqueID;
			break;

		case audioMasterIdle:
			// Ignore
			result = 1;
			break;

		case audioMasterGetTime:
			result = (VstIntPtr)(mTimeInfo.get());
			break;

		case audioMasterProcessEvents:
			unsupportedOpcode("audioMasterProcessEvents");
			break;

		case audioMasterIOChanged:
			unsupportedOpcode("audioMasterIOChanged");
			break;

		case audioMasterSizeWindow:
			unsupportedOpcode("audioMasterSizeWindow");
			break;

		case audioMasterGetSampleRate:
			result = mSampleRate;
			break;

		case audioMasterGetBlockSize:
			result = mBlockSize;
			break;

		case audioMasterGetInputLatency:
			break;

		case audioMasterGetOutputLatency:
			break;

		case audioMasterGetCurrentProcessLevel:
			// We are not a multi-threaded app and have no GUI, so this is unsupported
			result = kVstProcessLevelUnknown;
			break;

		case audioMasterGetAutomationState:
			result = kVstAutomationUnsupported;
			break;

		case audioMasterGetVendorString:
			std::strncpy(static_cast<char *>(aDataPtr), "vendor string", kVstMaxVendorStrLen);
			result = 1;
			break;

		case audioMasterGetProductString:
			std::strncpy(static_cast<char *>(aDataPtr), "vst-fx-loader", kVstMaxProductStrLen);
			result = 1;
			break;

		case audioMasterGetVendorVersion:
			result = 1;
			break;

		case audioMasterVendorSpecific:
			unsupportedOpcode("audioMasterVendorSpecific");
			break;

		case audioMasterCanDo:
		{
			auto canDoString = static_cast<char *>(aDataPtr);
			static const std::map< std::string, bool > cando = {
				{ "sendVstEvents" , false },
				{ "sendVstMidiEvent" , false },
				{ "sendVstTimeInfo" , true },
				{ "receiveVstEvents" , false },
				{ "receiveVstMidiEvent", false },
				{ "reportConnectionChanges", false },
				{ "acceptIOChanges", false },
				{ "sizeWindow", false },
				{ "offline", false },
				{ "openFileSelector",false },
				{ "closeFileSelector",false },
				{ "startStopProcess",true },
				{ "shellCategory",true },
				{ "sendVstMidiEventFlagIsRealtime",false },
			};

			if (!std::strcmp(canDoString, "")) {
				return 0;
			}
			int supported = (int)false;
			auto itr = cando.find(canDoString);
			if (itr != cando.end()) {
				supported = (int)(itr->second);
			}
			result = supported;
		}
		break;

		case audioMasterGetLanguage:
			result = kVstLangEnglish;
			break;

		case audioMasterOfflineStart:
			unsupportedOpcode("audioMasterOfflineStart");
			break;

		case audioMasterOfflineRead:
			unsupportedOpcode("audioMasterOfflineRead");
			break;

		case audioMasterOfflineWrite:
			unsupportedOpcode("audioMasterOfflineWrite");
			break;

		case audioMasterOfflineGetCurrentPass:
			unsupportedOpcode("audioMasterOfflineGetCurrentPass");
			break;

		case audioMasterOfflineGetCurrentMetaPass:
			unsupportedOpcode("audioMasterOfflineGetCurrentMetaPass");
			break;

		case audioMasterUpdateDisplay:
			// Ignore
			break;

		default:
			unsupportedOpcode("dont care");
			break;
		}

		return result;
	}

	/*
		mEffectName = 
		mVenderString = 


		mParamNames.resize(mAEffect->numParams);
		mParamLabels.resize(mAEffect->numParams);
		for (int i = 0; i < mAEffect->numParams; ++i) {
			mParamNames.at(i) = getStringData(mAEffect, effGetParamName, i);
			mParamLabels.at(i) = getStringData(mAEffect, effGetParamLabel, i);
		}

		mParamProperties.resize(mAEffect->numParams);
		for (int i = 0; i < mAEffect->numParams; ++i) {
			VstParameterProperties pp;
			if (dispatcher(effGetParameterProperties, i, 0, &pp, 0.0f)) {
				mParamProperties.at(i).isSwitch = (bool)(pp.flags & kVstParameterIsSwitch);
				if (pp.flags & kVstParameterUsesIntegerMinMax) {
					mParamProperties.at(i).maxValue = pp.maxInteger;
					mParamProperties.at(i).minValue = pp.minInteger;
				}

			}
			mParamLabels.at(i) = getStringData(mAEffect, effGetParamLabel, i);
		}

		mProgramName.resize(mAEffect->numPrograms);
		for (int i = 0; i < mAEffect->numPrograms; ++i) {
			mProgramName.at(i) = getStringData(mAEffect, effGetProgramNameIndexed, i);
		}

		return true;
	}
	*/
	namespace WinFunc {
		HMODULE LoadLibraryHandle(std::string aPath) {
			return LoadLibraryExA(
				aPath.c_str(),
				NULL,
				LOAD_WITH_ALTERED_SEARCH_PATH);
		}


		void CloseLibraryHandle(HMODULE aHandle) {
			FreeLibrary(aHandle);
		}


		std::string GetLastSystemError() {
			DWORD message_id = GetLastError();

			static const int buff_size = 2048;
			char buff[buff_size];

			FormatMessageA(
				FORMAT_MESSAGE_FROM_SYSTEM,
				0,
				message_id,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				buff,
				(DWORD)(buff_size - 1),
				NULL);
			// Windows adds newlines for error messages which we must trim
			std::string msg(buff);
			auto pos = msg.find("\n", 0);
			if (pos != std::string::npos) {
				msg.replace(pos, 1, "\0");
			}
			return msg;
		}
	}

	struct LibLoader::Impl {
		HMODULE mHandle;
	};

	LibLoader::LibLoader()
	{
	}

	LibLoader::~LibLoader()
	{
		free();
	}

	LibLoader & LibLoader::operator=(LibLoader && aSrc) noexcept
	{
		if (this != &aSrc) {
			if (bool(mImpl)) {
				free();
			}
			if (!bool(aSrc)) {
				return *this;
			}

			mImpl = std::unique_ptr<Impl>(std::move(aSrc.mImpl));

		}
		return *this;
	}

	LibLoader::operator bool() const
	{
		return bool(mImpl);
	}

	bool LibLoader::load(std::string aFileName)
	{
		if (bool(mImpl)) {
			throw std::exception("Reload LibLoader.");
		}

		mImpl = std::unique_ptr<Impl>(new Impl);
		mImpl->mHandle = WinFunc::LoadLibraryHandle(aFileName);

		if (mImpl->mHandle == NULL) {
			mImpl.release();
			return false;
		}

		return true;
	}

	void LibLoader::free()
	{
		if (bool(mImpl)) {
			WinFunc::CloseLibraryHandle(mImpl->mHandle);
			mImpl.release();
		}
	}

	void * LibLoader::getProcAddress(std::string aProcName)
	{
		if (!bool(mImpl)) {
			throw std::exception("getProcAddress() call empty LibLoader.");
		}

		return GetProcAddress(mImpl->mHandle, aProcName.c_str());
	}

	std::string LibLoader::getErrorMessage()
	{
		return WinFunc::GetLastSystemError();
	}



	
	
	void ResetStream(std::stringstream& aSs) {
		static const std::string empty_string;

		aSs.str(empty_string);
		aSs.clear();
		aSs << std::dec;
	}


	Logger::Logger()
	{
	}

	void Logger::push(std::string aMessage)
	{
		mMutex.lock();
		if (mMessageQueue.size() > mMaxQueueSize) {
			mMessageQueue.pop_front();

		}
		mMessageQueue.push_back(aMessage);
		mMutex.unlock();
	}

	std::string Logger::toString() const
	{
		return mStream.str();
	}

	std::stringstream& Logger::toStream()
	{
		ResetStream(mStream);
		while (!mMessageQueue.empty()) {
			mMutex.lock();
			mStream << mMessageQueue.front() << "\n";
			mMessageQueue.pop_front();
			mMutex.unlock();
		}
		return mStream;
	}

	void Logger::clear()
	{
		mMutex.lock();
		mMessageQueue.clear();
		mMutex.unlock();

		ResetStream(mStream);
	}

}

#pragma warning(pop)
#endif //_WIN32

#endif

#endif
