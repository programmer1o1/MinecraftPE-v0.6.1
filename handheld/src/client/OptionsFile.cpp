#include "OptionsFile.h"
#include <stdio.h>
#include "../util/StringUtils.h"

OptionsFile::OptionsFile() {
#ifdef __APPLE__
	settingsPath = "./Documents/options.txt";
#elif defined(ANDROID)
	settingsPath = "options.txt";
#else
	settingsPath = "options.txt";
#endif
}

void OptionsFile::save(const StringVector& settings) {
	FILE* pFile = fopen(settingsPath.c_str(), "w");
	if(pFile != NULL) {
		for(StringVector::const_iterator it = settings.begin(); it != settings.end(); ++it) {
			fprintf(pFile, "%s\n", it->c_str());
		}
		fclose(pFile);
	}
}

void OptionsFile::setSettingsPath(const std::string& path) {
	settingsPath = path;
}

StringVector OptionsFile::getOptionStrings() {
	StringVector returnVector;
	FILE* pFile = fopen(settingsPath.c_str(), "r");
	if(pFile != NULL) {
		char lineBuff[256];
		while(fgets(lineBuff, sizeof lineBuff, pFile)) {
			std::string line = Util::stringTrim(std::string(lineBuff));
			if(line.length() > 0)
				returnVector.push_back(line);
		}
		fclose(pFile);
	}
	return returnVector;
}
