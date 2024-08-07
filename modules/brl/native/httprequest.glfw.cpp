
// ***** HttpRequest.h *****

// Libcurl for Linux will be from a system repository when installed
#ifdef __linux__
#include <curl/curl.h>
#include "curl/include/curl.h"
#else
#include "curl/include/curl.h"
#endif

#include <string>
#include <vector>

class BBHttpRequest : public BBThread
{
public:

  BBHttpRequest();

  void Open( String req, String url, int timeout, bool httpsVerifyCertificate, bool httpsVerifyHost );
  void SetHeader( String name, String value );
  void Send();
  void SendText( String text, String encoding );
  String ResponseText();
  int Status();
  int BytesReceived();

private:

  CURL *_curl;
  char *_urlc;
  struct curl_slist *_header;
  CURLcode _res;
  std::vector<char> _response;
  long _status;

  char *convertBBString(String string);
  // fancy callback stuff for curl.
  // as curl needs a static function we have to use kind of a trick to use a member here
  static size_t DataCallback( void* buf, size_t size, size_t nmemb, void* userp );
  size_t DataCallbackImpl( void* buf, size_t size, size_t nmemb );
  void Run__UNSAFE__();
};

// ***** HttpRequest.cpp *****

BBHttpRequest::BBHttpRequest(): _status( -1 ), _curl( 0 )
{
}

size_t BBHttpRequest::DataCallback( void* buf, size_t size, size_t nmemb, void* userp )
{
  return static_cast<BBHttpRequest*>( userp )->DataCallbackImpl( buf, size, nmemb );
}

size_t BBHttpRequest::DataCallbackImpl( void* buf, size_t size, size_t nmemb )
{
	size_t plus = size * nmemb;
	const char *src = (const char *)buf;
	for(size_t i=0; i<plus; i++){
		_response.push_back(src[i]);
	}
	return plus;
}

void BBHttpRequest::Open( String req, String url, int timeout, bool httpsVerifyCertificate, bool httpsVerifyHost )
{
  curl_global_init( CURL_GLOBAL_DEFAULT );
  _header = 0;
  _curl = curl_easy_init();
  if( _curl )
  {
    //curl_easy_setopt(_curl, CURLOPT_VERBOSE, 1L);
    _urlc = convertBBString(url);
    curl_easy_setopt( _curl, CURLOPT_WRITEDATA, this );
    curl_easy_setopt( _curl, CURLOPT_WRITEFUNCTION, &BBHttpRequest::DataCallback );
    curl_easy_setopt( _curl, CURLOPT_URL, _urlc );
    curl_easy_setopt( _curl, CURLOPT_NOSIGNAL, 1L );
    curl_easy_setopt( _curl, CURLOPT_TIMEOUT, timeout );
    if( req == "GET" )
      curl_easy_setopt( _curl, CURLOPT_HTTPGET, 1L );
    if( !httpsVerifyCertificate )
      curl_easy_setopt( _curl, CURLOPT_SSL_VERIFYPEER, 0L );
    if( !httpsVerifyHost )
      curl_easy_setopt( _curl, CURLOPT_SSL_VERIFYHOST, 0L );

    free(_urlc);
  }
	
  _response.clear();
	_status = -1;
}

void BBHttpRequest::SetHeader( String name, String value )
{
  String namevalue = name + ": " + value;
  char *nvc = convertBBString(namevalue);
  _header = curl_slist_append( _header, nvc );
  curl_easy_setopt( _curl, CURLOPT_HTTPHEADER, _header );
  free(nvc);
  nvc = 0;
}

void BBHttpRequest::Send()
{
  Start();
}

void BBHttpRequest::SendText( String text, String encoding )
{
  char *textc = convertBBString(text);

  curl_easy_setopt( _curl, CURLOPT_POSTFIELDSIZE, text.Length() );
  curl_easy_setopt( _curl, CURLOPT_COPYPOSTFIELDS, textc );

  free(textc);
  Start();
}

void BBHttpRequest::Run__UNSAFE__()
{
  _res = curl_easy_perform( _curl );
  curl_easy_getinfo( _curl, CURLINFO_RESPONSE_CODE, &_status );
  if( _header )
    curl_slist_free_all( _header );
  curl_easy_cleanup( _curl );
}

String BBHttpRequest::ResponseText(){
	String result(_response.data(), _response.size());
	return result;
}

int BBHttpRequest::Status(){
	return _status;
}

int BBHttpRequest::BytesReceived(){
	return _response.size();
}


char *BBHttpRequest::convertBBString(String string){
	int srclength = string.Length();
	char *result = (char *)malloc(srclength+1);
	std::wstring string_ws(string.Data(), srclength);
	wcstombs(result, string_ws.c_str(), srclength+1);
	return result;
}
