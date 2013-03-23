
IFrameworkView ^Direct3DApplicationSource::CreateView(){
    return ref new MonkeyGame();
}

[Platform::MTAThread]
int main( Platform::Array<Platform::String^>^ ){
	auto direct3DApplicationSource=ref new Direct3DApplicationSource();
	CoreApplication::Run( direct3DApplicationSource );
	return 0;
}
