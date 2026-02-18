#!/bin/bash
# ScoreMaster Controller Auto-Update Script

REPO="rbierman/scoremaster"
PACKAGE="scoremaster-controller"

# Get currently installed version
INSTALLED_VER=$(dpkg-query -W -f='${Version}' $PACKAGE 2>/dev/null)

# Fetch latest release info from GitHub
LATEST_RELEASE_JSON=$(curl -s https://api.github.com/repos/$REPO/releases/latest)
LATEST_VER=$(echo "$LATEST_RELEASE_JSON" | jq -r .tag_name | sed 's/v//')

if [ -z "$LATEST_VER" ] || [ "$LATEST_VER" == "null" ]; then
    echo "Error: Could not fetch latest version from GitHub."
    exit 1
fi

if [ "$INSTALLED_VER" != "$LATEST_VER" ]; then
    echo "Update available: $INSTALLED_VER -> $LATEST_VER"
    
    DEB_URL=$(echo "$LATEST_RELEASE_JSON" | jq -r '.assets[] | select(.name | endswith(".deb")) | .browser_download_url')
    
    if [ -z "$DEB_URL" ] || [ "$DEB_URL" == "null" ]; then
        echo "Error: Could not find .deb asset in the latest release."
        exit 1
    fi

    echo "Downloading $DEB_URL..."
    TMP_DEB="/tmp/scoremaster_update.deb"
    curl -L -o "$TMP_DEB" "$DEB_URL"
    
    echo "Installing update..."
    # Using DEBIAN_FRONTEND=noninteractive to ensure it doesn't hang on prompts
    DEBIAN_FRONTEND=noninteractive apt-get install -y "$TMP_DEB"
    
    rm "$TMP_DEB"
    echo "Update complete."
else
    echo "ScoreMaster is up to date ($INSTALLED_VER)."
fi
