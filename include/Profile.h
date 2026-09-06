#pragma once
#include <string>
#include <filesystem>

class Profile {
public:
	Profile();
	explicit Profile(const std::string& name);

	// Profile ID
	const std::string& getId() const;
	void setId(const std::string& id);

	// The Profiles Name
	const std::string& getName() const;
	void setName(const std::string& name);

	// Profile-Picture & Banner
	const std::filesystem::path& getAvatar() const;
	void setAvatar(const std::filesystem::path& avatar);

	const std::filesystem::path& getBanner() const;
	void setBanner(const std::filesystem::path& banner);

	// Theme-Colors
	const std::string& getPrimaryColor() const;
	void setPrimaryColor(const std::string& color);

	const std::string& getSecondaryColor() const;
	void setSecondaryColor(const std::string& color);

	// Nameplate
	const std::string& getNameplate() const;
	void setNameplate(const std::string& nameplate);

	// Avatar Decoration
	const std::string& getAvatarDecoration() const;
	void setAvatarDecoration(const std::string& decoration);

	// Display Name Style
	const std::string& getDisplayNameStyle() const;
	void setDisplayNameStyle(const std::string& displayNameStyle);

	// Profile Effect
	const std::string& getProfileEffect() const;
	void setProfileEffect(const std::string& effect);

	// Profile Frame
	const std::string& getProfileFrame() const;
	void setProfileFrame(const std::string& frame);

private:

	std::string m_id;

	std::string m_name;

	std::filesystem::path m_avatar;
	std::filesystem::path m_banner;

	std::string m_primaryColor;
	std::string m_secondaryColor;

	std::string m_nameplate;
	std::string m_avatarDecoration;
	std::string m_displayNameStyle;
	std::string m_profileEffect;
	std::string m_profileFrame;
};