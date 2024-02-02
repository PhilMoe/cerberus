
#
#This script generates a 'key' that can be used to sign android apps.
#
#IMPORTANT: Each app should have it's own key which should not change once generated.
#
#If you run this script and use 'password' as the password for the keystore and alias, it can be used 'as is' simply by copying the output
#release-key.keystore file to your app's main source dir and using #ANDROID_SIGN_APP=True.
#
#If you change the name of the keystore file or use a password other than 'password' you will need to modify the following app config settings (defaults shown):
#
#ANDROID_KEY_STORE="../../release-key.keystore"
#ANDROID_KEY_ALIAS="release-key-alias"
#ANDROID_KEY_STORE_PASSWORD="password"
#ANDROID_KEY_ALIAS_PASSWORD="password"
#

#
#Generate key...
#
keytool -genkey -v -keystore release-key.keystore -alias release-key-alias -keyalg RSA -keysize 2048 -validity 10000
