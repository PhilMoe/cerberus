#!/bin/bash

# Change to the directory where this script is located
cd "$(dirname "$0")"

# URLs for iOS SDKs (as of June 2025)
ADMOB_IOS_URL="https://dl.google.com/googleadmobadssdk/googlemobileadssdkios.zip"

# Local file names
ADMOB_ZIP="./googlemobileads_ios.zip"

# Directory to store the SDKs
SDK_DIR="./admob-sdk_ios"

# delete existing SDK directory if it exists
if [ -d "$SDK_DIR" ]; then
    echo "🗑️ Removing existing SDK directory: $SDK_DIR"
    rm -rf "$SDK_DIR"
fi

# Create the SDK directory
mkdir -p "$SDK_DIR"

echo "🔽 Downloading Google Mobile Ads (AdMob) SDK for iOS..."
curl -L "$ADMOB_IOS_URL" -o "$ADMOB_ZIP"

echo "📦 Extracting AdMob SDK..."
unzip -o "$ADMOB_ZIP" -d "$SDK_DIR"
if [ $? -ne 0 ]; then
    echo "❌ Failed to extract the AdMob SDK. Please check the downloaded file."
    exit 1
fi

echo "🗑️ Removing the downloaded zip file..."
rm -f "$ADMOB_ZIP"

echo "✅ All iOS SDKs have been downloaded and extracted into: $SDK_DIR"
