#include "Settings.h"

#include "Log.h"
#include "platform.h"

#include "pugixml/pugixml.hpp"
#include <boost/filesystem.hpp>
#include <boost/assign.hpp>

Settings* Settings::sInstance = NULL;

// these values are **NOT** ever saved to es_settings.cfg - even when you manually put them in the file
// since they're set through command-line arguments, and not the in-program settings menu
// TODO: what happens when you want both a command line config AND define it in es_sessings.cfg?
std::vector<const char*> settings_dont_save = boost::assign::list_of
		("Debug")
		("DebugGrid")
		("DebugText")
		("ParseGamelistOnly")
		("ShowExit")
		("Windowed")
		("VSync")
		("HideConsole")
		("ConfigDirectory")
		("IgnoreGamelist");

Settings::Settings()
{
	setDefaults();
}

Settings* Settings::getInstance()
{
	if(sInstance == NULL) {
		sInstance = new Settings();
	}

	return sInstance;
}

void Settings::setDefaults()
{
	mBoolMap.clear();
	mIntMap.clear();

	mBoolMap["BackgroundJoystickInput"] = false;
	mBoolMap["ParseGamelistOnly"] = false;
	mBoolMap["DrawFramerate"] = false;
	mBoolMap["ShowExit"] = true;
	mBoolMap["Windowed"] = false;
	mBoolMap["SplashScreen"] = true;
	mBoolMap["ShowHiddenFiles"] = false;

#ifdef _RPI_
	// don't enable VSync by default on the Pi, since it already
	// has trouble trying to render things at 60fps in certain menus
	mBoolMap["VSync"] = false;
#else
	mBoolMap["VSync"] = true;
#endif

	mBoolMap["EnableSounds"] = true;
	mBoolMap["ShowHelpPrompts"] = true;
	mBoolMap["ScrapeRatings"] = true;
	mBoolMap["IgnoreGamelist"] = false;
	mBoolMap["HideConsole"] = true;
	mBoolMap["QuickSystemSelect"] = true;
	mBoolMap["SaveGamelistsOnExit"] = true;

	mBoolMap["Debug"] = false;
	mBoolMap["DebugGrid"] = false;
	mBoolMap["DebugText"] = false;

	mIntMap["ScreenSaverTime"] = 5*60*1000; // 5 minutes
	mIntMap["ScraperResizeWidth"] = 400;
	mIntMap["ScraperResizeHeight"] = 0;
	mIntMap["DisplayNumber"] = 0;

	mStringMap["TransitionStyle"] = "fade";
	mStringMap["ThemeSet"] = "";
	mStringMap["ScreenSaverBehavior"] = "dim";
	mStringMap["Scraper"] = "TheGamesDB";
	mStringMap["ConfigDirectory"] = "";
}

template <typename K, typename V>
void saveMap(pugi::xml_document& doc, std::map<K, V>& map, const char* type)
{
	for(auto iter = map.begin(); iter != map.end(); iter++) {
		// key is on the "don't save" list, so don't save it
		if(std::find(settings_dont_save.begin(), settings_dont_save.end(), iter->first) != settings_dont_save.end()) {
			continue;
		}

		pugi::xml_node node = doc.append_child(type);
		node.append_attribute("name").set_value(iter->first.c_str());
		node.append_attribute("value").set_value(iter->second);
	}
}

void Settings::saveFile()
{
	const std::string path = getConfigDirectory() + "/es_settings.cfg";

	pugi::xml_document doc;

	saveMap<std::string, bool>(doc, mBoolMap, "bool");
	saveMap<std::string, int>(doc, mIntMap, "int");
	saveMap<std::string, float>(doc, mFloatMap, "float");

	//saveMap<std::string, std::string>(doc, mStringMap, "string");
	for(auto iter = mStringMap.begin(); iter != mStringMap.end(); iter++) {
		pugi::xml_node node = doc.append_child("string");
		node.append_attribute("name").set_value(iter->first.c_str());
		node.append_attribute("value").set_value(iter->second.c_str());
	}

	doc.save_file(path.c_str());
}

bool Settings::loadFile()
{
	const std::string path = getConfigDirectory() + "/es_settings.cfg";

	if(!boost::filesystem::exists(path)) {
		return false;
	}

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(path.c_str());
	if(!result) {
		LOG(LogError) << "Could not parse Settings file!\n   " << result.description();
		return false;
	}

	for(pugi::xml_node node = doc.child("bool"); node; node = node.next_sibling("bool")) {
		setBool(node.attribute("name").as_string(), node.attribute("value").as_bool());
	}
	for(pugi::xml_node node = doc.child("int"); node; node = node.next_sibling("int")) {
		setInt(node.attribute("name").as_string(), node.attribute("value").as_int());
	}
	for(pugi::xml_node node = doc.child("float"); node; node = node.next_sibling("float")) {
		setFloat(node.attribute("name").as_string(), node.attribute("value").as_float());
	}
	for(pugi::xml_node node = doc.child("string"); node; node = node.next_sibling("string")) {
		setString(node.attribute("name").as_string(), node.attribute("value").as_string());
	}
	return true;
}

//Print a warning message if the setting we're trying to get doesn't already exist in the map, then return the value in the map.
#define SETTINGS_GETSET(type, mapName, getMethodName, setMethodName) type Settings::getMethodName(const std::string& name) \
{ \
	if(mapName.find(name) == mapName.end()) \
	{ \
		LOG(LogError) << "Tried to use unset setting " << name << "!"; \
	} \
	return mapName[name]; \
} \
void Settings::setMethodName(const std::string& name, type value) \
{ \
	mapName[name] = value; \
}

SETTINGS_GETSET(bool, mBoolMap, getBool, setBool);
SETTINGS_GETSET(int, mIntMap, getInt, setInt);
SETTINGS_GETSET(float, mFloatMap, getFloat, setFloat);
SETTINGS_GETSET(const std::string&, mStringMap, getString, setString);
