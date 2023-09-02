#include "Common/CCString.h"

CCString::CCString() {
	m_pCString = new char[1];
	memset(m_pCString, 0, 1);
	m_pCString[0] = '\0';
}

CCString::CCString(const char* str) {
	int len = strlen(str);

	m_pCString = new char[len+1];
	memset(m_pCString, 0, len+1);
	strncpy(m_pCString, str, len);
	m_pCString[len] = '\0';
}

char* CCString::c_str() {
	return m_pCString;
}

int CCString::length() {
	return strlen(m_pCString);
}

void CCString::operator=(const char* str) {
	if(str == NULL)
		return;

	int iLen = strlen(str);
	char* pNewStr = new char[iLen+1];
	memset(pNewStr, 0, iLen+1);
	strncpy(pNewStr, str, iLen);
	pNewStr[iLen] = '\0';
	
	SAFE_DELETE_ARRAY(m_pCString);
	m_pCString = pNewStr;
}

void CCString::operator=(CCString* pCStr) {
	if(pCStr == NULL)
		return;
	
	*this = pCStr->c_str();
}

void CCString::operator=(CCString cStr) {
	*this = cStr.c_str();
}

void CCString::operator+=(const char* pStr) {
	if(pStr == NULL)
		return;

	int iLen = strlen(pStr);
	int iMaxLen = (iLen + length());

	char* pNewStr = new char[iMaxLen+1];
	memset(pNewStr, 0, iMaxLen+1);
	strncpy(pNewStr, m_pCString, length());
	pNewStr[iMaxLen] = '\0';

	SAFE_DELETE_ARRAY(m_pCString);
	m_pCString = pNewStr;
	
	strncat(m_pCString, pStr, iLen);
	m_pCString[iMaxLen] = '\0';
}

void CCString::operator+=(CCString* pCString) {
	if(pCString == NULL)
		return;

	*this += pCString->c_str();
}

void CCString::operator+=(CCString cString) {
	*this += cString.c_str();
}

void CCString::operator+=(char ch) {
	char* pStr = new char[1];
	memset(pStr, 0, 1);
	pStr[0] = ch;
	pStr[1] = '\0';
	
	*this += pStr;
}

const char* CCString::toLower() {
	return CCString::toLower(m_pCString);
}

const char*	CCString::toLower(char* inStr) {
	_strlwr(inStr);
	inStr[strlen(inStr)] = '\0';

	return inStr;
}

const char* CCString::toUpper() {
	return CCString::toUpper(m_pCString);
}

const char* CCString::toUpper(char* inStr) {
	_strupr(inStr);
	inStr[strlen(inStr)] = '\0';

	return inStr;
}

int CCString::charAt(int index) {
	return CCString::charAt(m_pCString ,index);
}

int CCString::charAt(const char* inStr, int index) {
	if(	index < 0
		||
		index > strlen(inStr)
		)
		return -1;

	return inStr[index];
}

int	CCString::indexOf(char ch) {
	return CCString::indexOf(m_pCString, ch);
}

int	CCString::indexOf(const char* inStr, char ch) {
	/*//PSUEDO CODE
	int i = 0;
	char c = inStr[i];

	while(c) {
		if(c == ch)
			return i;
		
		c = inStr[++i];
	}

	return -1;
	//0R
	//*/

	char* ret = (char*)strchr(inStr, ch);
	if(ret != NULL)
		return (ret - inStr);

	return -1;
}

int	CCString::indexOf(char ch, int fromIndex) {
	return CCString::indexOf(m_pCString, ch, fromIndex);
}

int	CCString::indexOf(const char* inStr, char ch, int fromIndex) {
	if(	fromIndex < 0 || fromIndex > strlen(inStr))
		return -1;

	char* ret = (char*)strchr(inStr + fromIndex, ch);
	if(ret != NULL)
		return (ret - inStr);

	return -1;
}

int CCString::indexOf(const char* searchStr) {
	return CCString::indexOf(m_pCString, searchStr);
}

int CCString::indexOf(const char* inStr, const char* searchStr) {
	char* ret = (char*)strstr(inStr, searchStr);
	if(ret != NULL)
		return ret - inStr;
	return -1;
}

int	CCString::indexOf(const char* searchStr, int fromIndex) {
	return indexOf(m_pCString, searchStr, fromIndex);
}

int	CCString::indexOf(const char* inStr, const char* searchStr, int fromIndex) {
	if(fromIndex < 0 || fromIndex > strlen(inStr))
		return -1;

	return CCString::indexOf(inStr + fromIndex, searchStr);
}

int	CCString::lastIndexOf(char ch) {
	return CCString::lastIndexOf(m_pCString, ch);
}

int	CCString::lastIndexOf(const char* inStr, char ch) {
	/*//PSUEDO CODE
	int i = strlen(inStr) - 1;
	char c = inStr[i];
	
	while(c) {
		if(c == ch)
			return i;

		c = inStr[--i];
	}

	return -1;
	//0R
	//*/

	char* ret = (char*)strrchr(inStr, ch);
	if(ret != NULL)
		return (ret - inStr);

	return -1;
}

int	CCString::lastIndexOf(char ch, int fromIndex) {
	return CCString::lastIndexOf(m_pCString, ch, fromIndex);
}

int	CCString::lastIndexOf(const char* inStr, char ch, int fromIndex) {
	if(	fromIndex < 0 || fromIndex > strlen(inStr))
		return -1;

	int retPos = -1;
	_strrev((char*)inStr);

	fromIndex = strlen(inStr) - fromIndex;
	char* ret = (char*)strchr(inStr + fromIndex, ch);
	if(ret != NULL)
		retPos = strlen(inStr) - (ret - inStr + 1);

	_strrev((char*)inStr);
	return retPos;
}

int CCString::lastIndexOf(const char* searchStr) {
	int retPos = -1;
	_strrev(m_pCString);

	int iSearchStrLen = strlen(searchStr);
	char* search_strrev = new char[iSearchStrLen+1];
	memset(search_strrev, 0, iSearchStrLen+1);
	strncpy(search_strrev, searchStr, iSearchStrLen);
	search_strrev[iSearchStrLen] = '\0';
	_strrev((char*)search_strrev);

	char* ret = strstr(m_pCString, search_strrev);
	if(ret != NULL)
		retPos = length() - (ret - m_pCString + iSearchStrLen);

	_strrev(m_pCString);
	SAFE_DELETE_ARRAY(search_strrev);

	return retPos;
}

int CCString::lastIndexOf(const char* searchStr, int fromIndex) {
	return CCString::lastIndexOf(m_pCString, searchStr, fromIndex);
}

int CCString::lastIndexOf(const char* inStr, const char* searchStr, int fromIndex) {
	if(	fromIndex < 0 || fromIndex > strlen(inStr))
		return -1;

	int retPos = -1;
	_strrev((char*)inStr);

	fromIndex = strlen(inStr) - fromIndex;
	int iSearchStrLen = strlen(searchStr);
	char* search_strrev = new char[iSearchStrLen+1];
	memset(search_strrev, 0, iSearchStrLen+1);
	strncpy(search_strrev, searchStr, iSearchStrLen);
	search_strrev[iSearchStrLen] = '\0';
	_strrev(search_strrev);

	char* ret = (char*)strstr(inStr + fromIndex, search_strrev);
	if(ret != NULL)
		retPos = strlen(inStr) - (ret - inStr + iSearchStrLen);

	_strrev((char*)inStr);
	SAFE_DELETE_ARRAY(search_strrev);

	return retPos;
}

bool CCString::isWhiteSpace(char ch) {
	if(	(	ch == ' '
			||
			ch == '\t'
			||
			ch == '\r'
			||
			ch == '\n'
			)
	) {
		return true;
	}

	return false;
}

void CCString::lTrim() {
	m_pCString = CCString::lTrim(m_pCString);
}

char* CCString::lTrim(char* text) {
	char* lTrimText = text;

	while(*lTrimText) {
		if(!isWhiteSpace(*lTrimText))
			break;

		lTrimText++;
	}

	if(lTrimText > text) {
		int lTrimTextLen = strlen(lTrimText);

		char* newText = new char[lTrimTextLen+1];
		memset(newText, 0, lTrimTextLen+1);
		strncpy(newText, lTrimText, lTrimTextLen);
		newText[lTrimTextLen] = '\0';

		SAFE_DELETE_ARRAY(text);
		text = newText;
		text[strlen(text)] = '\0';

		return newText;
	}

	return text;
}

void CCString::rTrim() {
	m_pCString = CCString::rTrim(m_pCString);
}

char* CCString::rTrim(char* text) {
	char* rTrimText = text + strlen(text) - 1;
	int whiteSpacePos = 0;
	int pos = 0;

	while(*rTrimText) {
		if(!isWhiteSpace(*rTrimText))
			break;

		rTrimText--;
	}

	if(rTrimText > text) {
		int rTimeTextLen = rTrimText - text + 1;
		
		char* newText = new char[rTimeTextLen+1];
		memset(newText, 0, rTimeTextLen+1);
		strncpy(newText, text, rTimeTextLen);
		newText[rTimeTextLen] = '\0';

		SAFE_DELETE_ARRAY(text);
		text = newText;
		text[strlen(text)] = '\0';
		
		return newText;
	}

	return text;
}

void CCString::trim() {
	m_pCString = CCString::trim(m_pCString);
}

char* CCString::trim(char* text) {
	return rTrim(lTrim(text));
}

char* CCString::substr_c_str(int iBeginIndex) {
	return CCString::substr_c_str(m_pCString, iBeginIndex);
}

char* CCString::substr_c_str(char* inStr, int iBeginIndex) {
	if(iBeginIndex < 0 || iBeginIndex >= strlen(inStr))
		return NULL;

	int len = strlen(inStr) - iBeginIndex;
	char* subStr = new char[len+1];
	memset(subStr, 0, len+1);
	strncpy(subStr, inStr+iBeginIndex, len);
	subStr[len] = '\0';

	return subStr;
}

char* CCString::substr_c_str(int iBeginIndex, int iEndIndex) {
	return CCString::substr_c_str(m_pCString, iBeginIndex, iEndIndex);
}

char* CCString::substr_c_str(char* inStr, int iBeginIndex, int iEndIndex) {
	if(	iBeginIndex < 0 
		|| 
		iBeginIndex > iEndIndex
		|| 
		iBeginIndex >= strlen(inStr)
	)
		return NULL;

	int len = iEndIndex - iBeginIndex;

	char* subStr = new char[len+1];
	memset(subStr, 0, len+1);
	strncpy(subStr, inStr+iBeginIndex, len);
	subStr[len] = '\0';

	return subStr;
}

CCString* CCString::substr(int iBeginIndex) {
	return CCString::substr(this, iBeginIndex);
}

CCString* CCString::substr(CCString* inStr, int iBeginIndex) {
	if(iBeginIndex < 0 || iBeginIndex >= inStr->length())
		return NULL;

	CCString* subStr = new CCString(inStr->c_str()+iBeginIndex);

	return subStr;
}

CCString* CCString::substr(int iBeginIndex, int iEndIndex) {
	return CCString::substr(this, iBeginIndex, iEndIndex);
}

CCString* CCString::substr(CCString* inStr, int iBeginIndex, int iEndIndex) {
	if(	iBeginIndex < 0 
		|| 
		iBeginIndex > iEndIndex
		|| 
		iBeginIndex >= inStr->length()
	)
		return NULL;

	int len = iEndIndex - iBeginIndex;

	char* subStr = new char[len+1];
	memset(subStr, 0, len+1);
	strncpy(subStr, inStr->c_str()+iBeginIndex, len);
	subStr[len] = '\0';

	CCString* subCStr = new CCString(subStr);
	SAFE_DELETE_ARRAY(subStr);

	return subCStr;
}

void CCString::replace(char oldChar, char newChar) {
	m_pCString = CCString::replace(m_pCString, oldChar, newChar);
}

char* CCString::replace(char* inStr, char oldChar, char newChar) {
	int i = 0;
	char c = *inStr;

	while(c) {
		if(c == oldChar) {
			inStr[i] = newChar;
		}

		c = inStr[++i];
	}

	return inStr;
}

void CCString::replace(char* oldStr, char* newStr) {
	m_pCString = CCString::replace(m_pCString, oldStr, newStr);
}

char* CCString::replace(char* inStr, char* oldStr, char* newStr) {
	unsigned int numOfOccurances = 0;

	//scan for the total numbr of occurances of 'oldStr' in 'inStr'
	char* searchStr = inStr;
	int ret = indexOf(searchStr, oldStr) + 1;
	int offset = ret;
	while(ret >= 0) {
		numOfOccurances++;
		ret = indexOf(searchStr + offset, oldStr);
		offset += ret + 1;
	}

	if(numOfOccurances > 0) {
		//make sufficient space which might be after replacing 'oldStr' with 'newStr'
		unsigned int totalSize = strlen(inStr) + numOfOccurances*(strlen(newStr) - strlen(oldStr));
		char* retBuff = new char[totalSize+1];
		memset(retBuff, 0, totalSize+1);
		retBuff[totalSize] = '\0';

		unsigned int startOffset = 0, len = 0, off = 0;
		unsigned int destCopyOffset = 0, srcCopyOffset = 0;

		int oldStrLen = strlen(oldStr);
		int newStrLen = strlen(newStr);

		//get the next location of the 'oldStr'
		ret = indexOf(searchStr, oldStr);
		len = ret;

		//copy the 'string' that lies in-between
		strncpy(retBuff + destCopyOffset, inStr + srcCopyOffset, len);
		destCopyOffset += len;
		srcCopyOffset += len;

		while(true) {

			//copy/append the 'newStr' 
			strncpy(retBuff + destCopyOffset, newStr, strlen(newStr));
			destCopyOffset += newStrLen;
			srcCopyOffset += oldStrLen;

			//shift/move the 'startOffset'
			startOffset += ret + 1;

			//get the next location of the 'oldStr'
			ret = indexOf(searchStr + startOffset, oldStr);
			if(ret < 0)
				break;

			//calculate the length of the 'string' that lies in-between the occurances of 'oldStr'
			len = ret - oldStrLen + 1;

			//copy the 'string' that lies in-between
			strncpy(retBuff + destCopyOffset, inStr + srcCopyOffset, len);
			destCopyOffset += len;
			srcCopyOffset += len;
		}

		//copy the remaining tail of the 'inStr'
		strncpy(retBuff + destCopyOffset, inStr + srcCopyOffset, strlen(inStr + srcCopyOffset));

		SAFE_DELETE_ARRAY(inStr);
		inStr = retBuff;
		inStr[strlen(inStr)] = '\0';

		return retBuff;
	}

	return inStr;
}

bool CCString::startsWith(const char* startStr) {
	return CCString::startsWith(m_pCString, startStr);
}

bool CCString::startsWith(const char* str, const char* startStr){//, bool isCaseSensitive) {
	if(!str || !*str || !startStr || !*startStr)
		return false;

	char* idx = (char*)strstr(str, startStr);
	if(idx - str == 0) {
		return true;
	}

	return false;
}

bool CCString::endsWith(const char* endStr) {
	return CCString::endsWith(m_pCString, endStr);
}

bool CCString::endsWith(const char* str, const char* endStr) {
	if(!str || !*str || !endStr || !*endStr)
		return false;

	int strLen = strlen(str);
	char* revStr = new char[strLen + 1];
	memset(revStr, 0, strLen + 1);
	strncpy(revStr, str, strLen);
	revStr[strLen] = '\0';
	_strrev(revStr);

	int endStrLen = strlen(endStr);
	char* revEndStr = new char[endStrLen + 1];
	memset(revEndStr, 0, endStrLen + 1);
	strncpy(revEndStr, endStr, endStrLen);
	revStr[endStrLen] = '\0';
	_strrev(revEndStr);

	bool ret = startsWith(revStr, revEndStr);

	SAFE_DELETE_ARRAY(revStr);
	SAFE_DELETE_ARRAY(revEndStr);

	return ret;
}

void CCString::stripChars(char* sStripChars) {
	m_pCString = CCString::stripChars(m_pCString, sStripChars);
}

char* CCString::stripChars(char* text, char* sStripChars) {
	int iLen = strlen(text);
	char* strippedStr = new char[iLen+1];
	memset(strippedStr, 0, iLen+1);
	strncpy(strippedStr, text, iLen);
	strippedStr[iLen] = '\0';

	unsigned TEXT_LENGTH = iLen;
	unsigned pos = strcspn(text, sStripChars);
	while(pos < TEXT_LENGTH) {
		strcpy(strippedStr + pos, strippedStr + pos + 1);
		pos = strcspn(strippedStr, sStripChars);

		TEXT_LENGTH = strlen(strippedStr);
	}

	SAFE_DELETE_ARRAY(text);
	text = strippedStr;
	text[strlen(text)] = '\0';

	return strippedStr;
}

bool CCString::isLowerCase(char c) {
	return (c >= 97 && c <= 122);
}

bool CCString::isUpperCase(char c) {
	return (c >= 65 && c <= 90);
}

bool CCString::isAlphabet(char c) {
	return (CCString::isLowerCase(c) || CCString::isUpperCase(c));
}

CCString::~CCString() {
	SAFE_DELETE_ARRAY(m_pCString);
}

