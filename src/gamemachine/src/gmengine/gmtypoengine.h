﻿#ifndef __GM_TYPO_H__
#define __GM_TYPO_H__
#include <gmcommon.h>
#include <gamemachine.h>

BEGIN_NS

struct GMGlyphInfo;
struct GMTypoResult
{
	GMFontSizePt fontSize;
	GMfloat color[4];
	GMfloat x = 0;
	GMfloat y = 0;
	GMfloat width = 0;
	GMfloat height = 0;
	GMfloat lineHeight = 0;
	const GMGlyphInfo* glyph = nullptr;
	bool valid = true;
};

struct ITypoEngine;

GM_PRIVATE_OBJECT(GMTypoIterator)
{
	ITypoEngine* typo = nullptr;
	GMint index = 0;
	GMTypoResult result;
	bool invalid = true;
};

class GMTypoIterator : public GMObject
{
	DECLARE_PRIVATE(GMTypoIterator)

public:
	GMTypoIterator(ITypoEngine* typo, GMint index);

public:
	const GMTypoResult& operator*();
	bool operator==(const GMTypoIterator& rhs);
	bool operator!=(const GMTypoIterator& rhs);

	GMTypoIterator& operator ++(int)
	{
		return ++(*this);
	}
	GMTypoIterator& operator ++();
};

struct GMTypoOptions
{
	GMFontSizePt defaultFontSize = 12;
	GMint lineSpacing = 0;
	GMRect typoArea = { 0, 0, -1, -1 }; // 排版框，排版引擎将在此框内排版
};

GM_INTERFACE(ITypoEngine)
{
	virtual GMTypoIterator begin(const GMString& literature, const GMTypoOptions& options) = 0;
	virtual GMTypoIterator end() = 0;

private:
	friend class GMTypoIterator;
	virtual GMTypoResult getTypoResult(GMint index) = 0;
};

//////////////////////////////////////////////////////////////////////////
class GMTypoEngine;
enum class GMTypoStateMachineParseState
{
	Literature,
	WaitingForCommand,
	ParsingSymbol,
};

GM_PRIVATE_OBJECT(GMTypoStateMachine)
{
	GMTypoEngine* typoEngine = nullptr;
	GMTypoStateMachineParseState state = GMTypoStateMachineParseState::Literature;
	GMString parsedSymbol;
};

class GMTypoStateMachine : public GMObject
{
	DECLARE_PRIVATE(GMTypoStateMachine)

public:
	enum ParseResult
	{
		Okay,
		Newline,
		Ignore,
	};

	GMTypoStateMachine(GMTypoEngine* engine);

public:
	virtual ParseResult parse(REF GMwchar& ch);

protected:
	GMTypoStateMachine::ParseResult applyAttribute();

protected:
	virtual void setColor(GMfloat rgb[3]);
	virtual void setFontSize(GMint sz);

private:
	bool parsePair(const GMString& key, REF GMString& value);
	bool preciseParse(const GMString& name);
};

// 一个默认排版类
GM_PRIVATE_OBJECT(GMTypoEngine)
{
	GMTypoStateMachine* stateMachine = nullptr;
	bool insetStateMachine = false;

	GMGlyphManager* const glyphManager = GM.getGlyphManager();
	std::wstring literature;
	GMTypoOptions options;
	GMfloat lineHeight = 0;

	// 绘制状态
	GMint current_x = 0;
	GMint current_y = 0;
	GMFontSizePt fontSize = 12;
	GMfloat color[3] = { 1.f, 1.f, 1.f };
};

class GMTypoEngine : public GMObject, public ITypoEngine
{
	DECLARE_PRIVATE(GMTypoEngine);

public:
	GMTypoEngine();
	GMTypoEngine(AUTORELEASE GMTypoStateMachine* stateMachine);
	~GMTypoEngine();

public:
	virtual GMTypoIterator begin(const GMString& literature, const GMTypoOptions& options) override;
	virtual GMTypoIterator end() override;

private:
	virtual GMTypoResult getTypoResult(GMint index) override;

private:
	bool isValidTypeFrame();
	void newLine();

public:
	void setColor(GMfloat rgb[3]);
	void setFontSize(GMint pt);
};

END_NS
#endif