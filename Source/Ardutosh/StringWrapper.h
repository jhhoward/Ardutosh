#pragma once

#include <stdint.h>

class xString
{
public:
	enum class Type : uint8_t
	{
		RAM,
		Flash
	};
	xString() : data(nullptr), type(Type::RAM), length(0) {}
	xString(const char* inData, Type inType = Type::RAM) : data(inData), type(inType), length(CalculateLength()) {}

	xString SubstringAtLine(int lineNumber, int maxCharactersPerLine) const;
	int GetIndex(char search, int start = 0) const;
	int GetLineEndIndex(int start, int maxCharactersPerLine) const;
	int Length() const { return length; }
	int GetWordLength(int index) const;
	int NumLines(int maxCharactersPerLine) const;
	const char* GetData() const { return data; }
	char operator [] (int i) const;

private:
	char Read(int i) const ;
	int CalculateLength() const;

	const char* data;
	Type type;
	int length;
};

#define FlashString(x) xString(PSTR(x), xString::Type::Flash)
