#pragma once

#include "version.h"
#include "httplib.h"
#include <optional>
#include <QString>

using Version = std::vector<int>;


struct UpdateInfo {
	Version version;
	std::string downloadUrl;
};

bool checkForNewVersion();
bool updateApp(const std::string fullUrl, const QString& destPath);
std::optional<UpdateInfo> get_update();
inline static std::optional<Version> parseTags(const std::string& tag) {
	int a, b, c;
	char extra; // for the initial v if it is there
	const char* s = tag.c_str();
	if (*s == 'v' || *s == 'V') ++s;
	if (std::sscanf(s, "%d.%d.%d%c", &a, &b, &c, &extra) != 3) // if it is more than 4 characters (not including '.')
		return std::nullopt; // return a null value. Only capture vINT.INT.INT
	return Version{ a, b, c };
}