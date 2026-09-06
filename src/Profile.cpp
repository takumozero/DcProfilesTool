#include "Profile.h"

// Constructors

Profile::Profile()
{
}

Profile::Profile(const std::string& name)
    : m_name(name)
{
}

// ID
const std::string& Profile::getId() const
{
    return m_id;
}

void Profile::setId(const std::string& id)
{
    m_id = id;
}


// Name

const std::string& Profile::getName() const
{
    return m_name;
}

void Profile::setName(const std::string& name)
{
    m_name = name;
}


// Avatar

const std::filesystem::path& Profile::getAvatar() const
{
    return m_avatar;
}

void Profile::setAvatar(const std::filesystem::path& avatar)
{
    m_avatar = avatar;
}


// Banner

const std::filesystem::path& Profile::getBanner() const
{
    return m_banner;
}

void Profile::setBanner(const std::filesystem::path& banner)
{
    m_banner = banner;
}


// Primary color

const std::string& Profile::getPrimaryColor() const
{
    return m_primaryColor;
}

void Profile::setPrimaryColor(const std::string& color)
{
    m_primaryColor = color;
}


// Secondary color

const std::string& Profile::getSecondaryColor() const
{
    return m_secondaryColor;
}

void Profile::setSecondaryColor(const std::string& color)
{
    m_secondaryColor = color;
}


// Nameplate

const std::string& Profile::getNameplate() const
{
    return m_nameplate;
}

void Profile::setNameplate(const std::string& nameplate)
{
    m_nameplate = nameplate;
}


// Avatar decoration

const std::string& Profile::getAvatarDecoration() const
{
    return m_avatarDecoration;
}

void Profile::setAvatarDecoration(const std::string& decoration)
{
    m_avatarDecoration = decoration;
}


// Display name style

const std::string& Profile::getDisplayNameStyle() const
{
    return m_displayNameStyle;
}

void Profile::setDisplayNameStyle(const std::string& style)
{
    m_displayNameStyle = style;
}


// Profile effect

const std::string& Profile::getProfileEffect() const
{
    return m_profileEffect;
}

void Profile::setProfileEffect(const std::string& effect)
{
    m_profileEffect = effect;
}


// Profile frame

const std::string& Profile::getProfileFrame() const
{
    return m_profileFrame;
}

void Profile::setProfileFrame(const std::string& frame)
{
    m_profileFrame = frame;
}