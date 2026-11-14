#include "updateChecker.h"
#include <vector>
#include <nlohmann/json.hpp>
#include "httplib.h"
#include <QDebug>
#include <QApplication>
#include <QProgressDialog>
#include <fstream>


bool checkForNewVersion() {
	httplib::Client cli("https://api.github.com");
	cli.set_follow_location(true);
	cli.set_connection_timeout(5);
	cli.set_read_timeout(5);
	cli.set_ca_cert_path(
		(QCoreApplication::applicationDirPath() + "/cacert.pem").toStdString().c_str());
	cli.enable_server_certificate_verification(true);
	
	httplib::Headers headers = { { "User-Agent", "youtube-chat-on-screen" }, { "Accept", "application/vnd.github+json" } };
	std::string url = "/repos/Yahya-Alkhosoussy/youtube-chat-on-screen/tags?per_page=100";
	auto resp = cli.Get(url, headers);

	if (!resp) {
		qDebug() << "Request failed:" << QString::fromStdString(httplib::to_string(resp.error()));
		return false;
	}

	nlohmann::json result = nlohmann::json::parse(resp->body.data(), nullptr, false);

	auto tag = parseTags(result[0].value("name", ""));

	if (!tag) {
		qDebug() << "Could not parse tags";
		return false;
	}
	else {
		qDebug() << "Here's the tag value: " << QString::fromStdString(std::to_string(tag.value()[0]) + "." + std::to_string(tag.value()[1]) + "." + std::to_string(tag.value()[2]));
	}

	auto optional_current_version = parseTags(APP_VERSION);

	if (!optional_current_version) {
		qDebug() << "Could not parse tags of current version";
		return false;
	}
	auto current_version = optional_current_version.value();

	if (current_version[0] < tag.value()[0] || current_version[1] < tag.value()[1] || current_version[2] < tag.value()[2]) {
		return true;
	}
	return false;
}


bool updateApp(const std::string fullUrl, const QString& destPath) {
	auto schemeEnd = fullUrl.find("://");
	auto pathStart = fullUrl.find('/', schemeEnd + 3);
	std::string origin = fullUrl.substr(0, pathStart); // the domain name
	std::string path = fullUrl.substr(pathStart); // the rest

	httplib::Client cli(origin);
	cli.set_follow_location(true);
	cli.set_connection_timeout(5);
	cli.set_read_timeout(5);
	cli.set_ca_cert_path(
		(QCoreApplication::applicationDirPath() + "/cacert.pem").toStdString().c_str());

	std::ofstream out(destPath.toStdString(), std::ios::binary);
	if (!out) return false;

	QProgressDialog progress("Downloading update...", QString(), 0, 100);
	progress.setWindowModality(Qt::ApplicationModal);
	progress.show();

	auto resp = cli.Get(path, httplib::Headers{ {"User-Agent", "youtube-chat-on-screen"} },
		[&](const char* data, size_t len) {out.write(data, len); return true; },
		[&](uint64_t cur, uint64_t total) {
			if (total) progress.setValue(int(cur * 100 / total)); // progress bar for installing the update not downloading it
			QCoreApplication::processEvents();
			return true;
		});

	out.close();
	return resp && resp->status == 200;
}

std::optional<UpdateInfo> get_update() {
	httplib::Client cli("https://api.github.com");
	cli.set_follow_location(true);
	cli.set_connection_timeout(5);
	cli.set_read_timeout(5);
	cli.set_ca_cert_path(
		(QCoreApplication::applicationDirPath() + "/cacert.pem").toStdString().c_str());
	cli.enable_server_certificate_verification(true);

	httplib::Headers headers = { { "User-Agent", "youtube-chat-on-screen" }, { "Accept", "application/vnd.github+json" } };
	std::string repo_url = "/repos/Yahya-Alkhosoussy/youtube-chat-on-screen/releases/latest";
	auto resp = cli.Get(repo_url, headers);

	if (!resp || resp->status != 200) {
		qDebug() << "Could not get response status: " << resp->status;
		return std::nullopt;
	}

	auto json = nlohmann::json::parse(resp->body, nullptr, false);
	if (json.is_discarded() || !json.is_object()) return std::nullopt;

	auto value = parseTags(json.value("tag_name", ""));
	if (!value) return std::nullopt;

	std::string url;
	for (const auto& asset : json.value("assets", nlohmann::json::array())) {
		std::string name = asset.value("name", "");
		if (name.size() > 4 && name.substr(name.size() - 4) == ".exe") {
			url = asset.value("browser_download_url", "");
			break;
		}
	}
	if (url.empty()) return std::nullopt;
	return UpdateInfo{ *value, url };
};