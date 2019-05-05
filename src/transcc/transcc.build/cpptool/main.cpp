
#include "main.h"

//${CONFIG_BEGIN}
#define CFG_BRL_DATABUFFER_IMPLEMENTED 1
#define CFG_BRL_FILESTREAM_IMPLEMENTED 1
#define CFG_BRL_OS_IMPLEMENTED 1
#define CFG_BRL_STREAM_IMPLEMENTED 1
#define CFG_BRL_THREAD_IMPLEMENTED 1
#define CFG_CD 
#define CFG_CONFIG release
#define CFG_CPP_DOUBLE_PRECISION_FLOATS 1
#define CFG_CPP_GC_MODE 1
#define CFG_HOST winnt
#define CFG_LANG cpp
#define CFG_MODPATH 
#define CFG_RELEASE 1
#define CFG_SAFEMODE 0
#define CFG_TARGET stdcpp
//${CONFIG_END}

//${TRANSCODE_BEGIN}

#include <wctype.h>
#include <locale.h>

// C++ Cerberus runtime.
//
// Placed into the public domain 24/02/2011.
// No warranty implied; use at your own risk.

//***** Cerberus Types *****

typedef wchar_t Char;
template<class T> class Array;
class String;
class Object;

#if CFG_CPP_DOUBLE_PRECISION_FLOATS
typedef double Float;
#define FLOAT(X) X
#else
typedef float Float;
#define FLOAT(X) X##f
#endif

void dbg_error( const char *p );

#if !_MSC_VER
#define sprintf_s sprintf
#define sscanf_s sscanf
#endif

//***** GC Config *****

#if CFG_CPP_GC_DEBUG
#define DEBUG_GC 1
#else
#define DEBUG_GC 0
#endif

// GC mode:
//
// 0 = disabled
// 1 = Incremental GC every OnWhatever
// 2 = Incremental GC every allocation
//
#ifndef CFG_CPP_GC_MODE
#define CFG_CPP_GC_MODE 1
#endif

//How many bytes alloced to trigger GC
//
#ifndef CFG_CPP_GC_TRIGGER
#define CFG_CPP_GC_TRIGGER 8*1024*1024
#endif

//GC_MODE 2 needs to track locals on a stack - this may need to be bumped if your app uses a LOT of locals, eg: is heavily recursive...
//
#ifndef CFG_CPP_GC_MAX_LOCALS
#define CFG_CPP_GC_MAX_LOCALS 8192
#endif

// ***** GC *****

#if _WIN32

int gc_micros(){
	static int f;
	static LARGE_INTEGER pcf;
	if( !f ){
		if( QueryPerformanceFrequency( &pcf ) && pcf.QuadPart>=1000000L ){
			pcf.QuadPart/=1000000L;
			f=1;
		}else{
			f=-1;
		}
	}
	if( f>0 ){
		LARGE_INTEGER pc;
		if( QueryPerformanceCounter( &pc ) ) return pc.QuadPart/pcf.QuadPart;
		f=-1;
	}
	return 0;// timeGetTime()*1000;
}

#elif __APPLE__

#include <mach/mach_time.h>

int gc_micros(){
	static int f;
	static mach_timebase_info_data_t timeInfo;
	if( !f ){
		mach_timebase_info( &timeInfo );
		timeInfo.denom*=1000L;
		f=1;
	}
	return mach_absolute_time()*timeInfo.numer/timeInfo.denom;
}

#else

int gc_micros(){
	return 0;
}

#endif

#define gc_mark_roots gc_mark

void gc_mark_roots();

struct gc_object;

gc_object *gc_object_alloc( int size );
void gc_object_free( gc_object *p );

struct gc_object{
	gc_object *succ;
	gc_object *pred;
	int flags;
	
	virtual ~gc_object(){
	}
	
	virtual void mark(){
	}
	
	void *operator new( size_t size ){
		return gc_object_alloc( size );
	}
	
	void operator delete( void *p ){
		gc_object_free( (gc_object*)p );
	}
};

gc_object gc_free_list;
gc_object gc_marked_list;
gc_object gc_unmarked_list;
gc_object gc_queued_list;	//doesn't really need to be doubly linked...

int gc_free_bytes;
int gc_marked_bytes;
int gc_alloced_bytes;
int gc_max_alloced_bytes;
int gc_new_bytes;
int gc_markbit=1;

gc_object *gc_cache[8];

void gc_collect_all();
void gc_mark_queued( int n );

#define GC_CLEAR_LIST( LIST ) ((LIST).succ=(LIST).pred=&(LIST))

#define GC_LIST_IS_EMPTY( LIST ) ((LIST).succ==&(LIST))

#define GC_REMOVE_NODE( NODE ){\
(NODE)->pred->succ=(NODE)->succ;\
(NODE)->succ->pred=(NODE)->pred;}

#define GC_INSERT_NODE( NODE,SUCC ){\
(NODE)->pred=(SUCC)->pred;\
(NODE)->succ=(SUCC);\
(SUCC)->pred->succ=(NODE);\
(SUCC)->pred=(NODE);}

void gc_init1(){
	GC_CLEAR_LIST( gc_free_list );
	GC_CLEAR_LIST( gc_marked_list );
	GC_CLEAR_LIST( gc_unmarked_list);
	GC_CLEAR_LIST( gc_queued_list );
}

void gc_init2(){
	gc_mark_roots();
}

#if CFG_CPP_GC_MODE==2

int gc_ctor_nest;
gc_object *gc_locals[CFG_CPP_GC_MAX_LOCALS],**gc_locals_sp=gc_locals;

struct gc_ctor{
	gc_ctor(){ ++gc_ctor_nest; }
	~gc_ctor(){ --gc_ctor_nest; }
};

struct gc_enter{
	gc_object **sp;
	gc_enter():sp(gc_locals_sp){
	}
	~gc_enter(){
#if DEBUG_GC
		static int max_locals;
		int n=gc_locals_sp-gc_locals;
		if( n>max_locals ){
			max_locals=n;
			printf( "max_locals=%i\n",n );
		}
#endif		
		gc_locals_sp=sp;
	}
};

#define GC_CTOR gc_ctor _c;
#define GC_ENTER gc_enter _e;

#else

struct gc_ctor{
};
struct gc_enter{
};

#define GC_CTOR
#define GC_ENTER

#endif

//Can be modified off thread!
static volatile int gc_ext_new_bytes;

#if _MSC_VER
#define atomic_add(P,V) InterlockedExchangeAdd((volatile unsigned int*)P,V)			//(*(P)+=(V))
#define atomic_sub(P,V) InterlockedExchangeSubtract((volatile unsigned int*)P,V)	//(*(P)-=(V))
#else
#define atomic_add(P,V) __sync_fetch_and_add(P,V)
#define atomic_sub(P,V) __sync_fetch_and_sub(P,V)
#endif

//Careful! May be called off thread!
//
void gc_ext_malloced( int size ){
	atomic_add( &gc_ext_new_bytes,size );
}

void gc_object_free( gc_object *p ){

	int size=p->flags & ~7;
	gc_free_bytes-=size;
	
	if( size<64 ){
		p->succ=gc_cache[size>>3];
		gc_cache[size>>3]=p;
	}else{
		free( p );
	}
}

void gc_flush_free( int size ){

	int t=gc_free_bytes-size;
	if( t<0 ) t=0;
	
	//ignore bytes freed by released strings
	int new_bytes=gc_new_bytes;
	
	while( gc_free_bytes>t ){
	
		gc_object *p=gc_free_list.succ;

		GC_REMOVE_NODE( p );

#if DEBUG_GC
//		printf( "deleting @%p\n",p );fflush( stdout );
//		p->flags|=4;
//		continue;
#endif
		delete p;
	}
	
	gc_new_bytes=new_bytes;
}

gc_object *gc_object_alloc( int size ){

	size=(size+7)&~7;
	
	gc_new_bytes+=size;
	
#if CFG_CPP_GC_MODE==2

	if( !gc_ctor_nest ){

#if DEBUG_GC
		int ms=gc_micros();
#endif
		if( gc_new_bytes+gc_ext_new_bytes>(CFG_CPP_GC_TRIGGER) ){
			atomic_sub( &gc_ext_new_bytes,gc_ext_new_bytes );
			gc_collect_all();
			gc_new_bytes=0;
		}else{
			gc_mark_queued( (long long)(gc_new_bytes)*(gc_alloced_bytes-gc_new_bytes)/(CFG_CPP_GC_TRIGGER)+gc_new_bytes );
		}

#if DEBUG_GC
		ms=gc_micros()-ms;
		if( ms>=100 ) {printf( "gc time:%i\n",ms );fflush( stdout );}
#endif
	}
	
#endif

	gc_flush_free( size );

	gc_object *p;
	if( size<64 && (p=gc_cache[size>>3]) ){
		gc_cache[size>>3]=p->succ;
	}else{
		p=(gc_object*)malloc( size );
	}
	
	p->flags=size|gc_markbit;
	GC_INSERT_NODE( p,&gc_unmarked_list );

	gc_alloced_bytes+=size;
	if( gc_alloced_bytes>gc_max_alloced_bytes ) gc_max_alloced_bytes=gc_alloced_bytes;
	
#if CFG_CPP_GC_MODE==2
	*gc_locals_sp++=p;
#endif

	return p;
}

#if DEBUG_GC

template<class T> gc_object *to_gc_object( T *t ){
	gc_object *p=dynamic_cast<gc_object*>(t);
	if( p && (p->flags & 4) ){
		printf( "gc error : object already deleted @%p\n",p );fflush( stdout );
		exit(-1);
	}
	return p;
}

#else

#define to_gc_object(t) dynamic_cast<gc_object*>(t)

#endif

template<class T> T *gc_retain( T *t ){
#if CFG_CPP_GC_MODE==2
	*gc_locals_sp++=to_gc_object( t );
#endif
	return t;
}

template<class T> void gc_mark( T *t ){

	gc_object *p=to_gc_object( t );
	
	if( p && (p->flags & 3)==gc_markbit ){
		p->flags^=1;
		GC_REMOVE_NODE( p );
		GC_INSERT_NODE( p,&gc_marked_list );
		gc_marked_bytes+=(p->flags & ~7);
		p->mark();
	}
}

template<class T> void gc_mark_q( T *t ){

	gc_object *p=to_gc_object( t );
	
	if( p && (p->flags & 3)==gc_markbit ){
		p->flags^=1;
		GC_REMOVE_NODE( p );
		GC_INSERT_NODE( p,&gc_queued_list );
	}
}

template<class T,class V> void gc_assign( T *&lhs,V *rhs ){

	gc_object *p=to_gc_object( rhs );
	
	if( p && (p->flags & 3)==gc_markbit ){
		p->flags^=1;
		GC_REMOVE_NODE( p );
		GC_INSERT_NODE( p,&gc_queued_list );
	}
	lhs=rhs;
}

void gc_mark_locals(){

#if CFG_CPP_GC_MODE==2
	for( gc_object **pp=gc_locals;pp!=gc_locals_sp;++pp ){
		gc_object *p=*pp;
		if( p && (p->flags & 3)==gc_markbit ){
			p->flags^=1;
			GC_REMOVE_NODE( p );
			GC_INSERT_NODE( p,&gc_marked_list );
			gc_marked_bytes+=(p->flags & ~7);
			p->mark();
		}
	}
#endif	
}

void gc_mark_queued( int n ){
	while( gc_marked_bytes<n && !GC_LIST_IS_EMPTY( gc_queued_list ) ){
		gc_object *p=gc_queued_list.succ;
		GC_REMOVE_NODE( p );
		GC_INSERT_NODE( p,&gc_marked_list );
		gc_marked_bytes+=(p->flags & ~7);
		p->mark();
	}
}

void gc_validate_list( gc_object &list,const char *msg ){
	gc_object *node=list.succ;
	while( node ){
		if( node==&list ) return;
		if( !node->pred ) break;
		if( node->pred->succ!=node ) break;
		node=node->succ;
	}
	if( msg ){
		puts( msg );fflush( stdout );
	}
	puts( "LIST ERROR!" );
	exit(-1);
}

//returns reclaimed bytes
void gc_sweep(){

	int reclaimed_bytes=gc_alloced_bytes-gc_marked_bytes;
	
	if( reclaimed_bytes ){
	
		//append unmarked list to end of free list
		gc_object *head=gc_unmarked_list.succ;
		gc_object *tail=gc_unmarked_list.pred;
		gc_object *succ=&gc_free_list;
		gc_object *pred=succ->pred;
		
		head->pred=pred;
		tail->succ=succ;
		pred->succ=head;
		succ->pred=tail;
		
		gc_free_bytes+=reclaimed_bytes;
	}

	//move marked to unmarked.
	if( GC_LIST_IS_EMPTY( gc_marked_list ) ){
		GC_CLEAR_LIST( gc_unmarked_list );
	}else{
		gc_unmarked_list.succ=gc_marked_list.succ;
		gc_unmarked_list.pred=gc_marked_list.pred;
		gc_unmarked_list.succ->pred=gc_unmarked_list.pred->succ=&gc_unmarked_list;
		GC_CLEAR_LIST( gc_marked_list );
	}
	
	//adjust sizes
	gc_alloced_bytes=gc_marked_bytes;
	gc_marked_bytes=0;
	gc_markbit^=1;
}

void gc_collect_all(){

//	puts( "Mark locals" );
	gc_mark_locals();

//	puts( "Marked queued" );
	gc_mark_queued( 0x7fffffff );

//	puts( "Sweep" );
	gc_sweep();

//	puts( "Mark roots" );
	gc_mark_roots();

#if DEBUG_GC
	gc_validate_list( gc_marked_list,"Validating gc_marked_list"  );
	gc_validate_list( gc_unmarked_list,"Validating gc_unmarked_list"  );
	gc_validate_list( gc_free_list,"Validating gc_free_list" );
#endif

}

void gc_collect(){
	
#if CFG_CPP_GC_MODE==1

#if DEBUG_GC
	int ms=gc_micros();
#endif

	if( gc_new_bytes+gc_ext_new_bytes>(CFG_CPP_GC_TRIGGER) ){
		atomic_sub( &gc_ext_new_bytes,gc_ext_new_bytes );
		gc_collect_all();
		gc_new_bytes=0;
	}else{
		gc_mark_queued( (long long)(gc_new_bytes)*(gc_alloced_bytes-gc_new_bytes)/(CFG_CPP_GC_TRIGGER)+gc_new_bytes );
	}

#if DEBUG_GC
	ms=gc_micros()-ms;
//	if( ms>=100 ) {printf( "gc time:%i\n",ms );fflush( stdout );}
	if( ms>10 ) {printf( "gc time:%i\n",ms );fflush( stdout );}
#endif

#endif
}

// ***** Array *****

template<class T> T *t_memcpy( T *dst,const T *src,int n ){
	memcpy( dst,src,n*sizeof(T) );
	return dst+n;
}

template<class T> T *t_memset( T *dst,int val,int n ){
	memset( dst,val,n*sizeof(T) );
	return dst+n;
}

template<class T> int t_memcmp( const T *x,const T *y,int n ){
	return memcmp( x,y,n*sizeof(T) );
}

template<class T> int t_strlen( const T *p ){
	const T *q=p++;
	while( *q++ ){}
	return q-p;
}

template<class T> T *t_create( int n,T *p ){
	t_memset( p,0,n );
	return p+n;
}

template<class T> T *t_create( int n,T *p,const T *q ){
	t_memcpy( p,q,n );
	return p+n;
}

template<class T> void t_destroy( int n,T *p ){
}

template<class T> void gc_mark_elements( int n,T *p ){
}

template<class T> void gc_mark_elements( int n,T **p ){
	for( int i=0;i<n;++i ) gc_mark( p[i] );
}

template<class T> class Array{
public:
	Array():rep( &nullRep ){
	}

	//Uses default...
//	Array( const Array<T> &t )...
	
	Array( int length ):rep( Rep::alloc( length ) ){
		t_create( rep->length,rep->data );
	}
	
	Array( const T *p,int length ):rep( Rep::alloc(length) ){
		t_create( rep->length,rep->data,p );
	}
	
	~Array(){
	}

	//Uses default...
//	Array &operator=( const Array &t )...
	
	int Length()const{ 
		return rep->length; 
	}
	
	T &At( int index ){
		if( index<0 || index>=rep->length ) dbg_error( "Array index out of range" );
		return rep->data[index]; 
	}
	
	const T &At( int index )const{
		if( index<0 || index>=rep->length ) dbg_error( "Array index out of range" );
		return rep->data[index]; 
	}
	
	T &operator[]( int index ){
		return rep->data[index]; 
	}

	const T &operator[]( int index )const{
		return rep->data[index]; 
	}
	
	Array Slice( int from,int term )const{
		int len=rep->length;
		if( from<0 ){ 
			from+=len;
			if( from<0 ) from=0;
		}else if( from>len ){
			from=len;
		}
		if( term<0 ){
			term+=len;
		}else if( term>len ){
			term=len;
		}
		if( term<=from ) return Array();
		return Array( rep->data+from,term-from );
	}

	Array Slice( int from )const{
		return Slice( from,rep->length );
	}
	
	Array Resize( int newlen )const{
		if( newlen<=0 ) return Array();
		int n=rep->length;
		if( newlen<n ) n=newlen;
		Rep *p=Rep::alloc( newlen );
		T *q=p->data;
		q=t_create( n,q,rep->data );
		q=t_create( (newlen-n),q );
		return Array( p );
	}
	
private:
	struct Rep : public gc_object{
		int length;
		T data[0];
		
		Rep():length(0){
			flags=3;
		}
		
		Rep( int length ):length(length){
		}
		
		~Rep(){
			t_destroy( length,data );
		}
		
		void mark(){
			gc_mark_elements( length,data );
		}
		
		static Rep *alloc( int length ){
			if( !length ) return &nullRep;
			void *p=gc_object_alloc( sizeof(Rep)+length*sizeof(T) );
			return ::new(p) Rep( length );
		}
		
	};
	Rep *rep;
	
	static Rep nullRep;
	
	template<class C> friend void gc_mark( Array<C> t );
	template<class C> friend void gc_mark_q( Array<C> t );
	template<class C> friend Array<C> gc_retain( Array<C> t );
	template<class C> friend void gc_assign( Array<C> &lhs,Array<C> rhs );
	template<class C> friend void gc_mark_elements( int n,Array<C> *p );
	
	Array( Rep *rep ):rep(rep){
	}
};

template<class T> typename Array<T>::Rep Array<T>::nullRep;

template<class T> Array<T> *t_create( int n,Array<T> *p ){
	for( int i=0;i<n;++i ) *p++=Array<T>();
	return p;
}

template<class T> Array<T> *t_create( int n,Array<T> *p,const Array<T> *q ){
	for( int i=0;i<n;++i ) *p++=*q++;
	return p;
}

template<class T> void gc_mark( Array<T> t ){
	gc_mark( t.rep );
}

template<class T> void gc_mark_q( Array<T> t ){
	gc_mark_q( t.rep );
}

template<class T> Array<T> gc_retain( Array<T> t ){
#if CFG_CPP_GC_MODE==2
	gc_retain( t.rep );
#endif
	return t;
}

template<class T> void gc_assign( Array<T> &lhs,Array<T> rhs ){
	gc_mark( rhs.rep );
	lhs=rhs;
}

template<class T> void gc_mark_elements( int n,Array<T> *p ){
	for( int i=0;i<n;++i ) gc_mark( p[i].rep );
}
		
// ***** String *****

static const char *_str_load_err;

class String{
public:
	String():rep( &nullRep ){
	}
	
	String( const String &t ):rep( t.rep ){
		rep->retain();
	}

	String( int n ){
		char buf[256];
		sprintf_s( buf,"%i",n );
		rep=Rep::alloc( t_strlen(buf) );
		for( int i=0;i<rep->length;++i ) rep->data[i]=buf[i];
	}
	
	String( Float n ){
		char buf[256];
		
		//would rather use snprintf, but it's doing weird things in MingW.
		//
		sprintf_s( buf,"%.17lg",n );
		//
		char *p;
		for( p=buf;*p;++p ){
			if( *p=='.' || *p=='e' ) break;
		}
		if( !*p ){
			*p++='.';
			*p++='0';
			*p=0;
		}

		rep=Rep::alloc( t_strlen(buf) );
		for( int i=0;i<rep->length;++i ) rep->data[i]=buf[i];
	}

	String( Char ch,int length ):rep( Rep::alloc(length) ){
		for( int i=0;i<length;++i ) rep->data[i]=ch;
	}

	String( const Char *p ):rep( Rep::alloc(t_strlen(p)) ){
		t_memcpy( rep->data,p,rep->length );
	}

	String( const Char *p,int length ):rep( Rep::alloc(length) ){
		t_memcpy( rep->data,p,rep->length );
	}
	
#if __OBJC__	
	String( NSString *nsstr ):rep( Rep::alloc([nsstr length]) ){
		unichar *buf=(unichar*)malloc( rep->length * sizeof(unichar) );
		[nsstr getCharacters:buf range:NSMakeRange(0,rep->length)];
		for( int i=0;i<rep->length;++i ) rep->data[i]=buf[i];
		free( buf );
	}
#endif

#if __cplusplus_winrt
	String( Platform::String ^str ):rep( Rep::alloc(str->Length()) ){
		for( int i=0;i<rep->length;++i ) rep->data[i]=str->Data()[i];
	}
#endif

	~String(){
		rep->release();
	}
	
	template<class C> String( const C *p ):rep( Rep::alloc(t_strlen(p)) ){
		for( int i=0;i<rep->length;++i ) rep->data[i]=p[i];
	}
	
	template<class C> String( const C *p,int length ):rep( Rep::alloc(length) ){
		for( int i=0;i<rep->length;++i ) rep->data[i]=p[i];
	}
	
	String Copy()const{
		Rep *crep=Rep::alloc( rep->length );
		t_memcpy( crep->data,rep->data,rep->length );
		return String( crep );
	}
	
	int Length()const{
		return rep->length;
	}
	
	const Char *Data()const{
		return rep->data;
	}
	
	Char At( int index )const{
		if( index<0 || index>=rep->length ) dbg_error( "Character index out of range" );
		return rep->data[index]; 
	}
	
	Char operator[]( int index )const{
		return rep->data[index];
	}
	
	String &operator=( const String &t ){
		t.rep->retain();
		rep->release();
		rep=t.rep;
		return *this;
	}
	
	String &operator+=( const String &t ){
		return operator=( *this+t );
	}
	
	int Compare( const String &t )const{
		int n=rep->length<t.rep->length ? rep->length : t.rep->length;
		for( int i=0;i<n;++i ){
			if( int q=(int)(rep->data[i])-(int)(t.rep->data[i]) ) return q;
		}
		return rep->length-t.rep->length;
	}
	
	bool operator==( const String &t )const{
		return rep->length==t.rep->length && t_memcmp( rep->data,t.rep->data,rep->length )==0;
	}
	
	bool operator!=( const String &t )const{
		return rep->length!=t.rep->length || t_memcmp( rep->data,t.rep->data,rep->length )!=0;
	}
	
	bool operator<( const String &t )const{
		return Compare( t )<0;
	}
	
	bool operator<=( const String &t )const{
		return Compare( t )<=0;
	}
	
	bool operator>( const String &t )const{
		return Compare( t )>0;
	}
	
	bool operator>=( const String &t )const{
		return Compare( t )>=0;
	}
	
	String operator+( const String &t )const{
		if( !rep->length ) return t;
		if( !t.rep->length ) return *this;
		Rep *p=Rep::alloc( rep->length+t.rep->length );
		Char *q=p->data;
		q=t_memcpy( q,rep->data,rep->length );
		q=t_memcpy( q,t.rep->data,t.rep->length );
		return String( p );
	}
	
	int Find( String find,int start=0 )const{
		if( start<0 ) start=0;
		while( start+find.rep->length<=rep->length ){
			if( !t_memcmp( rep->data+start,find.rep->data,find.rep->length ) ) return start;
			++start;
		}
		return -1;
	}
	
	int FindLast( String find )const{
		int start=rep->length-find.rep->length;
		while( start>=0 ){
			if( !t_memcmp( rep->data+start,find.rep->data,find.rep->length ) ) return start;
			--start;
		}
		return -1;
	}
	
	int FindLast( String find,int start )const{
		if( start>rep->length-find.rep->length ) start=rep->length-find.rep->length;
		while( start>=0 ){
			if( !t_memcmp( rep->data+start,find.rep->data,find.rep->length ) ) return start;
			--start;
		}
		return -1;
	}
	
	String Trim()const{
		int i=0,i2=rep->length;
		while( i<i2 && rep->data[i]<=32 ) ++i;
		while( i2>i && rep->data[i2-1]<=32 ) --i2;
		if( i==0 && i2==rep->length ) return *this;
		return String( rep->data+i,i2-i );
	}

	Array<String> Split( String sep )const{
	
		if( !sep.rep->length ){
			Array<String> bits( rep->length );
			for( int i=0;i<rep->length;++i ){
				bits[i]=String( (Char)(*this)[i],1 );
			}
			return bits;
		}
		
		int i=0,i2,n=1;
		while( (i2=Find( sep,i ))!=-1 ){
			++n;
			i=i2+sep.rep->length;
		}
		Array<String> bits( n );
		if( n==1 ){
			bits[0]=*this;
			return bits;
		}
		i=0;n=0;
		while( (i2=Find( sep,i ))!=-1 ){
			bits[n++]=Slice( i,i2 );
			i=i2+sep.rep->length;
		}
		bits[n]=Slice( i );
		return bits;
	}

	String Join( Array<String> bits )const{
		if( bits.Length()==0 ) return String();
		if( bits.Length()==1 ) return bits[0];
		int newlen=rep->length * (bits.Length()-1);
		for( int i=0;i<bits.Length();++i ){
			newlen+=bits[i].rep->length;
		}
		Rep *p=Rep::alloc( newlen );
		Char *q=p->data;
		q=t_memcpy( q,bits[0].rep->data,bits[0].rep->length );
		for( int i=1;i<bits.Length();++i ){
			q=t_memcpy( q,rep->data,rep->length );
			q=t_memcpy( q,bits[i].rep->data,bits[i].rep->length );
		}
		return String( p );
	}

	String Replace( String find,String repl )const{
		int i=0,i2,newlen=0;
		while( (i2=Find( find,i ))!=-1 ){
			newlen+=(i2-i)+repl.rep->length;
			i=i2+find.rep->length;
		}
		if( !i ) return *this;
		newlen+=rep->length-i;
		Rep *p=Rep::alloc( newlen );
		Char *q=p->data;
		i=0;
		while( (i2=Find( find,i ))!=-1 ){
			q=t_memcpy( q,rep->data+i,i2-i );
			q=t_memcpy( q,repl.rep->data,repl.rep->length );
			i=i2+find.rep->length;
		}
		q=t_memcpy( q,rep->data+i,rep->length-i );
		return String( p );
	}

	String ToLower()const{
		for( int i=0;i<rep->length;++i ){
			Char t=towlower( rep->data[i] );
			if( t==rep->data[i] ) continue;
			Rep *p=Rep::alloc( rep->length );
			Char *q=p->data;
			t_memcpy( q,rep->data,i );
			for( q[i++]=t;i<rep->length;++i ){
				q[i]=towlower( rep->data[i] );
			}
			return String( p );
		}
		return *this;
	}

	String ToUpper()const{
		for( int i=0;i<rep->length;++i ){
			Char t=towupper( rep->data[i] );
			if( t==rep->data[i] ) continue;
			Rep *p=Rep::alloc( rep->length );
			Char *q=p->data;
			t_memcpy( q,rep->data,i );
			for( q[i++]=t;i<rep->length;++i ){
				q[i]=towupper( rep->data[i] );
			}
			return String( p );
		}
		return *this;
	}
	
	bool Contains( String sub )const{
		return Find( sub )!=-1;
	}

	bool StartsWith( String sub )const{
		return sub.rep->length<=rep->length && !t_memcmp( rep->data,sub.rep->data,sub.rep->length );
	}

	bool EndsWith( String sub )const{
		return sub.rep->length<=rep->length && !t_memcmp( rep->data+rep->length-sub.rep->length,sub.rep->data,sub.rep->length );
	}
	
	String Slice( int from,int term )const{
		int len=rep->length;
		if( from<0 ){
			from+=len;
			if( from<0 ) from=0;
		}else if( from>len ){
			from=len;
		}
		if( term<0 ){
			term+=len;
		}else if( term>len ){
			term=len;
		}
		if( term<from ) return String();
		if( from==0 && term==len ) return *this;
		return String( rep->data+from,term-from );
	}

	String Slice( int from )const{
		return Slice( from,rep->length );
	}
	
	Array<int> ToChars()const{
		Array<int> chars( rep->length );
		for( int i=0;i<rep->length;++i ) chars[i]=rep->data[i];
		return chars;
	}
	
	int ToInt()const{
		char buf[64];
		return atoi( ToCString<char>( buf,sizeof(buf) ) );
	}
	
	Float ToFloat()const{
		char buf[256];
		return atof( ToCString<char>( buf,sizeof(buf) ) );
	}

	template<class C> class CString{
		struct Rep{
			int refs;
			C data[1];
		};
		Rep *_rep;
		static Rep _nul;
	public:
		template<class T> CString( const T *data,int length ){
			_rep=(Rep*)malloc( length*sizeof(C)+sizeof(Rep) );
			_rep->refs=1;
			_rep->data[length]=0;
			for( int i=0;i<length;++i ){
				_rep->data[i]=(C)data[i];
			}
		}
		CString():_rep( new Rep ){
			_rep->refs=1;
		}
		CString( const CString &c ):_rep(c._rep){
			++_rep->refs;
		}
		~CString(){
			if( !--_rep->refs ) free( _rep );
		}
		CString &operator=( const CString &c ){
			++c._rep->refs;
			if( !--_rep->refs ) free( _rep );
			_rep=c._rep;
			return *this;
		}
		operator const C*()const{ 
			return _rep->data;
		}
	};
	
	template<class C> CString<C> ToCString()const{
		return CString<C>( rep->data,rep->length );
	}

	template<class C> C *ToCString( C *p,int length )const{
		if( --length>rep->length ) length=rep->length;
		for( int i=0;i<length;++i ) p[i]=rep->data[i];
		p[length]=0;
		return p;
	}
	
#if __OBJC__	
	NSString *ToNSString()const{
		return [NSString stringWithCharacters:ToCString<unichar>() length:rep->length];
	}
#endif

#if __cplusplus_winrt
	Platform::String ^ToWinRTString()const{
		return ref new Platform::String( rep->data,rep->length );
	}
#endif
	CString<char> ToUtf8()const{
		std::vector<unsigned char> buf;
		Save( buf );
		return CString<char>( &buf[0],buf.size() );
	}

	bool Save( FILE *fp )const{
		std::vector<unsigned char> buf;
		Save( buf );
		return buf.size() ? fwrite( &buf[0],1,buf.size(),fp )==buf.size() : true;
	}
	
	void Save( std::vector<unsigned char> &buf )const{
	
		Char *p=rep->data;
		Char *e=p+rep->length;
		
		while( p<e ){
			Char c=*p++;
			if( c<0x80 ){
				buf.push_back( c );
			}else if( c<0x800 ){
				buf.push_back( 0xc0 | (c>>6) );
				buf.push_back( 0x80 | (c & 0x3f) );
			}else{
				buf.push_back( 0xe0 | (c>>12) );
				buf.push_back( 0x80 | ((c>>6) & 0x3f) );
				buf.push_back( 0x80 | (c & 0x3f) );
			}
		}
	}
	
	static String FromChars( Array<int> chars ){
		int n=chars.Length();
		Rep *p=Rep::alloc( n );
		for( int i=0;i<n;++i ){
			p->data[i]=chars[i];
		}
		return String( p );
	}

	static String Load( FILE *fp ){
		unsigned char tmp[4096];
		std::vector<unsigned char> buf;
		for(;;){
			int n=fread( tmp,1,4096,fp );
			if( n>0 ) buf.insert( buf.end(),tmp,tmp+n );
			if( n!=4096 ) break;
		}
		return buf.size() ? String::Load( &buf[0],buf.size() ) : String();
	}
	
	static String Load( unsigned char *p,int n ){
	
		_str_load_err=0;
		
		unsigned char *e=p+n;
		std::vector<Char> chars;
		
		int t0=n>0 ? p[0] : -1;
		int t1=n>1 ? p[1] : -1;

		if( t0==0xfe && t1==0xff ){
			p+=2;
			while( p<e-1 ){
				int c=*p++;
				chars.push_back( (c<<8)|*p++ );
			}
		}else if( t0==0xff && t1==0xfe ){
			p+=2;
			while( p<e-1 ){
				int c=*p++;
				chars.push_back( (*p++<<8)|c );
			}
		}else{
			int t2=n>2 ? p[2] : -1;
			if( t0==0xef && t1==0xbb && t2==0xbf ) p+=3;
			unsigned char *q=p;
			bool fail=false;
			while( p<e ){
				unsigned int c=*p++;
				if( c & 0x80 ){
					if( (c & 0xe0)==0xc0 ){
						if( p>=e || (p[0] & 0xc0)!=0x80 ){
							fail=true;
							break;
						}
						c=((c & 0x1f)<<6) | (p[0] & 0x3f);
						p+=1;
					}else if( (c & 0xf0)==0xe0 ){
						if( p+1>=e || (p[0] & 0xc0)!=0x80 || (p[1] & 0xc0)!=0x80 ){
							fail=true;
							break;
						}
						c=((c & 0x0f)<<12) | ((p[0] & 0x3f)<<6) | (p[1] & 0x3f);
						p+=2;
					}else{
						fail=true;
						break;
					}
				}
				chars.push_back( c );
			}
			if( fail ){
				_str_load_err="Invalid UTF-8";
				return String( q,n );
			}
		}
		return chars.size() ? String( &chars[0],chars.size() ) : String();
	}

private:
	
	struct Rep{
		int refs;
		int length;
		Char data[0];
		
		Rep():refs(1),length(0){
		}
		
		Rep( int length ):refs(1),length(length){
		}
		
		void retain(){
			++refs;
		}
		
		void release(){
			if( --refs || this==&nullRep ) return;
			gc_new_bytes-=sizeof(Rep)+length*sizeof(Char);
			free( this );
		}

		static Rep *alloc( int length ){
			if( !length ) return &nullRep;
			void *p=malloc( sizeof(Rep)+length*sizeof(Char) );
			gc_new_bytes+=sizeof(Rep)+length*sizeof(Char);
			return new(p) Rep( length );
		}
	};
	Rep *rep;
	
	static Rep nullRep;
	
	String( Rep *rep ):rep(rep){
	}
};

String::Rep String::nullRep;

String *t_create( int n,String *p ){
	for( int i=0;i<n;++i ) new( &p[i] ) String();
	return p+n;
}

String *t_create( int n,String *p,const String *q ){
	for( int i=0;i<n;++i ) new( &p[i] ) String( q[i] );
	return p+n;
}

void t_destroy( int n,String *p ){
	for( int i=0;i<n;++i ) p[i].~String();
}

// ***** Object *****

String dbg_stacktrace();

class Object : public gc_object{
public:
	virtual bool Equals( Object *obj ){
		return this==obj;
	}
	
	virtual int Compare( Object *obj ){
		return (char*)this-(char*)obj;
	}
	
	virtual String debug(){
		return "+Object\n";
	}
};

class ThrowableObject : public Object{
#ifndef NDEBUG
public:
	String stackTrace;
	ThrowableObject():stackTrace( dbg_stacktrace() ){}
#endif
};

struct gc_interface{
	virtual ~gc_interface(){}
};

//***** Debugger *****

//#define Error bbError
//#define Print bbPrint

int bbPrint( String t );

#define dbg_stream stderr

#if _MSC_VER
#define dbg_typeof decltype
#else
#define dbg_typeof __typeof__
#endif 

struct dbg_func;
struct dbg_var_type;

static int dbg_suspend;
static int dbg_stepmode;

const char *dbg_info;
String dbg_exstack;

static void *dbg_var_buf[65536*3];
static void **dbg_var_ptr=dbg_var_buf;

static dbg_func *dbg_func_buf[1024];
static dbg_func **dbg_func_ptr=dbg_func_buf;

String dbg_type( bool *p ){
	return "Bool";
}

String dbg_type( int *p ){
	return "Int";
}

String dbg_type( Float *p ){
	return "Float";
}

String dbg_type( String *p ){
	return "String";
}

template<class T> String dbg_type( T **p ){
	return "Object";
}

template<class T> String dbg_type( Array<T> *p ){
	return dbg_type( &(*p)[0] )+"[]";
}

String dbg_value( bool *p ){
	return *p ? "True" : "False";
}

String dbg_value( int *p ){
	return String( *p );
}

String dbg_value( Float *p ){
	return String( *p );
}

String dbg_value( String *p ){
	String t=*p;
	if( t.Length()>100 ) t=t.Slice( 0,100 )+"...";
	t=t.Replace( "\"","~q" );
	t=t.Replace( "\t","~t" );
	t=t.Replace( "\n","~n" );
	t=t.Replace( "\r","~r" );
	return String("\"")+t+"\"";
}

template<class T> String dbg_value( T **t ){
	Object *p=dynamic_cast<Object*>( *t );
	char buf[64];
	sprintf_s( buf,"%p",p );
	return String("@") + (buf[0]=='0' && buf[1]=='x' ? buf+2 : buf );
}

template<class T> String dbg_value( Array<T> *p ){
	String t="[";
	int n=(*p).Length();
	if( n>100 ) n=100;
	for( int i=0;i<n;++i ){
		if( i ) t+=",";
		t+=dbg_value( &(*p)[i] );
	}
	return t+"]";
}

String dbg_ptr_value( void *p ){
	char buf[64];
	sprintf_s( buf,"%p",p );
	return (buf[0]=='0' && buf[1]=='x' ? buf+2 : buf );
}

template<class T> String dbg_decl( const char *id,T *ptr ){
	return String( id )+":"+dbg_type(ptr)+"="+dbg_value(ptr)+"\n";
}

struct dbg_var_type{
	virtual String type( void *p )=0;
	virtual String value( void *p )=0;
};

template<class T> struct dbg_var_type_t : public dbg_var_type{

	String type( void *p ){
		return dbg_type( (T*)p );
	}
	
	String value( void *p ){
		return dbg_value( (T*)p );
	}
	
	static dbg_var_type_t<T> info;
};
template<class T> dbg_var_type_t<T> dbg_var_type_t<T>::info;

struct dbg_blk{
	void **var_ptr;
	
	dbg_blk():var_ptr(dbg_var_ptr){
		if( dbg_stepmode=='l' ) --dbg_suspend;
	}
	
	~dbg_blk(){
		if( dbg_stepmode=='l' ) ++dbg_suspend;
		dbg_var_ptr=var_ptr;
	}
};

struct dbg_func : public dbg_blk{
	const char *id;
	const char *info;

	dbg_func( const char *p ):id(p),info(dbg_info){
		*dbg_func_ptr++=this;
		if( dbg_stepmode=='s' ) --dbg_suspend;
	}
	
	~dbg_func(){
		if( dbg_stepmode=='s' ) ++dbg_suspend;
		--dbg_func_ptr;
		dbg_info=info;
	}
};

int dbg_print( String t ){
	static char *buf;
	static int len;
	int n=t.Length();
	if( n+100>len ){
		len=n+100;
		free( buf );
		buf=(char*)malloc( len );
	}
	buf[n]='\n';
	for( int i=0;i<n;++i ) buf[i]=t[i];
	fwrite( buf,n+1,1,dbg_stream );
	fflush( dbg_stream );
	return 0;
}

void dbg_callstack(){

	void **var_ptr=dbg_var_buf;
	dbg_func **func_ptr=dbg_func_buf;
	
	while( var_ptr!=dbg_var_ptr ){
		while( func_ptr!=dbg_func_ptr && var_ptr==(*func_ptr)->var_ptr ){
			const char *id=(*func_ptr++)->id;
			const char *info=func_ptr!=dbg_func_ptr ? (*func_ptr)->info : dbg_info;
			fprintf( dbg_stream,"+%s;%s\n",id,info );
		}
		void *vp=*var_ptr++;
		const char *nm=(const char*)*var_ptr++;
		dbg_var_type *ty=(dbg_var_type*)*var_ptr++;
		dbg_print( String(nm)+":"+ty->type(vp)+"="+ty->value(vp) );
	}
	while( func_ptr!=dbg_func_ptr ){
		const char *id=(*func_ptr++)->id;
		const char *info=func_ptr!=dbg_func_ptr ? (*func_ptr)->info : dbg_info;
		fprintf( dbg_stream,"+%s;%s\n",id,info );
	}
}

String dbg_stacktrace(){
	if( !dbg_info || !dbg_info[0] ) return "";
	String str=String( dbg_info )+"\n";
	dbg_func **func_ptr=dbg_func_ptr;
	if( func_ptr==dbg_func_buf ) return str;
	while( --func_ptr!=dbg_func_buf ){
		str+=String( (*func_ptr)->info )+"\n";
	}
	return str;
}

void dbg_throw( const char *err ){
	dbg_exstack=dbg_stacktrace();
	throw err;
}

void dbg_stop(){

#if TARGET_OS_IPHONE
	dbg_throw( "STOP" );
#endif

	fprintf( dbg_stream,"{{~~%s~~}}\n",dbg_info );
	dbg_callstack();
	dbg_print( "" );
	
	for(;;){

		char buf[256];
		char *e=fgets( buf,256,stdin );
		if( !e ) exit( -1 );
		
		e=strchr( buf,'\n' );
		if( !e ) exit( -1 );
		
		*e=0;
		
		Object *p;
		
		switch( buf[0] ){
		case '?':
			break;
		case 'r':	//run
			dbg_suspend=0;		
			dbg_stepmode=0;
			return;
		case 's':	//step
			dbg_suspend=1;
			dbg_stepmode='s';
			return;
		case 'e':	//enter func
			dbg_suspend=1;
			dbg_stepmode='e';
			return;
		case 'l':	//leave block
			dbg_suspend=0;
			dbg_stepmode='l';
			return;
		case '@':	//dump object
			p=0;
			sscanf_s( buf+1,"%p",&p );
			if( p ){
				dbg_print( p->debug() );
			}else{
				dbg_print( "" );
			}
			break;
		case 'q':	//quit!
			exit( 0 );
			break;			
		default:
			printf( "????? %s ?????",buf );fflush( stdout );
			exit( -1 );
		}
	}
}

void dbg_error( const char *err ){

#if TARGET_OS_IPHONE
	dbg_throw( err );
#endif

	for(;;){
		bbPrint( String("Cerberus Runtime Error : ")+err );
		bbPrint( dbg_stacktrace() );
		dbg_stop();
	}
}

#define DBG_INFO(X) dbg_info=(X);if( dbg_suspend>0 ) dbg_stop();

#define DBG_ENTER(P) dbg_func _dbg_func(P);

#define DBG_BLOCK() dbg_blk _dbg_blk;

#define DBG_GLOBAL( ID,NAME )	//TODO!

#define DBG_LOCAL( ID,NAME )\
*dbg_var_ptr++=&ID;\
*dbg_var_ptr++=(void*)NAME;\
*dbg_var_ptr++=&dbg_var_type_t<dbg_typeof(ID)>::info;

//**** main ****

int argc;
const char **argv;

Float D2R=0.017453292519943295f;
Float R2D=57.29577951308232f;

int bbPrint( String t ){

	static std::vector<unsigned char> buf;
	buf.clear();
	t.Save( buf );
	buf.push_back( '\n' );
	buf.push_back( 0 );
	
#if __cplusplus_winrt	//winrt?

#if CFG_WINRT_PRINT_ENABLED
	OutputDebugStringA( (const char*)&buf[0] );
#endif

#elif _WIN32			//windows?

	fputs( (const char*)&buf[0],stdout );
	fflush( stdout );

#elif __APPLE__			//macos/ios?

	fputs( (const char*)&buf[0],stdout );
	fflush( stdout );
	
#elif __linux			//linux?

#if CFG_ANDROID_NDK_PRINT_ENABLED
	LOGI( (const char*)&buf[0] );
#else
	fputs( (const char*)&buf[0],stdout );
	fflush( stdout );
#endif

#endif

	return 0;
}

class BBExitApp{
};

int bbError( String err ){
	if( !err.Length() ){
#if __cplusplus_winrt
		throw BBExitApp();
#else
		exit( 0 );
#endif
	}
	dbg_error( err.ToCString<char>() );
	return 0;
}

int bbDebugLog( String t ){
	bbPrint( t );
	return 0;
}

int bbDebugStop(){
	dbg_stop();
	return 0;
}

int bbInit();
int bbMain();

#if _MSC_VER

static void _cdecl seTranslator( unsigned int ex,EXCEPTION_POINTERS *p ){

	switch( ex ){
	case EXCEPTION_ACCESS_VIOLATION:dbg_error( "Memory access violation" );
	case EXCEPTION_ILLEGAL_INSTRUCTION:dbg_error( "Illegal instruction" );
	case EXCEPTION_INT_DIVIDE_BY_ZERO:dbg_error( "Integer divide by zero" );
	case EXCEPTION_STACK_OVERFLOW:dbg_error( "Stack overflow" );
	}
	dbg_error( "Unknown exception" );
}

#else

void sighandler( int sig  ){
	switch( sig ){
	case SIGSEGV:dbg_error( "Memory access violation" );
	case SIGILL:dbg_error( "Illegal instruction" );
	case SIGFPE:dbg_error( "Floating point exception" );
#if !_WIN32
	case SIGBUS:dbg_error( "Bus error" );
#endif	
	}
	dbg_error( "Unknown signal" );
}

#endif

//entry point call by target main()...
//
int bb_std_main( int argc,const char **argv ){

	::argc=argc;
	::argv=argv;
	
#if _MSC_VER

	_set_se_translator( seTranslator );

#else
	
	signal( SIGSEGV,sighandler );
	signal( SIGILL,sighandler );
	signal( SIGFPE,sighandler );
#if !_WIN32
	signal( SIGBUS,sighandler );
#endif

#endif

	if( !setlocale( LC_CTYPE,"en_US.UTF-8" ) ){
		setlocale( LC_CTYPE,"" );
	}

	gc_init1();

	bbInit();
	
	gc_init2();

	bbMain();

	return 0;
}


//***** game.h *****

struct BBGameEvent{
	enum{
		None=0,
		KeyDown=1,KeyUp=2,KeyChar=3,
		MouseDown=4,MouseUp=5,MouseMove=6,
		TouchDown=7,TouchUp=8,TouchMove=9,
		MotionAccel=10
	};
};

class BBGameDelegate : public Object{
public:
	virtual void StartGame(){}
	virtual void SuspendGame(){}
	virtual void ResumeGame(){}
	virtual void UpdateGame(){}
	virtual void RenderGame(){}
	virtual void KeyEvent( int event,int data ){}
	virtual void MouseEvent( int event,int data,Float x,Float y, Float z ){}
	virtual void TouchEvent( int event,int data,Float x,Float y ){}
	virtual void MotionEvent( int event,int data,Float x,Float y,Float z ){}
	virtual void DiscardGraphics(){}
};

struct BBDisplayMode : public Object{
	int width;
	int height;
	int depth;
	int hertz;
	int flags;
	BBDisplayMode( int width=0,int height=0,int depth=0,int hertz=0,int flags=0 ):width(width),height(height),depth(depth),hertz(hertz),flags(flags){}
};

class BBGame{
public:
	BBGame();
	virtual ~BBGame(){}
	
	// ***** Extern *****
	static BBGame *Game(){ return _game; }
	
	virtual void SetDelegate( BBGameDelegate *delegate );
	virtual BBGameDelegate *Delegate(){ return _delegate; }
	
	virtual void SetKeyboardEnabled( bool enabled );
	virtual bool KeyboardEnabled();
	
	virtual void SetUpdateRate( int updateRate );
	virtual int UpdateRate();
	
	virtual bool Started(){ return _started; }
	virtual bool Suspended(){ return _suspended; }
	
	virtual int Millisecs();
	virtual void GetDate( Array<int> date );
	virtual int SaveState( String state );
	virtual String LoadState();
	virtual String LoadString( String path );
	virtual int CountJoysticks( bool update );
	virtual bool PollJoystick( int port,Array<Float> joyx,Array<Float> joyy,Array<Float> joyz,Array<bool> buttons );
	virtual void OpenUrl( String url );
	virtual void SetMouseVisible( bool visible );
	virtual void SetMousePos( double xpos, double ypos );
	virtual void SetClipboard( String _text );
	virtual String GetClipboard();
	
	virtual int GetDeviceWidth(){ return 0; }
	virtual int GetDeviceHeight(){ return 0; }
	virtual void SetDeviceWindow( int width,int height,int flags ){}
	virtual void SetDeviceWindowIcon( String _path ){}
	virtual void SetDeviceWindowPosition( int _x, int _y ){}
	virtual void SetDeviceWindowSize( int _width, int _height ){}
	virtual void SetDeviceWindowSizeLimits( int _minWidth, int _minHeight, int _maxWidth, int _maxHeight ){}
	virtual void SetDeviceWindowTitle( String _title ){}
	virtual Array<BBDisplayMode*> GetDisplayModes(){ return Array<BBDisplayMode*>(); }
	virtual BBDisplayMode *GetDesktopMode(){ return 0; }
	virtual void SetSwapInterval( int interval ){}

	// ***** Native *****
	virtual String PathToFilePath( String path );
	virtual FILE *OpenFile( String path,String mode );
	virtual unsigned char *LoadData( String path,int *plength );
	virtual unsigned char *LoadImageData( String path,int *width,int *height,int *depth ){ return 0; }
	virtual unsigned char *LoadAudioData( String path,int *length,int *channels,int *format,int *hertz ){ return 0; }
	
	//***** Internal *****
	virtual void Die( ThrowableObject *ex );
	virtual void gc_collect();
	virtual void StartGame();
	virtual void SuspendGame();
	virtual void ResumeGame();
	virtual void UpdateGame();
	virtual void RenderGame();
	virtual void KeyEvent( int ev,int data );
	virtual void MouseEvent( int ev,int data,float x,float y, float z );
	virtual void TouchEvent( int ev,int data,float x,float y );
	virtual void MotionEvent( int ev,int data,float x,float y,float z );
	virtual void DiscardGraphics();
	
protected:

	static BBGame *_game;

	BBGameDelegate *_delegate;
	bool _keyboardEnabled;
	int _updateRate;
	bool _started;
	bool _suspended;
};

//***** game.cpp *****

BBGame *BBGame::_game;

BBGame::BBGame():
_delegate( 0 ),
_keyboardEnabled( false ),
_updateRate( 0 ),
_started( false ),
_suspended( false ){
	_game=this;
}

void BBGame::SetDelegate( BBGameDelegate *delegate ){
	_delegate=delegate;
}

void BBGame::SetKeyboardEnabled( bool enabled ){
	_keyboardEnabled=enabled;
}

bool BBGame::KeyboardEnabled(){
	return _keyboardEnabled;
}

void BBGame::SetUpdateRate( int updateRate ){
	_updateRate=updateRate;
}

int BBGame::UpdateRate(){
	return _updateRate;
}

int BBGame::Millisecs(){
	return 0;
}

void BBGame::GetDate( Array<int> date ){
	int n=date.Length();
	if( n>0 ){
		time_t t=time( 0 );
		
#if _MSC_VER
		struct tm tii;
		struct tm *ti=&tii;
		localtime_s( ti,&t );
#else
		struct tm *ti=localtime( &t );
#endif

		date[0]=ti->tm_year+1900;
		if( n>1 ){ 
			date[1]=ti->tm_mon+1;
			if( n>2 ){
				date[2]=ti->tm_mday;
				if( n>3 ){
					date[3]=ti->tm_hour;
					if( n>4 ){
						date[4]=ti->tm_min;
						if( n>5 ){
							date[5]=ti->tm_sec;
							if( n>6 ){
								date[6]=0;
							}
						}
					}
				}
			}
		}
	}
}

int BBGame::SaveState( String state ){
	if( FILE *f=OpenFile( "./.cerberusstate","wb" ) ){
		bool ok=state.Save( f );
		fclose( f );
		return ok ? 0 : -2;
	}
	return -1;
}

String BBGame::LoadState(){
	if( FILE *f=OpenFile( "./.cerberusstate","rb" ) ){
		String str=String::Load( f );
		fclose( f );
		return str;
	}
	return "";
}

String BBGame::LoadString( String path ){
	if( FILE *fp=OpenFile( path,"rb" ) ){
		String str=String::Load( fp );
		fclose( fp );
		return str;
	}
	return "";
}

int BBGame::CountJoysticks( bool update ){
	return 0;
}

bool BBGame::PollJoystick( int port,Array<Float> joyx,Array<Float> joyy,Array<Float> joyz,Array<bool> buttons ){
	return false;
}

void BBGame::OpenUrl( String url ){
}

void BBGame::SetMouseVisible( bool visible ){
}

void BBGame::SetMousePos( double xpos, double ypos ){
}

void BBGame::SetClipboard( String _text ){
}

String BBGame::GetClipboard(){
	return "";
}

//***** C++ Game *****

String BBGame::PathToFilePath( String path ){
	return path;
}

FILE *BBGame::OpenFile( String path,String mode ){
	path=PathToFilePath( path );
	if( path=="" ) return 0;
	
#if __cplusplus_winrt
	path=path.Replace( "/","\\" );
	FILE *f;
	if( _wfopen_s( &f,path.ToCString<wchar_t>(),mode.ToCString<wchar_t>() ) ) return 0;
	return f;
#elif _WIN32
	return _wfopen( path.ToCString<wchar_t>(),mode.ToCString<wchar_t>() );
#else
	return fopen( path.ToCString<char>(),mode.ToCString<char>() );
#endif
}

unsigned char *BBGame::LoadData( String path,int *plength ){

	FILE *f=OpenFile( path,"rb" );
	if( !f ) return 0;

	const int BUF_SZ=4096;
	std::vector<void*> tmps;
	int length=0;
	
	for(;;){
		void *p=malloc( BUF_SZ );
		int n=fread( p,1,BUF_SZ,f );
		tmps.push_back( p );
		length+=n;
		if( n!=BUF_SZ ) break;
	}
	fclose( f );
	
	unsigned char *data=(unsigned char*)malloc( length );
	unsigned char *p=data;
	
	int sz=length;
	for( int i=0;i<tmps.size();++i ){
		int n=sz>BUF_SZ ? BUF_SZ : sz;
		memcpy( p,tmps[i],n );
		free( tmps[i] );
		sz-=n;
		p+=n;
	}
	
	*plength=length;
	
	gc_ext_malloced( length );
	
	return data;
}

//***** INTERNAL *****

void BBGame::Die( ThrowableObject *ex ){
	bbPrint( "Cerberus Runtime Error : Uncaught Cerberus Exception" );
#ifndef NDEBUG
	bbPrint( ex->stackTrace );
#endif
	exit( -1 );
}

void BBGame::gc_collect(){
	gc_mark( _delegate );
	::gc_collect();
}

void BBGame::StartGame(){

	if( _started ) return;
	_started=true;
	
	try{
		_delegate->StartGame();
	}catch( ThrowableObject *ex ){
		Die( ex );
	}
	gc_collect();
}

void BBGame::SuspendGame(){

	if( !_started || _suspended ) return;
	_suspended=true;
	
	try{
		_delegate->SuspendGame();
	}catch( ThrowableObject *ex ){
		Die( ex );
	}
	gc_collect();
}

void BBGame::ResumeGame(){

	if( !_started || !_suspended ) return;
	_suspended=false;
	
	try{
		_delegate->ResumeGame();
	}catch( ThrowableObject *ex ){
		Die( ex );
	}
	gc_collect();
}

void BBGame::UpdateGame(){

	if( !_started || _suspended ) return;
	
	try{
		_delegate->UpdateGame();
	}catch( ThrowableObject *ex ){
		Die( ex );
	}
	gc_collect();
}

void BBGame::RenderGame(){

	if( !_started ) return;
	
	try{
		_delegate->RenderGame();
	}catch( ThrowableObject *ex ){
		Die( ex );
	}
	gc_collect();
}

void BBGame::KeyEvent( int ev,int data ){

	if( !_started ) return;
	
	try{
		_delegate->KeyEvent( ev,data );
	}catch( ThrowableObject *ex ){
		Die( ex );
	}
	gc_collect();
}

void BBGame::MouseEvent( int ev,int data,float x,float y, float z ){

	if( !_started ) return;
	
	try{
		_delegate->MouseEvent( ev,data,x,y,z );
	}catch( ThrowableObject *ex ){
		Die( ex );
	}
	gc_collect();
}

void BBGame::TouchEvent( int ev,int data,float x,float y ){

	if( !_started ) return;
	
	try{
		_delegate->TouchEvent( ev,data,x,y );
	}catch( ThrowableObject *ex ){
		Die( ex );
	}
	gc_collect();
}

void BBGame::MotionEvent( int ev,int data,float x,float y,float z ){

	if( !_started ) return;
	
	try{
		_delegate->MotionEvent( ev,data,x,y,z );
	}catch( ThrowableObject *ex ){
		Die( ex );
	}
	gc_collect();
}

void BBGame::DiscardGraphics(){

	if( !_started ) return;
	
	try{
		_delegate->DiscardGraphics();
	}catch( ThrowableObject *ex ){
		Die( ex );
	}
	gc_collect();
}

// Stdcpp trans.system runtime.
//
// Placed into the public domain 24/02/2011.
// No warranty implied; use as your own risk.

#if _WIN32

#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif

typedef WCHAR OS_CHAR;
typedef struct _stat stat_t;

#define mkdir( X,Y ) _wmkdir( X )
#define rmdir _wrmdir
#define remove _wremove
#define rename _wrename
#define stat _wstat
#define _fopen _wfopen
#define putenv _wputenv
#define getenv _wgetenv
#define system _wsystem
#define chdir _wchdir
#define getcwd _wgetcwd
#define realpath(X,Y) _wfullpath( Y,X,PATH_MAX )	//Note: first args SWAPPED to be posix-like!
#define opendir _wopendir
#define readdir _wreaddir
#define closedir _wclosedir
#define DIR _WDIR
#define dirent _wdirent

#elif __APPLE__

typedef char OS_CHAR;
typedef struct stat stat_t;

#define _fopen fopen

#elif __linux

/*
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
*/

typedef char OS_CHAR;
typedef struct stat stat_t;

#define _fopen fopen

#endif

static String _appPath;
static Array<String> _appArgs;

static String::CString<char> C_STR( const String &t ){
	return t.ToCString<char>();
}

static String::CString<OS_CHAR> OS_STR( const String &t ){
	return t.ToCString<OS_CHAR>();
}

String HostOS(){
#if _WIN32
	return "winnt";
#elif __APPLE__
	return "macos";
#elif __linux
	return "linux";
#else
	return "";
#endif
}

String RealPath( String path ){
	std::vector<OS_CHAR> buf( PATH_MAX+1 );
	if( realpath( OS_STR( path ),&buf[0] ) ){}
	buf[buf.size()-1]=0;
	for( int i=0;i<PATH_MAX && buf[i];++i ){
		if( buf[i]=='\\' ) buf[i]='/';
		
	}
	return String( &buf[0] );
}

String AppPath(){

	if( _appPath.Length() ) return _appPath;
	
#if _WIN32

	OS_CHAR buf[PATH_MAX+1];
	GetModuleFileNameW( GetModuleHandleW(0),buf,PATH_MAX );
	buf[PATH_MAX]=0;
	_appPath=String( buf );
	
#elif __APPLE__

	char buf[PATH_MAX];
	uint32_t size=sizeof( buf );
	_NSGetExecutablePath( buf,&size );
	buf[PATH_MAX-1]=0;
	_appPath=String( buf );
	
#elif __linux

	char lnk[PATH_MAX],buf[PATH_MAX];
	pid_t pid=getpid();
	sprintf( lnk,"/proc/%i/exe",pid );
	int i=readlink( lnk,buf,PATH_MAX );
	if( i>0 && i<PATH_MAX ){
		buf[i]=0;
		_appPath=String( buf );
	}

#endif

	_appPath=RealPath( _appPath );
	return _appPath;
}

Array<String> AppArgs(){
	if( _appArgs.Length() ) return _appArgs;
	_appArgs=Array<String>( argc );
	for( int i=0;i<argc;++i ){
		_appArgs[i]=String( argv[i] );
	}
	return _appArgs;
}
	
int FileType( String path ){
	stat_t st;
	if( stat( OS_STR(path),&st ) ) return 0;
	switch( st.st_mode & S_IFMT ){
	case S_IFREG : return 1;
	case S_IFDIR : return 2;
	}
	return 0;
}

int FileSize( String path ){
	stat_t st;
	if( stat( OS_STR(path),&st ) ) return -1;
	return st.st_size;
}

int FileTime( String path ){
	stat_t st;
	if( stat( OS_STR(path),&st ) ) return -1;
	return st.st_mtime;
}

String LoadString( String path ){
	if( FILE *fp=_fopen( OS_STR(path),OS_STR("rb") ) ){
		String str=String::Load( fp );
		if( _str_load_err ){
			bbPrint( String( _str_load_err )+" in file: "+path );
		}
		fclose( fp );
		return str;
	}
	return "";
}
	
int SaveString( String str,String path ){
	if( FILE *fp=_fopen( OS_STR(path),OS_STR("wb") ) ){
		bool ok=str.Save( fp );
		fclose( fp );
		return ok ? 0 : -2;
	}else{
//		printf( "FOPEN 'wb' for SaveString '%s' failed\n",C_STR( path ) );
		fflush( stdout );
	}
	return -1;
}

Array<String> LoadDir( String path ){
	std::vector<String> files;
	
#if _WIN32

	WIN32_FIND_DATAW filedata;
	HANDLE handle=FindFirstFileW( OS_STR(path+"/*"),&filedata );
	if( handle!=INVALID_HANDLE_VALUE ){
		do{
			String f=filedata.cFileName;
			if( f=="." || f==".." ) continue;
			files.push_back( f );
		}while( FindNextFileW( handle,&filedata ) );
		FindClose( handle );
	}else{
//		printf( "FindFirstFileW for LoadDir(%s) failed\n",C_STR(path) );
		fflush( stdout );
	}
	
#else

	if( DIR *dir=opendir( OS_STR(path) ) ){
		while( dirent *ent=readdir( dir ) ){
			String f=ent->d_name;
			if( f=="." || f==".." ) continue;
			files.push_back( f );
		}
		closedir( dir );
	}else{
//		printf( "opendir for LoadDir(%s) failed\n",C_STR(path) );
		fflush( stdout );
	}

#endif

	return files.size() ? Array<String>( &files[0],files.size() ) : Array<String>();
}
	
int CopyFile( String srcpath,String dstpath ){

#if _WIN32

	if( CopyFileW( OS_STR(srcpath),OS_STR(dstpath),FALSE ) ) return 1;
	return 0;
	
#elif __APPLE__

	// Would like to use COPY_ALL here, but it breaks trans on MacOS - produces weird 'pch out of date' error with copied projects.
	//
	// Ranlib strikes back!
	//
	if( copyfile( OS_STR(srcpath),OS_STR(dstpath),0,COPYFILE_DATA )>=0 ) return 1;
	return 0;
	
#else

	int err=-1;
	if( FILE *srcp=_fopen( OS_STR( srcpath ),OS_STR( "rb" ) ) ){
		err=-2;
		if( FILE *dstp=_fopen( OS_STR( dstpath ),OS_STR( "wb" ) ) ){
			err=0;
			char buf[1024];
			while( int n=fread( buf,1,1024,srcp ) ){
				if( fwrite( buf,1,n,dstp )!=n ){
					err=-3;
					break;
				}
			}
			fclose( dstp );
		}else{
//			printf( "FOPEN 'wb' for CopyFile(%s,%s) failed\n",C_STR(srcpath),C_STR(dstpath) );
			fflush( stdout );
		}
		fclose( srcp );
	}else{
//		printf( "FOPEN 'rb' for CopyFile(%s,%s) failed\n",C_STR(srcpath),C_STR(dstpath) );
		fflush( stdout );
	}
	return err==0;
	
#endif
}

int ChangeDir( String path ){
	return chdir( OS_STR(path) );
}

String CurrentDir(){
	std::vector<OS_CHAR> buf( PATH_MAX+1 );
	if( getcwd( &buf[0],buf.size() ) ){}
	buf[buf.size()-1]=0;
	return String( &buf[0] );
}

int CreateDir( String path ){
	mkdir( OS_STR( path ),0777 );
	return FileType(path)==2;
}

int DeleteDir( String path ){
	rmdir( OS_STR(path) );
	return FileType(path)==0;
}

int DeleteFile( String path ){
	remove( OS_STR(path) );
	return FileType(path)==0;
}

int SetEnv( String name,String value ){
#if _WIN32
	return putenv( OS_STR( name+"="+value ) );
#else
	if( value.Length() ) return setenv( OS_STR( name ),OS_STR( value ),1 );
	unsetenv( OS_STR( name ) );
	return 0;
#endif
}

String GetEnv( String name ){
	if( OS_CHAR *p=getenv( OS_STR(name) ) ) return String( p );
	return "";
}

int Execute( String cmd ){

#if _WIN32

	cmd=String("cmd /S /C \"")+cmd+"\"";

	PROCESS_INFORMATION pi={0};
	STARTUPINFOW si={sizeof(si)};

	if( !CreateProcessW( 0,(WCHAR*)(const OS_CHAR*)OS_STR(cmd),0,0,1,CREATE_DEFAULT_ERROR_MODE,0,0,&si,&pi ) ) return -1;

	WaitForSingleObject( pi.hProcess,INFINITE );

	int res=GetExitCodeProcess( pi.hProcess,(DWORD*)&res ) ? res : -1;

	CloseHandle( pi.hProcess );
	CloseHandle( pi.hThread );

	return res;

#else

	return system( OS_STR(cmd) );

#endif
}

int ExitApp( int retcode ){
	exit( retcode );
	return 0;
}


// ***** thread.h *****

#if __cplusplus_winrt

using namespace Windows::System::Threading;

#endif

class BBThread : public Object{
public:
	Object *result;
	
	BBThread();
	
	virtual void Start();
	virtual bool IsRunning();
	
	virtual Object *Result();
	virtual void SetResult( Object *result );
	
	static  String Strdup( const String &str );
	
	virtual void Run__UNSAFE__();
	
	
private:

	enum{
		INIT=0,
		RUNNING=1,
		FINISHED=2
	};

	
	int _state;
	Object *_result;
	
#if __cplusplus_winrt

	friend class Launcher;

	class Launcher{
	
		friend class BBThread;
		BBThread *_thread;
		
		Launcher( BBThread *thread ):_thread(thread){
		}
		
		public:
		
		void operator()( IAsyncAction ^operation ){
			_thread->Run__UNSAFE__();
			_thread->_state=FINISHED;
		} 
	};
	
#elif _WIN32

	static DWORD WINAPI run( void *p );
	
#else

	static void *run( void *p );
	
#endif

};

// ***** thread.cpp *****

BBThread::BBThread():_state( INIT ),_result( 0 ){
}

bool BBThread::IsRunning(){
	return _state==RUNNING;
}

Object *BBThread::Result(){
	return _result;
}

void BBThread::SetResult( Object *result ){
	_result=result;
}

String BBThread::Strdup( const String &str ){
	return str.Copy();
}

void BBThread::Run__UNSAFE__(){
}

#if __cplusplus_winrt

void BBThread::Start(){
	if( _state==RUNNING ) return;
	
	_result=0;
	_state=RUNNING;
	
	Launcher launcher( this );
	
	auto handler=ref new WorkItemHandler( launcher );
	
	ThreadPool::RunAsync( handler );
}

#elif _WIN32

void BBThread::Start(){
	if( _state==RUNNING ) return;
	
	_result=0;
	_state=RUNNING;
	
	DWORD _id;
	HANDLE _handle;

	if( _handle=CreateThread( 0,0,run,this,0,&_id ) ){
		CloseHandle( _handle );
		return;
	}
	
	puts( "CreateThread failed!" );
	exit( -1 );
}

DWORD WINAPI BBThread::run( void *p ){
	BBThread *thread=(BBThread*)p;

	thread->Run__UNSAFE__();
	
	thread->_state=FINISHED;
	return 0;
}

#else

void BBThread::Start(){
	if( _state==RUNNING ) return;
	
	_result=0;
	_state=RUNNING;
	
	pthread_t _handle;
	
	if( !pthread_create( &_handle,0,run,this ) ){
		pthread_detach( _handle );
		return;
	}
	
	puts( "pthread_create failed!" );
	exit( -1 );
}

void *BBThread::run( void *p ){
	BBThread *thread=(BBThread*)p;

	thread->Run__UNSAFE__();

	thread->_state=FINISHED;
	return 0;
}

#endif


// ***** databuffer.h *****

class BBDataBuffer : public Object{
public:
	
	BBDataBuffer();
	
	~BBDataBuffer();
	
	bool _New( int length,void *data=0 );
	
	bool _Load( String path );
	
	void _LoadAsync( const String &path,BBThread *thread );

	void Discard();
	
	const void *ReadPointer( int offset=0 ){
		return _data+offset;
	}
	
	void *WritePointer( int offset=0 ){
		return _data+offset;
	}
	
	int Length(){
		return _length;
	}
	
	void PokeByte( int addr,int value ){
		*(_data+addr)=value;
	}

	void PokeShort( int addr,int value ){
		*(short*)(_data+addr)=value;
	}
	
	void PokeInt( int addr,int value ){
		*(int*)(_data+addr)=value;
	}
	
	void PokeFloat( int addr,float value ){
		*(float*)(_data+addr)=value;
	}

	int PeekByte( int addr ){
		return *(_data+addr);
	}
	
	int PeekShort( int addr ){
		return *(short*)(_data+addr);
	}
	
	int PeekInt( int addr ){
		return *(int*)(_data+addr);
	}
	
	float PeekFloat( int addr ){
		return *(float*)(_data+addr);
	}
	
private:
	signed char *_data;
	int _length;
};

// ***** databuffer.cpp *****

BBDataBuffer::BBDataBuffer():_data(0),_length(0){
}

BBDataBuffer::~BBDataBuffer(){
	if( _data ) free( _data );
}

bool BBDataBuffer::_New( int length,void *data ){
	if( _data ) return false;
	if( !data ) data=malloc( length );
	_data=(signed char*)data;
	_length=length;
	return true;
}

bool BBDataBuffer::_Load( String path ){
	if( _data ) return false;
	
	_data=(signed char*)BBGame::Game()->LoadData( path,&_length );
	if( !_data ) return false;
	
	return true;
}

void BBDataBuffer::_LoadAsync( const String &cpath,BBThread *thread ){

	String path=cpath.Copy();
	
	if( _Load( path ) ) thread->SetResult( this );
}

void BBDataBuffer::Discard(){
	if( !_data ) return;
	free( _data );
	_data=0;
	_length=0;
}


// ***** stream.h *****

class BBStream : public Object{
public:

	virtual int Eof(){
		return 0;
	}

	virtual void Close(){
	}

	virtual int Length(){
		return 0;
	}
	
	virtual int Position(){
		return 0;
	}
	
	virtual int Seek( int position ){
		return 0;
	}
	
	virtual int Read( BBDataBuffer *buffer,int offset,int count ){
		return 0;
	}

	virtual int Write( BBDataBuffer *buffer,int offset,int count ){
		return 0;
	}
};

// ***** stream.cpp *****


// ***** filestream.h *****

class BBFileStream : public BBStream{
public:

	BBFileStream();
	~BBFileStream();

	void Close();
	int Eof();
	int Length();
	int Position();
	int Seek( int position );
	int Read( BBDataBuffer *buffer,int offset,int count );
	int Write( BBDataBuffer *buffer,int offset,int count );

	bool Open( String path,String mode );
	
private:
	FILE *_file;
	int _position;
	int _length;
};

// ***** filestream.cpp *****

BBFileStream::BBFileStream():_file(0),_position(0),_length(0){
}

BBFileStream::~BBFileStream(){
	if( _file ) fclose( _file );
}

bool BBFileStream::Open( String path,String mode ){
	if( _file ) return false;

	String fmode;	
	if( mode=="r" ){
		fmode="rb";
	}else if( mode=="w" ){
		fmode="wb";
	}else if( mode=="u" ){
		fmode="rb+";
	}else{
		return false;
	}

	_file=BBGame::Game()->OpenFile( path,fmode );
	if( !_file && mode=="u" ) _file=BBGame::Game()->OpenFile( path,"wb+" );
	if( !_file ) return false;
	
	fseek( _file,0,SEEK_END );
	_length=ftell( _file );
	fseek( _file,0,SEEK_SET );
	_position=0;
	
	return true;
}

void BBFileStream::Close(){
	if( !_file ) return;
	
	fclose( _file );
	_file=0;
	_position=0;
	_length=0;
}

int BBFileStream::Eof(){
	if( !_file ) return -1;
	
	return _position==_length;
}

int BBFileStream::Length(){
	return _length;
}

int BBFileStream::Position(){
	return _position;
}

int BBFileStream::Seek( int position ){
	if( !_file ) return 0;
	
	fseek( _file,position,SEEK_SET );
	_position=ftell( _file );
	return _position;
}

int BBFileStream::Read( BBDataBuffer *buffer,int offset,int count ){
	if( !_file ) return 0;
	
	int n=fread( buffer->WritePointer(offset),1,count,_file );
	_position+=n;
	return n;
}

int BBFileStream::Write( BBDataBuffer *buffer,int offset,int count ){
	if( !_file ) return 0;
	
	int n=fwrite( buffer->ReadPointer(offset),1,count,_file );
	_position+=n;
	if( _position>_length ) _length=_position;
	return n;
}

class c_TransCC;
class c_Type;
class c_StringType;
class c_Decl;
class c_ScopeDecl;
class c_ConfigScope;
class c_ValDecl;
class c_ConstDecl;
class c_Map;
class c_StringMap;
class c_Node;
class c_Expr;
class c_BoolType;
class c_Map2;
class c_StringMap2;
class c_Node2;
class c_Stack;
class c_StringStack;
class c_Builder;
class c_Map3;
class c_StringMap3;
class c_AndroidBuilder;
class c_Node3;
class c_AndroidNdkBuilder;
class c_GlfwBuilder;
class c_Html5Builder;
class c_IosBuilder;
class c_FlashBuilder;
class c_PsmBuilder;
class c_StdcppBuilder;
class c_WinrtBuilder;
class c_XnaBuilder;
class c_AGKBuilder;
class c_AGKBuilder_ios;
class c_NodeEnumerator;
class c_List;
class c_StringList;
class c_Node4;
class c_HeadNode;
class c_Enumerator;
class c_Stack2;
class c_ModuleDecl;
class c_List2;
class c_Node5;
class c_HeadNode2;
class c_Toker;
class c_Set;
class c_StringSet;
class c_Map4;
class c_StringMap4;
class c_Node6;
class c_AppDecl;
class c_Map5;
class c_StringMap5;
class c_Node7;
class c_Parser;
class c_NumericType;
class c_IntType;
class c_FloatType;
class c_AliasDecl;
class c_List3;
class c_Node8;
class c_HeadNode3;
class c_BlockDecl;
class c_FuncDecl;
class c_List4;
class c_FuncDeclList;
class c_Node9;
class c_HeadNode4;
class c_ClassDecl;
class c_VoidType;
class c_IdentType;
class c_Stack3;
class c_ArrayType;
class c_UnaryExpr;
class c_ArrayExpr;
class c_Stack4;
class c_ConstExpr;
class c_ScopeExpr;
class c_NewArrayExpr;
class c_NewObjectExpr;
class c_CastExpr;
class c_IdentExpr;
class c_SelfExpr;
class c_Stmt;
class c_List5;
class c_Node10;
class c_HeadNode5;
class c_InvokeSuperExpr;
class c_IdentTypeExpr;
class c_FuncCallExpr;
class c_SliceExpr;
class c_IndexExpr;
class c_BinaryExpr;
class c_BinaryMathExpr;
class c_BinaryCompareExpr;
class c_BinaryLogicExpr;
class c_VarDecl;
class c_GlobalDecl;
class c_FieldDecl;
class c_LocalDecl;
class c_Enumerator2;
class c_DeclStmt;
class c_Stack5;
class c_ObjectType;
class c_List6;
class c_Node11;
class c_HeadNode6;
class c_ArgDecl;
class c_Stack6;
class c_List7;
class c_Node12;
class c_HeadNode7;
class c_ReturnStmt;
class c_BreakStmt;
class c_ContinueStmt;
class c_IfStmt;
class c_WhileStmt;
class c_RepeatStmt;
class c_ForEachinStmt;
class c_AssignStmt;
class c_ForStmt;
class c_CatchStmt;
class c_Stack7;
class c_TryStmt;
class c_ThrowStmt;
class c_ExprStmt;
class c_Enumerator3;
class c_List8;
class c_Node13;
class c_HeadNode8;
class c_InvokeMemberExpr;
class c_Target;
class c_Map6;
class c_StringMap6;
class c_Node14;
class c_NodeEnumerator2;
class c_Reflector;
class c_MapValues;
class c_ValueEnumerator;
class c_Map7;
class c_StringMap7;
class c_Node15;
class c_Enumerator4;
class c_Stack8;
class c_Translator;
class c_CTranslator;
class c_JavaTranslator;
class c_NodeEnumerator3;
class c_CppTranslator;
class c_JsTranslator;
class c_Stream;
class c_FileStream;
class c_DataBuffer;
class c_AsTranslator;
class c_CsTranslator;
class c_List9;
class c_Node16;
class c_HeadNode9;
class c_Enumerator5;
class c_InvokeExpr;
class c_StmtExpr;
class c_MemberVarExpr;
class c_VarExpr;
class c_Map8;
class c_StringMap8;
class c_Node17;
class c_Map9;
class c_StringMap9;
class c_Node18;
class c_Map10;
class c_StringMap10;
class c_Node19;
class c_Enumerator6;
class c_Stack9;
class c_Enumerator7;
class c_TransCC : public Object{
	public:
	Array<String > m_args;
	String m_cerberusdir;
	String m_opt_srcpath;
	bool m_opt_safe;
	bool m_opt_clean;
	bool m_opt_check;
	bool m_opt_update;
	bool m_opt_build;
	bool m_opt_run;
	String m_opt_cfgfile;
	String m_opt_output;
	String m_opt_config;
	String m_opt_target;
	String m_opt_modpath;
	String m_opt_builddir;
	String m_ANDROID_PATH;
	String m_ANDROID_NDK_PATH;
	String m_JDK_PATH;
	String m_ANT_PATH;
	String m_FLEX_PATH;
	String m_MINGW_PATH;
	String m_PSM_PATH;
	String m_MSBUILD_PATH;
	String m_AGK_PATH;
	String m_HTML_PLAYER;
	String m_FLASH_PLAYER;
	String m__libs;
	c_StringMap3* m__builders;
	c_StringMap6* m__targets;
	c_Target* m_target;
	c_TransCC();
	c_TransCC* m_new();
	void p_ParseArgs();
	void p_LoadConfig();
	void p_EnumBuilders();
	void p_EnumTargets(String);
	String p_GetReleaseVersion();
	void p_Run(Array<String >);
	bool p_Execute(String,bool);
	void mark();
};
String bb_os_ExtractDir(String);
String bb_transcc_StripQuotes(String);
int bb_transcc_Die(String);
class c_Type : public Object{
	public:
	c_ArrayType* m_arrayOf;
	c_Type();
	c_Type* m_new();
	static c_StringType* m_stringType;
	static c_IntType* m_intType;
	static c_FloatType* m_floatType;
	static c_BoolType* m_boolType;
	static c_VoidType* m_voidType;
	static c_IdentType* m_objectType;
	static c_IdentType* m_throwableType;
	c_ArrayType* p_ArrayOf();
	static c_ArrayType* m_emptyArrayType;
	static c_IdentType* m_nullObjectType;
	virtual String p_ToString();
	virtual int p_EqualsType(c_Type*);
	virtual c_Type* p_Semant();
	virtual int p_ExtendsType(c_Type*);
	virtual c_ClassDecl* p_GetClass();
	void mark();
};
class c_StringType : public c_Type{
	public:
	c_StringType();
	c_StringType* m_new();
	int p_EqualsType(c_Type*);
	int p_ExtendsType(c_Type*);
	c_ClassDecl* p_GetClass();
	String p_ToString();
	void mark();
};
class c_Decl : public Object{
	public:
	String m_errInfo;
	String m_ident;
	String m_munged;
	int m_attrs;
	c_ScopeDecl* m_scope;
	c_Decl();
	c_Decl* m_new();
	int p_IsSemanted();
	int p_IsPublic();
	c_ModuleDecl* p_ModuleScope();
	int p_IsProtected();
	c_ClassDecl* p_ClassScope();
	c_FuncDecl* p_FuncScope();
	int p_CheckAccess();
	int p_IsExtern();
	virtual String p_ToString();
	int p_IsAbstract();
	int p_IsSemanting();
	virtual int p_OnSemant()=0;
	c_AppDecl* p_AppScope();
	int p_Semant();
	int p_IsPrivate();
	int p_AssertAccess();
	virtual c_Decl* p_OnCopy()=0;
	c_Decl* p_Copy();
	int p_IsFinal();
	void mark();
};
class c_ScopeDecl : public c_Decl{
	public:
	c_List3* m_decls;
	c_StringMap4* m_declsMap;
	c_List3* m_semanted;
	c_ScopeDecl();
	c_ScopeDecl* m_new();
	int p_InsertDecl(c_Decl*);
	virtual Object* p_GetDecl(String);
	Object* p_FindDecl(String);
	int p_InsertDecls(c_List3*);
	virtual c_FuncDecl* p_FindFuncDecl(String,Array<c_Expr* >,int);
	c_List3* p_Decls();
	c_Type* p_FindType(String,Array<c_Type* >);
	c_List4* p_MethodDecls(String);
	c_List3* p_Semanted();
	c_List4* p_SemantedMethods(String);
	virtual c_ValDecl* p_FindValDecl(String);
	c_Decl* p_OnCopy();
	int p_OnSemant();
	c_List4* p_SemantedFuncs(String);
	c_ModuleDecl* p_FindModuleDecl(String);
	c_List4* p_FuncDecls(String);
	c_ScopeDecl* p_FindScopeDecl(String);
	void mark();
};
class c_ConfigScope : public c_ScopeDecl{
	public:
	c_StringMap* m_cdecls;
	c_StringMap2* m_vars;
	c_ConfigScope();
	c_ConfigScope* m_new();
	c_ValDecl* p_FindValDecl(String);
	void mark();
};
extern String bb_config__errInfo;
extern c_ConfigScope* bb_config__cfgScope;
class c_ValDecl : public c_Decl{
	public:
	c_Type* m_type;
	c_Expr* m_init;
	c_ValDecl();
	c_ValDecl* m_new();
	String p_ToString();
	int p_OnSemant();
	c_Expr* p_CopyInit();
	void mark();
};
class c_ConstDecl : public c_ValDecl{
	public:
	String m_value;
	c_ConstDecl();
	c_ConstDecl* m_new(String,int,c_Type*,c_Expr*);
	c_ConstDecl* m_new2();
	c_Decl* p_OnCopy();
	int p_OnSemant();
	void mark();
};
class c_Map : public Object{
	public:
	c_Node* m_root;
	c_Map();
	c_Map* m_new();
	virtual int p_Compare(String,String)=0;
	c_Node* p_FindNode(String);
	c_ConstDecl* p_Get(String);
	int p_RotateLeft(c_Node*);
	int p_RotateRight(c_Node*);
	int p_InsertFixup(c_Node*);
	bool p_Set(String,c_ConstDecl*);
	bool p_Contains(String);
	void mark();
};
class c_StringMap : public c_Map{
	public:
	c_StringMap();
	c_StringMap* m_new();
	int p_Compare(String,String);
	void mark();
};
class c_Node : public Object{
	public:
	String m_key;
	c_Node* m_right;
	c_Node* m_left;
	c_ConstDecl* m_value;
	int m_color;
	c_Node* m_parent;
	c_Node();
	c_Node* m_new(String,c_ConstDecl*,int,c_Node*);
	c_Node* m_new2();
	void mark();
};
class c_Expr : public Object{
	public:
	c_Type* m_exprType;
	c_Expr();
	c_Expr* m_new();
	virtual c_Expr* p_Copy();
	virtual c_Expr* p_Semant();
	Array<c_Expr* > p_SemantArgs(Array<c_Expr* >);
	c_Expr* p_Cast(c_Type*,int);
	Array<c_Expr* > p_CastArgs(Array<c_Expr* >,c_FuncDecl*);
	virtual String p_ToString();
	virtual String p_Eval();
	virtual c_Expr* p_EvalConst();
	c_Expr* p_Semant2(c_Type*,int);
	c_Expr* p_CopyExpr(c_Expr*);
	Array<c_Expr* > p_CopyArgs(Array<c_Expr* >);
	c_Type* p_BalanceTypes(c_Type*,c_Type*);
	virtual c_Expr* p_SemantSet(String,c_Expr*);
	virtual c_ScopeDecl* p_SemantScope();
	virtual c_Expr* p_SemantFunc(Array<c_Expr* >);
	virtual bool p_SideEffects();
	virtual String p_Trans();
	virtual String p_TransStmt();
	virtual String p_TransVar();
	void mark();
};
class c_BoolType : public c_Type{
	public:
	c_BoolType();
	c_BoolType* m_new();
	int p_EqualsType(c_Type*);
	int p_ExtendsType(c_Type*);
	c_ClassDecl* p_GetClass();
	String p_ToString();
	void mark();
};
class c_Map2 : public Object{
	public:
	c_Node2* m_root;
	c_Map2();
	c_Map2* m_new();
	virtual int p_Compare(String,String)=0;
	int p_RotateLeft2(c_Node2*);
	int p_RotateRight2(c_Node2*);
	int p_InsertFixup2(c_Node2*);
	bool p_Set2(String,String);
	c_Node2* p_FindNode(String);
	String p_Get(String);
	bool p_Contains(String);
	c_Node2* p_FirstNode();
	c_NodeEnumerator3* p_ObjectEnumerator();
	void mark();
};
class c_StringMap2 : public c_Map2{
	public:
	c_StringMap2();
	c_StringMap2* m_new();
	int p_Compare(String,String);
	void mark();
};
class c_Node2 : public Object{
	public:
	String m_key;
	c_Node2* m_right;
	c_Node2* m_left;
	String m_value;
	int m_color;
	c_Node2* m_parent;
	c_Node2();
	c_Node2* m_new(String,String,int,c_Node2*);
	c_Node2* m_new2();
	c_Node2* p_NextNode();
	String p_Key();
	String p_Value();
	void mark();
};
int bb_config_SetConfigVar(String,String,c_Type*);
int bb_config_SetConfigVar2(String,String);
class c_Stack : public Object{
	public:
	Array<String > m_data;
	int m_length;
	c_Stack();
	c_Stack* m_new();
	c_Stack* m_new2(Array<String >);
	void p_Push(String);
	void p_Push2(Array<String >,int,int);
	void p_Push3(Array<String >,int);
	bool p_IsEmpty();
	Array<String > p_ToArray();
	static String m_NIL;
	void p_Length(int);
	int p_Length2();
	String p_Get2(int);
	String p_Pop();
	void p_Clear();
	void mark();
};
class c_StringStack : public c_Stack{
	public:
	c_StringStack();
	c_StringStack* m_new(Array<String >);
	c_StringStack* m_new2();
	String p_Join(String);
	void mark();
};
String bb_config_GetConfigVar(String);
String bb_transcc_ReplaceEnv(String);
class c_Builder : public Object{
	public:
	c_TransCC* m_tcc;
	String m_casedConfig;
	c_AppDecl* m_app;
	String m_transCode;
	String m_TEXT_FILES;
	String m_IMAGE_FILES;
	String m_SOUND_FILES;
	String m_MUSIC_FILES;
	String m_BINARY_FILES;
	String m_DATA_FILES;
	bool m_syncData;
	c_StringMap2* m_dataFiles;
	c_Builder();
	c_Builder* m_new(c_TransCC*);
	c_Builder* m_new2();
	virtual bool p_IsValid()=0;
	virtual void p_Begin()=0;
	virtual void p_MakeTarget()=0;
	void p_Make();
	void p_CCopyFile(String,String);
	void p_CreateDataDir(String);
	bool p_Execute(String,bool);
	void mark();
};
class c_Map3 : public Object{
	public:
	c_Node3* m_root;
	c_Map3();
	c_Map3* m_new();
	virtual int p_Compare(String,String)=0;
	int p_RotateLeft3(c_Node3*);
	int p_RotateRight3(c_Node3*);
	int p_InsertFixup3(c_Node3*);
	bool p_Set3(String,c_Builder*);
	c_Node3* p_FirstNode();
	c_NodeEnumerator* p_ObjectEnumerator();
	c_Node3* p_FindNode(String);
	c_Builder* p_Get(String);
	void mark();
};
class c_StringMap3 : public c_Map3{
	public:
	c_StringMap3();
	c_StringMap3* m_new();
	int p_Compare(String,String);
	void mark();
};
class c_AndroidBuilder : public c_Builder{
	public:
	c_AndroidBuilder();
	c_AndroidBuilder* m_new(c_TransCC*);
	c_AndroidBuilder* m_new2();
	bool p_IsValid();
	void p_Begin();
	bool p_CreateDirRecursive(String);
	String p_Config();
	void p_MakeTarget();
	void mark();
};
class c_Node3 : public Object{
	public:
	String m_key;
	c_Node3* m_right;
	c_Node3* m_left;
	c_Builder* m_value;
	int m_color;
	c_Node3* m_parent;
	c_Node3();
	c_Node3* m_new(String,c_Builder*,int,c_Node3*);
	c_Node3* m_new2();
	c_Node3* p_NextNode();
	c_Builder* p_Value();
	String p_Key();
	void mark();
};
class c_AndroidNdkBuilder : public c_Builder{
	public:
	c_AndroidNdkBuilder();
	c_AndroidNdkBuilder* m_new(c_TransCC*);
	c_AndroidNdkBuilder* m_new2();
	bool p_IsValid();
	void p_Begin();
	String p_Config();
	bool p_CreateDirRecursive(String);
	void p_MakeTarget();
	void mark();
};
class c_GlfwBuilder : public c_Builder{
	public:
	c_GlfwBuilder();
	c_GlfwBuilder* m_new(c_TransCC*);
	c_GlfwBuilder* m_new2();
	bool p_IsValid();
	void p_Begin();
	String p_Config();
	int p_ProcessExternalLibs(String,String,String);
	void p_MakeGcc();
	void p_MakeVc2010();
	void p_MakeMsvc();
	void p_MakeXcode();
	void p_MakeTarget();
	void mark();
};
class c_Html5Builder : public c_Builder{
	public:
	c_Html5Builder();
	c_Html5Builder* m_new(c_TransCC*);
	c_Html5Builder* m_new2();
	bool p_IsValid();
	void p_Begin();
	String p_MetaData();
	String p_Config();
	void p_MakeTarget();
	void mark();
};
class c_IosBuilder : public c_Builder{
	public:
	c_StringMap2* m__buildFiles;
	int m__nextFileId;
	c_StringMap2* m__fileRefs;
	c_IosBuilder();
	c_IosBuilder* m_new(c_TransCC*);
	c_IosBuilder* m_new2();
	bool p_IsValid();
	void p_Begin();
	String p_Config();
	String p_FileId(String,c_StringMap2*);
	void p_AddBuildFile(String);
	int p_FindEol(String,String,int);
	String p_BuildFiles();
	String p_FileRefs();
	String p_FrameworksBuildPhase();
	String p_FrameworksGroup();
	String p_LibsGroup();
	String p_MungProj(String);
	void p_MungProj2();
	void p_MakeTarget();
	void mark();
};
class c_FlashBuilder : public c_Builder{
	public:
	c_FlashBuilder();
	c_FlashBuilder* m_new(c_TransCC*);
	c_FlashBuilder* m_new2();
	bool p_IsValid();
	void p_Begin();
	String p_Assets();
	String p_Config();
	void p_MakeTarget();
	void mark();
};
class c_PsmBuilder : public c_Builder{
	public:
	c_PsmBuilder();
	c_PsmBuilder* m_new(c_TransCC*);
	c_PsmBuilder* m_new2();
	bool p_IsValid();
	void p_Begin();
	String p_Content();
	String p_Config();
	void p_MakeTarget();
	void mark();
};
class c_StdcppBuilder : public c_Builder{
	public:
	c_StdcppBuilder();
	c_StdcppBuilder* m_new(c_TransCC*);
	c_StdcppBuilder* m_new2();
	bool p_IsValid();
	void p_Begin();
	String p_Config();
	void p_MakeTarget();
	void mark();
};
class c_WinrtBuilder : public c_Builder{
	public:
	c_WinrtBuilder();
	c_WinrtBuilder* m_new(c_TransCC*);
	c_WinrtBuilder* m_new2();
	bool p_IsValid();
	void p_Begin();
	String p_Content2(bool);
	String p_Config();
	void p_MakeTarget();
	void mark();
};
class c_XnaBuilder : public c_Builder{
	public:
	c_XnaBuilder();
	c_XnaBuilder* m_new(c_TransCC*);
	c_XnaBuilder* m_new2();
	bool p_IsValid();
	void p_Begin();
	String p_Content();
	String p_Config();
	void p_MakeTarget();
	void mark();
};
class c_AGKBuilder : public c_Builder{
	public:
	c_AGKBuilder();
	c_AGKBuilder* m_new(c_TransCC*);
	c_AGKBuilder* m_new2();
	bool p_IsValid();
	void p_Begin();
	String p_Config();
	void p_CreateMediaDir(String);
	void p_MakeVc2017();
	void p_MakeXcode();
	void p_MakeTarget();
	void mark();
};
class c_AGKBuilder_ios : public c_Builder{
	public:
	c_AGKBuilder_ios();
	c_AGKBuilder_ios* m_new(c_TransCC*);
	c_AGKBuilder_ios* m_new2();
	bool p_IsValid();
	void p_Begin();
	String p_Config();
	void p_CreateMediaDir(String);
	void p_MakeXcode();
	void p_MakeTarget();
	void mark();
};
c_StringMap3* bb_builders_Builders(c_TransCC*);
class c_NodeEnumerator : public Object{
	public:
	c_Node3* m_node;
	c_NodeEnumerator();
	c_NodeEnumerator* m_new(c_Node3*);
	c_NodeEnumerator* m_new2();
	bool p_HasNext();
	c_Node3* p_NextObject();
	void mark();
};
class c_List : public Object{
	public:
	c_Node4* m__head;
	c_List();
	c_List* m_new();
	c_Node4* p_AddLast(String);
	c_List* m_new2(Array<String >);
	bool p_IsEmpty();
	String p_RemoveFirst();
	virtual bool p_Equals(String,String);
	c_Node4* p_Find(String,c_Node4*);
	c_Node4* p_Find2(String);
	void p_RemoveFirst2(String);
	int p_Count();
	c_Enumerator* p_ObjectEnumerator();
	Array<String > p_ToArray();
	String p_RemoveLast();
	c_Node4* p_FindLast(String,c_Node4*);
	c_Node4* p_FindLast2(String);
	void p_RemoveLast2(String);
	void mark();
};
class c_StringList : public c_List{
	public:
	c_StringList();
	c_StringList* m_new(Array<String >);
	c_StringList* m_new2();
	bool p_Equals(String,String);
	void mark();
};
class c_Node4 : public Object{
	public:
	c_Node4* m__succ;
	c_Node4* m__pred;
	String m__data;
	c_Node4();
	c_Node4* m_new(c_Node4*,c_Node4*,String);
	c_Node4* m_new2();
	int p_Remove();
	void mark();
};
class c_HeadNode : public c_Node4{
	public:
	c_HeadNode();
	c_HeadNode* m_new();
	void mark();
};
class c_Enumerator : public Object{
	public:
	c_List* m__list;
	c_Node4* m__curr;
	c_Enumerator();
	c_Enumerator* m_new(c_List*);
	c_Enumerator* m_new2();
	bool p_HasNext();
	String p_NextObject();
	void mark();
};
Array<String > bb_os_LoadDir(String,bool,bool);
class c_Stack2 : public Object{
	public:
	Array<c_ConfigScope* > m_data;
	int m_length;
	c_Stack2();
	c_Stack2* m_new();
	c_Stack2* m_new2(Array<c_ConfigScope* >);
	void p_Push4(c_ConfigScope*);
	void p_Push5(Array<c_ConfigScope* >,int,int);
	void p_Push6(Array<c_ConfigScope* >,int);
	static c_ConfigScope* m_NIL;
	c_ConfigScope* p_Pop();
	void mark();
};
extern c_Stack2* bb_config__cfgScopeStack;
void bb_config_PushConfigScope();
class c_ModuleDecl : public c_ScopeDecl{
	public:
	String m_rmodpath;
	String m_filepath;
	String m_modpath;
	c_StringMap5* m_imported;
	c_StringSet* m_friends;
	c_StringMap5* m_pubImported;
	c_ModuleDecl();
	c_ModuleDecl* m_new(String,int,String,String,String,c_AppDecl*);
	c_ModuleDecl* m_new2();
	int p_IsStrict();
	int p_ImportModule(String,int);
	int p_SemantAll();
	String p_ToString();
	Object* p_GetDecl2(String);
	Object* p_GetDecl(String);
	int p_OnSemant();
	void mark();
};
c_ScopeDecl* bb_config_GetConfigScope();
extern c_ScopeDecl* bb_decl__env;
class c_List2 : public Object{
	public:
	c_Node5* m__head;
	c_List2();
	c_List2* m_new();
	c_Node5* p_AddLast2(c_ScopeDecl*);
	c_List2* m_new2(Array<c_ScopeDecl* >);
	bool p_IsEmpty();
	c_ScopeDecl* p_RemoveLast();
	bool p_Equals2(c_ScopeDecl*,c_ScopeDecl*);
	c_Node5* p_FindLast3(c_ScopeDecl*,c_Node5*);
	c_Node5* p_FindLast4(c_ScopeDecl*);
	void p_RemoveLast3(c_ScopeDecl*);
	void mark();
};
class c_Node5 : public Object{
	public:
	c_Node5* m__succ;
	c_Node5* m__pred;
	c_ScopeDecl* m__data;
	c_Node5();
	c_Node5* m_new(c_Node5*,c_Node5*,c_ScopeDecl*);
	c_Node5* m_new2();
	int p_Remove();
	void mark();
};
class c_HeadNode2 : public c_Node5{
	public:
	c_HeadNode2();
	c_HeadNode2* m_new();
	void mark();
};
extern c_List2* bb_decl__envStack;
int bb_decl_PushEnv(c_ScopeDecl*);
class c_Toker : public Object{
	public:
	String m__path;
	int m__line;
	String m__source;
	int m__length;
	String m__toke;
	int m__tokeType;
	int m__tokePos;
	c_Toker();
	static c_StringSet* m__keywords;
	static c_StringSet* m__symbols;
	int p__init();
	c_Toker* m_new(String,String);
	c_Toker* m_new2(c_Toker*);
	c_Toker* m_new3();
	int p_TCHR(int);
	String p_TSTR(int);
	String p_NextToke();
	String p_Toke();
	int p_TokeType();
	String p_Path();
	int p_Line();
	int p_SkipSpace();
	void mark();
};
class c_Set : public Object{
	public:
	c_Map4* m_map;
	c_Set();
	c_Set* m_new(c_Map4*);
	c_Set* m_new2();
	int p_Insert(String);
	bool p_Contains(String);
	void mark();
};
class c_StringSet : public c_Set{
	public:
	c_StringSet();
	c_StringSet* m_new();
	void mark();
};
class c_Map4 : public Object{
	public:
	c_Node6* m_root;
	c_Map4();
	c_Map4* m_new();
	virtual int p_Compare(String,String)=0;
	int p_RotateLeft4(c_Node6*);
	int p_RotateRight4(c_Node6*);
	int p_InsertFixup4(c_Node6*);
	bool p_Set4(String,Object*);
	bool p_Insert2(String,Object*);
	c_Node6* p_FindNode(String);
	bool p_Contains(String);
	Object* p_Get(String);
	void mark();
};
class c_StringMap4 : public c_Map4{
	public:
	c_StringMap4();
	c_StringMap4* m_new();
	int p_Compare(String,String);
	void mark();
};
class c_Node6 : public Object{
	public:
	String m_key;
	c_Node6* m_right;
	c_Node6* m_left;
	Object* m_value;
	int m_color;
	c_Node6* m_parent;
	c_Node6();
	c_Node6* m_new(String,Object*,int,c_Node6*);
	c_Node6* m_new2();
	void mark();
};
int bb_config_IsSpace(int);
int bb_config_IsAlpha(int);
int bb_config_IsDigit(int);
int bb_config_IsBinDigit(int);
int bb_config_IsHexDigit(int);
extern String bb_config_ENV_MODPATH;
String bb_os_StripExt(String);
String bb_os_StripDir(String);
extern String bb_parser_FILE_EXT;
extern String bb_parser_FILE_EXT_OLD;
int bb_config_Err(String);
String bb_os_ExtractExt(String);
class c_AppDecl : public c_ScopeDecl{
	public:
	c_StringMap5* m_imported;
	c_ModuleDecl* m_mainModule;
	c_StringList* m_fileImports;
	c_List3* m_allSemantedDecls;
	c_List8* m_semantedGlobals;
	c_List6* m_semantedClasses;
	c_FuncDecl* m_mainFunc;
	c_AppDecl();
	int p_InsertModule(c_ModuleDecl*);
	c_AppDecl* m_new();
	int p_FinalizeClasses();
	int p_OnSemant();
	void mark();
};
class c_Map5 : public Object{
	public:
	c_Node7* m_root;
	c_Map5();
	c_Map5* m_new();
	virtual int p_Compare(String,String)=0;
	c_Node7* p_FindNode(String);
	c_ModuleDecl* p_Get(String);
	bool p_Contains(String);
	int p_RotateLeft5(c_Node7*);
	int p_RotateRight5(c_Node7*);
	int p_InsertFixup5(c_Node7*);
	bool p_Set5(String,c_ModuleDecl*);
	bool p_Insert3(String,c_ModuleDecl*);
	c_MapValues* p_Values();
	c_Node7* p_FirstNode();
	void mark();
};
class c_StringMap5 : public c_Map5{
	public:
	c_StringMap5();
	c_StringMap5* m_new();
	int p_Compare(String,String);
	void mark();
};
class c_Node7 : public Object{
	public:
	String m_key;
	c_Node7* m_right;
	c_Node7* m_left;
	c_ModuleDecl* m_value;
	int m_color;
	c_Node7* m_parent;
	c_Node7();
	c_Node7* m_new(String,c_ModuleDecl*,int,c_Node7*);
	c_Node7* m_new2();
	c_Node7* p_NextNode();
	void mark();
};
class c_Parser : public Object{
	public:
	String m__toke;
	c_Toker* m__toker;
	c_AppDecl* m__app;
	c_ModuleDecl* m__module;
	int m__defattrs;
	int m__tokeType;
	c_BlockDecl* m__block;
	c_List7* m__blockStack;
	c_StringList* m__errStack;
	int m__selTmpId;
	c_Parser();
	int p_SetErr();
	int p_CParse(String);
	int p_SkipEols();
	String p_NextToke();
	c_Parser* m_new(c_Toker*,c_AppDecl*,c_ModuleDecl*,int);
	c_Parser* m_new2();
	String p_ParseStringLit();
	String p_RealPath(String);
	int p_ImportFile(String);
	String p_ParseIdent();
	String p_ParseModPath();
	int p_ImportModule(String,int);
	int p_Parse(String);
	c_Type* p_CParsePrimitiveType();
	c_IdentType* p_ParseIdentType();
	c_Type* p_ParseType();
	c_Type* p_ParseDeclType();
	c_ArrayExpr* p_ParseArrayExpr();
	int p_AtEos();
	Array<c_Expr* > p_ParseArgs2(int);
	c_IdentType* p_CParseIdentType(bool);
	c_Expr* p_ParsePrimaryExpr(int);
	c_Expr* p_ParseUnaryExpr();
	c_Expr* p_ParseMulDivExpr();
	c_Expr* p_ParseAddSubExpr();
	c_Expr* p_ParseBitandExpr();
	c_Expr* p_ParseBitorExpr();
	c_Expr* p_ParseCompareExpr();
	c_Expr* p_ParseAndExpr();
	c_Expr* p_ParseOrExpr();
	c_Expr* p_ParseExpr();
	c_Decl* p_ParseDecl(String,int);
	c_List3* p_ParseDecls(String,int);
	c_List3* p_ParseEnumStmt(int,bool);
	c_List3* p_ParseEnum(String,int);
	int p_PushBlock(c_BlockDecl*);
	int p_ParseDeclStmts();
	int p_ParseReturnStmt();
	int p_ParseExitStmt();
	int p_ParseContinueStmt();
	int p_PopBlock();
	int p_ParseIfStmt(String);
	int p_ParseWhileStmt();
	int p_PushErr();
	int p_PopErr();
	int p_ParseRepeatStmt();
	int p_ParseForStmt();
	int p_ParseSelectStmt();
	int p_ParseTryStmt();
	int p_ParseThrowStmt();
	int p_ParseStmt();
	c_FuncDecl* p_ParseFuncDecl(int);
	c_ClassDecl* p_ParseClassDecl(int);
	int p_ParseMain();
	void mark();
};
int bb_config_InternalErr(String);
int bb_config_StringToInt(String,int);
String bb_config_Dequote(String,String);
String bb_config_EvalConfigTags(String);
extern int bb_config_ENV_SAFEMODE;
class c_NumericType : public c_Type{
	public:
	c_NumericType();
	c_NumericType* m_new();
	void mark();
};
class c_IntType : public c_NumericType{
	public:
	c_IntType();
	c_IntType* m_new();
	int p_EqualsType(c_Type*);
	int p_ExtendsType(c_Type*);
	c_ClassDecl* p_GetClass();
	String p_ToString();
	void mark();
};
class c_FloatType : public c_NumericType{
	public:
	c_FloatType();
	c_FloatType* m_new();
	int p_EqualsType(c_Type*);
	int p_ExtendsType(c_Type*);
	c_ClassDecl* p_GetClass();
	String p_ToString();
	void mark();
};
class c_AliasDecl : public c_Decl{
	public:
	Object* m_decl;
	c_AliasDecl();
	c_AliasDecl* m_new(String,int,Object*);
	c_AliasDecl* m_new2();
	c_Decl* p_OnCopy();
	int p_OnSemant();
	void mark();
};
class c_List3 : public Object{
	public:
	c_Node8* m__head;
	c_List3();
	c_List3* m_new();
	c_Node8* p_AddLast3(c_Decl*);
	c_List3* m_new2(Array<c_Decl* >);
	c_Enumerator2* p_ObjectEnumerator();
	int p_Count();
	void mark();
};
class c_Node8 : public Object{
	public:
	c_Node8* m__succ;
	c_Node8* m__pred;
	c_Decl* m__data;
	c_Node8();
	c_Node8* m_new(c_Node8*,c_Node8*,c_Decl*);
	c_Node8* m_new2();
	void mark();
};
class c_HeadNode3 : public c_Node8{
	public:
	c_HeadNode3();
	c_HeadNode3* m_new();
	void mark();
};
class c_BlockDecl : public c_ScopeDecl{
	public:
	c_List5* m_stmts;
	c_BlockDecl();
	int p_AddStmt(c_Stmt*);
	c_BlockDecl* m_new(c_ScopeDecl*);
	c_BlockDecl* m_new2();
	c_Decl* p_OnCopy();
	int p_OnSemant();
	c_BlockDecl* p_CopyBlock(c_ScopeDecl*);
	void mark();
};
class c_FuncDecl : public c_BlockDecl{
	public:
	c_Type* m_retType;
	Array<c_ArgDecl* > m_argDecls;
	c_FuncDecl* m_overrides;
	c_FuncDecl();
	bool p_IsCtor();
	c_FuncDecl* m_new(String,int,c_Type*,Array<c_ArgDecl* >);
	c_FuncDecl* m_new2();
	bool p_IsMethod();
	String p_ToString();
	bool p_EqualsArgs(c_FuncDecl*);
	bool p_EqualsFunc(c_FuncDecl*);
	c_Decl* p_OnCopy();
	int p_OnSemant();
	bool p_IsStatic();
	bool p_IsProperty();
	bool p_IsVirtual();
	void mark();
};
class c_List4 : public Object{
	public:
	c_Node9* m__head;
	c_List4();
	c_List4* m_new();
	c_Node9* p_AddLast4(c_FuncDecl*);
	c_List4* m_new2(Array<c_FuncDecl* >);
	c_Enumerator3* p_ObjectEnumerator();
	void mark();
};
class c_FuncDeclList : public c_List4{
	public:
	c_FuncDeclList();
	c_FuncDeclList* m_new();
	void mark();
};
class c_Node9 : public Object{
	public:
	c_Node9* m__succ;
	c_Node9* m__pred;
	c_FuncDecl* m__data;
	c_Node9();
	c_Node9* m_new(c_Node9*,c_Node9*,c_FuncDecl*);
	c_Node9* m_new2();
	void mark();
};
class c_HeadNode4 : public c_Node9{
	public:
	c_HeadNode4();
	c_HeadNode4* m_new();
	void mark();
};
class c_ClassDecl : public c_ScopeDecl{
	public:
	c_ClassDecl* m_superClass;
	Array<String > m_args;
	c_IdentType* m_superTy;
	Array<c_IdentType* > m_impltys;
	c_ObjectType* m_objectType;
	c_List6* m_instances;
	c_ClassDecl* m_instanceof;
	Array<c_Type* > m_instArgs;
	Array<c_ClassDecl* > m_implmentsAll;
	Array<c_ClassDecl* > m_implments;
	c_ClassDecl();
	c_ClassDecl* m_new(String,int,Array<String >,c_IdentType*,Array<c_IdentType* >);
	c_ClassDecl* m_new2();
	int p_IsInterface();
	String p_ToString();
	c_FuncDecl* p_FindFuncDecl2(String,Array<c_Expr* >,int);
	c_FuncDecl* p_FindFuncDecl(String,Array<c_Expr* >,int);
	int p_ExtendsObject();
	c_ClassDecl* p_GenClassInstance(Array<c_Type* >);
	int p_IsFinalized();
	int p_UpdateLiveMethods();
	int p_IsInstanced();
	int p_FinalizeClass();
	c_Decl* p_OnCopy();
	Object* p_GetDecl2(String);
	Object* p_GetDecl(String);
	static c_ClassDecl* m_nullObjectClass;
	int p_IsThrowable();
	int p_OnSemant();
	int p_ExtendsClass(c_ClassDecl*);
	void mark();
};
int bb_decl_PopEnv();
class c_VoidType : public c_Type{
	public:
	c_VoidType();
	c_VoidType* m_new();
	int p_EqualsType(c_Type*);
	String p_ToString();
	void mark();
};
class c_IdentType : public c_Type{
	public:
	String m_ident;
	Array<c_Type* > m_args;
	c_IdentType();
	c_IdentType* m_new(String,Array<c_Type* >);
	c_IdentType* m_new2();
	c_Type* p_Semant();
	c_ClassDecl* p_SemantClass();
	int p_EqualsType(c_Type*);
	int p_ExtendsType(c_Type*);
	String p_ToString();
	void mark();
};
class c_Stack3 : public Object{
	public:
	Array<c_Type* > m_data;
	int m_length;
	c_Stack3();
	c_Stack3* m_new();
	c_Stack3* m_new2(Array<c_Type* >);
	void p_Push7(c_Type*);
	void p_Push8(Array<c_Type* >,int,int);
	void p_Push9(Array<c_Type* >,int);
	Array<c_Type* > p_ToArray();
	void mark();
};
class c_ArrayType : public c_Type{
	public:
	c_Type* m_elemType;
	c_ArrayType();
	c_ArrayType* m_new(c_Type*);
	c_ArrayType* m_new2();
	int p_EqualsType(c_Type*);
	int p_ExtendsType(c_Type*);
	c_Type* p_Semant();
	c_ClassDecl* p_GetClass();
	String p_ToString();
	void mark();
};
class c_UnaryExpr : public c_Expr{
	public:
	String m_op;
	c_Expr* m_expr;
	c_UnaryExpr();
	c_UnaryExpr* m_new(String,c_Expr*);
	c_UnaryExpr* m_new2();
	c_Expr* p_Copy();
	c_Expr* p_Semant();
	String p_Eval();
	String p_Trans();
	void mark();
};
class c_ArrayExpr : public c_Expr{
	public:
	Array<c_Expr* > m_exprs;
	c_ArrayExpr();
	c_ArrayExpr* m_new(Array<c_Expr* >);
	c_ArrayExpr* m_new2();
	c_Expr* p_Copy();
	c_Expr* p_Semant();
	String p_Trans();
	void mark();
};
class c_Stack4 : public Object{
	public:
	Array<c_Expr* > m_data;
	int m_length;
	c_Stack4();
	c_Stack4* m_new();
	c_Stack4* m_new2(Array<c_Expr* >);
	void p_Push10(c_Expr*);
	void p_Push11(Array<c_Expr* >,int,int);
	void p_Push12(Array<c_Expr* >,int);
	Array<c_Expr* > p_ToArray();
	void mark();
};
class c_ConstExpr : public c_Expr{
	public:
	c_Type* m_ty;
	String m_value;
	c_ConstExpr();
	c_ConstExpr* m_new(c_Type*,String);
	c_ConstExpr* m_new2();
	c_Expr* p_Semant();
	c_Expr* p_Copy();
	String p_ToString();
	String p_Eval();
	c_Expr* p_EvalConst();
	bool p_SideEffects();
	String p_Trans();
	void mark();
};
class c_ScopeExpr : public c_Expr{
	public:
	c_ScopeDecl* m_scope;
	c_ScopeExpr();
	c_ScopeExpr* m_new(c_ScopeDecl*);
	c_ScopeExpr* m_new2();
	c_Expr* p_Copy();
	String p_ToString();
	c_Expr* p_Semant();
	c_ScopeDecl* p_SemantScope();
	void mark();
};
class c_NewArrayExpr : public c_Expr{
	public:
	c_Type* m_ty;
	c_Expr* m_expr;
	c_NewArrayExpr();
	c_NewArrayExpr* m_new(c_Type*,c_Expr*);
	c_NewArrayExpr* m_new2();
	c_Expr* p_Copy();
	c_Expr* p_Semant();
	String p_Trans();
	void mark();
};
class c_NewObjectExpr : public c_Expr{
	public:
	c_Type* m_ty;
	Array<c_Expr* > m_args;
	c_ClassDecl* m_classDecl;
	c_FuncDecl* m_ctor;
	c_NewObjectExpr();
	c_NewObjectExpr* m_new(c_Type*,Array<c_Expr* >);
	c_NewObjectExpr* m_new2();
	c_Expr* p_Semant();
	c_Expr* p_Copy();
	String p_Trans();
	void mark();
};
class c_CastExpr : public c_Expr{
	public:
	c_Type* m_ty;
	c_Expr* m_expr;
	int m_flags;
	c_CastExpr();
	c_CastExpr* m_new(c_Type*,c_Expr*,int);
	c_CastExpr* m_new2();
	c_Expr* p_Semant();
	c_Expr* p_Copy();
	String p_Eval();
	String p_Trans();
	void mark();
};
class c_IdentExpr : public c_Expr{
	public:
	String m_ident;
	c_Expr* m_expr;
	c_ScopeDecl* m_scope;
	bool m_static;
	c_IdentExpr();
	c_IdentExpr* m_new(String,c_Expr*);
	c_IdentExpr* m_new2();
	c_Expr* p_Copy();
	String p_ToString();
	int p__Semant();
	int p_IdentErr();
	c_Expr* p_SemantSet(String,c_Expr*);
	c_Expr* p_Semant();
	c_ScopeDecl* p_SemantScope();
	c_Expr* p_SemantFunc(Array<c_Expr* >);
	void mark();
};
class c_SelfExpr : public c_Expr{
	public:
	c_SelfExpr();
	c_SelfExpr* m_new();
	c_Expr* p_Copy();
	c_Expr* p_Semant();
	bool p_SideEffects();
	String p_Trans();
	void mark();
};
class c_Stmt : public Object{
	public:
	String m_errInfo;
	c_Stmt();
	c_Stmt* m_new();
	virtual c_Stmt* p_OnCopy2(c_ScopeDecl*)=0;
	c_Stmt* p_Copy2(c_ScopeDecl*);
	virtual int p_OnSemant()=0;
	int p_Semant();
	virtual String p_Trans()=0;
	void mark();
};
class c_List5 : public Object{
	public:
	c_Node10* m__head;
	c_List5();
	c_List5* m_new();
	c_Node10* p_AddLast5(c_Stmt*);
	c_List5* m_new2(Array<c_Stmt* >);
	bool p_IsEmpty();
	c_Enumerator5* p_ObjectEnumerator();
	c_Node10* p_AddFirst(c_Stmt*);
	void mark();
};
class c_Node10 : public Object{
	public:
	c_Node10* m__succ;
	c_Node10* m__pred;
	c_Stmt* m__data;
	c_Node10();
	c_Node10* m_new(c_Node10*,c_Node10*,c_Stmt*);
	c_Node10* m_new2();
	void mark();
};
class c_HeadNode5 : public c_Node10{
	public:
	c_HeadNode5();
	c_HeadNode5* m_new();
	void mark();
};
class c_InvokeSuperExpr : public c_Expr{
	public:
	String m_ident;
	Array<c_Expr* > m_args;
	c_FuncDecl* m_funcDecl;
	c_InvokeSuperExpr();
	c_InvokeSuperExpr* m_new(String,Array<c_Expr* >);
	c_InvokeSuperExpr* m_new2();
	c_Expr* p_Copy();
	c_Expr* p_Semant();
	String p_Trans();
	void mark();
};
class c_IdentTypeExpr : public c_Expr{
	public:
	c_ClassDecl* m_cdecl;
	c_IdentTypeExpr();
	c_IdentTypeExpr* m_new(c_Type*);
	c_IdentTypeExpr* m_new2();
	c_Expr* p_Copy();
	int p__Semant();
	c_Expr* p_Semant();
	c_ScopeDecl* p_SemantScope();
	c_Expr* p_SemantFunc(Array<c_Expr* >);
	void mark();
};
class c_FuncCallExpr : public c_Expr{
	public:
	c_Expr* m_expr;
	Array<c_Expr* > m_args;
	c_FuncCallExpr();
	c_FuncCallExpr* m_new(c_Expr*,Array<c_Expr* >);
	c_FuncCallExpr* m_new2();
	c_Expr* p_Copy();
	String p_ToString();
	c_Expr* p_Semant();
	void mark();
};
class c_SliceExpr : public c_Expr{
	public:
	c_Expr* m_expr;
	c_Expr* m_from;
	c_Expr* m_term;
	c_SliceExpr();
	c_SliceExpr* m_new(c_Expr*,c_Expr*,c_Expr*);
	c_SliceExpr* m_new2();
	c_Expr* p_Copy();
	c_Expr* p_Semant();
	String p_Eval();
	String p_Trans();
	void mark();
};
class c_IndexExpr : public c_Expr{
	public:
	c_Expr* m_expr;
	c_Expr* m_index;
	c_IndexExpr();
	c_IndexExpr* m_new(c_Expr*,c_Expr*);
	c_IndexExpr* m_new2();
	c_Expr* p_Copy();
	c_Expr* p_Semant();
	String p_Eval();
	c_Expr* p_SemantSet(String,c_Expr*);
	bool p_SideEffects();
	String p_Trans();
	String p_TransVar();
	void mark();
};
class c_BinaryExpr : public c_Expr{
	public:
	String m_op;
	c_Expr* m_lhs;
	c_Expr* m_rhs;
	c_BinaryExpr();
	c_BinaryExpr* m_new(String,c_Expr*,c_Expr*);
	c_BinaryExpr* m_new2();
	String p_Trans();
	void mark();
};
class c_BinaryMathExpr : public c_BinaryExpr{
	public:
	c_BinaryMathExpr();
	c_BinaryMathExpr* m_new(String,c_Expr*,c_Expr*);
	c_BinaryMathExpr* m_new2();
	c_Expr* p_Copy();
	c_Expr* p_Semant();
	String p_Eval();
	void mark();
};
class c_BinaryCompareExpr : public c_BinaryExpr{
	public:
	c_Type* m_ty;
	c_BinaryCompareExpr();
	c_BinaryCompareExpr* m_new(String,c_Expr*,c_Expr*);
	c_BinaryCompareExpr* m_new2();
	c_Expr* p_Copy();
	c_Expr* p_Semant();
	String p_Eval();
	void mark();
};
class c_BinaryLogicExpr : public c_BinaryExpr{
	public:
	c_BinaryLogicExpr();
	c_BinaryLogicExpr* m_new(String,c_Expr*,c_Expr*);
	c_BinaryLogicExpr* m_new2();
	c_Expr* p_Copy();
	c_Expr* p_Semant();
	String p_Eval();
	void mark();
};
class c_VarDecl : public c_ValDecl{
	public:
	c_VarDecl();
	c_VarDecl* m_new();
	void mark();
};
class c_GlobalDecl : public c_VarDecl{
	public:
	c_GlobalDecl();
	c_GlobalDecl* m_new(String,int,c_Type*,c_Expr*);
	c_GlobalDecl* m_new2();
	String p_ToString();
	c_Decl* p_OnCopy();
	void mark();
};
class c_FieldDecl : public c_VarDecl{
	public:
	c_FieldDecl();
	c_FieldDecl* m_new(String,int,c_Type*,c_Expr*);
	c_FieldDecl* m_new2();
	String p_ToString();
	c_Decl* p_OnCopy();
	void mark();
};
class c_LocalDecl : public c_VarDecl{
	public:
	c_LocalDecl();
	c_LocalDecl* m_new(String,int,c_Type*,c_Expr*);
	c_LocalDecl* m_new2();
	String p_ToString();
	c_Decl* p_OnCopy();
	void mark();
};
class c_Enumerator2 : public Object{
	public:
	c_List3* m__list;
	c_Node8* m__curr;
	c_Enumerator2();
	c_Enumerator2* m_new(c_List3*);
	c_Enumerator2* m_new2();
	bool p_HasNext();
	c_Decl* p_NextObject();
	void mark();
};
class c_DeclStmt : public c_Stmt{
	public:
	c_Decl* m_decl;
	c_DeclStmt();
	c_DeclStmt* m_new(c_Decl*);
	c_DeclStmt* m_new2(String,c_Type*,c_Expr*);
	c_DeclStmt* m_new3();
	c_Stmt* p_OnCopy2(c_ScopeDecl*);
	int p_OnSemant();
	String p_Trans();
	void mark();
};
class c_Stack5 : public Object{
	public:
	Array<c_IdentType* > m_data;
	int m_length;
	c_Stack5();
	c_Stack5* m_new();
	c_Stack5* m_new2(Array<c_IdentType* >);
	void p_Push13(c_IdentType*);
	void p_Push14(Array<c_IdentType* >,int,int);
	void p_Push15(Array<c_IdentType* >,int);
	Array<c_IdentType* > p_ToArray();
	void mark();
};
class c_ObjectType : public c_Type{
	public:
	c_ClassDecl* m_classDecl;
	c_ObjectType();
	c_ObjectType* m_new(c_ClassDecl*);
	c_ObjectType* m_new2();
	int p_EqualsType(c_Type*);
	c_ClassDecl* p_GetClass();
	int p_ExtendsType(c_Type*);
	String p_ToString();
	void mark();
};
class c_List6 : public Object{
	public:
	c_Node11* m__head;
	c_List6();
	c_List6* m_new();
	c_Node11* p_AddLast6(c_ClassDecl*);
	c_List6* m_new2(Array<c_ClassDecl* >);
	c_Enumerator4* p_ObjectEnumerator();
	void mark();
};
class c_Node11 : public Object{
	public:
	c_Node11* m__succ;
	c_Node11* m__pred;
	c_ClassDecl* m__data;
	c_Node11();
	c_Node11* m_new(c_Node11*,c_Node11*,c_ClassDecl*);
	c_Node11* m_new2();
	void mark();
};
class c_HeadNode6 : public c_Node11{
	public:
	c_HeadNode6();
	c_HeadNode6* m_new();
	void mark();
};
class c_ArgDecl : public c_LocalDecl{
	public:
	c_ArgDecl();
	c_ArgDecl* m_new(String,int,c_Type*,c_Expr*);
	c_ArgDecl* m_new2();
	String p_ToString();
	c_Decl* p_OnCopy();
	void mark();
};
class c_Stack6 : public Object{
	public:
	Array<c_ArgDecl* > m_data;
	int m_length;
	c_Stack6();
	c_Stack6* m_new();
	c_Stack6* m_new2(Array<c_ArgDecl* >);
	void p_Push16(c_ArgDecl*);
	void p_Push17(Array<c_ArgDecl* >,int,int);
	void p_Push18(Array<c_ArgDecl* >,int);
	Array<c_ArgDecl* > p_ToArray();
	void mark();
};
class c_List7 : public Object{
	public:
	c_Node12* m__head;
	c_List7();
	c_List7* m_new();
	c_Node12* p_AddLast7(c_BlockDecl*);
	c_List7* m_new2(Array<c_BlockDecl* >);
	c_BlockDecl* p_RemoveLast();
	bool p_Equals3(c_BlockDecl*,c_BlockDecl*);
	c_Node12* p_FindLast5(c_BlockDecl*,c_Node12*);
	c_Node12* p_FindLast6(c_BlockDecl*);
	void p_RemoveLast4(c_BlockDecl*);
	void mark();
};
class c_Node12 : public Object{
	public:
	c_Node12* m__succ;
	c_Node12* m__pred;
	c_BlockDecl* m__data;
	c_Node12();
	c_Node12* m_new(c_Node12*,c_Node12*,c_BlockDecl*);
	c_Node12* m_new2();
	int p_Remove();
	void mark();
};
class c_HeadNode7 : public c_Node12{
	public:
	c_HeadNode7();
	c_HeadNode7* m_new();
	void mark();
};
class c_ReturnStmt : public c_Stmt{
	public:
	c_Expr* m_expr;
	c_ReturnStmt();
	c_ReturnStmt* m_new(c_Expr*);
	c_ReturnStmt* m_new2();
	c_Stmt* p_OnCopy2(c_ScopeDecl*);
	int p_OnSemant();
	String p_Trans();
	void mark();
};
class c_BreakStmt : public c_Stmt{
	public:
	c_BreakStmt();
	c_BreakStmt* m_new();
	c_Stmt* p_OnCopy2(c_ScopeDecl*);
	int p_OnSemant();
	String p_Trans();
	void mark();
};
class c_ContinueStmt : public c_Stmt{
	public:
	c_ContinueStmt();
	c_ContinueStmt* m_new();
	c_Stmt* p_OnCopy2(c_ScopeDecl*);
	int p_OnSemant();
	String p_Trans();
	void mark();
};
class c_IfStmt : public c_Stmt{
	public:
	c_Expr* m_expr;
	c_BlockDecl* m_thenBlock;
	c_BlockDecl* m_elseBlock;
	c_IfStmt();
	c_IfStmt* m_new(c_Expr*,c_BlockDecl*,c_BlockDecl*);
	c_IfStmt* m_new2();
	c_Stmt* p_OnCopy2(c_ScopeDecl*);
	int p_OnSemant();
	String p_Trans();
	void mark();
};
class c_WhileStmt : public c_Stmt{
	public:
	c_Expr* m_expr;
	c_BlockDecl* m_block;
	c_WhileStmt();
	c_WhileStmt* m_new(c_Expr*,c_BlockDecl*);
	c_WhileStmt* m_new2();
	c_Stmt* p_OnCopy2(c_ScopeDecl*);
	int p_OnSemant();
	String p_Trans();
	void mark();
};
class c_RepeatStmt : public c_Stmt{
	public:
	c_BlockDecl* m_block;
	c_Expr* m_expr;
	c_RepeatStmt();
	c_RepeatStmt* m_new(c_BlockDecl*,c_Expr*);
	c_RepeatStmt* m_new2();
	c_Stmt* p_OnCopy2(c_ScopeDecl*);
	int p_OnSemant();
	String p_Trans();
	void mark();
};
class c_ForEachinStmt : public c_Stmt{
	public:
	String m_varid;
	c_Type* m_varty;
	int m_varlocal;
	c_Expr* m_expr;
	c_BlockDecl* m_block;
	c_ForEachinStmt();
	c_ForEachinStmt* m_new(String,c_Type*,int,c_Expr*,c_BlockDecl*);
	c_ForEachinStmt* m_new2();
	c_Stmt* p_OnCopy2(c_ScopeDecl*);
	int p_OnSemant();
	String p_Trans();
	void mark();
};
class c_AssignStmt : public c_Stmt{
	public:
	String m_op;
	c_Expr* m_lhs;
	c_Expr* m_rhs;
	c_LocalDecl* m_tmp1;
	c_LocalDecl* m_tmp2;
	c_AssignStmt();
	c_AssignStmt* m_new(String,c_Expr*,c_Expr*);
	c_AssignStmt* m_new2();
	c_Stmt* p_OnCopy2(c_ScopeDecl*);
	int p_FixSideEffects();
	int p_OnSemant();
	String p_Trans();
	void mark();
};
class c_ForStmt : public c_Stmt{
	public:
	c_Stmt* m_init;
	c_Expr* m_expr;
	c_Stmt* m_incr;
	c_BlockDecl* m_block;
	c_ForStmt();
	c_ForStmt* m_new(c_Stmt*,c_Expr*,c_Stmt*,c_BlockDecl*);
	c_ForStmt* m_new2();
	c_Stmt* p_OnCopy2(c_ScopeDecl*);
	int p_OnSemant();
	String p_Trans();
	void mark();
};
class c_CatchStmt : public c_Stmt{
	public:
	c_LocalDecl* m_init;
	c_BlockDecl* m_block;
	c_CatchStmt();
	c_CatchStmt* m_new(c_LocalDecl*,c_BlockDecl*);
	c_CatchStmt* m_new2();
	c_Stmt* p_OnCopy2(c_ScopeDecl*);
	int p_OnSemant();
	String p_Trans();
	void mark();
};
class c_Stack7 : public Object{
	public:
	Array<c_CatchStmt* > m_data;
	int m_length;
	c_Stack7();
	c_Stack7* m_new();
	c_Stack7* m_new2(Array<c_CatchStmt* >);
	void p_Push19(c_CatchStmt*);
	void p_Push20(Array<c_CatchStmt* >,int,int);
	void p_Push21(Array<c_CatchStmt* >,int);
	static c_CatchStmt* m_NIL;
	void p_Length(int);
	int p_Length2();
	Array<c_CatchStmt* > p_ToArray();
	void mark();
};
int bb_math_Max(int,int);
Float bb_math_Max2(Float,Float);
class c_TryStmt : public c_Stmt{
	public:
	c_BlockDecl* m_block;
	Array<c_CatchStmt* > m_catches;
	c_TryStmt();
	c_TryStmt* m_new(c_BlockDecl*,Array<c_CatchStmt* >);
	c_TryStmt* m_new2();
	c_Stmt* p_OnCopy2(c_ScopeDecl*);
	int p_OnSemant();
	String p_Trans();
	void mark();
};
class c_ThrowStmt : public c_Stmt{
	public:
	c_Expr* m_expr;
	c_ThrowStmt();
	c_ThrowStmt* m_new(c_Expr*);
	c_ThrowStmt* m_new2();
	c_Stmt* p_OnCopy2(c_ScopeDecl*);
	int p_OnSemant();
	String p_Trans();
	void mark();
};
class c_ExprStmt : public c_Stmt{
	public:
	c_Expr* m_expr;
	c_ExprStmt();
	c_ExprStmt* m_new(c_Expr*);
	c_ExprStmt* m_new2();
	c_Stmt* p_OnCopy2(c_ScopeDecl*);
	int p_OnSemant();
	String p_Trans();
	void mark();
};
c_ModuleDecl* bb_parser_ParseModule(String,String,c_AppDecl*);
class c_Enumerator3 : public Object{
	public:
	c_List4* m__list;
	c_Node9* m__curr;
	c_Enumerator3();
	c_Enumerator3* m_new(c_List4*);
	c_Enumerator3* m_new2();
	bool p_HasNext();
	c_FuncDecl* p_NextObject();
	void mark();
};
extern c_StringList* bb_config__errStack;
int bb_config_PushErr(String);
class c_List8 : public Object{
	public:
	c_Node13* m__head;
	c_List8();
	c_List8* m_new();
	c_Node13* p_AddLast8(c_GlobalDecl*);
	c_List8* m_new2(Array<c_GlobalDecl* >);
	c_Enumerator6* p_ObjectEnumerator();
	void mark();
};
class c_Node13 : public Object{
	public:
	c_Node13* m__succ;
	c_Node13* m__pred;
	c_GlobalDecl* m__data;
	c_Node13();
	c_Node13* m_new(c_Node13*,c_Node13*,c_GlobalDecl*);
	c_Node13* m_new2();
	void mark();
};
class c_HeadNode8 : public c_Node13{
	public:
	c_HeadNode8();
	c_HeadNode8* m_new();
	void mark();
};
int bb_config_PopErr();
class c_InvokeMemberExpr : public c_Expr{
	public:
	c_Expr* m_expr;
	c_FuncDecl* m_decl;
	Array<c_Expr* > m_args;
	int m_isResize;
	c_InvokeMemberExpr();
	c_InvokeMemberExpr* m_new(c_Expr*,c_FuncDecl*,Array<c_Expr* >);
	c_InvokeMemberExpr* m_new2();
	c_Expr* p_Semant();
	String p_ToString();
	String p_Trans();
	String p_TransStmt();
	void mark();
};
c_Expr* bb_preprocessor_EvalExpr(c_Toker*);
bool bb_preprocessor_EvalBool(c_Toker*);
String bb_preprocessor_EvalText(c_Toker*);
c_StringMap2* bb_config_GetConfigVars();
c_Type* bb_config_GetConfigVarType(String);
String bb_preprocessor_PreProcess(String,c_ModuleDecl*);
class c_Target : public Object{
	public:
	String m_dir;
	String m_name;
	String m_system;
	c_Builder* m_builder;
	c_Target();
	c_Target* m_new(String,String,String,c_Builder*);
	c_Target* m_new2();
	void mark();
};
class c_Map6 : public Object{
	public:
	c_Node14* m_root;
	c_Map6();
	c_Map6* m_new();
	virtual int p_Compare(String,String)=0;
	int p_RotateLeft6(c_Node14*);
	int p_RotateRight6(c_Node14*);
	int p_InsertFixup6(c_Node14*);
	bool p_Set6(String,c_Target*);
	c_Node14* p_FirstNode();
	c_NodeEnumerator2* p_ObjectEnumerator();
	c_Node14* p_FindNode(String);
	c_Target* p_Get(String);
	void mark();
};
class c_StringMap6 : public c_Map6{
	public:
	c_StringMap6();
	c_StringMap6* m_new();
	int p_Compare(String,String);
	void mark();
};
class c_Node14 : public Object{
	public:
	String m_key;
	c_Node14* m_right;
	c_Node14* m_left;
	c_Target* m_value;
	int m_color;
	c_Node14* m_parent;
	c_Node14();
	c_Node14* m_new(String,c_Target*,int,c_Node14*);
	c_Node14* m_new2();
	c_Node14* p_NextNode();
	String p_Key();
	void mark();
};
void bb_config_PopConfigScope();
class c_NodeEnumerator2 : public Object{
	public:
	c_Node14* m_node;
	c_NodeEnumerator2();
	c_NodeEnumerator2* m_new(c_Node14*);
	c_NodeEnumerator2* m_new2();
	bool p_HasNext();
	c_Node14* p_NextObject();
	void mark();
};
extern String bb_config_ENV_HOST;
extern String bb_config_ENV_CONFIG;
extern String bb_config_ENV_TARGET;
extern String bb_config_ENV_LANG;
String bb_os_StripAll(String);
c_AppDecl* bb_parser_ParseApp(String);
class c_Reflector : public Object{
	public:
	bool m_debug;
	c_ModuleDecl* m_refmod;
	c_ModuleDecl* m_langmod;
	c_ModuleDecl* m_boxesmod;
	c_StringMap7* m_munged;
	c_StringMap2* m_modexprs;
	c_StringSet* m_refmods;
	c_Stack8* m_classdecls;
	c_StringMap7* m_classids;
	c_StringStack* m_output;
	c_Reflector();
	c_Reflector* m_new();
	static bool m_MatchPath(String,String);
	String p_Mung(String);
	bool p_ValidClass(c_ClassDecl*);
	String p_TypeExpr(c_Type*,bool);
	String p_DeclExpr(c_Decl*,bool);
	int p_Emit(String);
	bool p_ValidType(c_Type*);
	String p_TypeInfo(c_Type*);
	int p_Attrs(c_Decl*);
	String p_Box(c_Type*,String);
	String p_Emit2(c_ConstDecl*);
	String p_Unbox(c_Type*,String);
	String p_Emit3(c_ClassDecl*);
	String p_Emit4(c_FuncDecl*);
	String p_Emit5(c_FieldDecl*);
	String p_Emit6(c_GlobalDecl*);
	int p_Semant3(c_AppDecl*);
	void mark();
};
class c_MapValues : public Object{
	public:
	c_Map5* m_map;
	c_MapValues();
	c_MapValues* m_new(c_Map5*);
	c_MapValues* m_new2();
	c_ValueEnumerator* p_ObjectEnumerator();
	void mark();
};
class c_ValueEnumerator : public Object{
	public:
	c_Node7* m_node;
	c_ValueEnumerator();
	c_ValueEnumerator* m_new(c_Node7*);
	c_ValueEnumerator* m_new2();
	bool p_HasNext();
	c_ModuleDecl* p_NextObject();
	void mark();
};
class c_Map7 : public Object{
	public:
	c_Node15* m_root;
	c_Map7();
	c_Map7* m_new();
	virtual int p_Compare(String,String)=0;
	c_Node15* p_FindNode(String);
	bool p_Contains(String);
	int p_Get(String);
	int p_RotateLeft7(c_Node15*);
	int p_RotateRight7(c_Node15*);
	int p_InsertFixup7(c_Node15*);
	bool p_Set7(String,int);
	void mark();
};
class c_StringMap7 : public c_Map7{
	public:
	c_StringMap7();
	c_StringMap7* m_new();
	int p_Compare(String,String);
	void mark();
};
class c_Node15 : public Object{
	public:
	String m_key;
	c_Node15* m_right;
	c_Node15* m_left;
	int m_value;
	int m_color;
	c_Node15* m_parent;
	c_Node15();
	c_Node15* m_new(String,int,int,c_Node15*);
	c_Node15* m_new2();
	void mark();
};
class c_Enumerator4 : public Object{
	public:
	c_List6* m__list;
	c_Node11* m__curr;
	c_Enumerator4();
	c_Enumerator4* m_new(c_List6*);
	c_Enumerator4* m_new2();
	bool p_HasNext();
	c_ClassDecl* p_NextObject();
	void mark();
};
class c_Stack8 : public Object{
	public:
	Array<c_ClassDecl* > m_data;
	int m_length;
	c_Stack8();
	c_Stack8* m_new();
	c_Stack8* m_new2(Array<c_ClassDecl* >);
	static c_ClassDecl* m_NIL;
	void p_Length(int);
	int p_Length2();
	void p_Push22(c_ClassDecl*);
	void p_Push23(Array<c_ClassDecl* >,int,int);
	void p_Push24(Array<c_ClassDecl* >,int);
	c_ClassDecl* p_Get2(int);
	void mark();
};
int bb_parser_ParseSource(String,c_AppDecl*,c_ModuleDecl*,int);
class c_Translator : public Object{
	public:
	c_Translator();
	virtual String p_TransApp(c_AppDecl*)=0;
	c_Translator* m_new();
	virtual String p_TransInvokeExpr(c_InvokeExpr*)=0;
	virtual String p_TransStmtExpr(c_StmtExpr*)=0;
	virtual String p_TransMemberVarExpr(c_MemberVarExpr*)=0;
	virtual String p_TransVarExpr(c_VarExpr*)=0;
	virtual String p_TransUnaryExpr(c_UnaryExpr*)=0;
	virtual String p_TransArrayExpr(c_ArrayExpr*)=0;
	virtual String p_TransConstExpr(c_ConstExpr*)=0;
	virtual String p_TransNewArrayExpr(c_NewArrayExpr*)=0;
	virtual String p_TransNewObjectExpr(c_NewObjectExpr*)=0;
	virtual String p_TransCastExpr(c_CastExpr*)=0;
	virtual String p_TransSelfExpr(c_SelfExpr*)=0;
	virtual String p_TransInvokeSuperExpr(c_InvokeSuperExpr*)=0;
	virtual String p_TransSliceExpr(c_SliceExpr*)=0;
	virtual String p_TransIndexExpr(c_IndexExpr*)=0;
	virtual String p_TransBinaryExpr(c_BinaryExpr*)=0;
	virtual String p_TransDeclStmt(c_DeclStmt*)=0;
	virtual String p_TransReturnStmt(c_ReturnStmt*)=0;
	virtual String p_TransBreakStmt(c_BreakStmt*)=0;
	virtual String p_TransContinueStmt(c_ContinueStmt*)=0;
	virtual String p_TransIfStmt(c_IfStmt*)=0;
	virtual String p_TransWhileStmt(c_WhileStmt*)=0;
	virtual String p_TransRepeatStmt(c_RepeatStmt*)=0;
	virtual String p_TransBlock(c_BlockDecl*)=0;
	virtual String p_TransAssignStmt(c_AssignStmt*)=0;
	virtual String p_TransForStmt(c_ForStmt*)=0;
	virtual String p_TransTryStmt(c_TryStmt*)=0;
	virtual String p_TransThrowStmt(c_ThrowStmt*)=0;
	virtual String p_TransExprStmt(c_ExprStmt*)=0;
	virtual String p_TransInvokeMemberExpr(c_InvokeMemberExpr*)=0;
	void mark();
};
extern c_Translator* bb_translator__trans;
int bb_os_DeleteDir(String,bool);
int bb_os_CopyDir(String,String,bool,bool);
int bbMain();
class c_CTranslator : public c_Translator{
	public:
	c_StringMap8* m_funcMungs;
	c_StringMap9* m_mungedFuncs;
	c_StringMap10* m_mungedScopes;
	String m_indent;
	c_StringStack* m_lines;
	bool m_emitDebugInfo;
	int m_unreachable;
	int m_broken;
	c_CTranslator();
	c_CTranslator* m_new();
	int p_MungMethodDecl(c_FuncDecl*);
	int p_MungDecl(c_Decl*);
	int p_Emit(String);
	virtual int p_BeginLocalScope();
	String p_Bra(String);
	virtual int p_EmitEnter(c_FuncDecl*);
	virtual int p_EmitEnterBlock();
	virtual int p_EmitSetErr(String);
	virtual String p_TransLocalDecl(String,c_Expr*)=0;
	String p_CreateLocal(c_Expr*);
	String p_TransExprNS(c_Expr*);
	virtual int p_EmitLeave();
	virtual String p_TransValue(c_Type*,String)=0;
	virtual int p_EmitLeaveBlock();
	int p_EmitBlock(c_BlockDecl*,bool);
	virtual int p_EndLocalScope();
	virtual String p_TransGlobal(c_GlobalDecl*)=0;
	String p_JoinLines();
	String p_Enquote(String);
	virtual int p_BeginLoop();
	virtual int p_EndLoop();
	virtual String p_TransField(c_FieldDecl*,c_Expr*)=0;
	int p_ExprPri(c_Expr*);
	String p_TransSubExpr(c_Expr*,int);
	String p_TransStmtExpr(c_StmtExpr*);
	virtual String p_TransIntrinsicExpr(c_Decl*,c_Expr*,Array<c_Expr* >)=0;
	String p_TransVarExpr(c_VarExpr*);
	String p_TransMemberVarExpr(c_MemberVarExpr*);
	virtual String p_TransFunc(c_FuncDecl*,Array<c_Expr* >,c_Expr*)=0;
	String p_TransInvokeExpr(c_InvokeExpr*);
	String p_TransInvokeMemberExpr(c_InvokeMemberExpr*);
	virtual String p_TransSuperFunc(c_FuncDecl*,Array<c_Expr* >)=0;
	String p_TransInvokeSuperExpr(c_InvokeSuperExpr*);
	String p_TransExprStmt(c_ExprStmt*);
	String p_TransAssignOp(String);
	virtual String p_TransAssignStmt2(c_AssignStmt*);
	String p_TransAssignStmt(c_AssignStmt*);
	String p_TransReturnStmt(c_ReturnStmt*);
	String p_TransContinueStmt(c_ContinueStmt*);
	String p_TransBreakStmt(c_BreakStmt*);
	String p_TransBlock(c_BlockDecl*);
	String p_TransDeclStmt(c_DeclStmt*);
	String p_TransIfStmt(c_IfStmt*);
	String p_TransWhileStmt(c_WhileStmt*);
	String p_TransRepeatStmt(c_RepeatStmt*);
	String p_TransForStmt(c_ForStmt*);
	String p_TransTryStmt(c_TryStmt*);
	String p_TransThrowStmt(c_ThrowStmt*);
	String p_TransUnaryOp(String);
	String p_TransBinaryOp(String,String);
	void mark();
};
class c_JavaTranslator : public c_CTranslator{
	public:
	bool m_langutil;
	int m_unsafe;
	c_JavaTranslator();
	c_JavaTranslator* m_new();
	String p_TransType(c_Type*);
	int p_EmitFuncDecl(c_FuncDecl*);
	String p_TransDecl(c_Decl*);
	int p_EmitClassDecl(c_ClassDecl*);
	String p_TransStatic(c_Decl*);
	String p_TransGlobal(c_GlobalDecl*);
	String p_TransApp(c_AppDecl*);
	String p_TransValue(c_Type*,String);
	String p_TransLocalDecl(String,c_Expr*);
	int p_EmitEnter(c_FuncDecl*);
	int p_EmitSetErr(String);
	int p_EmitLeave();
	String p_TransField(c_FieldDecl*,c_Expr*);
	String p_TransArgs(Array<c_Expr* >);
	String p_TransFunc(c_FuncDecl*,Array<c_Expr* >,c_Expr*);
	String p_TransSuperFunc(c_FuncDecl*,Array<c_Expr* >);
	String p_TransConstExpr(c_ConstExpr*);
	String p_TransNewObjectExpr(c_NewObjectExpr*);
	String p_TransNewArrayExpr(c_NewArrayExpr*);
	String p_TransSelfExpr(c_SelfExpr*);
	String p_TransCastExpr(c_CastExpr*);
	String p_TransUnaryExpr(c_UnaryExpr*);
	String p_TransBinaryExpr(c_BinaryExpr*);
	String p_TransIndexExpr(c_IndexExpr*);
	String p_TransSliceExpr(c_SliceExpr*);
	String p_TransArrayExpr(c_ArrayExpr*);
	String p_TransIntrinsicExpr(c_Decl*,c_Expr*,Array<c_Expr* >);
	String p_TransTryStmt(c_TryStmt*);
	void mark();
};
bool bb_transcc_MatchPathAlt(String,String);
bool bb_transcc_MatchPath(String,String);
String bb_transcc_ReplaceBlock(String,String,String,String);
class c_NodeEnumerator3 : public Object{
	public:
	c_Node2* m_node;
	c_NodeEnumerator3();
	c_NodeEnumerator3* m_new(c_Node2*);
	c_NodeEnumerator3* m_new2();
	bool p_HasNext();
	c_Node2* p_NextObject();
	void mark();
};
String bb_config_Enquote(String,String);
class c_CppTranslator : public c_CTranslator{
	public:
	bool m_unsafe;
	int m_gc_mode;
	c_Stack9* m_dbgLocals;
	String m_lastDbgInfo;
	int m_pure;
	c_CppTranslator();
	c_CppTranslator* m_new();
	String p_TransType(c_Type*);
	String p_TransRefType(c_Type*);
	String p_TransValue(c_Type*,String);
	c_Expr* p_Uncast(c_Expr*);
	bool p_IsGcObject(c_Expr*);
	String p_GcRetain(c_Expr*,String);
	String p_TransLocalDecl(String,c_Expr*);
	int p_BeginLocalScope();
	int p_EndLocalScope();
	int p_EmitEnter(c_FuncDecl*);
	int p_EmitEnterBlock();
	bool p_IsDebuggable(c_Type*);
	int p_EmitSetErr(String);
	int p_EmitLeaveBlock();
	String p_TransStatic(c_Decl*);
	String p_TransGlobal(c_GlobalDecl*);
	int p_EmitFuncProto(c_FuncDecl*);
	int p_EmitClassProto(c_ClassDecl*);
	int p_BeginLoop();
	int p_EndLoop();
	int p_EmitFuncDecl(c_FuncDecl*);
	String p_TransField(c_FieldDecl*,c_Expr*);
	int p_EmitMark(String,c_Type*,bool);
	int p_EmitClassDecl(c_ClassDecl*);
	String p_TransApp(c_AppDecl*);
	int p_CheckSafe(c_Decl*);
	String p_TransArgs2(Array<c_Expr* >,c_FuncDecl*);
	String p_TransFunc(c_FuncDecl*,Array<c_Expr* >,c_Expr*);
	String p_TransSuperFunc(c_FuncDecl*,Array<c_Expr* >);
	String p_TransConstExpr(c_ConstExpr*);
	String p_TransNewObjectExpr(c_NewObjectExpr*);
	String p_TransNewArrayExpr(c_NewArrayExpr*);
	String p_TransSelfExpr(c_SelfExpr*);
	String p_TransCastExpr(c_CastExpr*);
	String p_TransUnaryExpr(c_UnaryExpr*);
	String p_TransBinaryExpr(c_BinaryExpr*);
	String p_TransIndexExpr(c_IndexExpr*);
	String p_TransSliceExpr(c_SliceExpr*);
	String p_TransArrayExpr(c_ArrayExpr*);
	String p_TransIntrinsicExpr(c_Decl*,c_Expr*,Array<c_Expr* >);
	String p_TransTryStmt(c_TryStmt*);
	String p_TransDeclStmt(c_DeclStmt*);
	bool p_IsLocalVar(c_Expr*);
	String p_TransAssignStmt2(c_AssignStmt*);
	void mark();
};
class c_JsTranslator : public c_CTranslator{
	public:
	c_JsTranslator();
	c_JsTranslator* m_new();
	String p_TransValue(c_Type*,String);
	String p_TransLocalDecl(String,c_Expr*);
	int p_EmitEnter(c_FuncDecl*);
	int p_EmitSetErr(String);
	int p_EmitLeave();
	String p_TransStatic(c_Decl*);
	String p_TransGlobal(c_GlobalDecl*);
	String p_TransField(c_FieldDecl*,c_Expr*);
	int p_EmitFuncDecl(c_FuncDecl*);
	int p_EmitClassDecl(c_ClassDecl*);
	String p_TransApp(c_AppDecl*);
	String p_TransArgs3(Array<c_Expr* >,String);
	String p_TransFunc(c_FuncDecl*,Array<c_Expr* >,c_Expr*);
	String p_TransSuperFunc(c_FuncDecl*,Array<c_Expr* >);
	String p_TransConstExpr(c_ConstExpr*);
	String p_TransNewObjectExpr(c_NewObjectExpr*);
	String p_TransNewArrayExpr(c_NewArrayExpr*);
	String p_TransSelfExpr(c_SelfExpr*);
	String p_TransCastExpr(c_CastExpr*);
	String p_TransUnaryExpr(c_UnaryExpr*);
	String p_TransBinaryExpr(c_BinaryExpr*);
	String p_TransIndexExpr(c_IndexExpr*);
	String p_TransSliceExpr(c_SliceExpr*);
	String p_TransArrayExpr(c_ArrayExpr*);
	String p_TransTryStmt(c_TryStmt*);
	String p_TransIntrinsicExpr(c_Decl*,c_Expr*,Array<c_Expr* >);
	void mark();
};
extern int bb_html5_Info_Width;
extern int bb_html5_Info_Height;
class c_Stream : public Object{
	public:
	c_Stream();
	c_Stream* m_new();
	virtual int p_Read(c_DataBuffer*,int,int)=0;
	virtual void p_Close()=0;
	virtual int p_Eof()=0;
	virtual int p_Position()=0;
	virtual int p_Seek(int)=0;
	void mark();
};
class c_FileStream : public c_Stream{
	public:
	BBFileStream* m__stream;
	c_FileStream();
	static BBFileStream* m_OpenStream(String,String);
	c_FileStream* m_new(String,String);
	c_FileStream* m_new2(BBFileStream*);
	c_FileStream* m_new3();
	static c_FileStream* m_Open(String,String);
	int p_Read(c_DataBuffer*,int,int);
	void p_Close();
	int p_Eof();
	int p_Position();
	int p_Seek(int);
	void mark();
};
class c_DataBuffer : public BBDataBuffer{
	public:
	c_DataBuffer();
	c_DataBuffer* m_new(int,bool);
	c_DataBuffer* m_new2();
	void mark();
};
int bb_html5_GetInfo_PNG(String);
int bb_html5_GetInfo_JPG(String);
int bb_html5_GetInfo_GIF(String);
class c_AsTranslator : public c_CTranslator{
	public:
	c_AsTranslator();
	c_AsTranslator* m_new();
	String p_TransValue(c_Type*,String);
	String p_TransType(c_Type*);
	String p_TransLocalDecl(String,c_Expr*);
	int p_EmitEnter(c_FuncDecl*);
	int p_EmitSetErr(String);
	int p_EmitLeave();
	String p_TransStatic(c_Decl*);
	String p_TransGlobal(c_GlobalDecl*);
	String p_TransField(c_FieldDecl*,c_Expr*);
	String p_TransValDecl(c_ValDecl*);
	int p_EmitFuncDecl(c_FuncDecl*);
	int p_EmitClassDecl(c_ClassDecl*);
	String p_TransApp(c_AppDecl*);
	String p_TransArgs(Array<c_Expr* >);
	String p_TransFunc(c_FuncDecl*,Array<c_Expr* >,c_Expr*);
	String p_TransSuperFunc(c_FuncDecl*,Array<c_Expr* >);
	String p_TransConstExpr(c_ConstExpr*);
	String p_TransNewObjectExpr(c_NewObjectExpr*);
	String p_TransNewArrayExpr(c_NewArrayExpr*);
	String p_TransSelfExpr(c_SelfExpr*);
	String p_TransCastExpr(c_CastExpr*);
	String p_TransUnaryExpr(c_UnaryExpr*);
	String p_TransBinaryExpr(c_BinaryExpr*);
	String p_TransIndexExpr(c_IndexExpr*);
	String p_TransSliceExpr(c_SliceExpr*);
	String p_TransArrayExpr(c_ArrayExpr*);
	String p_TransIntrinsicExpr(c_Decl*,c_Expr*,Array<c_Expr* >);
	String p_TransTryStmt(c_TryStmt*);
	void mark();
};
class c_CsTranslator : public c_CTranslator{
	public:
	c_CsTranslator();
	c_CsTranslator* m_new();
	String p_TransType(c_Type*);
	String p_TransValue(c_Type*,String);
	String p_TransLocalDecl(String,c_Expr*);
	int p_EmitEnter(c_FuncDecl*);
	int p_EmitSetErr(String);
	int p_EmitLeave();
	String p_TransStatic(c_Decl*);
	String p_TransGlobal(c_GlobalDecl*);
	String p_TransField(c_FieldDecl*,c_Expr*);
	int p_EmitFuncDecl(c_FuncDecl*);
	String p_TransDecl(c_Decl*);
	int p_EmitClassDecl(c_ClassDecl*);
	String p_TransApp(c_AppDecl*);
	String p_TransArgs(Array<c_Expr* >);
	String p_TransFunc(c_FuncDecl*,Array<c_Expr* >,c_Expr*);
	String p_TransSuperFunc(c_FuncDecl*,Array<c_Expr* >);
	String p_TransConstExpr(c_ConstExpr*);
	String p_TransNewObjectExpr(c_NewObjectExpr*);
	String p_TransNewArrayExpr(c_NewArrayExpr*);
	String p_TransSelfExpr(c_SelfExpr*);
	String p_TransCastExpr(c_CastExpr*);
	String p_TransUnaryExpr(c_UnaryExpr*);
	String p_TransBinaryExpr(c_BinaryExpr*);
	String p_TransIndexExpr(c_IndexExpr*);
	String p_TransSliceExpr(c_SliceExpr*);
	String p_TransArrayExpr(c_ArrayExpr*);
	String p_TransIntrinsicExpr(c_Decl*,c_Expr*,Array<c_Expr* >);
	String p_TransTryStmt(c_TryStmt*);
	void mark();
};
class c_List9 : public Object{
	public:
	c_Node16* m__head;
	c_List9();
	c_List9* m_new();
	c_Node16* p_AddLast9(c_ModuleDecl*);
	c_List9* m_new2(Array<c_ModuleDecl* >);
	bool p_IsEmpty();
	c_ModuleDecl* p_RemoveLast();
	bool p_Equals4(c_ModuleDecl*,c_ModuleDecl*);
	c_Node16* p_FindLast7(c_ModuleDecl*,c_Node16*);
	c_Node16* p_FindLast8(c_ModuleDecl*);
	void p_RemoveLast5(c_ModuleDecl*);
	void mark();
};
class c_Node16 : public Object{
	public:
	c_Node16* m__succ;
	c_Node16* m__pred;
	c_ModuleDecl* m__data;
	c_Node16();
	c_Node16* m_new(c_Node16*,c_Node16*,c_ModuleDecl*);
	c_Node16* m_new2();
	int p_Remove();
	void mark();
};
class c_HeadNode9 : public c_Node16{
	public:
	c_HeadNode9();
	c_HeadNode9* m_new();
	void mark();
};
class c_Enumerator5 : public Object{
	public:
	c_List5* m__list;
	c_Node10* m__curr;
	c_Enumerator5();
	c_Enumerator5* m_new(c_List5*);
	c_Enumerator5* m_new2();
	bool p_HasNext();
	c_Stmt* p_NextObject();
	void mark();
};
class c_InvokeExpr : public c_Expr{
	public:
	c_FuncDecl* m_decl;
	Array<c_Expr* > m_args;
	c_InvokeExpr();
	c_InvokeExpr* m_new(c_FuncDecl*,Array<c_Expr* >);
	c_InvokeExpr* m_new2();
	c_Expr* p_Semant();
	String p_ToString();
	String p_Trans();
	String p_TransStmt();
	void mark();
};
class c_StmtExpr : public c_Expr{
	public:
	c_Stmt* m_stmt;
	c_Expr* m_expr;
	c_StmtExpr();
	c_StmtExpr* m_new(c_Stmt*,c_Expr*);
	c_StmtExpr* m_new2();
	c_Expr* p_Semant();
	c_Expr* p_Copy();
	String p_ToString();
	String p_Trans();
	void mark();
};
class c_MemberVarExpr : public c_Expr{
	public:
	c_Expr* m_expr;
	c_VarDecl* m_decl;
	c_MemberVarExpr();
	c_MemberVarExpr* m_new(c_Expr*,c_VarDecl*);
	c_MemberVarExpr* m_new2();
	c_Expr* p_Semant();
	String p_ToString();
	bool p_SideEffects();
	c_Expr* p_SemantSet(String,c_Expr*);
	String p_Trans();
	String p_TransVar();
	void mark();
};
class c_VarExpr : public c_Expr{
	public:
	c_VarDecl* m_decl;
	c_VarExpr();
	c_VarExpr* m_new(c_VarDecl*);
	c_VarExpr* m_new2();
	c_Expr* p_Semant();
	String p_ToString();
	bool p_SideEffects();
	c_Expr* p_SemantSet(String,c_Expr*);
	String p_Trans();
	String p_TransVar();
	void mark();
};
extern int bb_decl__loopnest;
class c_Map8 : public Object{
	public:
	c_Node17* m_root;
	c_Map8();
	c_Map8* m_new();
	virtual int p_Compare(String,String)=0;
	c_Node17* p_FindNode(String);
	c_FuncDeclList* p_Get(String);
	int p_RotateLeft8(c_Node17*);
	int p_RotateRight8(c_Node17*);
	int p_InsertFixup8(c_Node17*);
	bool p_Set8(String,c_FuncDeclList*);
	void mark();
};
class c_StringMap8 : public c_Map8{
	public:
	c_StringMap8();
	c_StringMap8* m_new();
	int p_Compare(String,String);
	void mark();
};
class c_Node17 : public Object{
	public:
	String m_key;
	c_Node17* m_right;
	c_Node17* m_left;
	c_FuncDeclList* m_value;
	int m_color;
	c_Node17* m_parent;
	c_Node17();
	c_Node17* m_new(String,c_FuncDeclList*,int,c_Node17*);
	c_Node17* m_new2();
	void mark();
};
class c_Map9 : public Object{
	public:
	c_Node18* m_root;
	c_Map9();
	c_Map9* m_new();
	virtual int p_Compare(String,String)=0;
	c_Node18* p_FindNode(String);
	bool p_Contains(String);
	int p_RotateLeft9(c_Node18*);
	int p_RotateRight9(c_Node18*);
	int p_InsertFixup9(c_Node18*);
	bool p_Set9(String,c_FuncDecl*);
	void mark();
};
class c_StringMap9 : public c_Map9{
	public:
	c_StringMap9();
	c_StringMap9* m_new();
	int p_Compare(String,String);
	void mark();
};
class c_Node18 : public Object{
	public:
	String m_key;
	c_Node18* m_right;
	c_Node18* m_left;
	c_FuncDecl* m_value;
	int m_color;
	c_Node18* m_parent;
	c_Node18();
	c_Node18* m_new(String,c_FuncDecl*,int,c_Node18*);
	c_Node18* m_new2();
	void mark();
};
class c_Map10 : public Object{
	public:
	c_Node19* m_root;
	c_Map10();
	c_Map10* m_new();
	virtual int p_Compare(String,String)=0;
	c_Node19* p_FindNode(String);
	c_StringSet* p_Get(String);
	int p_RotateLeft10(c_Node19*);
	int p_RotateRight10(c_Node19*);
	int p_InsertFixup10(c_Node19*);
	bool p_Set10(String,c_StringSet*);
	void mark();
};
class c_StringMap10 : public c_Map10{
	public:
	c_StringMap10();
	c_StringMap10* m_new();
	int p_Compare(String,String);
	void mark();
};
class c_Node19 : public Object{
	public:
	String m_key;
	c_Node19* m_right;
	c_Node19* m_left;
	c_StringSet* m_value;
	int m_color;
	c_Node19* m_parent;
	c_Node19();
	c_Node19* m_new(String,c_StringSet*,int,c_Node19*);
	c_Node19* m_new2();
	void mark();
};
class c_Enumerator6 : public Object{
	public:
	c_List8* m__list;
	c_Node13* m__curr;
	c_Enumerator6();
	c_Enumerator6* m_new(c_List8*);
	c_Enumerator6* m_new2();
	bool p_HasNext();
	c_GlobalDecl* p_NextObject();
	void mark();
};
class c_Stack9 : public Object{
	public:
	Array<c_LocalDecl* > m_data;
	int m_length;
	c_Stack9();
	c_Stack9* m_new();
	c_Stack9* m_new2(Array<c_LocalDecl* >);
	static c_LocalDecl* m_NIL;
	void p_Clear();
	c_Enumerator7* p_ObjectEnumerator();
	void p_Length(int);
	int p_Length2();
	void p_Push25(c_LocalDecl*);
	void p_Push26(Array<c_LocalDecl* >,int,int);
	void p_Push27(Array<c_LocalDecl* >,int);
	void mark();
};
class c_Enumerator7 : public Object{
	public:
	c_Stack9* m_stack;
	int m_index;
	c_Enumerator7();
	c_Enumerator7* m_new(c_Stack9*);
	c_Enumerator7* m_new2();
	bool p_HasNext();
	c_LocalDecl* p_NextObject();
	void mark();
};
c_TransCC::c_TransCC(){
	m_args=Array<String >();
	m_cerberusdir=String();
	m_opt_srcpath=String();
	m_opt_safe=false;
	m_opt_clean=false;
	m_opt_check=false;
	m_opt_update=false;
	m_opt_build=false;
	m_opt_run=false;
	m_opt_cfgfile=String();
	m_opt_output=String();
	m_opt_config=String();
	m_opt_target=String();
	m_opt_modpath=String();
	m_opt_builddir=String();
	m_ANDROID_PATH=String();
	m_ANDROID_NDK_PATH=String();
	m_JDK_PATH=String();
	m_ANT_PATH=String();
	m_FLEX_PATH=String();
	m_MINGW_PATH=String();
	m_PSM_PATH=String();
	m_MSBUILD_PATH=String();
	m_AGK_PATH=String();
	m_HTML_PLAYER=String();
	m_FLASH_PLAYER=String();
	m__libs=String();
	m__builders=(new c_StringMap3)->m_new();
	m__targets=(new c_StringMap6)->m_new();
	m_target=0;
}
c_TransCC* c_TransCC::m_new(){
	return this;
}
void c_TransCC::p_ParseArgs(){
	if(m_args.Length()>1){
		m_opt_srcpath=bb_transcc_StripQuotes(m_args[m_args.Length()-1].Trim());
	}
	for(int t_i=1;t_i<m_args.Length()-1;t_i=t_i+1){
		String t_arg=m_args[t_i].Trim();
		String t_rhs=String();
		int t_j=t_arg.Find(String(L"=",1),0);
		if(t_j!=-1){
			t_rhs=bb_transcc_StripQuotes(t_arg.Slice(t_j+1));
			t_arg=t_arg.Slice(0,t_j);
		}
		if(t_j==-1){
			String t_1=t_arg.ToLower();
			if(t_1==String(L"-safe",5)){
				m_opt_safe=true;
			}else{
				if(t_1==String(L"-clean",6)){
					m_opt_clean=true;
				}else{
					if(t_1==String(L"-check",6)){
						m_opt_check=true;
					}else{
						if(t_1==String(L"-update",7)){
							m_opt_check=true;
							m_opt_update=true;
						}else{
							if(t_1==String(L"-build",6)){
								m_opt_check=true;
								m_opt_update=true;
								m_opt_build=true;
							}else{
								if(t_1==String(L"-run",4)){
									m_opt_check=true;
									m_opt_update=true;
									m_opt_build=true;
									m_opt_run=true;
								}else{
									bb_transcc_Die(String(L"Unrecognized command line option: ",34)+t_arg);
								}
							}
						}
					}
				}
			}
		}else{
			if(t_arg.StartsWith(String(L"-",1))){
				String t_2=t_arg.ToLower();
				if(t_2==String(L"-cfgfile",8)){
					m_opt_cfgfile=t_rhs;
				}else{
					if(t_2==String(L"-output",7)){
						m_opt_output=t_rhs;
					}else{
						if(t_2==String(L"-config",7)){
							m_opt_config=t_rhs.ToLower();
						}else{
							if(t_2==String(L"-target",7)){
								m_opt_target=t_rhs;
							}else{
								if(t_2==String(L"-modpath",8)){
									m_opt_modpath=t_rhs;
								}else{
									if(t_2==String(L"-builddir",9)){
										m_opt_builddir=t_rhs;
									}else{
										bb_transcc_Die(String(L"Unrecognized command line option: ",34)+t_arg);
									}
								}
							}
						}
					}
				}
			}else{
				if(t_arg.StartsWith(String(L"+",1))){
					bb_config_SetConfigVar2(t_arg.Slice(1),t_rhs);
				}else{
					bb_transcc_Die(String(L"Command line arg error: ",24)+t_arg);
				}
			}
		}
	}
}
void c_TransCC::p_LoadConfig(){
	String t_cfgpath=m_cerberusdir+String(L"/bin/",5);
	if((m_opt_cfgfile).Length()!=0){
		t_cfgpath=t_cfgpath+m_opt_cfgfile;
	}else{
		t_cfgpath=t_cfgpath+(String(L"config.",7)+HostOS()+String(L".txt",4));
	}
	if(FileType(t_cfgpath)!=1){
		bb_transcc_Die(String(L"Failed to open config file",26));
	}
	String t_cfg=LoadString(t_cfgpath);
	Array<String > t_=t_cfg.Split(String(L"\n",1));
	int t_2=0;
	while(t_2<t_.Length()){
		String t_line=t_[t_2];
		t_2=t_2+1;
		t_line=t_line.Trim();
		if(!((t_line).Length()!=0) || t_line.StartsWith(String(L"'",1))){
			continue;
		}
		int t_i=t_line.Find(String(L"=",1),0);
		if(t_i==-1){
			bb_transcc_Die(String(L"Error in config file, line=",27)+t_line);
		}
		String t_lhs=t_line.Slice(0,t_i).Trim();
		String t_rhs=t_line.Slice(t_i+1).Trim();
		t_rhs=bb_transcc_ReplaceEnv(t_rhs);
		String t_path=bb_transcc_StripQuotes(t_rhs);
		while(t_path.EndsWith(String(L"/",1)) || t_path.EndsWith(String(L"\\",1))){
			t_path=t_path.Slice(0,-1);
		}
		String t_3=t_lhs;
		if(t_3==String(L"MODPATH",7)){
			if(!((m_opt_modpath).Length()!=0)){
				m_opt_modpath=t_path;
			}
		}else{
			if(t_3==String(L"ANDROID_PATH",12)){
				if(!((m_ANDROID_PATH).Length()!=0) && FileType(t_path)==2){
					m_ANDROID_PATH=t_path;
				}
			}else{
				if(t_3==String(L"ANDROID_NDK_PATH",16)){
					if(!((m_ANDROID_NDK_PATH).Length()!=0) && FileType(t_path)==2){
						m_ANDROID_NDK_PATH=t_path;
					}
				}else{
					if(t_3==String(L"JDK_PATH",8)){
						if(!((m_JDK_PATH).Length()!=0) && FileType(t_path)==2){
							m_JDK_PATH=t_path;
						}
					}else{
						if(t_3==String(L"ANT_PATH",8)){
							if(!((m_ANT_PATH).Length()!=0) && FileType(t_path)==2){
								m_ANT_PATH=t_path;
							}
						}else{
							if(t_3==String(L"FLEX_PATH",9)){
								if(!((m_FLEX_PATH).Length()!=0) && FileType(t_path)==2){
									m_FLEX_PATH=t_path;
								}
							}else{
								if(t_3==String(L"MINGW_PATH",10)){
									if(!((m_MINGW_PATH).Length()!=0) && FileType(t_path)==2){
										m_MINGW_PATH=t_path;
									}
								}else{
									if(t_3==String(L"PSM_PATH",8)){
										if(!((m_PSM_PATH).Length()!=0) && FileType(t_path)==2){
											m_PSM_PATH=t_path;
										}
									}else{
										if(t_3==String(L"MSBUILD_PATH",12)){
											if(!((m_MSBUILD_PATH).Length()!=0) && FileType(t_path)==1){
												m_MSBUILD_PATH=t_path;
											}
										}else{
											if(t_3==String(L"AGK_PATH",8)){
												if(!((m_AGK_PATH).Length()!=0) && FileType(t_path)==2){
													m_AGK_PATH=t_path;
												}
											}else{
												if(t_3==String(L"HTML_PLAYER",11)){
													m_HTML_PLAYER=t_rhs;
												}else{
													if(t_3==String(L"FLASH_PLAYER",12)){
														m_FLASH_PLAYER=t_rhs;
													}else{
														bbPrint(String(L"Trans: ignoring unrecognized config var: ",41)+t_lhs);
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	String t_4=HostOS();
	if(t_4==String(L"winnt",5)){
		String t_path2=GetEnv(String(L"PATH",4));
		if((m_ANDROID_PATH).Length()!=0){
			t_path2=t_path2+(String(L";",1)+m_ANDROID_PATH+String(L"/tools",6));
		}
		if((m_ANDROID_PATH).Length()!=0){
			t_path2=t_path2+(String(L";",1)+m_ANDROID_PATH+String(L"/platform-tools",15));
		}
		if((m_JDK_PATH).Length()!=0){
			t_path2=t_path2+(String(L";",1)+m_JDK_PATH+String(L"/bin",4));
		}
		if((m_ANT_PATH).Length()!=0){
			t_path2=t_path2+(String(L";",1)+m_ANT_PATH+String(L"/bin",4));
		}
		if((m_FLEX_PATH).Length()!=0){
			t_path2=t_path2+(String(L";",1)+m_FLEX_PATH+String(L"/bin",4));
		}
		if((m_MINGW_PATH).Length()!=0){
			t_path2=m_MINGW_PATH+String(L"/bin;",5)+t_path2;
		}
		SetEnv(String(L"PATH",4),t_path2);
		if((m_JDK_PATH).Length()!=0){
			SetEnv(String(L"JAVA_HOME",9),m_JDK_PATH);
		}
		m__libs=AppPath();
		m__libs=m__libs.Slice(0,m__libs.Find(String(L"bin/transcc_winnt.exe",21),0))+String(L"libs/",5);
	}else{
		if(t_4==String(L"macos",5)){
			String t_path3=GetEnv(String(L"PATH",4));
			if((m_ANDROID_PATH).Length()!=0){
				t_path3=t_path3+(String(L":",1)+m_ANDROID_PATH+String(L"/tools",6));
			}
			if((m_ANDROID_PATH).Length()!=0){
				t_path3=t_path3+(String(L":",1)+m_ANDROID_PATH+String(L"/platform-tools",15));
			}
			if((m_ANT_PATH).Length()!=0){
				t_path3=t_path3+(String(L":",1)+m_ANT_PATH+String(L"/bin",4));
			}
			if((m_FLEX_PATH).Length()!=0){
				t_path3=t_path3+(String(L":",1)+m_FLEX_PATH+String(L"/bin",4));
			}
			SetEnv(String(L"PATH",4),t_path3);
		}else{
			if(t_4==String(L"linux",5)){
				String t_path4=GetEnv(String(L"PATH",4));
				if((m_JDK_PATH).Length()!=0){
					t_path4=m_JDK_PATH+String(L"/bin:",5)+t_path4;
				}
				if((m_ANDROID_PATH).Length()!=0){
					t_path4=m_ANDROID_PATH+String(L"/platform-tools:",16)+t_path4;
				}
				if((m_FLEX_PATH).Length()!=0){
					t_path4=m_FLEX_PATH+String(L"/bin:",5)+t_path4;
				}
				SetEnv(String(L"PATH",4),t_path4);
			}
		}
	}
}
void c_TransCC::p_EnumBuilders(){
	c_NodeEnumerator* t_=bb_builders_Builders(this)->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_Node3* t_it=t_->p_NextObject();
		if(t_it->p_Value()->p_IsValid()){
			m__builders->p_Set3(t_it->p_Key(),t_it->p_Value());
		}
	}
}
void c_TransCC::p_EnumTargets(String t_dir){
	String t_p=m_cerberusdir+String(L"/",1)+t_dir;
	Array<String > t_=LoadDir(t_p);
	int t_2=0;
	while(t_2<t_.Length()){
		String t_f=t_[t_2];
		t_2=t_2+1;
		String t_t=t_p+String(L"/",1)+t_f+String(L"/target.cxs",11);
		if(FileType(t_t)!=1){
			t_t=t_p+String(L"/",1)+t_f+String(L"/TARGET.MONKEY",14);
			if(FileType(t_t)!=1){
				continue;
			}
		}
		bb_config_PushConfigScope();
		bb_preprocessor_PreProcess(t_t,0);
		String t_name=bb_config_GetConfigVar(String(L"TARGET_NAME",11));
		if((t_name).Length()!=0){
			String t_system=bb_config_GetConfigVar(String(L"TARGET_SYSTEM",13));
			if((t_system).Length()!=0){
				c_Builder* t_builder=m__builders->p_Get(bb_config_GetConfigVar(String(L"TARGET_BUILDER",14)));
				if((t_builder)!=0){
					String t_host=bb_config_GetConfigVar(String(L"TARGET_HOST",11));
					if(!((t_host).Length()!=0) || t_host==HostOS()){
						m__targets->p_Set6(t_name,(new c_Target)->m_new(t_f,t_name,t_system,t_builder));
					}
				}
			}
		}
		bb_config_PopConfigScope();
	}
}
String c_TransCC::p_GetReleaseVersion(){
	String t_f=LoadString(m_cerberusdir+String(L"/VERSIONS.TXT",13));
	Array<String > t_=t_f.Split(String(L"\n",1));
	int t_2=0;
	while(t_2<t_.Length()){
		String t_t=t_[t_2];
		t_2=t_2+1;
		t_t=t_t.Trim();
		if(t_t.StartsWith(String(L"***** v",7)) && t_t.EndsWith(String(L" *****",6))){
			return t_t.Slice(6,-6);
		}
	}
	return String();
}
void c_TransCC::p_Run(Array<String > t_args){
	gc_assign(this->m_args,t_args);
	bbPrint(String(L"TRANS cerberus compiler V2019-02-21",35));
	m_cerberusdir=RealPath(bb_os_ExtractDir(AppPath())+String(L"/..",3));
	SetEnv(String(L"CERBERUSDIR",11),m_cerberusdir);
	SetEnv(String(L"MONKEYDIR",9),m_cerberusdir);
	SetEnv(String(L"TRANSDIR",8),m_cerberusdir+String(L"/bin",4));
	p_ParseArgs();
	p_LoadConfig();
	p_EnumBuilders();
	p_EnumTargets(String(L"targets",7));
	if(t_args.Length()<2){
		String t_valid=String();
		c_NodeEnumerator2* t_=m__targets->p_ObjectEnumerator();
		while(t_->p_HasNext()){
			c_Node14* t_it=t_->p_NextObject();
			t_valid=t_valid+(String(L" ",1)+t_it->p_Key().Replace(String(L" ",1),String(L"_",1)));
		}
		bbPrint(String(L"TRANS Usage: transcc [-update] [-build] [-run] [-clean] [-config=...] [-target=...] [-cfgfile=...] [-modpath=...] <main_cerberus_source_file>",141));
		bbPrint(String(L"Valid targets:",14)+t_valid);
		bbPrint(String(L"Valid configs: debug release",28));
		ExitApp(0);
	}
	gc_assign(m_target,m__targets->p_Get(m_opt_target.Replace(String(L"_",1),String(L" ",1))));
	if(!((m_target)!=0)){
		bb_transcc_Die(String(L"Invalid target",14));
	}
	m_target->m_builder->p_Make();
}
bool c_TransCC::p_Execute(String t_cmd,bool t_failHard){
	int t_r=Execute(t_cmd);
	if(!((t_r)!=0)){
		return true;
	}
	if(t_failHard){
		bb_transcc_Die(String(L"Error executing '",17)+t_cmd+String(L"', return code=",15)+String(t_r));
	}
	return false;
}
void c_TransCC::mark(){
	Object::mark();
	gc_mark_q(m_args);
	gc_mark_q(m__builders);
	gc_mark_q(m__targets);
	gc_mark_q(m_target);
}
String bb_os_ExtractDir(String t_path){
	int t_i=t_path.FindLast(String(L"/",1));
	if(t_i==-1){
		t_i=t_path.FindLast(String(L"\\",1));
	}
	if(t_i!=-1){
		return t_path.Slice(0,t_i);
	}
	return String();
}
String bb_transcc_StripQuotes(String t_str){
	if(t_str.Length()>=2 && t_str.StartsWith(String(L"\"",1)) && t_str.EndsWith(String(L"\"",1))){
		return t_str.Slice(1,-1);
	}
	return t_str;
}
int bb_transcc_Die(String t_msg){
	bbPrint(String(L"TRANS FAILED: ",14)+t_msg);
	ExitApp(-1);
	return 0;
}
c_Type::c_Type(){
	m_arrayOf=0;
}
c_Type* c_Type::m_new(){
	return this;
}
c_StringType* c_Type::m_stringType;
c_IntType* c_Type::m_intType;
c_FloatType* c_Type::m_floatType;
c_BoolType* c_Type::m_boolType;
c_VoidType* c_Type::m_voidType;
c_IdentType* c_Type::m_objectType;
c_IdentType* c_Type::m_throwableType;
c_ArrayType* c_Type::p_ArrayOf(){
	if(!((m_arrayOf)!=0)){
		gc_assign(m_arrayOf,(new c_ArrayType)->m_new(this));
	}
	return m_arrayOf;
}
c_ArrayType* c_Type::m_emptyArrayType;
c_IdentType* c_Type::m_nullObjectType;
String c_Type::p_ToString(){
	return String(L"??Type??",8);
}
int c_Type::p_EqualsType(c_Type* t_ty){
	return 0;
}
c_Type* c_Type::p_Semant(){
	return this;
}
int c_Type::p_ExtendsType(c_Type* t_ty){
	return p_EqualsType(t_ty);
}
c_ClassDecl* c_Type::p_GetClass(){
	return 0;
}
void c_Type::mark(){
	Object::mark();
	gc_mark_q(m_arrayOf);
}
c_StringType::c_StringType(){
}
c_StringType* c_StringType::m_new(){
	c_Type::m_new();
	return this;
}
int c_StringType::p_EqualsType(c_Type* t_ty){
	return ((dynamic_cast<c_StringType*>(t_ty)!=0)?1:0);
}
int c_StringType::p_ExtendsType(c_Type* t_ty){
	if((dynamic_cast<c_ObjectType*>(t_ty))!=0){
		c_Expr* t_expr=((new c_ConstExpr)->m_new((this),String()))->p_Semant();
		c_Expr* t_[]={t_expr};
		c_FuncDecl* t_ctor=t_ty->p_GetClass()->p_FindFuncDecl(String(L"new",3),Array<c_Expr* >(t_,1),1);
		return ((((t_ctor)!=0) && t_ctor->p_IsCtor())?1:0);
	}
	return p_EqualsType(t_ty);
}
c_ClassDecl* c_StringType::p_GetClass(){
	return dynamic_cast<c_ClassDecl*>(bb_decl__env->p_FindDecl(String(L"string",6)));
}
String c_StringType::p_ToString(){
	return String(L"String",6);
}
void c_StringType::mark(){
	c_Type::mark();
}
c_Decl::c_Decl(){
	m_errInfo=String();
	m_ident=String();
	m_munged=String();
	m_attrs=0;
	m_scope=0;
}
c_Decl* c_Decl::m_new(){
	m_errInfo=bb_config__errInfo;
	return this;
}
int c_Decl::p_IsSemanted(){
	return (((m_attrs&1048576)!=0)?1:0);
}
int c_Decl::p_IsPublic(){
	return (((m_attrs&16896)==0)?1:0);
}
c_ModuleDecl* c_Decl::p_ModuleScope(){
	if((dynamic_cast<c_ModuleDecl*>(this))!=0){
		return dynamic_cast<c_ModuleDecl*>(this);
	}
	if((m_scope)!=0){
		return m_scope->p_ModuleScope();
	}
	return 0;
}
int c_Decl::p_IsProtected(){
	return (((m_attrs&16384)!=0)?1:0);
}
c_ClassDecl* c_Decl::p_ClassScope(){
	if((dynamic_cast<c_ClassDecl*>(this))!=0){
		return dynamic_cast<c_ClassDecl*>(this);
	}
	if((m_scope)!=0){
		return m_scope->p_ClassScope();
	}
	return 0;
}
c_FuncDecl* c_Decl::p_FuncScope(){
	if((dynamic_cast<c_FuncDecl*>(this))!=0){
		return dynamic_cast<c_FuncDecl*>(this);
	}
	if((m_scope)!=0){
		return m_scope->p_FuncScope();
	}
	return 0;
}
int c_Decl::p_CheckAccess(){
	if(!((bb_decl__env)!=0)){
		return 1;
	}
	if((p_IsPublic())!=0){
		return 1;
	}
	c_ModuleDecl* t_mdecl=p_ModuleScope();
	if((t_mdecl)!=0){
		c_ModuleDecl* t_mdecl2=bb_decl__env->p_ModuleScope();
		if(t_mdecl==t_mdecl2){
			return 1;
		}
		if(((t_mdecl2)!=0) && t_mdecl->m_friends->p_Contains(t_mdecl2->m_rmodpath)){
			return 1;
		}
	}
	if((p_IsProtected())!=0){
		c_ClassDecl* t_thisClass=p_ClassScope();
		c_ClassDecl* t_currentClass=bb_decl__env->p_ClassScope();
		while((t_currentClass)!=0){
			if(t_currentClass==t_thisClass){
				return 1;
			}
			t_currentClass=t_currentClass->m_superClass;
		}
	}
	c_FuncDecl* t_fdecl=bb_decl__env->p_FuncScope();
	if(((t_fdecl)!=0) && ((t_fdecl->m_attrs&8388608)!=0)){
		return 1;
	}
	return 0;
}
int c_Decl::p_IsExtern(){
	return (((m_attrs&256)!=0)?1:0);
}
String c_Decl::p_ToString(){
	if((dynamic_cast<c_ClassDecl*>(m_scope))!=0){
		return m_scope->p_ToString()+String(L".",1)+m_ident;
	}
	return m_ident;
}
int c_Decl::p_IsAbstract(){
	return (((m_attrs&1024)!=0)?1:0);
}
int c_Decl::p_IsSemanting(){
	return (((m_attrs&2097152)!=0)?1:0);
}
c_AppDecl* c_Decl::p_AppScope(){
	if((dynamic_cast<c_AppDecl*>(this))!=0){
		return dynamic_cast<c_AppDecl*>(this);
	}
	if((m_scope)!=0){
		return m_scope->p_AppScope();
	}
	return 0;
}
int c_Decl::p_Semant(){
	if((p_IsSemanted())!=0){
		return 0;
	}
	if((p_IsSemanting())!=0){
		bb_config_Err(String(L"Cyclic declaration of '",23)+m_ident+String(L"'.",2));
	}
	c_ClassDecl* t_cscope=dynamic_cast<c_ClassDecl*>(m_scope);
	if((t_cscope)!=0){
		t_cscope->m_attrs&=-5;
	}
	bb_config_PushErr(m_errInfo);
	if((m_scope)!=0){
		bb_decl_PushEnv(m_scope);
	}
	m_attrs|=2097152;
	p_OnSemant();
	m_attrs&=-2097153;
	m_attrs|=1048576;
	if((m_scope)!=0){
		if((p_IsExtern())!=0){
			if((dynamic_cast<c_ModuleDecl*>(m_scope))!=0){
				p_AppScope()->m_allSemantedDecls->p_AddLast3(this);
			}
		}else{
			m_scope->m_semanted->p_AddLast3(this);
			if((dynamic_cast<c_GlobalDecl*>(this))!=0){
				p_AppScope()->m_semantedGlobals->p_AddLast8(dynamic_cast<c_GlobalDecl*>(this));
			}
			if((dynamic_cast<c_ModuleDecl*>(m_scope))!=0){
				p_AppScope()->m_semanted->p_AddLast3(this);
				p_AppScope()->m_allSemantedDecls->p_AddLast3(this);
			}
		}
		bb_decl_PopEnv();
	}
	bb_config_PopErr();
	return 0;
}
int c_Decl::p_IsPrivate(){
	return (((m_attrs&512)!=0)?1:0);
}
int c_Decl::p_AssertAccess(){
	if((p_CheckAccess())!=0){
		return 0;
	}
	if((p_IsPrivate())!=0){
		bb_config_Err(p_ToString()+String(L" is private.",12));
	}
	if((p_IsProtected())!=0){
		bb_config_Err(p_ToString()+String(L" is protected.",14));
	}
	bb_config_Err(p_ToString()+String(L" is inaccessible.",17));
	return 0;
}
c_Decl* c_Decl::p_Copy(){
	c_Decl* t_t=p_OnCopy();
	t_t->m_munged=m_munged;
	t_t->m_errInfo=m_errInfo;
	return t_t;
}
int c_Decl::p_IsFinal(){
	return (((m_attrs&2048)!=0)?1:0);
}
void c_Decl::mark(){
	Object::mark();
	gc_mark_q(m_scope);
}
c_ScopeDecl::c_ScopeDecl(){
	m_decls=(new c_List3)->m_new();
	m_declsMap=(new c_StringMap4)->m_new();
	m_semanted=(new c_List3)->m_new();
}
c_ScopeDecl* c_ScopeDecl::m_new(){
	c_Decl::m_new();
	return this;
}
int c_ScopeDecl::p_InsertDecl(c_Decl* t_decl){
	if((t_decl->m_scope)!=0){
		bb_config_InternalErr(String(L"Internal error",14));
	}
	String t_ident=t_decl->m_ident;
	if(!((t_ident).Length()!=0)){
		return 0;
	}
	gc_assign(t_decl->m_scope,this);
	m_decls->p_AddLast3(t_decl);
	c_StringMap4* t_decls=0;
	Object* t_tdecl=m_declsMap->p_Get(t_ident);
	if((dynamic_cast<c_FuncDecl*>(t_decl))!=0){
		c_FuncDeclList* t_funcs=dynamic_cast<c_FuncDeclList*>(t_tdecl);
		if(((t_funcs)!=0) || !((t_tdecl)!=0)){
			if(!((t_funcs)!=0)){
				t_funcs=(new c_FuncDeclList)->m_new();
				m_declsMap->p_Insert2(t_ident,(t_funcs));
			}
			t_funcs->p_AddLast4(dynamic_cast<c_FuncDecl*>(t_decl));
		}else{
			bb_config_Err(String(L"Duplicate identifier '",22)+t_ident+String(L"'.",2));
		}
	}else{
		if(!((t_tdecl)!=0)){
			m_declsMap->p_Insert2(t_ident,(t_decl));
		}else{
			bb_config_Err(String(L"Duplicate identifier '",22)+t_ident+String(L"'.",2));
		}
	}
	if((t_decl->p_IsSemanted())!=0){
		m_semanted->p_AddLast3(t_decl);
	}
	return 0;
}
Object* c_ScopeDecl::p_GetDecl(String t_ident){
	Object* t_decl=m_declsMap->p_Get(t_ident);
	if(!((t_decl)!=0)){
		return 0;
	}
	c_AliasDecl* t_adecl=dynamic_cast<c_AliasDecl*>(t_decl);
	if(!((t_adecl)!=0)){
		return t_decl;
	}
	if((t_adecl->p_CheckAccess())!=0){
		return t_adecl->m_decl;
	}
	return 0;
}
Object* c_ScopeDecl::p_FindDecl(String t_ident){
	if(bb_decl__env!=this){
		return p_GetDecl(t_ident);
	}
	c_ScopeDecl* t_tscope=this;
	while((t_tscope)!=0){
		Object* t_decl=t_tscope->p_GetDecl(t_ident);
		if((t_decl)!=0){
			return t_decl;
		}
		t_tscope=t_tscope->m_scope;
	}
	return 0;
}
int c_ScopeDecl::p_InsertDecls(c_List3* t_decls){
	c_Enumerator2* t_=t_decls->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_Decl* t_decl=t_->p_NextObject();
		p_InsertDecl(t_decl);
	}
	return 0;
}
c_FuncDecl* c_ScopeDecl::p_FindFuncDecl(String t_ident,Array<c_Expr* > t_argExprs,int t_explicit){
	c_FuncDeclList* t_funcs=dynamic_cast<c_FuncDeclList*>(p_FindDecl(t_ident));
	if(!((t_funcs)!=0)){
		return 0;
	}
	c_Enumerator3* t_=t_funcs->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_FuncDecl* t_func=t_->p_NextObject();
		t_func->p_Semant();
	}
	c_FuncDecl* t_match=0;
	int t_isexact=0;
	String t_err=String();
	c_Enumerator3* t_2=t_funcs->p_ObjectEnumerator();
	while(t_2->p_HasNext()){
		c_FuncDecl* t_func2=t_2->p_NextObject();
		Array<c_ArgDecl* > t_argDecls=t_func2->m_argDecls;
		if(t_argExprs.Length()>t_argDecls.Length()){
			continue;
		}
		int t_exact=1;
		int t_possible=1;
		for(int t_i=0;t_i<t_argDecls.Length();t_i=t_i+1){
			if(t_i<t_argExprs.Length() && ((t_argExprs[t_i])!=0)){
				c_Type* t_declTy=t_argDecls[t_i]->m_type;
				c_Type* t_exprTy=t_argExprs[t_i]->m_exprType;
				if((t_exprTy->p_EqualsType(t_declTy))!=0){
					continue;
				}
				t_exact=0;
				if(!((t_explicit)!=0) && ((t_exprTy->p_ExtendsType(t_declTy))!=0)){
					continue;
				}
			}else{
				if((t_argDecls[t_i]->m_init)!=0){
					if(!((t_explicit)!=0)){
						continue;
					}
				}
			}
			t_possible=0;
			break;
		}
		if(!((t_possible)!=0)){
			continue;
		}
		if((t_exact)!=0){
			if((t_isexact)!=0){
				bb_config_Err(String(L"Unable to determine overload to use: ",37)+t_match->p_ToString()+String(L" or ",4)+t_func2->p_ToString()+String(L".",1));
			}else{
				t_err=String();
				t_match=t_func2;
				t_isexact=1;
			}
		}else{
			if(!((t_isexact)!=0)){
				if((t_match)!=0){
					t_err=String(L"Unable to determine overload to use: ",37)+t_match->p_ToString()+String(L" or ",4)+t_func2->p_ToString()+String(L".",1);
				}else{
					t_match=t_func2;
				}
			}
		}
	}
	if(!((t_isexact)!=0)){
		if((t_err).Length()!=0){
			bb_config_Err(t_err);
		}
		if((t_explicit)!=0){
			return 0;
		}
	}
	if(!((t_match)!=0)){
		String t_t=String();
		for(int t_i2=0;t_i2<t_argExprs.Length();t_i2=t_i2+1){
			if((t_t).Length()!=0){
				t_t=t_t+String(L",",1);
			}
			if((t_argExprs[t_i2])!=0){
				t_t=t_t+t_argExprs[t_i2]->m_exprType->p_ToString();
			}
		}
		bb_config_Err(String(L"Unable to find overload for ",28)+t_ident+String(L"(",1)+t_t+String(L").",2));
	}
	t_match->p_AssertAccess();
	return t_match;
}
c_List3* c_ScopeDecl::p_Decls(){
	return m_decls;
}
c_Type* c_ScopeDecl::p_FindType(String t_ident,Array<c_Type* > t_args){
	Object* t_decl=p_GetDecl(t_ident);
	if((t_decl)!=0){
		c_Type* t_type=dynamic_cast<c_Type*>(t_decl);
		if((t_type)!=0){
			if((t_args.Length())!=0){
				bb_config_Err(String(L"Wrong number of type arguments",30));
			}
			return t_type;
		}
		c_ClassDecl* t_cdecl=dynamic_cast<c_ClassDecl*>(t_decl);
		if((t_cdecl)!=0){
			t_cdecl->p_AssertAccess();
			t_cdecl=t_cdecl->p_GenClassInstance(t_args);
			t_cdecl->p_Semant();
			return (t_cdecl->m_objectType);
		}
	}
	if((m_scope)!=0){
		return m_scope->p_FindType(t_ident,t_args);
	}
	return 0;
}
c_List4* c_ScopeDecl::p_MethodDecls(String t_id){
	c_List4* t_fdecls=(new c_List4)->m_new();
	c_Enumerator2* t_=m_decls->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_Decl* t_decl=t_->p_NextObject();
		if(((t_id).Length()!=0) && t_decl->m_ident!=t_id){
			continue;
		}
		c_FuncDecl* t_fdecl=dynamic_cast<c_FuncDecl*>(t_decl);
		if(((t_fdecl)!=0) && t_fdecl->p_IsMethod()){
			t_fdecls->p_AddLast4(t_fdecl);
		}
	}
	return t_fdecls;
}
c_List3* c_ScopeDecl::p_Semanted(){
	return m_semanted;
}
c_List4* c_ScopeDecl::p_SemantedMethods(String t_id){
	c_List4* t_fdecls=(new c_List4)->m_new();
	c_Enumerator2* t_=m_semanted->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_Decl* t_decl=t_->p_NextObject();
		if(((t_id).Length()!=0) && t_decl->m_ident!=t_id){
			continue;
		}
		c_FuncDecl* t_fdecl=dynamic_cast<c_FuncDecl*>(t_decl);
		if(((t_fdecl)!=0) && t_fdecl->p_IsMethod()){
			t_fdecls->p_AddLast4(t_fdecl);
		}
	}
	return t_fdecls;
}
c_ValDecl* c_ScopeDecl::p_FindValDecl(String t_ident){
	c_ValDecl* t_decl=dynamic_cast<c_ValDecl*>(p_FindDecl(t_ident));
	if(!((t_decl)!=0)){
		return 0;
	}
	t_decl->p_AssertAccess();
	t_decl->p_Semant();
	return t_decl;
}
c_Decl* c_ScopeDecl::p_OnCopy(){
	bb_config_InternalErr(String(L"Internal error",14));
	return 0;
}
int c_ScopeDecl::p_OnSemant(){
	return 0;
}
c_List4* c_ScopeDecl::p_SemantedFuncs(String t_id){
	c_List4* t_fdecls=(new c_List4)->m_new();
	c_Enumerator2* t_=m_semanted->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_Decl* t_decl=t_->p_NextObject();
		if(((t_id).Length()!=0) && t_decl->m_ident!=t_id){
			continue;
		}
		c_FuncDecl* t_fdecl=dynamic_cast<c_FuncDecl*>(t_decl);
		if((t_fdecl)!=0){
			t_fdecls->p_AddLast4(t_fdecl);
		}
	}
	return t_fdecls;
}
c_ModuleDecl* c_ScopeDecl::p_FindModuleDecl(String t_ident){
	c_ModuleDecl* t_decl=dynamic_cast<c_ModuleDecl*>(p_GetDecl(t_ident));
	if((t_decl)!=0){
		t_decl->p_AssertAccess();
		t_decl->p_Semant();
		return t_decl;
	}
	if((m_scope)!=0){
		return m_scope->p_FindModuleDecl(t_ident);
	}
	return 0;
}
c_List4* c_ScopeDecl::p_FuncDecls(String t_id){
	c_List4* t_fdecls=(new c_List4)->m_new();
	c_Enumerator2* t_=m_decls->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_Decl* t_decl=t_->p_NextObject();
		if(((t_id).Length()!=0) && t_decl->m_ident!=t_id){
			continue;
		}
		c_FuncDecl* t_fdecl=dynamic_cast<c_FuncDecl*>(t_decl);
		if((t_fdecl)!=0){
			t_fdecls->p_AddLast4(t_fdecl);
		}
	}
	return t_fdecls;
}
c_ScopeDecl* c_ScopeDecl::p_FindScopeDecl(String t_ident){
	Object* t_decl=p_FindDecl(t_ident);
	c_Type* t_type=dynamic_cast<c_Type*>(t_decl);
	if((t_type)!=0){
		if(!((dynamic_cast<c_ObjectType*>(t_type))!=0)){
			return 0;
		}
		return (t_type->p_GetClass());
	}
	c_ScopeDecl* t_scope=dynamic_cast<c_ScopeDecl*>(t_decl);
	if((t_scope)!=0){
		c_ClassDecl* t_cdecl=dynamic_cast<c_ClassDecl*>(t_scope);
		if(((t_cdecl)!=0) && ((t_cdecl->m_args).Length()!=0)){
			return 0;
		}
		t_scope->p_AssertAccess();
		t_scope->p_Semant();
		return t_scope;
	}
	return 0;
}
void c_ScopeDecl::mark(){
	c_Decl::mark();
	gc_mark_q(m_decls);
	gc_mark_q(m_declsMap);
	gc_mark_q(m_semanted);
}
c_ConfigScope::c_ConfigScope(){
	m_cdecls=(new c_StringMap)->m_new();
	m_vars=(new c_StringMap2)->m_new();
}
c_ConfigScope* c_ConfigScope::m_new(){
	c_ScopeDecl::m_new();
	return this;
}
c_ValDecl* c_ConfigScope::p_FindValDecl(String t_ident){
	if(m_cdecls->p_Contains(t_ident)){
		return (m_cdecls->p_Get(t_ident));
	}
	return ((new c_ConstDecl)->m_new(t_ident,1048576,(c_Type::m_boolType),0));
}
void c_ConfigScope::mark(){
	c_ScopeDecl::mark();
	gc_mark_q(m_cdecls);
	gc_mark_q(m_vars);
}
String bb_config__errInfo;
c_ConfigScope* bb_config__cfgScope;
c_ValDecl::c_ValDecl(){
	m_type=0;
	m_init=0;
}
c_ValDecl* c_ValDecl::m_new(){
	c_Decl::m_new();
	return this;
}
String c_ValDecl::p_ToString(){
	String t_t=c_Decl::p_ToString();
	if((m_type)!=0){
		return t_t+String(L":",1)+m_type->p_ToString();
	}
	return t_t;
}
int c_ValDecl::p_OnSemant(){
	if((m_type)!=0){
		gc_assign(m_type,m_type->p_Semant());
		if((m_init)!=0){
			gc_assign(m_init,m_init->p_Semant2(m_type,0));
		}
	}else{
		if((m_init)!=0){
			gc_assign(m_init,m_init->p_Semant());
			gc_assign(m_type,m_init->m_exprType);
		}else{
			bb_config_InternalErr(String(L"Internal error",14));
		}
	}
	if((dynamic_cast<c_VoidType*>(m_type))!=0){
		bb_config_Err(String(L"Declaration has void type.",26));
	}
	return 0;
}
c_Expr* c_ValDecl::p_CopyInit(){
	if((m_init)!=0){
		return m_init->p_Copy();
	}
	return 0;
}
void c_ValDecl::mark(){
	c_Decl::mark();
	gc_mark_q(m_type);
	gc_mark_q(m_init);
}
c_ConstDecl::c_ConstDecl(){
	m_value=String();
}
c_ConstDecl* c_ConstDecl::m_new(String t_ident,int t_attrs,c_Type* t_type,c_Expr* t_init){
	c_ValDecl::m_new();
	this->m_ident=t_ident;
	this->m_munged=t_ident;
	this->m_attrs=t_attrs;
	gc_assign(this->m_type,t_type);
	gc_assign(this->m_init,t_init);
	return this;
}
c_ConstDecl* c_ConstDecl::m_new2(){
	c_ValDecl::m_new();
	return this;
}
c_Decl* c_ConstDecl::p_OnCopy(){
	return ((new c_ConstDecl)->m_new(m_ident,m_attrs,m_type,p_CopyInit()));
}
int c_ConstDecl::p_OnSemant(){
	c_ValDecl::p_OnSemant();
	if(!((p_IsExtern())!=0)){
		m_value=m_init->p_Eval();
	}
	return 0;
}
void c_ConstDecl::mark(){
	c_ValDecl::mark();
}
c_Map::c_Map(){
	m_root=0;
}
c_Map* c_Map::m_new(){
	return this;
}
c_Node* c_Map::p_FindNode(String t_key){
	c_Node* t_node=m_root;
	while((t_node)!=0){
		int t_cmp=p_Compare(t_key,t_node->m_key);
		if(t_cmp>0){
			t_node=t_node->m_right;
		}else{
			if(t_cmp<0){
				t_node=t_node->m_left;
			}else{
				return t_node;
			}
		}
	}
	return t_node;
}
c_ConstDecl* c_Map::p_Get(String t_key){
	c_Node* t_node=p_FindNode(t_key);
	if((t_node)!=0){
		return t_node->m_value;
	}
	return 0;
}
int c_Map::p_RotateLeft(c_Node* t_node){
	c_Node* t_child=t_node->m_right;
	gc_assign(t_node->m_right,t_child->m_left);
	if((t_child->m_left)!=0){
		gc_assign(t_child->m_left->m_parent,t_node);
	}
	gc_assign(t_child->m_parent,t_node->m_parent);
	if((t_node->m_parent)!=0){
		if(t_node==t_node->m_parent->m_left){
			gc_assign(t_node->m_parent->m_left,t_child);
		}else{
			gc_assign(t_node->m_parent->m_right,t_child);
		}
	}else{
		gc_assign(m_root,t_child);
	}
	gc_assign(t_child->m_left,t_node);
	gc_assign(t_node->m_parent,t_child);
	return 0;
}
int c_Map::p_RotateRight(c_Node* t_node){
	c_Node* t_child=t_node->m_left;
	gc_assign(t_node->m_left,t_child->m_right);
	if((t_child->m_right)!=0){
		gc_assign(t_child->m_right->m_parent,t_node);
	}
	gc_assign(t_child->m_parent,t_node->m_parent);
	if((t_node->m_parent)!=0){
		if(t_node==t_node->m_parent->m_right){
			gc_assign(t_node->m_parent->m_right,t_child);
		}else{
			gc_assign(t_node->m_parent->m_left,t_child);
		}
	}else{
		gc_assign(m_root,t_child);
	}
	gc_assign(t_child->m_right,t_node);
	gc_assign(t_node->m_parent,t_child);
	return 0;
}
int c_Map::p_InsertFixup(c_Node* t_node){
	while(((t_node->m_parent)!=0) && t_node->m_parent->m_color==-1 && ((t_node->m_parent->m_parent)!=0)){
		if(t_node->m_parent==t_node->m_parent->m_parent->m_left){
			c_Node* t_uncle=t_node->m_parent->m_parent->m_right;
			if(((t_uncle)!=0) && t_uncle->m_color==-1){
				t_node->m_parent->m_color=1;
				t_uncle->m_color=1;
				t_uncle->m_parent->m_color=-1;
				t_node=t_uncle->m_parent;
			}else{
				if(t_node==t_node->m_parent->m_right){
					t_node=t_node->m_parent;
					p_RotateLeft(t_node);
				}
				t_node->m_parent->m_color=1;
				t_node->m_parent->m_parent->m_color=-1;
				p_RotateRight(t_node->m_parent->m_parent);
			}
		}else{
			c_Node* t_uncle2=t_node->m_parent->m_parent->m_left;
			if(((t_uncle2)!=0) && t_uncle2->m_color==-1){
				t_node->m_parent->m_color=1;
				t_uncle2->m_color=1;
				t_uncle2->m_parent->m_color=-1;
				t_node=t_uncle2->m_parent;
			}else{
				if(t_node==t_node->m_parent->m_left){
					t_node=t_node->m_parent;
					p_RotateRight(t_node);
				}
				t_node->m_parent->m_color=1;
				t_node->m_parent->m_parent->m_color=-1;
				p_RotateLeft(t_node->m_parent->m_parent);
			}
		}
	}
	m_root->m_color=1;
	return 0;
}
bool c_Map::p_Set(String t_key,c_ConstDecl* t_value){
	c_Node* t_node=m_root;
	c_Node* t_parent=0;
	int t_cmp=0;
	while((t_node)!=0){
		t_parent=t_node;
		t_cmp=p_Compare(t_key,t_node->m_key);
		if(t_cmp>0){
			t_node=t_node->m_right;
		}else{
			if(t_cmp<0){
				t_node=t_node->m_left;
			}else{
				gc_assign(t_node->m_value,t_value);
				return false;
			}
		}
	}
	t_node=(new c_Node)->m_new(t_key,t_value,-1,t_parent);
	if((t_parent)!=0){
		if(t_cmp>0){
			gc_assign(t_parent->m_right,t_node);
		}else{
			gc_assign(t_parent->m_left,t_node);
		}
		p_InsertFixup(t_node);
	}else{
		gc_assign(m_root,t_node);
	}
	return true;
}
bool c_Map::p_Contains(String t_key){
	return p_FindNode(t_key)!=0;
}
void c_Map::mark(){
	Object::mark();
	gc_mark_q(m_root);
}
c_StringMap::c_StringMap(){
}
c_StringMap* c_StringMap::m_new(){
	c_Map::m_new();
	return this;
}
int c_StringMap::p_Compare(String t_lhs,String t_rhs){
	return t_lhs.Compare(t_rhs);
}
void c_StringMap::mark(){
	c_Map::mark();
}
c_Node::c_Node(){
	m_key=String();
	m_right=0;
	m_left=0;
	m_value=0;
	m_color=0;
	m_parent=0;
}
c_Node* c_Node::m_new(String t_key,c_ConstDecl* t_value,int t_color,c_Node* t_parent){
	this->m_key=t_key;
	gc_assign(this->m_value,t_value);
	this->m_color=t_color;
	gc_assign(this->m_parent,t_parent);
	return this;
}
c_Node* c_Node::m_new2(){
	return this;
}
void c_Node::mark(){
	Object::mark();
	gc_mark_q(m_right);
	gc_mark_q(m_left);
	gc_mark_q(m_value);
	gc_mark_q(m_parent);
}
c_Expr::c_Expr(){
	m_exprType=0;
}
c_Expr* c_Expr::m_new(){
	return this;
}
c_Expr* c_Expr::p_Copy(){
	bb_config_InternalErr(String(L"Internal error",14));
	return 0;
}
c_Expr* c_Expr::p_Semant(){
	bb_config_InternalErr(String(L"Internal error",14));
	return 0;
}
Array<c_Expr* > c_Expr::p_SemantArgs(Array<c_Expr* > t_args){
	t_args=t_args.Slice(0);
	for(int t_i=0;t_i<t_args.Length();t_i=t_i+1){
		if((t_args[t_i])!=0){
			gc_assign(t_args[t_i],t_args[t_i]->p_Semant());
		}
	}
	return t_args;
}
c_Expr* c_Expr::p_Cast(c_Type* t_ty,int t_castFlags){
	if((m_exprType->p_EqualsType(t_ty))!=0){
		return this;
	}
	return ((new c_CastExpr)->m_new(t_ty,this,t_castFlags))->p_Semant();
}
Array<c_Expr* > c_Expr::p_CastArgs(Array<c_Expr* > t_args,c_FuncDecl* t_funcDecl){
	if(t_args.Length()>t_funcDecl->m_argDecls.Length()){
		bb_config_InternalErr(String(L"Internal error",14));
	}
	t_args=t_args.Resize(t_funcDecl->m_argDecls.Length());
	for(int t_i=0;t_i<t_args.Length();t_i=t_i+1){
		if((t_args[t_i])!=0){
			gc_assign(t_args[t_i],t_args[t_i]->p_Cast(t_funcDecl->m_argDecls[t_i]->m_type,0));
		}else{
			if((t_funcDecl->m_argDecls[t_i]->m_init)!=0){
				gc_assign(t_args[t_i],t_funcDecl->m_argDecls[t_i]->m_init);
			}else{
				bb_config_Err(String(L"Missing function argument '",27)+t_funcDecl->m_argDecls[t_i]->m_ident+String(L"'.",2));
			}
		}
	}
	return t_args;
}
String c_Expr::p_ToString(){
	return String(L"<Expr>",6);
}
String c_Expr::p_Eval(){
	bb_config_Err(p_ToString()+String(L" cannot be statically evaluated.",32));
	return String();
}
c_Expr* c_Expr::p_EvalConst(){
	return ((new c_ConstExpr)->m_new(m_exprType,p_Eval()))->p_Semant();
}
c_Expr* c_Expr::p_Semant2(c_Type* t_ty,int t_castFlags){
	c_Expr* t_expr=p_Semant();
	if((t_expr->m_exprType->p_EqualsType(t_ty))!=0){
		return t_expr;
	}
	return ((new c_CastExpr)->m_new(t_ty,t_expr,t_castFlags))->p_Semant();
}
c_Expr* c_Expr::p_CopyExpr(c_Expr* t_expr){
	if(!((t_expr)!=0)){
		return 0;
	}
	return t_expr->p_Copy();
}
Array<c_Expr* > c_Expr::p_CopyArgs(Array<c_Expr* > t_exprs){
	t_exprs=t_exprs.Slice(0);
	for(int t_i=0;t_i<t_exprs.Length();t_i=t_i+1){
		gc_assign(t_exprs[t_i],p_CopyExpr(t_exprs[t_i]));
	}
	return t_exprs;
}
c_Type* c_Expr::p_BalanceTypes(c_Type* t_lhs,c_Type* t_rhs){
	if(((dynamic_cast<c_StringType*>(t_lhs))!=0) || ((dynamic_cast<c_StringType*>(t_rhs))!=0)){
		return (c_Type::m_stringType);
	}
	if(((dynamic_cast<c_FloatType*>(t_lhs))!=0) || ((dynamic_cast<c_FloatType*>(t_rhs))!=0)){
		return (c_Type::m_floatType);
	}
	if(((dynamic_cast<c_IntType*>(t_lhs))!=0) || ((dynamic_cast<c_IntType*>(t_rhs))!=0)){
		return (c_Type::m_intType);
	}
	if((t_lhs->p_ExtendsType(t_rhs))!=0){
		return t_rhs;
	}
	if((t_rhs->p_ExtendsType(t_lhs))!=0){
		return t_lhs;
	}
	bb_config_Err(String(L"Can't balance types ",20)+t_lhs->p_ToString()+String(L" and ",5)+t_rhs->p_ToString()+String(L".",1));
	return 0;
}
c_Expr* c_Expr::p_SemantSet(String t_op,c_Expr* t_rhs){
	bb_config_Err(p_ToString()+String(L" cannot be assigned to.",23));
	return 0;
}
c_ScopeDecl* c_Expr::p_SemantScope(){
	return 0;
}
c_Expr* c_Expr::p_SemantFunc(Array<c_Expr* > t_args){
	bb_config_Err(p_ToString()+String(L" cannot be invoked.",19));
	return 0;
}
bool c_Expr::p_SideEffects(){
	return true;
}
String c_Expr::p_Trans(){
	bb_config_Err(String(L"TODO!",5));
	return String();
}
String c_Expr::p_TransStmt(){
	return p_Trans();
}
String c_Expr::p_TransVar(){
	bb_config_InternalErr(String(L"Internal error",14));
	return String();
}
void c_Expr::mark(){
	Object::mark();
	gc_mark_q(m_exprType);
}
c_BoolType::c_BoolType(){
}
c_BoolType* c_BoolType::m_new(){
	c_Type::m_new();
	return this;
}
int c_BoolType::p_EqualsType(c_Type* t_ty){
	return ((dynamic_cast<c_BoolType*>(t_ty)!=0)?1:0);
}
int c_BoolType::p_ExtendsType(c_Type* t_ty){
	if((dynamic_cast<c_ObjectType*>(t_ty))!=0){
		c_Expr* t_expr=((new c_ConstExpr)->m_new((this),String()))->p_Semant();
		c_Expr* t_[]={t_expr};
		c_FuncDecl* t_ctor=t_ty->p_GetClass()->p_FindFuncDecl(String(L"new",3),Array<c_Expr* >(t_,1),1);
		return ((((t_ctor)!=0) && t_ctor->p_IsCtor())?1:0);
	}
	return ((dynamic_cast<c_IntType*>(t_ty)!=0 || dynamic_cast<c_BoolType*>(t_ty)!=0)?1:0);
}
c_ClassDecl* c_BoolType::p_GetClass(){
	return dynamic_cast<c_ClassDecl*>(bb_decl__env->p_FindDecl(String(L"bool",4)));
}
String c_BoolType::p_ToString(){
	return String(L"Bool",4);
}
void c_BoolType::mark(){
	c_Type::mark();
}
c_Map2::c_Map2(){
	m_root=0;
}
c_Map2* c_Map2::m_new(){
	return this;
}
int c_Map2::p_RotateLeft2(c_Node2* t_node){
	c_Node2* t_child=t_node->m_right;
	gc_assign(t_node->m_right,t_child->m_left);
	if((t_child->m_left)!=0){
		gc_assign(t_child->m_left->m_parent,t_node);
	}
	gc_assign(t_child->m_parent,t_node->m_parent);
	if((t_node->m_parent)!=0){
		if(t_node==t_node->m_parent->m_left){
			gc_assign(t_node->m_parent->m_left,t_child);
		}else{
			gc_assign(t_node->m_parent->m_right,t_child);
		}
	}else{
		gc_assign(m_root,t_child);
	}
	gc_assign(t_child->m_left,t_node);
	gc_assign(t_node->m_parent,t_child);
	return 0;
}
int c_Map2::p_RotateRight2(c_Node2* t_node){
	c_Node2* t_child=t_node->m_left;
	gc_assign(t_node->m_left,t_child->m_right);
	if((t_child->m_right)!=0){
		gc_assign(t_child->m_right->m_parent,t_node);
	}
	gc_assign(t_child->m_parent,t_node->m_parent);
	if((t_node->m_parent)!=0){
		if(t_node==t_node->m_parent->m_right){
			gc_assign(t_node->m_parent->m_right,t_child);
		}else{
			gc_assign(t_node->m_parent->m_left,t_child);
		}
	}else{
		gc_assign(m_root,t_child);
	}
	gc_assign(t_child->m_right,t_node);
	gc_assign(t_node->m_parent,t_child);
	return 0;
}
int c_Map2::p_InsertFixup2(c_Node2* t_node){
	while(((t_node->m_parent)!=0) && t_node->m_parent->m_color==-1 && ((t_node->m_parent->m_parent)!=0)){
		if(t_node->m_parent==t_node->m_parent->m_parent->m_left){
			c_Node2* t_uncle=t_node->m_parent->m_parent->m_right;
			if(((t_uncle)!=0) && t_uncle->m_color==-1){
				t_node->m_parent->m_color=1;
				t_uncle->m_color=1;
				t_uncle->m_parent->m_color=-1;
				t_node=t_uncle->m_parent;
			}else{
				if(t_node==t_node->m_parent->m_right){
					t_node=t_node->m_parent;
					p_RotateLeft2(t_node);
				}
				t_node->m_parent->m_color=1;
				t_node->m_parent->m_parent->m_color=-1;
				p_RotateRight2(t_node->m_parent->m_parent);
			}
		}else{
			c_Node2* t_uncle2=t_node->m_parent->m_parent->m_left;
			if(((t_uncle2)!=0) && t_uncle2->m_color==-1){
				t_node->m_parent->m_color=1;
				t_uncle2->m_color=1;
				t_uncle2->m_parent->m_color=-1;
				t_node=t_uncle2->m_parent;
			}else{
				if(t_node==t_node->m_parent->m_left){
					t_node=t_node->m_parent;
					p_RotateRight2(t_node);
				}
				t_node->m_parent->m_color=1;
				t_node->m_parent->m_parent->m_color=-1;
				p_RotateLeft2(t_node->m_parent->m_parent);
			}
		}
	}
	m_root->m_color=1;
	return 0;
}
bool c_Map2::p_Set2(String t_key,String t_value){
	c_Node2* t_node=m_root;
	c_Node2* t_parent=0;
	int t_cmp=0;
	while((t_node)!=0){
		t_parent=t_node;
		t_cmp=p_Compare(t_key,t_node->m_key);
		if(t_cmp>0){
			t_node=t_node->m_right;
		}else{
			if(t_cmp<0){
				t_node=t_node->m_left;
			}else{
				t_node->m_value=t_value;
				return false;
			}
		}
	}
	t_node=(new c_Node2)->m_new(t_key,t_value,-1,t_parent);
	if((t_parent)!=0){
		if(t_cmp>0){
			gc_assign(t_parent->m_right,t_node);
		}else{
			gc_assign(t_parent->m_left,t_node);
		}
		p_InsertFixup2(t_node);
	}else{
		gc_assign(m_root,t_node);
	}
	return true;
}
c_Node2* c_Map2::p_FindNode(String t_key){
	c_Node2* t_node=m_root;
	while((t_node)!=0){
		int t_cmp=p_Compare(t_key,t_node->m_key);
		if(t_cmp>0){
			t_node=t_node->m_right;
		}else{
			if(t_cmp<0){
				t_node=t_node->m_left;
			}else{
				return t_node;
			}
		}
	}
	return t_node;
}
String c_Map2::p_Get(String t_key){
	c_Node2* t_node=p_FindNode(t_key);
	if((t_node)!=0){
		return t_node->m_value;
	}
	return String();
}
bool c_Map2::p_Contains(String t_key){
	return p_FindNode(t_key)!=0;
}
c_Node2* c_Map2::p_FirstNode(){
	if(!((m_root)!=0)){
		return 0;
	}
	c_Node2* t_node=m_root;
	while((t_node->m_left)!=0){
		t_node=t_node->m_left;
	}
	return t_node;
}
c_NodeEnumerator3* c_Map2::p_ObjectEnumerator(){
	return (new c_NodeEnumerator3)->m_new(p_FirstNode());
}
void c_Map2::mark(){
	Object::mark();
	gc_mark_q(m_root);
}
c_StringMap2::c_StringMap2(){
}
c_StringMap2* c_StringMap2::m_new(){
	c_Map2::m_new();
	return this;
}
int c_StringMap2::p_Compare(String t_lhs,String t_rhs){
	return t_lhs.Compare(t_rhs);
}
void c_StringMap2::mark(){
	c_Map2::mark();
}
c_Node2::c_Node2(){
	m_key=String();
	m_right=0;
	m_left=0;
	m_value=String();
	m_color=0;
	m_parent=0;
}
c_Node2* c_Node2::m_new(String t_key,String t_value,int t_color,c_Node2* t_parent){
	this->m_key=t_key;
	this->m_value=t_value;
	this->m_color=t_color;
	gc_assign(this->m_parent,t_parent);
	return this;
}
c_Node2* c_Node2::m_new2(){
	return this;
}
c_Node2* c_Node2::p_NextNode(){
	c_Node2* t_node=0;
	if((m_right)!=0){
		t_node=m_right;
		while((t_node->m_left)!=0){
			t_node=t_node->m_left;
		}
		return t_node;
	}
	t_node=this;
	c_Node2* t_parent=this->m_parent;
	while(((t_parent)!=0) && t_node==t_parent->m_right){
		t_node=t_parent;
		t_parent=t_parent->m_parent;
	}
	return t_parent;
}
String c_Node2::p_Key(){
	return m_key;
}
String c_Node2::p_Value(){
	return m_value;
}
void c_Node2::mark(){
	Object::mark();
	gc_mark_q(m_right);
	gc_mark_q(m_left);
	gc_mark_q(m_parent);
}
int bb_config_SetConfigVar(String t_key,String t_val,c_Type* t_type){
	c_ConstDecl* t_decl=bb_config__cfgScope->m_cdecls->p_Get(t_key);
	if((t_decl)!=0){
		gc_assign(t_decl->m_type,t_type);
	}else{
		t_decl=(new c_ConstDecl)->m_new(t_key,1048576,t_type,0);
		bb_config__cfgScope->m_cdecls->p_Set(t_key,t_decl);
	}
	t_decl->m_value=t_val;
	if((dynamic_cast<c_BoolType*>(t_type))!=0){
		if((t_val).Length()!=0){
			t_val=String(L"1",1);
		}else{
			t_val=String(L"0",1);
		}
	}
	bb_config__cfgScope->m_vars->p_Set2(t_key,t_val);
	return 0;
}
int bb_config_SetConfigVar2(String t_key,String t_val){
	bb_config_SetConfigVar(t_key,t_val,(c_Type::m_stringType));
	return 0;
}
c_Stack::c_Stack(){
	m_data=Array<String >();
	m_length=0;
}
c_Stack* c_Stack::m_new(){
	return this;
}
c_Stack* c_Stack::m_new2(Array<String > t_data){
	gc_assign(this->m_data,t_data.Slice(0));
	this->m_length=t_data.Length();
	return this;
}
void c_Stack::p_Push(String t_value){
	if(m_length==m_data.Length()){
		gc_assign(m_data,m_data.Resize(m_length*2+10));
	}
	m_data[m_length]=t_value;
	m_length+=1;
}
void c_Stack::p_Push2(Array<String > t_values,int t_offset,int t_count){
	for(int t_i=0;t_i<t_count;t_i=t_i+1){
		p_Push(t_values[t_offset+t_i]);
	}
}
void c_Stack::p_Push3(Array<String > t_values,int t_offset){
	p_Push2(t_values,t_offset,t_values.Length()-t_offset);
}
bool c_Stack::p_IsEmpty(){
	return m_length==0;
}
Array<String > c_Stack::p_ToArray(){
	Array<String > t_t=Array<String >(m_length);
	for(int t_i=0;t_i<m_length;t_i=t_i+1){
		t_t[t_i]=m_data[t_i];
	}
	return t_t;
}
String c_Stack::m_NIL;
void c_Stack::p_Length(int t_newlength){
	if(t_newlength<m_length){
		for(int t_i=t_newlength;t_i<m_length;t_i=t_i+1){
			m_data[t_i]=m_NIL;
		}
	}else{
		if(t_newlength>m_data.Length()){
			gc_assign(m_data,m_data.Resize(bb_math_Max(m_length*2+10,t_newlength)));
		}
	}
	m_length=t_newlength;
}
int c_Stack::p_Length2(){
	return m_length;
}
String c_Stack::p_Get2(int t_index){
	return m_data[t_index];
}
String c_Stack::p_Pop(){
	m_length-=1;
	String t_v=m_data[m_length];
	m_data[m_length]=m_NIL;
	return t_v;
}
void c_Stack::p_Clear(){
	for(int t_i=0;t_i<m_length;t_i=t_i+1){
		m_data[t_i]=m_NIL;
	}
	m_length=0;
}
void c_Stack::mark(){
	Object::mark();
	gc_mark_q(m_data);
}
c_StringStack::c_StringStack(){
}
c_StringStack* c_StringStack::m_new(Array<String > t_data){
	c_Stack::m_new2(t_data);
	return this;
}
c_StringStack* c_StringStack::m_new2(){
	c_Stack::m_new();
	return this;
}
String c_StringStack::p_Join(String t_separator){
	return t_separator.Join(p_ToArray());
}
void c_StringStack::mark(){
	c_Stack::mark();
}
String bb_config_GetConfigVar(String t_key){
	return bb_config__cfgScope->m_vars->p_Get(t_key);
}
String bb_transcc_ReplaceEnv(String t_str){
	c_StringStack* t_bits=(new c_StringStack)->m_new2();
	do{
		int t_i=t_str.Find(String(L"${",2),0);
		if(t_i==-1){
			break;
		}
		int t_e=t_str.Find(String(L"}",1),t_i+2);
		if(t_e==-1){
			break;
		}
		if(t_i>=2 && t_str.Slice(t_i-2,t_i)==String(L"//",2)){
			t_bits->p_Push(t_str.Slice(0,t_e+1));
			t_str=t_str.Slice(t_e+1);
			continue;
		}
		String t_t=t_str.Slice(t_i+2,t_e);
		String t_v=bb_config_GetConfigVar(t_t);
		if(!((t_v).Length()!=0)){
			t_v=GetEnv(t_t);
		}
		t_bits->p_Push(t_str.Slice(0,t_i));
		t_bits->p_Push(t_v);
		t_str=t_str.Slice(t_e+1);
	}while(!(false));
	if(t_bits->p_IsEmpty()){
		return t_str;
	}
	t_bits->p_Push(t_str);
	return t_bits->p_Join(String());
}
c_Builder::c_Builder(){
	m_tcc=0;
	m_casedConfig=String();
	m_app=0;
	m_transCode=String();
	m_TEXT_FILES=String();
	m_IMAGE_FILES=String();
	m_SOUND_FILES=String();
	m_MUSIC_FILES=String();
	m_BINARY_FILES=String();
	m_DATA_FILES=String();
	m_syncData=false;
	m_dataFiles=(new c_StringMap2)->m_new();
}
c_Builder* c_Builder::m_new(c_TransCC* t_tcc){
	gc_assign(this->m_tcc,t_tcc);
	return this;
}
c_Builder* c_Builder::m_new2(){
	return this;
}
void c_Builder::p_Make(){
	String t_1=m_tcc->m_opt_config;
	if(t_1==String() || t_1==String(L"debug",5)){
		m_tcc->m_opt_config=String(L"debug",5);
		m_casedConfig=String(L"Debug",5);
	}else{
		if(t_1==String(L"release",7)){
			m_casedConfig=String(L"Release",7);
		}else{
			bb_transcc_Die(String(L"Invalid config",14));
		}
	}
	if(FileType(m_tcc->m_opt_srcpath)!=1){
		bb_transcc_Die(String(L"Invalid source file",19));
	}
	m_tcc->m_opt_srcpath=RealPath(m_tcc->m_opt_srcpath);
	if(!((m_tcc->m_opt_modpath).Length()!=0)){
		m_tcc->m_opt_modpath=m_tcc->m_cerberusdir+String(L"/modules",8);
	}
	m_tcc->m_opt_modpath=String(L".;",2)+bb_os_ExtractDir(m_tcc->m_opt_srcpath)+String(L";",1)+m_tcc->m_opt_modpath+String(L";",1)+m_tcc->m_cerberusdir+String(L"/targets/",9)+m_tcc->m_target->m_dir+String(L"/modules",8);
	if(!m_tcc->m_opt_check){
		m_tcc->m_opt_check=true;
		m_tcc->m_opt_update=true;
		m_tcc->m_opt_build=true;
	}
	bb_config_ENV_HOST=HostOS();
	bb_config_ENV_CONFIG=m_tcc->m_opt_config;
	bb_config_ENV_SAFEMODE=((m_tcc->m_opt_safe)?1:0);
	bb_config_ENV_MODPATH=m_tcc->m_opt_modpath;
	bb_config_ENV_TARGET=m_tcc->m_target->m_system;
	this->p_Begin();
	if(!m_tcc->m_opt_check){
		return;
	}
	bbPrint(String(L"Parsing...",10));
	bb_config_SetConfigVar2(String(L"HOST",4),bb_config_ENV_HOST);
	bb_config_SetConfigVar2(String(L"LANG",4),bb_config_ENV_LANG);
	bb_config_SetConfigVar2(String(L"TARGET",6),bb_config_ENV_TARGET);
	bb_config_SetConfigVar2(String(L"CONFIG",6),bb_config_ENV_CONFIG);
	bb_config_SetConfigVar2(String(L"SAFEMODE",8),String(bb_config_ENV_SAFEMODE));
	gc_assign(m_app,bb_parser_ParseApp(m_tcc->m_opt_srcpath));
	bbPrint(String(L"Semanting...",12));
	if((bb_config_GetConfigVar(String(L"REFLECTION_FILTER",17))).Length()!=0){
		c_Reflector* t_r=(new c_Reflector)->m_new();
		t_r->p_Semant3(m_app);
	}else{
		m_app->p_Semant();
	}
	bbPrint(String(L"Translating...",14));
	c_StringStack* t_transbuf=(new c_StringStack)->m_new2();
	c_Enumerator* t_=m_app->m_fileImports->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		String t_file=t_->p_NextObject();
		if(bb_os_ExtractExt(t_file).ToLower()==bb_config_ENV_LANG){
			t_transbuf->p_Push(LoadString(t_file));
			t_transbuf->p_Push(String(L"\n",1));
		}
	}
	t_transbuf->p_Push(bb_translator__trans->p_TransApp(m_app));
	if(!m_tcc->m_opt_update){
		return;
	}
	bbPrint(String(L"Building...",11));
	m_transCode=t_transbuf->p_Join(String());
	String t_buildPath=String();
	if((m_tcc->m_opt_builddir).Length()!=0){
		t_buildPath=bb_os_ExtractDir(m_tcc->m_opt_srcpath)+String(L"/",1)+m_tcc->m_opt_builddir;
	}else{
		t_buildPath=bb_os_StripExt(m_tcc->m_opt_srcpath)+String(L".build",6)+m_tcc->p_GetReleaseVersion();
	}
	String t_targetPath=t_buildPath+String(L"/",1)+m_tcc->m_target->m_dir;
	if(m_tcc->m_opt_clean){
		bb_os_DeleteDir(t_targetPath,true);
		if(FileType(t_targetPath)!=0){
			bb_transcc_Die(String(L"Failed to clean target dir",26));
		}
	}
	if(FileType(t_targetPath)==0){
		if(FileType(t_buildPath)==0){
			CreateDir(t_buildPath);
		}
		if(FileType(t_buildPath)!=2){
			bb_transcc_Die(String(L"Failed to create build dir: ",28)+t_buildPath);
		}
		if(!((bb_os_CopyDir(m_tcc->m_cerberusdir+String(L"/targets/",9)+m_tcc->m_target->m_dir+String(L"/template",9),t_targetPath,true,false))!=0)){
			bb_transcc_Die(String(L"Failed to copy target dir",25));
		}
	}
	if(FileType(t_targetPath)!=2){
		bb_transcc_Die(String(L"Failed to create target dir: ",29)+t_targetPath);
	}
	String t_cfgPath=t_targetPath+String(L"/config.cxs",11);
	if(FileType(t_cfgPath)==1){
		bb_preprocessor_PreProcess(t_cfgPath,0);
	}
	m_TEXT_FILES=bb_config_GetConfigVar(String(L"TEXT_FILES",10));
	m_IMAGE_FILES=bb_config_GetConfigVar(String(L"IMAGE_FILES",11));
	m_SOUND_FILES=bb_config_GetConfigVar(String(L"SOUND_FILES",11));
	m_MUSIC_FILES=bb_config_GetConfigVar(String(L"MUSIC_FILES",11));
	m_BINARY_FILES=bb_config_GetConfigVar(String(L"BINARY_FILES",12));
	m_DATA_FILES=m_TEXT_FILES;
	if((m_IMAGE_FILES).Length()!=0){
		m_DATA_FILES=m_DATA_FILES+(String(L"|",1)+m_IMAGE_FILES);
	}
	if((m_SOUND_FILES).Length()!=0){
		m_DATA_FILES=m_DATA_FILES+(String(L"|",1)+m_SOUND_FILES);
	}
	if((m_MUSIC_FILES).Length()!=0){
		m_DATA_FILES=m_DATA_FILES+(String(L"|",1)+m_MUSIC_FILES);
	}
	if((m_BINARY_FILES).Length()!=0){
		m_DATA_FILES=m_DATA_FILES+(String(L"|",1)+m_BINARY_FILES);
	}
	m_DATA_FILES=m_DATA_FILES.Replace(String(L";",1),String(L"|",1));
	m_syncData=bb_config_GetConfigVar(String(L"FAST_SYNC_PROJECT_DATA",22))==String(L"1",1);
	String t_cd=CurrentDir();
	ChangeDir(t_targetPath);
	this->p_MakeTarget();
	ChangeDir(t_cd);
}
void c_Builder::p_CCopyFile(String t_src,String t_dst){
	if(FileTime(t_src)>FileTime(t_dst) || FileSize(t_src)!=FileSize(t_dst)){
		DeleteFile(t_dst);
		CopyFile(t_src,t_dst);
	}
}
void c_Builder::p_CreateDataDir(String t_dir){
	t_dir=RealPath(t_dir);
	if(!m_syncData){
		bb_os_DeleteDir(t_dir,true);
	}
	CreateDir(t_dir);
	if(FileType(t_dir)!=2){
		bb_transcc_Die(String(L"Failed to create target project data dir: ",42)+t_dir);
	}
	String t_dataPath=bb_os_StripExt(m_tcc->m_opt_srcpath)+String(L".data",5);
	if(FileType(t_dataPath)!=2){
		t_dataPath=String();
	}
	c_StringSet* t_udata=(new c_StringSet)->m_new();
	if((t_dataPath).Length()!=0){
		c_StringStack* t_srcs=(new c_StringStack)->m_new2();
		t_srcs->p_Push(t_dataPath);
		while(!t_srcs->p_IsEmpty()){
			String t_src=t_srcs->p_Pop();
			Array<String > t_=LoadDir(t_src);
			int t_2=0;
			while(t_2<t_.Length()){
				String t_f=t_[t_2];
				t_2=t_2+1;
				if(t_f.StartsWith(String(L".",1))){
					continue;
				}
				String t_p=t_src+String(L"/",1)+t_f;
				String t_r=t_p.Slice(t_dataPath.Length()+1);
				String t_t=t_dir+String(L"/",1)+t_r;
				int t_22=FileType(t_p);
				if(t_22==1){
					if(bb_transcc_MatchPath(t_r,m_DATA_FILES)){
						p_CCopyFile(t_p,t_t);
						t_udata->p_Insert(t_t);
						m_dataFiles->p_Set2(t_p,t_r);
					}
				}else{
					if(t_22==2){
						CreateDir(t_t);
						t_srcs->p_Push(t_p);
					}
				}
			}
		}
	}
	c_Enumerator* t_3=m_app->m_fileImports->p_ObjectEnumerator();
	while(t_3->p_HasNext()){
		String t_p2=t_3->p_NextObject();
		String t_r2=bb_os_StripDir(t_p2);
		String t_t2=t_dir+String(L"/",1)+t_r2;
		if(bb_transcc_MatchPath(t_r2,m_DATA_FILES)){
			p_CCopyFile(t_p2,t_t2);
			t_udata->p_Insert(t_t2);
			m_dataFiles->p_Set2(t_p2,t_r2);
		}
	}
	if((t_dataPath).Length()!=0){
		c_StringStack* t_dsts=(new c_StringStack)->m_new2();
		t_dsts->p_Push(t_dir);
		while(!t_dsts->p_IsEmpty()){
			String t_dst=t_dsts->p_Pop();
			Array<String > t_4=LoadDir(t_dst);
			int t_5=0;
			while(t_5<t_4.Length()){
				String t_f2=t_4[t_5];
				t_5=t_5+1;
				if(t_f2.StartsWith(String(L".",1))){
					continue;
				}
				String t_p3=t_dst+String(L"/",1)+t_f2;
				String t_r3=t_p3.Slice(t_dir.Length()+1);
				String t_t3=t_dataPath+String(L"/",1)+t_r3;
				int t_32=FileType(t_p3);
				if(t_32==1){
					if(!t_udata->p_Contains(t_p3)){
						DeleteFile(t_p3);
					}
				}else{
					if(t_32==2){
						if(FileType(t_t3)==2){
							t_dsts->p_Push(t_p3);
						}else{
							bb_os_DeleteDir(t_p3,true);
						}
					}
				}
			}
		}
	}
}
bool c_Builder::p_Execute(String t_cmd,bool t_failHard){
	return m_tcc->p_Execute(t_cmd,t_failHard);
}
void c_Builder::mark(){
	Object::mark();
	gc_mark_q(m_tcc);
	gc_mark_q(m_app);
	gc_mark_q(m_dataFiles);
}
c_Map3::c_Map3(){
	m_root=0;
}
c_Map3* c_Map3::m_new(){
	return this;
}
int c_Map3::p_RotateLeft3(c_Node3* t_node){
	c_Node3* t_child=t_node->m_right;
	gc_assign(t_node->m_right,t_child->m_left);
	if((t_child->m_left)!=0){
		gc_assign(t_child->m_left->m_parent,t_node);
	}
	gc_assign(t_child->m_parent,t_node->m_parent);
	if((t_node->m_parent)!=0){
		if(t_node==t_node->m_parent->m_left){
			gc_assign(t_node->m_parent->m_left,t_child);
		}else{
			gc_assign(t_node->m_parent->m_right,t_child);
		}
	}else{
		gc_assign(m_root,t_child);
	}
	gc_assign(t_child->m_left,t_node);
	gc_assign(t_node->m_parent,t_child);
	return 0;
}
int c_Map3::p_RotateRight3(c_Node3* t_node){
	c_Node3* t_child=t_node->m_left;
	gc_assign(t_node->m_left,t_child->m_right);
	if((t_child->m_right)!=0){
		gc_assign(t_child->m_right->m_parent,t_node);
	}
	gc_assign(t_child->m_parent,t_node->m_parent);
	if((t_node->m_parent)!=0){
		if(t_node==t_node->m_parent->m_right){
			gc_assign(t_node->m_parent->m_right,t_child);
		}else{
			gc_assign(t_node->m_parent->m_left,t_child);
		}
	}else{
		gc_assign(m_root,t_child);
	}
	gc_assign(t_child->m_right,t_node);
	gc_assign(t_node->m_parent,t_child);
	return 0;
}
int c_Map3::p_InsertFixup3(c_Node3* t_node){
	while(((t_node->m_parent)!=0) && t_node->m_parent->m_color==-1 && ((t_node->m_parent->m_parent)!=0)){
		if(t_node->m_parent==t_node->m_parent->m_parent->m_left){
			c_Node3* t_uncle=t_node->m_parent->m_parent->m_right;
			if(((t_uncle)!=0) && t_uncle->m_color==-1){
				t_node->m_parent->m_color=1;
				t_uncle->m_color=1;
				t_uncle->m_parent->m_color=-1;
				t_node=t_uncle->m_parent;
			}else{
				if(t_node==t_node->m_parent->m_right){
					t_node=t_node->m_parent;
					p_RotateLeft3(t_node);
				}
				t_node->m_parent->m_color=1;
				t_node->m_parent->m_parent->m_color=-1;
				p_RotateRight3(t_node->m_parent->m_parent);
			}
		}else{
			c_Node3* t_uncle2=t_node->m_parent->m_parent->m_left;
			if(((t_uncle2)!=0) && t_uncle2->m_color==-1){
				t_node->m_parent->m_color=1;
				t_uncle2->m_color=1;
				t_uncle2->m_parent->m_color=-1;
				t_node=t_uncle2->m_parent;
			}else{
				if(t_node==t_node->m_parent->m_left){
					t_node=t_node->m_parent;
					p_RotateRight3(t_node);
				}
				t_node->m_parent->m_color=1;
				t_node->m_parent->m_parent->m_color=-1;
				p_RotateLeft3(t_node->m_parent->m_parent);
			}
		}
	}
	m_root->m_color=1;
	return 0;
}
bool c_Map3::p_Set3(String t_key,c_Builder* t_value){
	c_Node3* t_node=m_root;
	c_Node3* t_parent=0;
	int t_cmp=0;
	while((t_node)!=0){
		t_parent=t_node;
		t_cmp=p_Compare(t_key,t_node->m_key);
		if(t_cmp>0){
			t_node=t_node->m_right;
		}else{
			if(t_cmp<0){
				t_node=t_node->m_left;
			}else{
				gc_assign(t_node->m_value,t_value);
				return false;
			}
		}
	}
	t_node=(new c_Node3)->m_new(t_key,t_value,-1,t_parent);
	if((t_parent)!=0){
		if(t_cmp>0){
			gc_assign(t_parent->m_right,t_node);
		}else{
			gc_assign(t_parent->m_left,t_node);
		}
		p_InsertFixup3(t_node);
	}else{
		gc_assign(m_root,t_node);
	}
	return true;
}
c_Node3* c_Map3::p_FirstNode(){
	if(!((m_root)!=0)){
		return 0;
	}
	c_Node3* t_node=m_root;
	while((t_node->m_left)!=0){
		t_node=t_node->m_left;
	}
	return t_node;
}
c_NodeEnumerator* c_Map3::p_ObjectEnumerator(){
	return (new c_NodeEnumerator)->m_new(p_FirstNode());
}
c_Node3* c_Map3::p_FindNode(String t_key){
	c_Node3* t_node=m_root;
	while((t_node)!=0){
		int t_cmp=p_Compare(t_key,t_node->m_key);
		if(t_cmp>0){
			t_node=t_node->m_right;
		}else{
			if(t_cmp<0){
				t_node=t_node->m_left;
			}else{
				return t_node;
			}
		}
	}
	return t_node;
}
c_Builder* c_Map3::p_Get(String t_key){
	c_Node3* t_node=p_FindNode(t_key);
	if((t_node)!=0){
		return t_node->m_value;
	}
	return 0;
}
void c_Map3::mark(){
	Object::mark();
	gc_mark_q(m_root);
}
c_StringMap3::c_StringMap3(){
}
c_StringMap3* c_StringMap3::m_new(){
	c_Map3::m_new();
	return this;
}
int c_StringMap3::p_Compare(String t_lhs,String t_rhs){
	return t_lhs.Compare(t_rhs);
}
void c_StringMap3::mark(){
	c_Map3::mark();
}
c_AndroidBuilder::c_AndroidBuilder(){
}
c_AndroidBuilder* c_AndroidBuilder::m_new(c_TransCC* t_tcc){
	c_Builder::m_new(t_tcc);
	return this;
}
c_AndroidBuilder* c_AndroidBuilder::m_new2(){
	c_Builder::m_new2();
	return this;
}
bool c_AndroidBuilder::p_IsValid(){
	return m_tcc->m_ANDROID_PATH!=String();
}
void c_AndroidBuilder::p_Begin(){
	bb_config_ENV_LANG=String(L"java",4);
	gc_assign(bb_translator__trans,((new c_JavaTranslator)->m_new()));
}
bool c_AndroidBuilder::p_CreateDirRecursive(String t_path){
	int t_i=0;
	do{
		t_i=t_path.Find(String(L"/",1),t_i);
		if(t_i==-1){
			CreateDir(t_path);
			return FileType(t_path)==2;
		}
		String t_t=t_path.Slice(0,t_i);
		CreateDir(t_t);
		if(FileType(t_t)!=2){
			return false;
		}
		t_i+=1;
	}while(!(false));
}
String c_AndroidBuilder::p_Config(){
	c_StringStack* t_config=(new c_StringStack)->m_new2();
	c_NodeEnumerator3* t_=bb_config_GetConfigVars()->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_Node2* t_kv=t_->p_NextObject();
		t_config->p_Push(String(L"static final String ",20)+t_kv->p_Key()+String(L"=",1)+bb_config_Enquote(t_kv->p_Value(),String(L"java",4))+String(L";",1));
	}
	return t_config->p_Join(String(L"\n",1));
}
void c_AndroidBuilder::p_MakeTarget(){
	p_CreateDirRecursive(String(L"app/src/main/assets",19));
	p_CreateDataDir(String(L"app/src/main/assets/cerberus",28));
	String t_app_label=bb_config_GetConfigVar(String(L"ANDROID_APP_LABEL",17));
	String t_app_package=bb_config_GetConfigVar(String(L"ANDROID_APP_PACKAGE",19));
	SetEnv(String(L"ANDROID_SDK_DIR",15),m_tcc->m_ANDROID_PATH.Replace(String(L"\\",1),String(L"\\\\",2)));
	SetEnv(String(L"ANDROID_NDK_DIR",15),m_tcc->m_ANDROID_NDK_PATH.Replace(String(L"\\",1),String(L"\\\\",2)));
	bb_config_SetConfigVar2(String(L"ANDROID_MANIFEST_MAIN",21),bb_config_GetConfigVar(String(L"ANDROID_MANIFEST_MAIN",21)).Replace(String(L";",1),String(L"\n",1))+String(L"\n",1));
	bb_config_SetConfigVar2(String(L"ANDROID_MANIFEST_APPLICATION",28),bb_config_GetConfigVar(String(L"ANDROID_MANIFEST_APPLICATION",28)).Replace(String(L";",1),String(L"\n",1))+String(L"\n",1));
	bb_config_SetConfigVar2(String(L"ANDROID_MANIFEST_ACTIVITY",25),bb_config_GetConfigVar(String(L"ANDROID_MANIFEST_ACTIVITY",25)).Replace(String(L";",1),String(L"\n",1))+String(L"\n",1));
	String t_jpath=String(L"app/src/main/java",17);
	bb_os_DeleteDir(t_jpath,true);
	CreateDir(t_jpath);
	Array<String > t_=t_app_package.Split(String(L".",1));
	int t_2=0;
	while(t_2<t_.Length()){
		String t_t=t_[t_2];
		t_2=t_2+1;
		t_jpath=t_jpath+(String(L"/",1)+t_t);
		CreateDir(t_jpath);
	}
	t_jpath=t_jpath+String(L"/CerberusGame.java",18);
	Array<String > t_3=bb_os_LoadDir(String(L"gradletemplate",14),true,false);
	int t_4=0;
	while(t_4<t_3.Length()){
		String t_file=t_3[t_4];
		t_4=t_4+1;
		int t_i=0;
		do{
			t_i=t_file.Find(String(L"/",1),t_i);
			if(t_i==-1){
				break;
			}
			CreateDir(t_file.Slice(0,t_i));
			if(FileType(t_file.Slice(0,t_i))!=2){
				t_file=String();
				break;
			}
			t_i+=1;
		}while(!(false));
		if(!((t_file).Length()!=0)){
			continue;
		}
		String t_1=bb_os_ExtractExt(t_file).ToLower();
		if(t_1==String(L"xml",3) || t_1==String(L"properties",10) || t_1==String(L"java",4) || t_1==String(L"gradle",6)){
			String t_str=LoadString(String(L"gradletemplate/",15)+t_file);
			t_str=bb_transcc_ReplaceEnv(t_str);
			SaveString(t_str,t_file);
		}else{
			CopyFile(String(L"gradletemplate/",15)+t_file,t_file);
		}
	}
	String t_main=LoadString(String(L"CerberusGame.java",17));
	t_main=bb_transcc_ReplaceBlock(t_main,String(L"TRANSCODE",9),m_transCode,String(L"\n//",3));
	t_main=bb_transcc_ReplaceBlock(t_main,String(L"CONFIG",6),p_Config(),String(L"\n//",3));
	c_StringStack* t_imps=(new c_StringStack)->m_new2();
	c_StringSet* t_done=(new c_StringSet)->m_new();
	c_StringStack* t_out=(new c_StringStack)->m_new2();
	Array<String > t_5=t_main.Split(String(L"\n",1));
	int t_6=0;
	while(t_6<t_5.Length()){
		String t_line=t_5[t_6];
		t_6=t_6+1;
		if(t_line.StartsWith(String(L"import ",7))){
			int t_i2=t_line.Find(String(L";",1),7);
			if(t_i2!=-1){
				String t_id=t_line.Slice(7,t_i2+1);
				if(!t_done->p_Contains(t_id)){
					t_done->p_Insert(t_id);
					t_imps->p_Push(String(L"import ",7)+t_id);
				}
			}
		}else{
			t_out->p_Push(t_line);
		}
	}
	t_main=t_out->p_Join(String(L"\n",1));
	t_main=bb_transcc_ReplaceBlock(t_main,String(L"IMPORTS",7),t_imps->p_Join(String(L"\n",1)),String(L"\n//",3));
	t_main=bb_transcc_ReplaceBlock(t_main,String(L"PACKAGE",7),String(L"package ",8)+t_app_package+String(L";",1),String(L"\n//",3));
	SaveString(t_main,t_jpath);
	Array<String > t_7=bb_config_GetConfigVar(String(L"LIBS",4)).Split(String(L";",1));
	int t_8=0;
	while(t_8<t_7.Length()){
		String t_lib=t_7[t_8];
		t_8=t_8+1;
		String t_22=bb_os_ExtractExt(t_lib);
		if(t_22==String(L"jar",3) || t_22==String(L"so",2)){
			String t_tdir=String();
			if(t_lib.Contains(String(L"/",1))){
				t_tdir=bb_os_ExtractDir(t_lib);
				if(t_tdir.Contains(String(L"/",1))){
					t_tdir=bb_os_StripDir(t_tdir);
				}
				String t_32=t_tdir;
				if(t_32==String(L"x86",3) || t_32==String(L"mips",4) || t_32==String(L"armeabi",7) || t_32==String(L"armeabi-v7a",11)){
					CreateDir(String(L"libs/",5)+t_tdir);
					t_tdir=t_tdir+String(L"/",1);
				}else{
					t_tdir=String();
				}
			}
			CopyFile(t_lib,String(L"libs/",5)+t_tdir+bb_os_StripDir(t_lib));
		}
	}
	Array<String > t_9=bb_config_GetConfigVar(String(L"SRCS",4)).Split(String(L";",1));
	int t_10=0;
	while(t_10<t_9.Length()){
		String t_src=t_9[t_10];
		t_10=t_10+1;
		String t_42=bb_os_ExtractExt(t_src);
		if(t_42==String(L"java",4) || t_42==String(L"aidl",4)){
			int t_i3=t_src.FindLast(String(L"/src/",5));
			if(t_i3!=-1){
				String t_dst=t_src.Slice(t_i3+1);
				if(p_CreateDirRecursive(bb_os_ExtractDir(t_dst))){
					CopyFile(t_src,t_dst);
				}
			}
		}
	}
	if(bb_config_GetConfigVar(String(L"ANDROID_NATIVE_GL_ENABLED",25))==String(L"1",1)){
		bb_os_CopyDir(String(L"nativegl/libs",13),String(L"libs",4),true,false);
		CreateDir(String(L"src/com",7));
		CreateDir(String(L"src/com/cerberus",16));
		CopyFile(String(L"nativegl/NativeGL.java",22),String(L"src/com/cerberus/NativeGL.java",30));
	}
	if(m_tcc->m_opt_build){
		String t_gradlecfg=String(L"assembleDebug installDebug",26);
		if(m_tcc->m_opt_config==String(L"release",7)){
			t_gradlecfg=String(L"assembleRelease",15);
		}
		String t_gradle=String();
		if(HostOS()==String(L"winnt",5)){
			t_gradle=String(L"gradlew",7);
		}else{
			t_gradle=String(L"./gradlew",9);
		}
		if(!p_Execute(t_gradle+String(L" clean ",7)+t_gradlecfg,false)){
			bb_transcc_Die(String(L"Android build failed.",21));
		}else{
			if(m_tcc->m_opt_config==String(L"release",7)){
				String t_adb=String(L"adb",3);
				if((m_tcc->m_ANDROID_PATH).Length()!=0){
					t_adb=String(L"\"",1)+m_tcc->m_ANDROID_PATH+String(L"/platform-tools/adb\"",20);
				}
				String t__file=CurrentDir();
				t__file=t__file+String(L"\\app\\build\\outputs\\apk\\release\\app-release.apk",46);
				bbPrint(String(L"installing ",11)+t__file+String(L" ...",4));
				p_Execute(t_adb+String(L" install -r ",12)+t__file,false);
			}
		}
		if(m_tcc->m_opt_run){
			String t_adb2=String(L"adb",3);
			if((m_tcc->m_ANDROID_PATH).Length()!=0){
				t_adb2=String(L"\"",1)+m_tcc->m_ANDROID_PATH+String(L"/platform-tools/adb\"",20);
			}
			p_Execute(t_adb2+String(L" logcat -c",10),false);
			p_Execute(t_adb2+String(L" shell am start -n ",19)+t_app_package+String(L"/",1)+t_app_package+String(L".CerberusGame",13),false);
			p_Execute(t_adb2+String(L" logcat [Cerberus]:I *:E",24),false);
		}
	}
}
void c_AndroidBuilder::mark(){
	c_Builder::mark();
}
c_Node3::c_Node3(){
	m_key=String();
	m_right=0;
	m_left=0;
	m_value=0;
	m_color=0;
	m_parent=0;
}
c_Node3* c_Node3::m_new(String t_key,c_Builder* t_value,int t_color,c_Node3* t_parent){
	this->m_key=t_key;
	gc_assign(this->m_value,t_value);
	this->m_color=t_color;
	gc_assign(this->m_parent,t_parent);
	return this;
}
c_Node3* c_Node3::m_new2(){
	return this;
}
c_Node3* c_Node3::p_NextNode(){
	c_Node3* t_node=0;
	if((m_right)!=0){
		t_node=m_right;
		while((t_node->m_left)!=0){
			t_node=t_node->m_left;
		}
		return t_node;
	}
	t_node=this;
	c_Node3* t_parent=this->m_parent;
	while(((t_parent)!=0) && t_node==t_parent->m_right){
		t_node=t_parent;
		t_parent=t_parent->m_parent;
	}
	return t_parent;
}
c_Builder* c_Node3::p_Value(){
	return m_value;
}
String c_Node3::p_Key(){
	return m_key;
}
void c_Node3::mark(){
	Object::mark();
	gc_mark_q(m_right);
	gc_mark_q(m_left);
	gc_mark_q(m_value);
	gc_mark_q(m_parent);
}
c_AndroidNdkBuilder::c_AndroidNdkBuilder(){
}
c_AndroidNdkBuilder* c_AndroidNdkBuilder::m_new(c_TransCC* t_tcc){
	c_Builder::m_new(t_tcc);
	return this;
}
c_AndroidNdkBuilder* c_AndroidNdkBuilder::m_new2(){
	c_Builder::m_new2();
	return this;
}
bool c_AndroidNdkBuilder::p_IsValid(){
	return m_tcc->m_ANDROID_PATH!=String() && m_tcc->m_ANDROID_NDK_PATH!=String();
}
void c_AndroidNdkBuilder::p_Begin(){
	bb_config_ENV_LANG=String(L"cpp",3);
	gc_assign(bb_translator__trans,((new c_CppTranslator)->m_new()));
}
String c_AndroidNdkBuilder::p_Config(){
	c_StringStack* t_config=(new c_StringStack)->m_new2();
	c_NodeEnumerator3* t_=bb_config_GetConfigVars()->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_Node2* t_kv=t_->p_NextObject();
		t_config->p_Push(String(L"#define CFG_",12)+t_kv->p_Key()+String(L" ",1)+t_kv->p_Value());
	}
	return t_config->p_Join(String(L"\n",1));
}
bool c_AndroidNdkBuilder::p_CreateDirRecursive(String t_path){
	int t_i=0;
	do{
		t_i=t_path.Find(String(L"/",1),t_i);
		if(t_i==-1){
			CreateDir(t_path);
			return FileType(t_path)==2;
		}
		String t_t=t_path.Slice(0,t_i);
		CreateDir(t_t);
		if(FileType(t_t)!=2){
			return false;
		}
		t_i+=1;
	}while(!(false));
}
void c_AndroidNdkBuilder::p_MakeTarget(){
	bb_config_SetConfigVar2(String(L"ANDROID_SDK_DIR",15),m_tcc->m_ANDROID_PATH.Replace(String(L"\\",1),String(L"\\\\",2)));
	bb_config_SetConfigVar2(String(L"ANDROID_MANIFEST_MAIN",21),bb_config_GetConfigVar(String(L"ANDROID_MANIFEST_MAIN",21)).Replace(String(L";",1),String(L"\n",1))+String(L"\n",1));
	bb_config_SetConfigVar2(String(L"ANDROID_MANIFEST_APPLICATION",28),bb_config_GetConfigVar(String(L"ANDROID_MANIFEST_APPLICATION",28)).Replace(String(L";",1),String(L"\n",1))+String(L"\n",1));
	bb_config_SetConfigVar2(String(L"ANDROID_MANIFEST_ACTIVITY",25),bb_config_GetConfigVar(String(L"ANDROID_MAINFEST_ACTIVITY",25)).Replace(String(L";",1),String(L"\n",1))+String(L"\n",1));
	p_CreateDataDir(String(L"assets/cerberus",15));
	String t_app_label=bb_config_GetConfigVar(String(L"ANDROID_APP_LABEL",17));
	String t_app_package=bb_config_GetConfigVar(String(L"ANDROID_APP_PACKAGE",19));
	String t_main=LoadString(String(L"jni/main.cpp",12));
	t_main=bb_transcc_ReplaceBlock(t_main,String(L"TRANSCODE",9),m_transCode,String(L"\n//",3));
	t_main=bb_transcc_ReplaceBlock(t_main,String(L"CONFIG",6),p_Config(),String(L"\n//",3));
	SaveString(t_main,String(L"jni/main.cpp",12));
	bb_os_DeleteDir(String(L"src",3),true);
	String t_jmain=LoadString(String(L"CerberusGame.java",17));
	t_jmain=bb_transcc_ReplaceBlock(t_jmain,String(L"PACKAGE",7),String(L"package ",8)+t_app_package+String(L";",1),String(L"\n//",3));
	String t_dir=String(L"src/",4)+t_app_package.Replace(String(L".",1),String(L"/",1));
	if(!p_CreateDirRecursive(t_dir)){
		bbError(String(L"Failed to create dir:",21)+t_dir);
	}
	SaveString(t_jmain,t_dir+String(L"/CerberusGame.java",18));
	Array<String > t_=bb_config_GetConfigVar(String(L"LIBS",4)).Split(String(L";",1));
	int t_2=0;
	while(t_2<t_.Length()){
		String t_lib=t_[t_2];
		t_2=t_2+1;
		String t_1=bb_os_ExtractExt(t_lib);
		if(t_1==String(L"jar",3) || t_1==String(L"so",2)){
			String t_tdir=String();
			if(t_lib.Contains(String(L"/",1))){
				t_tdir=bb_os_ExtractDir(t_lib);
				if(t_tdir.Contains(String(L"/",1))){
					t_tdir=bb_os_StripDir(t_tdir);
				}
				String t_22=t_tdir;
				if(t_22==String(L"x86",3) || t_22==String(L"mips",4) || t_22==String(L"armeabi",7) || t_22==String(L"armeabi-v7a",11)){
					CreateDir(String(L"libs/",5)+t_tdir);
					t_tdir=t_tdir+String(L"/",1);
				}else{
					t_tdir=String();
				}
			}
			CopyFile(t_lib,String(L"libs/",5)+t_tdir+bb_os_StripDir(t_lib));
		}
	}
	Array<String > t_3=bb_config_GetConfigVar(String(L"SRCS",4)).Split(String(L";",1));
	int t_4=0;
	while(t_4<t_3.Length()){
		String t_src=t_3[t_4];
		t_4=t_4+1;
		String t_32=bb_os_ExtractExt(t_src);
		if(t_32==String(L"java",4) || t_32==String(L"aidl",4)){
			int t_i=t_src.FindLast(String(L"/src/",5));
			if(t_i!=-1){
				String t_dst=t_src.Slice(t_i+1);
				if(!p_CreateDirRecursive(bb_os_ExtractDir(t_dst))){
					bbError(String(L"Failed to create dir:",21)+bb_os_ExtractDir(t_dst));
				}
				CopyFile(t_src,t_dst);
			}
		}
	}
	Array<String > t_5=bb_os_LoadDir(String(L"templates",9),true,false);
	int t_6=0;
	while(t_6<t_5.Length()){
		String t_file=t_5[t_6];
		t_6=t_6+1;
		int t_i2=0;
		do{
			t_i2=t_file.Find(String(L"/",1),t_i2);
			if(t_i2==-1){
				break;
			}
			CreateDir(t_file.Slice(0,t_i2));
			if(FileType(t_file.Slice(0,t_i2))!=2){
				t_file=String();
				break;
			}
			t_i2+=1;
		}while(!(false));
		if(!((t_file).Length()!=0)){
			continue;
		}
		String t_42=bb_os_ExtractExt(t_file).ToLower();
		if(t_42==String(L"xml",3) || t_42==String(L"properties",10) || t_42==String(L"java",4)){
			String t_str=LoadString(String(L"templates/",10)+t_file);
			t_str=bb_transcc_ReplaceEnv(t_str);
			SaveString(t_str,t_file);
		}else{
			CopyFile(String(L"templates/",10)+t_file,t_file);
		}
	}
	if(m_tcc->m_opt_build){
		if(!p_Execute(m_tcc->m_ANDROID_NDK_PATH+String(L"/ndk-build",10),true)){
			bb_transcc_Die(String(L"Failed to build native code",27));
		}
		bool t_r=p_Execute(String(L"ant clean",9),false) && p_Execute(String(L"ant debug install",17),false);
		if(!t_r){
			bb_transcc_Die(String(L"Android build failed.",21));
		}else{
			if(m_tcc->m_opt_run){
				p_Execute(String(L"adb logcat -c",13),false);
				p_Execute(String(L"adb shell am start -n ",22)+t_app_package+String(L"/",1)+t_app_package+String(L".CerberusGame",13),false);
				p_Execute(String(L"adb logcat [Cerberus]:I *:E",27),false);
			}
		}
	}
}
void c_AndroidNdkBuilder::mark(){
	c_Builder::mark();
}
c_GlfwBuilder::c_GlfwBuilder(){
}
c_GlfwBuilder* c_GlfwBuilder::m_new(c_TransCC* t_tcc){
	c_Builder::m_new(t_tcc);
	return this;
}
c_GlfwBuilder* c_GlfwBuilder::m_new2(){
	c_Builder::m_new2();
	return this;
}
bool c_GlfwBuilder::p_IsValid(){
	String t_2=HostOS();
	if(t_2==String(L"winnt",5)){
		if(((m_tcc->m_MINGW_PATH).Length()!=0) || ((m_tcc->m_MSBUILD_PATH).Length()!=0)){
			return true;
		}
	}else{
		return true;
	}
	return false;
}
void c_GlfwBuilder::p_Begin(){
	bb_config_ENV_LANG=String(L"cpp",3);
	gc_assign(bb_translator__trans,((new c_CppTranslator)->m_new()));
}
String c_GlfwBuilder::p_Config(){
	c_StringStack* t_config=(new c_StringStack)->m_new2();
	c_NodeEnumerator3* t_=bb_config_GetConfigVars()->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_Node2* t_kv=t_->p_NextObject();
		t_config->p_Push(String(L"#define CFG_",12)+t_kv->p_Key()+String(L" ",1)+t_kv->p_Value());
	}
	return t_config->p_Join(String(L"\n",1));
}
int c_GlfwBuilder::p_ProcessExternalLibs(String t_config,String t_arch,String t_link){
	String t_libStr=bb_config_GetConfigVar(String(L"GLFW_COPY_LIBS",14)).ToLower();
	if(t_libStr==String()){
		return 0;
	}
	Array<String > t_libcopy=t_libStr.Split(String(L";",1));
	String t_dst=t_config+t_arch;
	Array<String > t_=t_libcopy;
	int t_2=0;
	while(t_2<t_.Length()){
		String t_i=t_[t_2];
		t_2=t_2+1;
		if(HostOS()==String(L"winnt",5)){
			if(t_i!=String()){
				if(FileType(t_dst+String(L"/",1)+t_i+String(L".dll",4))==1){
					continue;
				}
				if(CopyFile(m_tcc->m__libs+String(L"Win",3)+t_arch+String(L"/",1)+t_i+String(L".dll",4),t_dst+String(L"/",1)+t_i+String(L".dll",4))==1){
					bbPrint(String(L"Copied ",7)+t_i+String(L".dll to ",8)+t_dst);
				}else{
					bbPrint(String(L"Failed to copy ",15)+t_i+String(L" to ",4)+t_dst);
				}
				if(FileType(m_tcc->m__libs+String(L"/",1)+t_i+String(L"_COPYING",8))!=0){
					if(CopyFile(m_tcc->m__libs+String(L"/",1)+t_i+String(L"_COPYING",8),t_dst+String(L"/",1)+t_i+String(L"_COPYING",8))==1){
						bbPrint(String(L"Copied ",7)+t_i+String(L"_COPYING to ",12)+t_dst);
					}else{
						bbPrint(String(L"Failed to copy ",15)+t_i+String(L"_COPYING To ",12)+t_dst);
					}
				}
				if(FileType(m_tcc->m__libs+String(L"/",1)+t_i+String(L"_LICENCE",8))!=0){
					if(CopyFile(m_tcc->m__libs+String(L"/",1)+t_i+String(L"_LICENCE",8),t_dst+String(L"/",1)+t_i+String(L"_LICENCE",8))==1){
						bbPrint(String(L"Copied ",7)+t_i+String(L"_LICENCE to ",12)+t_dst);
					}else{
						bbPrint(String(L"Failed to copy ",15)+t_i+String(L"_COPYING To ",12)+t_dst);
					}
				}
			}
		}
	}
	return 0;
}
void c_GlfwBuilder::p_MakeGcc(){
	String t_msize=bb_config_GetConfigVar(String(L"GLFW_GCC_MSIZE_",15)+HostOS().ToUpper());
	String t_tconfig=m_casedConfig+t_msize;
	String t_dst=String(L"gcc_",4)+HostOS();
	CreateDir(t_dst+String(L"/",1)+t_tconfig);
	CreateDir(t_dst+String(L"/",1)+t_tconfig+String(L"/internal",9));
	CreateDir(t_dst+String(L"/",1)+t_tconfig+String(L"/external",9));
	p_CreateDataDir(t_dst+String(L"/",1)+t_tconfig+String(L"/data",5));
	String t_main=LoadString(String(L"main.cpp",8));
	t_main=bb_transcc_ReplaceBlock(t_main,String(L"TRANSCODE",9),m_transCode,String(L"\n//",3));
	t_main=bb_transcc_ReplaceBlock(t_main,String(L"CONFIG",6),p_Config(),String(L"\n//",3));
	SaveString(t_main,String(L"main.cpp",8));
	if(m_tcc->m_opt_build){
		ChangeDir(t_dst);
		CreateDir(String(L"build",5));
		CreateDir(String(L"build/",6)+t_tconfig);
		String t_ccopts=String();
		String t_ldopts=String();
		String t_libopts=String();
		String t_libcopy=String();
		if((t_msize).Length()!=0){
			t_ccopts=t_ccopts+(String(L" -m",3)+t_msize);
			t_ldopts=t_ldopts+(String(L" -m",3)+t_msize);
		}
		t_ccopts=t_ccopts+(String(L" ",1)+bb_config_GetConfigVar(String(L"GLFW_GCC_CC_OPTS",16)).Replace(String(L";",1),String(L" ",1)));
		t_ldopts=t_ldopts+(String(L" ",1)+bb_config_GetConfigVar(String(L"GLFW_GCC_LD_OPTS",16)).Replace(String(L";",1),String(L" ",1)));
		t_libopts=t_libopts+(String(L" ",1)+bb_config_GetConfigVar(String(L"GLFW_GCC_LIB_OPTS",17)).Replace(String(L";",1),String(L" ",1)));
		String t_1=bb_config_ENV_CONFIG;
		if(t_1==String(L"debug",5)){
			t_ccopts=t_ccopts+String(L" -O0",4);
		}else{
			if(t_1==String(L"release",7)){
				t_ccopts=t_ccopts+String(L" -O3 -DNDEBUG",13);
				t_ldopts=t_ldopts+String(L" -s",3);
			}
		}
		p_ProcessExternalLibs(CurrentDir()+String(L"\\",1)+m_casedConfig,t_msize,t_libopts);
		if(HostOS()==String(L"winnt",5)){
			t_ldopts=t_ldopts+(String(L" -L",3)+m_tcc->m__libs+String(L"Win",3)+t_msize);
		}
		String t_cmd=String(L"make",4);
		if(HostOS()==String(L"winnt",5) && ((FileType(m_tcc->m_MINGW_PATH+String(L"/bin/mingw32-make.exe",21)))!=0)){
			t_cmd=String(L"mingw32-make",12);
		}
		p_Execute(t_cmd+String(L" CCOPTS=\"",9)+t_ccopts+String(L"\" LDOPTS=\"",10)+t_ldopts+String(L"\" LIBOPTS=\"",11)+t_libopts+String(L"\" OUT=\"",7)+t_tconfig+String(L"/CerberusGame\"",14),true);
		if(m_tcc->m_opt_run){
			ChangeDir(t_tconfig);
			if(HostOS()==String(L"winnt",5)){
				p_Execute(String(L"CerberusGame",12),true);
			}else{
				p_Execute(String(L"./CerberusGame",14),true);
			}
		}
	}
}
void c_GlfwBuilder::p_MakeVc2010(){
	CreateDir(String(L"vc2010/",7)+m_casedConfig);
	CreateDir(String(L"vc2010/",7)+m_casedConfig+String(L"/internal",9));
	CreateDir(String(L"vc2010/",7)+m_casedConfig+String(L"/external",9));
	p_CreateDataDir(String(L"vc2010/",7)+m_casedConfig+String(L"/data",5));
	String t_main=LoadString(String(L"main.cpp",8));
	t_main=bb_transcc_ReplaceBlock(t_main,String(L"TRANSCODE",9),m_transCode,String(L"\n//",3));
	t_main=bb_transcc_ReplaceBlock(t_main,String(L"CONFIG",6),p_Config(),String(L"\n//",3));
	SaveString(t_main,String(L"main.cpp",8));
	if(m_tcc->m_opt_build){
		ChangeDir(String(L"vc2010",6));
		p_Execute(String(L"\"",1)+m_tcc->m_MSBUILD_PATH+String(L"\" /p:Configuration=",19)+m_casedConfig+String(L" /p:Platform=Win32 CerberusGame.sln",35),true);
		if(m_tcc->m_opt_run){
			ChangeDir(m_casedConfig);
			p_Execute(String(L"CerberusGame",12),true);
		}
	}
}
void c_GlfwBuilder::p_MakeMsvc(){
	CreateDir(String(L"msvc/",5)+m_casedConfig);
	CreateDir(String(L"msvc/",5)+m_casedConfig+String(L"/internal",9));
	CreateDir(String(L"msvc/",5)+m_casedConfig+String(L"/external",9));
	p_CreateDataDir(String(L"msvc/",5)+m_casedConfig+String(L"/data",5));
	String t_main=LoadString(String(L"main.cpp",8));
	t_main=bb_transcc_ReplaceBlock(t_main,String(L"TRANSCODE",9),m_transCode,String(L"\n//",3));
	t_main=bb_transcc_ReplaceBlock(t_main,String(L"CONFIG",6),p_Config(),String(L"\n//",3));
	SaveString(t_main,String(L"main.cpp",8));
	if(m_tcc->m_opt_build){
		ChangeDir(String(L"msvc",4));
		p_Execute(String(L"\"",1)+m_tcc->m_MSBUILD_PATH+String(L"\" /p:Configuration=",19)+m_casedConfig,true);
		if(m_tcc->m_opt_run){
			ChangeDir(m_casedConfig);
			p_Execute(String(L"CerberusGame",12),true);
		}
	}
}
void c_GlfwBuilder::p_MakeXcode(){
	p_CreateDataDir(String(L"xcode/data",10));
	String t_main=LoadString(String(L"main.cpp",8));
	t_main=bb_transcc_ReplaceBlock(t_main,String(L"TRANSCODE",9),m_transCode,String(L"\n//",3));
	t_main=bb_transcc_ReplaceBlock(t_main,String(L"CONFIG",6),p_Config(),String(L"\n//",3));
	SaveString(t_main,String(L"main.cpp",8));
	if(m_tcc->m_opt_build){
		ChangeDir(String(L"xcode",5));
		p_Execute(String(L"xcodebuild -configuration ",26)+m_casedConfig,true);
		if(m_tcc->m_opt_run){
			ChangeDir(String(L"build/",6)+m_casedConfig);
			ChangeDir(String(L"CerberusGame.app/Contents/MacOS",31));
			p_Execute(String(L"./CerberusGame",14),true);
		}
	}
}
void c_GlfwBuilder::p_MakeTarget(){
	String t_3=HostOS();
	if(t_3==String(L"winnt",5)){
		if(bb_config_GetConfigVar(String(L"GLFW_USE_MINGW",14))==String(L"1",1) && ((m_tcc->m_MINGW_PATH).Length()!=0)){
			p_MakeGcc();
		}else{
			if(FileType(String(L"vc2010",6))==2){
				p_MakeVc2010();
			}else{
				if(FileType(String(L"msvc",4))==2){
					p_MakeMsvc();
				}else{
					if((m_tcc->m_MINGW_PATH).Length()!=0){
						p_MakeGcc();
					}
				}
			}
		}
	}else{
		if(t_3==String(L"macos",5)){
			p_MakeXcode();
		}else{
			if(t_3==String(L"linux",5)){
				p_MakeGcc();
			}
		}
	}
}
void c_GlfwBuilder::mark(){
	c_Builder::mark();
}
c_Html5Builder::c_Html5Builder(){
}
c_Html5Builder* c_Html5Builder::m_new(c_TransCC* t_tcc){
	c_Builder::m_new(t_tcc);
	return this;
}
c_Html5Builder* c_Html5Builder::m_new2(){
	c_Builder::m_new2();
	return this;
}
bool c_Html5Builder::p_IsValid(){
	return true;
}
void c_Html5Builder::p_Begin(){
	bb_config_ENV_LANG=String(L"js",2);
	gc_assign(bb_translator__trans,((new c_JsTranslator)->m_new()));
}
String c_Html5Builder::p_MetaData(){
	c_StringStack* t_meta=(new c_StringStack)->m_new2();
	c_NodeEnumerator3* t_=m_dataFiles->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_Node2* t_kv=t_->p_NextObject();
		String t_src=t_kv->p_Key();
		String t_ext=bb_os_ExtractExt(t_src).ToLower();
		String t_3=t_ext;
		if(t_3==String(L"png",3) || t_3==String(L"jpg",3) || t_3==String(L"gif",3)){
			bb_html5_Info_Width=0;
			bb_html5_Info_Height=0;
			String t_4=t_ext;
			if(t_4==String(L"png",3)){
				bb_html5_GetInfo_PNG(t_src);
			}else{
				if(t_4==String(L"jpg",3)){
					bb_html5_GetInfo_JPG(t_src);
				}else{
					if(t_4==String(L"gif",3)){
						bb_html5_GetInfo_GIF(t_src);
					}
				}
			}
			if(bb_html5_Info_Width==0 || bb_html5_Info_Height==0){
				bb_transcc_Die(String(L"Unable to load image file '",27)+t_src+String(L"'.",2));
			}
			t_meta->p_Push(String(L"[",1)+t_kv->p_Value()+String(L"];type=image/",13)+t_ext+String(L";",1));
			t_meta->p_Push(String(L"width=",6)+String(bb_html5_Info_Width)+String(L";",1));
			t_meta->p_Push(String(L"height=",7)+String(bb_html5_Info_Height)+String(L";",1));
			t_meta->p_Push(String(L"\\n",2));
		}
	}
	return t_meta->p_Join(String());
}
String c_Html5Builder::p_Config(){
	c_StringStack* t_config=(new c_StringStack)->m_new2();
	c_NodeEnumerator3* t_=bb_config_GetConfigVars()->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_Node2* t_kv=t_->p_NextObject();
		t_config->p_Push(String(L"CFG_",4)+t_kv->p_Key()+String(L"=",1)+bb_config_Enquote(t_kv->p_Value(),String(L"js",2))+String(L";",1));
	}
	return t_config->p_Join(String(L"\n",1));
}
void c_Html5Builder::p_MakeTarget(){
	p_CreateDataDir(String(L"data",4));
	String t_meta=String(L"var META_DATA=\"",15)+p_MetaData()+String(L"\";\n",3);
	String t_main=LoadString(String(L"main.js",7));
	t_main=bb_transcc_ReplaceBlock(t_main,String(L"TRANSCODE",9),m_transCode,String(L"\n//",3));
	t_main=bb_transcc_ReplaceBlock(t_main,String(L"METADATA",8),t_meta,String(L"\n//",3));
	t_main=bb_transcc_ReplaceBlock(t_main,String(L"CONFIG",6),p_Config(),String(L"\n//",3));
	SaveString(t_main,String(L"main.js",7));
	if(m_tcc->m_opt_run){
		String t_p=RealPath(String(L"CerberusGame.html",17));
		String t_t=m_tcc->m_HTML_PLAYER+String(L" \"",2)+t_p+String(L"\"",1);
		p_Execute(t_t,false);
	}
}
void c_Html5Builder::mark(){
	c_Builder::mark();
}
c_IosBuilder::c_IosBuilder(){
	m__buildFiles=(new c_StringMap2)->m_new();
	m__nextFileId=0;
	m__fileRefs=(new c_StringMap2)->m_new();
}
c_IosBuilder* c_IosBuilder::m_new(c_TransCC* t_tcc){
	c_Builder::m_new(t_tcc);
	return this;
}
c_IosBuilder* c_IosBuilder::m_new2(){
	c_Builder::m_new2();
	return this;
}
bool c_IosBuilder::p_IsValid(){
	String t_1=HostOS();
	if(t_1==String(L"macos",5)){
		return true;
	}
	return false;
}
void c_IosBuilder::p_Begin(){
	bb_config_ENV_LANG=String(L"cpp",3);
	gc_assign(bb_translator__trans,((new c_CppTranslator)->m_new()));
}
String c_IosBuilder::p_Config(){
	c_StringStack* t_config=(new c_StringStack)->m_new2();
	c_NodeEnumerator3* t_=bb_config_GetConfigVars()->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_Node2* t_kv=t_->p_NextObject();
		t_config->p_Push(String(L"#define CFG_",12)+t_kv->p_Key()+String(L" ",1)+t_kv->p_Value());
	}
	return t_config->p_Join(String(L"\n",1));
}
String c_IosBuilder::p_FileId(String t_path,c_StringMap2* t_map){
	String t_id=t_map->p_Get(t_path);
	if((t_id).Length()!=0){
		return t_id;
	}
	m__nextFileId+=1;
	t_id=String(L"1ACECAFEBABE",12)+(String(L"0000000000000000",16)+String(m__nextFileId)).Slice(-12);
	t_map->p_Set2(t_path,t_id);
	return t_id;
}
void c_IosBuilder::p_AddBuildFile(String t_path){
	p_FileId(t_path,m__buildFiles);
}
int c_IosBuilder::p_FindEol(String t_str,String t_substr,int t_start){
	int t_i=t_str.Find(t_substr,t_start);
	if(t_i==-1){
		bbPrint(String(L"Can't find ",11)+t_substr);
		return -1;
	}
	t_i+=t_substr.Length();
	int t_eol=t_str.Find(String(L"\n",1),t_i)+1;
	if(t_eol==0){
		return t_str.Length();
	}
	return t_eol;
}
String c_IosBuilder::p_BuildFiles(){
	c_StringStack* t_buf=(new c_StringStack)->m_new2();
	c_NodeEnumerator3* t_=m__buildFiles->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_Node2* t_it=t_->p_NextObject();
		String t_path=t_it->p_Key();
		String t_id=t_it->p_Value();
		String t_fileRef=p_FileId(t_path,m__fileRefs);
		String t_dir=bb_os_ExtractDir(t_path);
		String t_name=bb_os_StripDir(t_path);
		String t_2=bb_os_ExtractExt(t_name);
		if(t_2==String(L"a",1) || t_2==String(L"framework",9)){
			t_buf->p_Push(String(L"\t\t",2)+t_id+String(L" = {isa = PBXBuildFile; fileRef = ",34)+t_fileRef+String(L"; };",4));
		}
	}
	if((t_buf->p_Length2())!=0){
		t_buf->p_Push(String());
	}
	return t_buf->p_Join(String(L"\n",1));
}
String c_IosBuilder::p_FileRefs(){
	c_StringStack* t_buf=(new c_StringStack)->m_new2();
	c_NodeEnumerator3* t_=m__fileRefs->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_Node2* t_it=t_->p_NextObject();
		String t_path=t_it->p_Key();
		String t_id=t_it->p_Value();
		String t_dir=bb_os_ExtractDir(t_path);
		String t_name=bb_os_StripDir(t_path);
		String t_3=bb_os_ExtractExt(t_name);
		if(t_3==String(L"a",1)){
			t_buf->p_Push(String(L"\t\t",2)+t_id+String(L" = {isa = PBXFileReference; lastKnownFileType = archive.ar; path = \"",68)+t_name+String(L"\"; sourceTree = \"<group>\"; };",29));
		}else{
			if(t_3==String(L"h",1)){
				t_buf->p_Push(String(L"\t\t",2)+t_id+String(L" = {isa = PBXFileReference; fileEncoding = 4; lastKnownFileType = sourcecode.c.h; path = ",89)+t_name+String(L"; sourceTree = \"<group>\"; };",28));
			}else{
				if(t_3==String(L"framework",9)){
					if((t_dir).Length()!=0){
						t_buf->p_Push(String(L"\t\t",2)+t_id+String(L" = {isa = PBXFileReference; lastKnownFileType = wrapper.framework; name = ",74)+t_name+String(L"; path = ",9)+t_name+String(L"; sourceTree = \"<group>\"; };",28));
					}else{
						t_buf->p_Push(String(L"\t\t",2)+t_id+String(L" = {isa = PBXFileReference; lastKnownFileType = wrapper.framework; name = ",74)+t_name+String(L"; path = System/Library/Frameworks/",35)+t_name+String(L"; sourceTree = SDKROOT; };",26));
					}
				}
			}
		}
	}
	if((t_buf->p_Length2())!=0){
		t_buf->p_Push(String());
	}
	return t_buf->p_Join(String(L"\n",1));
}
String c_IosBuilder::p_FrameworksBuildPhase(){
	c_StringStack* t_buf=(new c_StringStack)->m_new2();
	c_NodeEnumerator3* t_=m__buildFiles->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_Node2* t_it=t_->p_NextObject();
		String t_path=t_it->p_Key();
		String t_id=t_it->p_Value();
		String t_4=bb_os_ExtractExt(t_path);
		if(t_4==String(L"a",1) || t_4==String(L"framework",9)){
			t_buf->p_Push(String(L"\t\t\t\t",4)+t_id);
		}
	}
	if((t_buf->p_Length2())!=0){
		t_buf->p_Push(String());
	}
	return t_buf->p_Join(String(L",\n",2));
}
String c_IosBuilder::p_FrameworksGroup(){
	c_StringStack* t_buf=(new c_StringStack)->m_new2();
	c_NodeEnumerator3* t_=m__fileRefs->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_Node2* t_it=t_->p_NextObject();
		String t_path=t_it->p_Key();
		String t_id=t_it->p_Value();
		String t_5=bb_os_ExtractExt(t_path);
		if(t_5==String(L"framework",9)){
			t_buf->p_Push(String(L"\t\t\t\t",4)+t_id);
		}
	}
	if((t_buf->p_Length2())!=0){
		t_buf->p_Push(String());
	}
	return t_buf->p_Join(String(L",\n",2));
}
String c_IosBuilder::p_LibsGroup(){
	c_StringStack* t_buf=(new c_StringStack)->m_new2();
	c_NodeEnumerator3* t_=m__fileRefs->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_Node2* t_it=t_->p_NextObject();
		String t_path=t_it->p_Key();
		String t_id=t_it->p_Value();
		String t_6=bb_os_ExtractExt(t_path);
		if(t_6==String(L"a",1) || t_6==String(L"h",1)){
			t_buf->p_Push(String(L"\t\t\t\t",4)+t_id);
		}
	}
	if((t_buf->p_Length2())!=0){
		t_buf->p_Push(String());
	}
	return t_buf->p_Join(String(L",\n",2));
}
String c_IosBuilder::p_MungProj(String t_proj){
	int t_i=-1;
	t_i=p_FindEol(t_proj,String(L"/* Begin PBXBuildFile section */",32),0);
	if(t_i==-1){
		return String();
	}
	t_proj=t_proj.Slice(0,t_i)+p_BuildFiles()+t_proj.Slice(t_i);
	t_i=p_FindEol(t_proj,String(L"/* Begin PBXFileReference section */",36),0);
	if(t_i==-1){
		return String();
	}
	t_proj=t_proj.Slice(0,t_i)+p_FileRefs()+t_proj.Slice(t_i);
	t_i=p_FindEol(t_proj,String(L"/* Begin PBXFrameworksBuildPhase section */",43),0);
	if(t_i!=-1){
		t_i=p_FindEol(t_proj,String(L"/* Frameworks */ = {",20),t_i);
	}
	if(t_i!=-1){
		t_i=p_FindEol(t_proj,String(L"files = (",9),t_i);
	}
	if(t_i==-1){
		return String();
	}
	t_proj=t_proj.Slice(0,t_i)+p_FrameworksBuildPhase()+t_proj.Slice(t_i);
	t_i=p_FindEol(t_proj,String(L"/* Begin PBXGroup section */",28),0);
	if(t_i!=-1){
		t_i=p_FindEol(t_proj,String(L"/* Frameworks */ = {",20),t_i);
	}
	if(t_i!=-1){
		t_i=p_FindEol(t_proj,String(L"children = (",12),t_i);
	}
	if(t_i==-1){
		return String();
	}
	t_proj=t_proj.Slice(0,t_i)+p_FrameworksGroup()+t_proj.Slice(t_i);
	t_i=p_FindEol(t_proj,String(L"/* Begin PBXGroup section */",28),0);
	if(t_i!=-1){
		t_i=p_FindEol(t_proj,String(L"/* libs */ = {",14),t_i);
	}
	if(t_i!=-1){
		t_i=p_FindEol(t_proj,String(L"children = (",12),t_i);
	}
	if(t_i==-1){
		return String();
	}
	t_proj=t_proj.Slice(0,t_i)+p_LibsGroup()+t_proj.Slice(t_i);
	return t_proj;
}
void c_IosBuilder::p_MungProj2(){
	String t_path=String(L"CerberusGame.xcodeproj/project.pbxproj",38);
	String t_proj=LoadString(t_path);
	c_StringStack* t_buf=(new c_StringStack)->m_new2();
	Array<String > t_=t_proj.Split(String(L"\n",1));
	int t_2=0;
	while(t_2<t_.Length()){
		String t_line=t_[t_2];
		t_2=t_2+1;
		if(!t_line.Trim().StartsWith(String(L"1ACECAFEBABE",12))){
			t_buf->p_Push(t_line);
		}
	}
	t_proj=t_buf->p_Join(String(L"\n",1));
	t_proj=p_MungProj(t_proj);
	if(!((t_proj).Length()!=0)){
		bb_transcc_Die(String(L"Failed to mung XCode project file",33));
	}
	SaveString(t_proj,t_path);
}
void c_IosBuilder::p_MakeTarget(){
	String t_sim_path=String(L"/Applications/Xcode.app/Contents/Developer/Applications/Simulator.app",69);
	if(m_tcc->m_opt_run==true){
		bbPrint(String(L"Starting iOS simulator...",25));
		p_Execute(String(L"open \"",6)+t_sim_path+String(L"\"",1),true);
	}
	p_CreateDataDir(String(L"data",4));
	String t_main=LoadString(String(L"main.mm",7));
	t_main=bb_transcc_ReplaceBlock(t_main,String(L"TRANSCODE",9),m_transCode,String(L"\n//",3));
	t_main=bb_transcc_ReplaceBlock(t_main,String(L"CONFIG",6),p_Config(),String(L"\n//",3));
	SaveString(t_main,String(L"main.mm",7));
	String t_libs=bb_config_GetConfigVar(String(L"LIBS",4));
	if((t_libs).Length()!=0){
		Array<String > t_=t_libs.Split(String(L";",1));
		int t_2=0;
		while(t_2<t_.Length()){
			String t_lib=t_[t_2];
			t_2=t_2+1;
			if(!((t_lib).Length()!=0)){
				continue;
			}
			String t_7=bb_os_ExtractExt(t_lib);
			if(t_7==String(L"a",1) || t_7==String(L"h",1)){
				String t_path=String(L"libs/",5)+bb_os_StripDir(t_lib);
				CopyFile(t_lib,t_path);
				p_AddBuildFile(t_path);
			}else{
				if(t_7==String(L"framework",9)){
					if(FileType(t_lib)==2){
						String t_path2=String(L"libs/",5)+bb_os_StripDir(t_lib);
						bb_os_CopyDir(t_lib,t_path2,true,false);
					}
					p_AddBuildFile(t_lib);
				}else{
					bb_transcc_Die(String(L"Unrecognized lib file type:",27)+t_lib);
				}
			}
		}
	}
	p_MungProj2();
	if(!m_tcc->m_opt_build){
		return;
	}
	p_Execute(String(L"xcodebuild -configuration ",26)+m_casedConfig+String(L" -sdk iphonesimulator",21),true);
	if(!m_tcc->m_opt_run){
		return;
	}
	String t_src=String(L"build/",6)+m_casedConfig+String(L"-iphonesimulator/CerberusGame.app",33);
	if(FileType(t_sim_path)==2){
		bbPrint(String(L"Installing GerberusGame.app",27));
		p_Execute(String(L"xcrun simctl install booted \"",29)+t_src+String(L"\"",1),true);
		bbPrint(String(L"Running CerberusGame.app",24));
		p_Execute(String(L"xcrun simctl launch booted com.yourcompany.CerberusGame",55),true);
		return;
	}
}
void c_IosBuilder::mark(){
	c_Builder::mark();
	gc_mark_q(m__buildFiles);
	gc_mark_q(m__fileRefs);
}
c_FlashBuilder::c_FlashBuilder(){
}
c_FlashBuilder* c_FlashBuilder::m_new(c_TransCC* t_tcc){
	c_Builder::m_new(t_tcc);
	return this;
}
c_FlashBuilder* c_FlashBuilder::m_new2(){
	c_Builder::m_new2();
	return this;
}
bool c_FlashBuilder::p_IsValid(){
	return m_tcc->m_FLEX_PATH!=String();
}
void c_FlashBuilder::p_Begin(){
	bb_config_ENV_LANG=String(L"as",2);
	gc_assign(bb_translator__trans,((new c_AsTranslator)->m_new()));
}
String c_FlashBuilder::p_Assets(){
	c_StringStack* t_assets=(new c_StringStack)->m_new2();
	c_NodeEnumerator3* t_=m_dataFiles->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_Node2* t_kv=t_->p_NextObject();
		String t_ext=bb_os_ExtractExt(t_kv->p_Value());
		String t_munged=String(L"_",1);
		Array<String > t_2=bb_os_StripExt(t_kv->p_Value()).Split(String(L"/",1));
		int t_3=0;
		while(t_3<t_2.Length()){
			String t_q=t_2[t_3];
			t_3=t_3+1;
			for(int t_i=0;t_i<t_q.Length();t_i=t_i+1){
				if(((bb_config_IsAlpha((int)t_q[t_i]))!=0) || ((bb_config_IsDigit((int)t_q[t_i]))!=0) || (int)t_q[t_i]==95){
					continue;
				}
				bb_transcc_Die(String(L"Invalid character in flash filename: ",37)+t_kv->p_Value()+String(L".",1));
			}
			t_munged=t_munged+(String(t_q.Length())+t_q);
		}
		t_munged=t_munged+(String(t_ext.Length())+t_ext);
		String t_1=t_ext.ToLower();
		if(t_1==String(L"png",3) || t_1==String(L"jpg",3) || t_1==String(L"mp3",3)){
			t_assets->p_Push(String(L"[Embed(source=\"data/",20)+t_kv->p_Value()+String(L"\")]",3));
			t_assets->p_Push(String(L"public static var ",18)+t_munged+String(L":Class;",7));
		}else{
			t_assets->p_Push(String(L"[Embed(source=\"data/",20)+t_kv->p_Value()+String(L"\",mimeType=\"application/octet-stream\")]",39));
			t_assets->p_Push(String(L"public static var ",18)+t_munged+String(L":Class;",7));
		}
	}
	return t_assets->p_Join(String(L"\n",1));
}
String c_FlashBuilder::p_Config(){
	c_StringStack* t_config=(new c_StringStack)->m_new2();
	c_NodeEnumerator3* t_=bb_config_GetConfigVars()->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_Node2* t_kv=t_->p_NextObject();
		t_config->p_Push(String(L"internal static var ",20)+t_kv->p_Key()+String(L":String=",8)+bb_config_Enquote(t_kv->p_Value(),String(L"as",2)));
	}
	return t_config->p_Join(String(L"\n",1));
}
void c_FlashBuilder::p_MakeTarget(){
	p_CreateDataDir(String(L"data",4));
	String t_main=LoadString(String(L"CerberusGame.as",15));
	t_main=bb_transcc_ReplaceBlock(t_main,String(L"TRANSCODE",9),m_transCode,String(L"\n//",3));
	t_main=bb_transcc_ReplaceBlock(t_main,String(L"ASSETS",6),p_Assets(),String(L"\n//",3));
	t_main=bb_transcc_ReplaceBlock(t_main,String(L"CONFIG",6),p_Config(),String(L"\n//",3));
	SaveString(t_main,String(L"CerberusGame.as",15));
	if(m_tcc->m_opt_build){
		String t_cc_opts=String(L" -static-link-runtime-shared-libraries=true",43);
		if(bb_config_ENV_CONFIG==String(L"debug",5)){
			t_cc_opts=t_cc_opts+String(L" -debug=true",12);
		}
		DeleteFile(String(L"main.swf",8));
		p_Execute(String(L"mxmlc",5)+t_cc_opts+String(L" CerberusGame.as",16),true);
		if(m_tcc->m_opt_run){
			if((m_tcc->m_FLASH_PLAYER).Length()!=0){
				p_Execute(m_tcc->m_FLASH_PLAYER+String(L" \"",2)+RealPath(String(L"CerberusGame.swf",16))+String(L"\"",1),false);
			}else{
				if((m_tcc->m_HTML_PLAYER).Length()!=0){
					p_Execute(m_tcc->m_HTML_PLAYER+String(L" \"",2)+RealPath(String(L"CerberusGame.html",17))+String(L"\"",1),false);
				}
			}
		}
	}
}
void c_FlashBuilder::mark(){
	c_Builder::mark();
}
c_PsmBuilder::c_PsmBuilder(){
}
c_PsmBuilder* c_PsmBuilder::m_new(c_TransCC* t_tcc){
	c_Builder::m_new(t_tcc);
	return this;
}
c_PsmBuilder* c_PsmBuilder::m_new2(){
	c_Builder::m_new2();
	return this;
}
bool c_PsmBuilder::p_IsValid(){
	String t_4=HostOS();
	if(t_4==String(L"winnt",5)){
		if(((m_tcc->m_PSM_PATH).Length()!=0) && FileType(m_tcc->m_PSM_PATH+String(L"/tools/PsmStudio/bin/mdtool.exe",31))==1){
			return true;
		}
	}
	return false;
}
void c_PsmBuilder::p_Begin(){
	bb_config_ENV_LANG=String(L"cs",2);
	gc_assign(bb_translator__trans,((new c_CsTranslator)->m_new()));
}
String c_PsmBuilder::p_Content(){
	c_StringStack* t_cont=(new c_StringStack)->m_new2();
	c_NodeEnumerator3* t_=m_dataFiles->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_Node2* t_kv=t_->p_NextObject();
		String t_p=t_kv->p_Key();
		String t_r=t_kv->p_Value();
		String t_f=bb_os_StripDir(t_r);
		String t_t=(String(L"data/",5)+t_r).Replace(String(L"/",1),String(L"\\",1));
		String t_ext=bb_os_ExtractExt(t_r).ToLower();
		if(bb_transcc_MatchPath(t_r,m_TEXT_FILES)){
			t_cont->p_Push(String(L"    <Content Include=\"",22)+t_t+String(L"\">",2));
			t_cont->p_Push(String(L"      <CopyToOutputDirectory>PreserveNewest</CopyToOutputDirectory>",67));
			t_cont->p_Push(String(L"    </Content>",14));
		}else{
			if(bb_transcc_MatchPath(t_r,m_IMAGE_FILES)){
				String t_1=t_ext;
				if(t_1==String(L"png",3) || t_1==String(L"jpg",3) || t_1==String(L"bmp",3) || t_1==String(L"gif",3)){
					t_cont->p_Push(String(L"    <Content Include=\"",22)+t_t+String(L"\">",2));
					t_cont->p_Push(String(L"      <CopyToOutputDirectory>PreserveNewest</CopyToOutputDirectory>",67));
					t_cont->p_Push(String(L"    </Content>",14));
				}else{
					bb_transcc_Die(String(L"Invalid image file type",23));
				}
			}else{
				if(bb_transcc_MatchPath(t_r,m_SOUND_FILES)){
					String t_2=t_ext;
					if(t_2==String(L"wav",3)){
						t_cont->p_Push(String(L"    <Content Include=\"",22)+t_t+String(L"\">",2));
						t_cont->p_Push(String(L"      <CopyToOutputDirectory>PreserveNewest</CopyToOutputDirectory>",67));
						t_cont->p_Push(String(L"    </Content>",14));
					}else{
						bb_transcc_Die(String(L"Invalid sound file type",23));
					}
				}else{
					if(bb_transcc_MatchPath(t_r,m_MUSIC_FILES)){
						String t_3=t_ext;
						if(t_3==String(L"mp3",3)){
							t_cont->p_Push(String(L"    <Content Include=\"",22)+t_t+String(L"\">",2));
							t_cont->p_Push(String(L"      <CopyToOutputDirectory>PreserveNewest</CopyToOutputDirectory>",67));
							t_cont->p_Push(String(L"    </Content>",14));
						}else{
							bb_transcc_Die(String(L"Invalid music file type",23));
						}
					}else{
						if(bb_transcc_MatchPath(t_r,m_BINARY_FILES)){
							t_cont->p_Push(String(L"    <Content Include=\"",22)+t_t+String(L"\">",2));
							t_cont->p_Push(String(L"      <CopyToOutputDirectory>PreserveNewest</CopyToOutputDirectory>",67));
							t_cont->p_Push(String(L"    </Content>",14));
						}
					}
				}
			}
		}
	}
	return t_cont->p_Join(String(L"\n",1));
}
String c_PsmBuilder::p_Config(){
	c_StringStack* t_config=(new c_StringStack)->m_new2();
	c_NodeEnumerator3* t_=bb_config_GetConfigVars()->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_Node2* t_kv=t_->p_NextObject();
		t_config->p_Push(String(L"public const String ",20)+t_kv->p_Key()+String(L"=",1)+bb_config_Enquote(t_kv->p_Value(),String(L"cs",2))+String(L";",1));
	}
	return t_config->p_Join(String(L"\n",1));
}
void c_PsmBuilder::p_MakeTarget(){
	p_CreateDataDir(String(L"data",4));
	String t_proj=LoadString(String(L"CerberusGame.csproj",19));
	t_proj=bb_transcc_ReplaceBlock(t_proj,String(L"CONTENT",7),p_Content(),String(L"\n<!-- ",6));
	SaveString(t_proj,String(L"CerberusGame.csproj",19));
	String t_main=LoadString(String(L"CerberusGame.cs",15));
	t_main=bb_transcc_ReplaceBlock(t_main,String(L"TRANSCODE",9),m_transCode,String(L"\n//",3));
	t_main=bb_transcc_ReplaceBlock(t_main,String(L"CONFIG",6),p_Config(),String(L"\n//",3));
	SaveString(t_main,String(L"CerberusGame.cs",15));
	if(m_tcc->m_opt_build){
		if(m_tcc->m_opt_run){
			p_Execute(String(L"\"",1)+m_tcc->m_PSM_PATH+String(L"/tools/PsmStudio/bin/mdtool\" psm windows run-project CerberusGame.sln",69),true);
		}
	}
}
void c_PsmBuilder::mark(){
	c_Builder::mark();
}
c_StdcppBuilder::c_StdcppBuilder(){
}
c_StdcppBuilder* c_StdcppBuilder::m_new(c_TransCC* t_tcc){
	c_Builder::m_new(t_tcc);
	return this;
}
c_StdcppBuilder* c_StdcppBuilder::m_new2(){
	c_Builder::m_new2();
	return this;
}
bool c_StdcppBuilder::p_IsValid(){
	String t_1=HostOS();
	if(t_1==String(L"winnt",5)){
		if((m_tcc->m_MINGW_PATH).Length()!=0){
			return true;
		}
	}else{
		return true;
	}
	return false;
}
void c_StdcppBuilder::p_Begin(){
	bb_config_ENV_LANG=String(L"cpp",3);
	gc_assign(bb_translator__trans,((new c_CppTranslator)->m_new()));
}
String c_StdcppBuilder::p_Config(){
	c_StringStack* t_config=(new c_StringStack)->m_new2();
	c_NodeEnumerator3* t_=bb_config_GetConfigVars()->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_Node2* t_kv=t_->p_NextObject();
		t_config->p_Push(String(L"#define CFG_",12)+t_kv->p_Key()+String(L" ",1)+t_kv->p_Value());
	}
	return t_config->p_Join(String(L"\n",1));
}
void c_StdcppBuilder::p_MakeTarget(){
	String t_2=bb_config_ENV_CONFIG;
	if(t_2==String(L"debug",5)){
		bb_config_SetConfigVar2(String(L"DEBUG",5),String(L"1",1));
	}else{
		if(t_2==String(L"release",7)){
			bb_config_SetConfigVar2(String(L"RELEASE",7),String(L"1",1));
		}else{
			if(t_2==String(L"profile",7)){
				bb_config_SetConfigVar2(String(L"PROFILE",7),String(L"1",1));
			}
		}
	}
	String t_main=LoadString(String(L"main.cpp",8));
	t_main=bb_transcc_ReplaceBlock(t_main,String(L"TRANSCODE",9),m_transCode,String(L"\n//",3));
	t_main=bb_transcc_ReplaceBlock(t_main,String(L"CONFIG",6),p_Config(),String(L"\n//",3));
	SaveString(t_main,String(L"main.cpp",8));
	if(m_tcc->m_opt_build){
		String t_out=String(L"main_",5)+HostOS();
		DeleteFile(t_out);
		String t_OPTS=String();
		String t_LIBS=String();
		String t_3=bb_config_ENV_HOST;
		if(t_3==String(L"winnt",5)){
			t_OPTS=t_OPTS+String(L" -Wno-free-nonheap-object",25);
			t_LIBS=t_LIBS+String(L" -lwinmm -lws2_32",17);
		}else{
			if(t_3==String(L"macos",5)){
				t_OPTS=t_OPTS+String(L" -Wno-parentheses -Wno-dangling-else",36);
				t_OPTS=t_OPTS+String(L" -mmacosx-version-min=10.9 -std=gnu++0x -stdlib=libc++",54);
			}else{
				if(t_3==String(L"linux",5)){
					t_OPTS=t_OPTS+String(L" -Wno-unused-result",19);
					t_LIBS=t_LIBS+String(L" -lpthread",10);
				}
			}
		}
		String t_4=bb_config_ENV_CONFIG;
		if(t_4==String(L"debug",5)){
			t_OPTS=t_OPTS+String(L" -O0",4);
		}else{
			if(t_4==String(L"release",7)){
				t_OPTS=t_OPTS+String(L" -O3 -DNDEBUG",13);
			}
		}
		String t_cc_opts=bb_config_GetConfigVar(String(L"CC_OPTS",7));
		if((t_cc_opts).Length()!=0){
			t_OPTS=t_OPTS+(String(L" ",1)+t_cc_opts.Replace(String(L";",1),String(L" ",1)));
		}
		String t_cc_libs=bb_config_GetConfigVar(String(L"CC_LIBS",7));
		if((t_cc_libs).Length()!=0){
			t_LIBS=t_LIBS+(String(L" ",1)+t_cc_libs.Replace(String(L";",1),String(L" ",1)));
		}
		if(HostOS()==String(L"macos",5)){
			p_Execute(String(L"clang++",7)+t_OPTS+String(L" -o ",4)+t_out+String(L" main.cpp",9)+t_LIBS,true);
		}else{
			p_Execute(String(L"g++",3)+t_OPTS+String(L" -o ",4)+t_out+String(L" main.cpp",9)+t_LIBS,true);
		}
		if(m_tcc->m_opt_run){
			p_Execute(String(L"\"",1)+RealPath(t_out)+String(L"\"",1),true);
		}
	}
}
void c_StdcppBuilder::mark(){
	c_Builder::mark();
}
c_WinrtBuilder::c_WinrtBuilder(){
}
c_WinrtBuilder* c_WinrtBuilder::m_new(c_TransCC* t_tcc){
	c_Builder::m_new(t_tcc);
	return this;
}
c_WinrtBuilder* c_WinrtBuilder::m_new2(){
	c_Builder::m_new2();
	return this;
}
bool c_WinrtBuilder::p_IsValid(){
	String t_1=HostOS();
	if(t_1==String(L"winnt",5)){
		if((m_tcc->m_MSBUILD_PATH).Length()!=0){
			return true;
		}
	}
	return false;
}
void c_WinrtBuilder::p_Begin(){
	bb_config_ENV_LANG=String(L"cpp",3);
	gc_assign(bb_translator__trans,((new c_CppTranslator)->m_new()));
}
String c_WinrtBuilder::p_Content2(bool t_csharp){
	c_StringStack* t_cont=(new c_StringStack)->m_new2();
	c_NodeEnumerator3* t_=m_dataFiles->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_Node2* t_kv=t_->p_NextObject();
		if(t_csharp){
			t_cont->p_Push(String(L"    <Content Include=\"Assets\\cerberus\\",38)+t_kv->p_Value().Replace(String(L"/",1),String(L"\\",1))+String(L"\">",2));
			t_cont->p_Push(String(L"      <CopyToOutputDirectory>PreserveNewest</CopyToOutputDirectory>",67));
			t_cont->p_Push(String(L"    </Content>",14));
		}else{
			t_cont->p_Push(String(L"    <None Include=\"Assets\\cerberus\\",35)+t_kv->p_Value().Replace(String(L"/",1),String(L"\\",1))+String(L"\">",2));
			t_cont->p_Push(String(L"      <DeploymentContent>true</DeploymentContent>",49));
			t_cont->p_Push(String(L"    </None>",11));
		}
	}
	return t_cont->p_Join(String(L"\n",1));
}
String c_WinrtBuilder::p_Config(){
	c_StringStack* t_config=(new c_StringStack)->m_new2();
	c_NodeEnumerator3* t_=bb_config_GetConfigVars()->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_Node2* t_kv=t_->p_NextObject();
		t_config->p_Push(String(L"#define CFG_",12)+t_kv->p_Key()+String(L" ",1)+t_kv->p_Value());
	}
	return t_config->p_Join(String(L"\n",1));
}
void c_WinrtBuilder::p_MakeTarget(){
	p_CreateDataDir(String(L"Assets/cerberus",15));
	String t_proj=LoadString(String(L"CerberusGame.vcxproj",20));
	if((t_proj).Length()!=0){
		t_proj=bb_transcc_ReplaceBlock(t_proj,String(L"CONTENT",7),p_Content2(false),String(L"\n    <!-- ",10));
		SaveString(t_proj,String(L"CerberusGame.vcxproj",20));
	}else{
		String t_proj2=LoadString(String(L"CerberusGame.csproj",19));
		t_proj2=bb_transcc_ReplaceBlock(t_proj2,String(L"CONTENT",7),p_Content2(true),String(L"\n    <!-- ",10));
		SaveString(t_proj2,String(L"CerberusGame.csproj",19));
	}
	String t_main=LoadString(String(L"CerberusGame.cpp",16));
	t_main=bb_transcc_ReplaceBlock(t_main,String(L"TRANSCODE",9),m_transCode,String(L"\n//",3));
	t_main=bb_transcc_ReplaceBlock(t_main,String(L"CONFIG",6),p_Config(),String(L"\n//",3));
	SaveString(t_main,String(L"CerberusGame.cpp",16));
	if(m_tcc->m_opt_build){
		p_Execute(String(L"\"",1)+m_tcc->m_MSBUILD_PATH+String(L"\" /p:Configuration=",19)+m_casedConfig+String(L" /p:Platform=Win32 CerberusGame.sln",35),true);
		if(m_tcc->m_opt_run){
		}
	}
}
void c_WinrtBuilder::mark(){
	c_Builder::mark();
}
c_XnaBuilder::c_XnaBuilder(){
}
c_XnaBuilder* c_XnaBuilder::m_new(c_TransCC* t_tcc){
	c_Builder::m_new(t_tcc);
	return this;
}
c_XnaBuilder* c_XnaBuilder::m_new2(){
	c_Builder::m_new2();
	return this;
}
bool c_XnaBuilder::p_IsValid(){
	String t_4=HostOS();
	if(t_4==String(L"winnt",5)){
		if((m_tcc->m_MSBUILD_PATH).Length()!=0){
			return true;
		}
	}
	return false;
}
void c_XnaBuilder::p_Begin(){
	bb_config_ENV_LANG=String(L"cs",2);
	gc_assign(bb_translator__trans,((new c_CsTranslator)->m_new()));
}
String c_XnaBuilder::p_Content(){
	c_StringStack* t_cont=(new c_StringStack)->m_new2();
	c_NodeEnumerator3* t_=m_dataFiles->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_Node2* t_kv=t_->p_NextObject();
		String t_p=t_kv->p_Key();
		String t_r=t_kv->p_Value();
		String t_f=bb_os_StripDir(t_r);
		String t_t=(String(L"cerberus/",9)+t_r).Replace(String(L"/",1),String(L"\\",1));
		String t_ext=bb_os_ExtractExt(t_r).ToLower();
		if(bb_transcc_MatchPath(t_r,m_TEXT_FILES)){
			t_cont->p_Push(String(L"  <ItemGroup>",13));
			t_cont->p_Push(String(L"    <Content Include=\"",22)+t_t+String(L"\">",2));
			t_cont->p_Push(String(L"      <Name>",12)+t_f+String(L"</Name>",7));
			t_cont->p_Push(String(L"      <CopyToOutputDirectory>Always</CopyToOutputDirectory>",59));
			t_cont->p_Push(String(L"    </Content>",14));
			t_cont->p_Push(String(L"  </ItemGroup>",14));
		}else{
			if(bb_transcc_MatchPath(t_r,m_IMAGE_FILES)){
				String t_1=t_ext;
				if(t_1==String(L"bmp",3) || t_1==String(L"dds",3) || t_1==String(L"dib",3) || t_1==String(L"hdr",3) || t_1==String(L"jpg",3) || t_1==String(L"pfm",3) || t_1==String(L"png",3) || t_1==String(L"ppm",3) || t_1==String(L"tga",3)){
					t_cont->p_Push(String(L"  <ItemGroup>",13));
					t_cont->p_Push(String(L"    <Compile Include=\"",22)+t_t+String(L"\">",2));
					t_cont->p_Push(String(L"      <Name>",12)+t_f+String(L"</Name>",7));
					t_cont->p_Push(String(L"      <Importer>TextureImporter</Importer>",42));
					t_cont->p_Push(String(L"      <Processor>TextureProcessor</Processor>",45));
					t_cont->p_Push(String(L"      <ProcessorParameters_ColorKeyEnabled>False</ProcessorParameters_ColorKeyEnabled>",86));
					t_cont->p_Push(String(L"      <ProcessorParameters_PremultiplyAlpha>False</ProcessorParameters_PremultiplyAlpha>",88));
					t_cont->p_Push(String(L"\t   </Compile>",14));
					t_cont->p_Push(String(L"  </ItemGroup>",14));
				}else{
					bb_transcc_Die(String(L"Invalid image file type",23));
				}
			}else{
				if(bb_transcc_MatchPath(t_r,m_SOUND_FILES)){
					String t_2=t_ext;
					if(t_2==String(L"wav",3) || t_2==String(L"mp3",3) || t_2==String(L"wma",3)){
						String t_imp=t_ext.Slice(0,1).ToUpper()+t_ext.Slice(1)+String(L"Importer",8);
						t_cont->p_Push(String(L"  <ItemGroup>",13));
						t_cont->p_Push(String(L"    <Compile Include=\"",22)+t_t+String(L"\">",2));
						t_cont->p_Push(String(L"      <Name>",12)+t_f+String(L"</Name>",7));
						t_cont->p_Push(String(L"      <Importer>",16)+t_imp+String(L"</Importer>",11));
						t_cont->p_Push(String(L"      <Processor>SoundEffectProcessor</Processor>",49));
						t_cont->p_Push(String(L"\t   </Compile>",14));
						t_cont->p_Push(String(L"  </ItemGroup>",14));
					}else{
						bb_transcc_Die(String(L"Invalid sound file type",23));
					}
				}else{
					if(bb_transcc_MatchPath(t_r,m_MUSIC_FILES)){
						String t_3=t_ext;
						if(t_3==String(L"wav",3) || t_3==String(L"mp3",3) || t_3==String(L"wma",3)){
							String t_imp2=t_ext.Slice(0,1).ToUpper()+t_ext.Slice(1)+String(L"Importer",8);
							t_cont->p_Push(String(L"  <ItemGroup>",13));
							t_cont->p_Push(String(L"    <Compile Include=\"",22)+t_t+String(L"\">",2));
							t_cont->p_Push(String(L"      <Name>",12)+t_f+String(L"</Name>",7));
							t_cont->p_Push(String(L"      <Importer>",16)+t_imp2+String(L"</Importer>",11));
							t_cont->p_Push(String(L"      <Processor>SongProcessor</Processor>",42));
							t_cont->p_Push(String(L"\t   </Compile>",14));
							t_cont->p_Push(String(L"  </ItemGroup>",14));
						}else{
							bb_transcc_Die(String(L"Invalid music file type",23));
						}
					}else{
						if(bb_transcc_MatchPath(t_r,m_BINARY_FILES)){
							t_cont->p_Push(String(L"  <ItemGroup>",13));
							t_cont->p_Push(String(L"    <Content Include=\"",22)+t_t+String(L"\">",2));
							t_cont->p_Push(String(L"      <Name>",12)+t_f+String(L"</Name>",7));
							t_cont->p_Push(String(L"      <CopyToOutputDirectory>Always</CopyToOutputDirectory>",59));
							t_cont->p_Push(String(L"    </Content>",14));
							t_cont->p_Push(String(L"  </ItemGroup>",14));
						}
					}
				}
			}
		}
	}
	return t_cont->p_Join(String(L"\n",1));
}
String c_XnaBuilder::p_Config(){
	c_StringStack* t_config=(new c_StringStack)->m_new2();
	c_NodeEnumerator3* t_=bb_config_GetConfigVars()->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_Node2* t_kv=t_->p_NextObject();
		t_config->p_Push(String(L"public const String ",20)+t_kv->p_Key()+String(L"=",1)+bb_config_Enquote(t_kv->p_Value(),String(L"cs",2))+String(L";",1));
	}
	return t_config->p_Join(String(L"\n",1));
}
void c_XnaBuilder::p_MakeTarget(){
	p_CreateDataDir(String(L"CerberusGame/CerberusGameContent/cerberus",41));
	String t_contproj=LoadString(String(L"CerberusGame/CerberusGameContent/CerberusGameContent.contentproj",64));
	t_contproj=bb_transcc_ReplaceBlock(t_contproj,String(L"CONTENT",7),p_Content(),String(L"\n<!-- ",6));
	SaveString(t_contproj,String(L"CerberusGame/CerberusGameContent/CerberusGameContent.contentproj",64));
	String t_main=LoadString(String(L"CerberusGame/CerberusGame/Program.cs",36));
	t_main=bb_transcc_ReplaceBlock(t_main,String(L"TRANSCODE",9),m_transCode,String(L"\n//",3));
	t_main=bb_transcc_ReplaceBlock(t_main,String(L"CONFIG",6),p_Config(),String(L"\n//",3));
	SaveString(t_main,String(L"CerberusGame/CerberusGame/Program.cs",36));
	if(m_tcc->m_opt_build){
		p_Execute(String(L"\"",1)+m_tcc->m_MSBUILD_PATH+String(L"\" /t:CerberusGame /p:Configuration=",35)+m_casedConfig+String(L" CerberusGame.sln",17),true);
		if(m_tcc->m_opt_run){
			ChangeDir(String(L"CerberusGame/CerberusGame/bin/x86/",34)+m_casedConfig);
			p_Execute(String(L"CerberusGame",12),false);
		}
	}
}
void c_XnaBuilder::mark(){
	c_Builder::mark();
}
c_AGKBuilder::c_AGKBuilder(){
}
c_AGKBuilder* c_AGKBuilder::m_new(c_TransCC* t_tcc){
	c_Builder::m_new(t_tcc);
	return this;
}
c_AGKBuilder* c_AGKBuilder::m_new2(){
	c_Builder::m_new2();
	return this;
}
bool c_AGKBuilder::p_IsValid(){
	String t_1=HostOS();
	if(t_1==String(L"winnt",5)){
		if(FileType(m_tcc->m_AGK_PATH+String(L"/Tier 1/Compiler/AGKBroadcaster.exe",35))==1 && ((m_tcc->m_MSBUILD_PATH).Length()!=0)){
			return true;
		}
	}else{
		if(t_1==String(L"macos",5)){
			if(FileType(m_tcc->m_AGK_PATH+String(L"/AppGameKit.app",15))==2){
				return true;
			}
		}else{
			return true;
		}
	}
	return false;
}
void c_AGKBuilder::p_Begin(){
	bb_config_ENV_LANG=String(L"cpp",3);
	gc_assign(bb_translator__trans,((new c_CppTranslator)->m_new()));
}
String c_AGKBuilder::p_Config(){
	c_StringStack* t_config=(new c_StringStack)->m_new2();
	int t_l=0;
	c_NodeEnumerator3* t_=bb_config_GetConfigVars()->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_Node2* t_kv=t_->p_NextObject();
		if(t_kv->p_Key().StartsWith(String(L"AGK_",4))){
			t_l=t_kv->p_Key().Length();
			t_config->p_Push(String(L"#define ",8)+t_kv->p_Key().Slice(4,t_l)+String(L" ",1)+t_kv->p_Value());
		}else{
			t_config->p_Push(String(L"#define CFG_",12)+t_kv->p_Key()+String(L" ",1)+t_kv->p_Value());
		}
	}
	t_config->p_Push(String(L"#define WINDOW_TITLE \"CerberusGame\"",35));
	return t_config->p_Join(String(L"\n",1));
}
void c_AGKBuilder::p_CreateMediaDir(String t_dir){
	t_dir=RealPath(t_dir);
	if(!m_syncData){
		bb_os_DeleteDir(t_dir,true);
	}
	CreateDir(t_dir);
	if(FileType(t_dir)!=2){
		bb_transcc_Die(String(L"Failed to create target project data dir: ",42)+t_dir);
	}
	String t_dataPath=bb_os_ExtractDir(bb_os_StripExt(m_tcc->m_opt_srcpath))+String(L"/media",6);
	if(FileType(t_dataPath)!=2){
		t_dataPath=String();
	}
	c_StringSet* t_udata=(new c_StringSet)->m_new();
	if((t_dataPath).Length()!=0){
		c_StringStack* t_srcs=(new c_StringStack)->m_new2();
		t_srcs->p_Push(t_dataPath);
		while(!t_srcs->p_IsEmpty()){
			String t_src=t_srcs->p_Pop();
			Array<String > t_=LoadDir(t_src);
			int t_2=0;
			while(t_2<t_.Length()){
				String t_f=t_[t_2];
				t_2=t_2+1;
				if(t_f.StartsWith(String(L".",1))){
					continue;
				}
				String t_p=t_src+String(L"/",1)+t_f;
				String t_r=t_p.Slice(t_dataPath.Length()+1);
				String t_t=t_dir+String(L"/",1)+t_r;
				int t_3=FileType(t_p);
				if(t_3==1){
					if(bb_transcc_MatchPath(t_r,m_DATA_FILES)){
						p_CCopyFile(t_p,t_t);
						t_udata->p_Insert(t_t);
						m_dataFiles->p_Set2(t_p,t_r);
					}
				}else{
					if(t_3==2){
						CreateDir(t_t);
						t_srcs->p_Push(t_p);
					}
				}
			}
		}
	}
	c_Enumerator* t_4=m_app->m_fileImports->p_ObjectEnumerator();
	while(t_4->p_HasNext()){
		String t_p2=t_4->p_NextObject();
		String t_r2=bb_os_StripDir(t_p2);
		String t_t2=t_dir+String(L"/",1)+t_r2;
		if(bb_transcc_MatchPath(t_r2,m_DATA_FILES)){
			p_CCopyFile(t_p2,t_t2);
			t_udata->p_Insert(t_t2);
			m_dataFiles->p_Set2(t_p2,t_r2);
		}
	}
	if((t_dataPath).Length()!=0){
		c_StringStack* t_dsts=(new c_StringStack)->m_new2();
		t_dsts->p_Push(t_dir);
		while(!t_dsts->p_IsEmpty()){
			String t_dst=t_dsts->p_Pop();
			Array<String > t_5=LoadDir(t_dst);
			int t_6=0;
			while(t_6<t_5.Length()){
				String t_f2=t_5[t_6];
				t_6=t_6+1;
				if(t_f2.StartsWith(String(L".",1))){
					continue;
				}
				String t_p3=t_dst+String(L"/",1)+t_f2;
				String t_r3=t_p3.Slice(t_dir.Length()+1);
				String t_t3=t_dataPath+String(L"/",1)+t_r3;
				int t_42=FileType(t_p3);
				if(t_42==1){
					if(!t_udata->p_Contains(t_p3)){
						DeleteFile(t_p3);
					}
				}else{
					if(t_42==2){
						if(FileType(t_t3)==2){
							t_dsts->p_Push(t_p3);
						}else{
							bb_os_DeleteDir(t_p3,true);
						}
					}
				}
			}
		}
	}
}
void c_AGKBuilder::p_MakeVc2017(){
	String t_buildpath=String();
	t_buildpath=CurrentDir()+String(L"\\AGKTemplate\\apps\\template_windows_vs2017_64",44);
	String t_template=LoadString(t_buildpath+String(L"\\template.cpp",13));
	String t_templateh=LoadString(t_buildpath+String(L"\\template.h",11));
	t_template=bb_transcc_ReplaceBlock(t_template,String(L"TRANSCODE",9),m_transCode,String(L"\n//",3));
	t_templateh=bb_transcc_ReplaceBlock(t_templateh,String(L"CONFIG",6),p_Config(),String(L"\n//",3));
	SaveString(t_template,t_buildpath+String(L"\\template.cpp",13));
	SaveString(t_templateh,t_buildpath+String(L"\\template.h",11));
	p_CreateMediaDir(t_buildpath+String(L"\\Final\\media",12));
	if(m_tcc->m_opt_build){
		p_Execute(String(L"\"",1)+m_tcc->m_MSBUILD_PATH+String(L"\" /p:Configuration=",19)+m_casedConfig+String(L" ",1)+t_buildpath+String(L"\\Template.sln",13),true);
		if(m_tcc->m_opt_run){
			ChangeDir(t_buildpath+String(L"\\Final",6));
			p_Execute(String(L"Template64",10),true);
		}
	}
}
void c_AGKBuilder::p_MakeXcode(){
	String t_buildpath=String();
	t_buildpath=CurrentDir()+String(L"/AGKTemplate/apps/template_mac",30);
	String t_template=LoadString(t_buildpath+String(L"/template.cpp",13));
	String t_templateh=LoadString(t_buildpath+String(L"/template.h",11));
	t_template=bb_transcc_ReplaceBlock(t_template,String(L"TRANSCODE",9),m_transCode,String(L"\n//",3));
	t_templateh=bb_transcc_ReplaceBlock(t_templateh,String(L"CONFIG",6),p_Config(),String(L"\n//",3));
	SaveString(t_template,t_buildpath+String(L"/template.cpp",13));
	SaveString(t_templateh,t_buildpath+String(L"/template.h",11));
	p_CreateMediaDir(t_buildpath+String(L"/media",6));
	if(m_tcc->m_opt_build){
		ChangeDir(t_buildpath);
		p_Execute(String(L"xcodebuild -configuration ",26)+m_casedConfig+String(L" PRODUCT_NAME=CerberusGame",26),true);
		if(m_tcc->m_opt_run){
			ChangeDir(t_buildpath+String(L"/build/",7)+m_casedConfig);
			ChangeDir(String(L"CerberusGame.app/Contents/MacOS",31));
			p_Execute(String(L"./CerberusGame",14),true);
		}
	}
}
void c_AGKBuilder::p_MakeTarget(){
	String t_2=HostOS();
	if(t_2==String(L"winnt",5)){
		p_MakeVc2017();
	}else{
		if(t_2==String(L"macos",5)){
			p_MakeXcode();
		}
	}
}
void c_AGKBuilder::mark(){
	c_Builder::mark();
}
c_AGKBuilder_ios::c_AGKBuilder_ios(){
}
c_AGKBuilder_ios* c_AGKBuilder_ios::m_new(c_TransCC* t_tcc){
	c_Builder::m_new(t_tcc);
	return this;
}
c_AGKBuilder_ios* c_AGKBuilder_ios::m_new2(){
	c_Builder::m_new2();
	return this;
}
bool c_AGKBuilder_ios::p_IsValid(){
	String t_1=HostOS();
	if(t_1==String(L"macos",5)){
		if(FileType(m_tcc->m_AGK_PATH+String(L"/AppGameKit.app",15))==2){
			return true;
		}
	}
	return false;
}
void c_AGKBuilder_ios::p_Begin(){
	bb_config_ENV_LANG=String(L"cpp",3);
	gc_assign(bb_translator__trans,((new c_CppTranslator)->m_new()));
}
String c_AGKBuilder_ios::p_Config(){
	c_StringStack* t_config=(new c_StringStack)->m_new2();
	int t_l=0;
	c_NodeEnumerator3* t_=bb_config_GetConfigVars()->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_Node2* t_kv=t_->p_NextObject();
		if(t_kv->p_Key().StartsWith(String(L"AGK_",4))){
			t_config->p_Push(String(L"#define ",8)+t_kv->p_Key()+String(L" ",1)+t_kv->p_Value());
		}else{
			t_config->p_Push(String(L"#define CFG_",12)+t_kv->p_Key()+String(L" ",1)+t_kv->p_Value());
		}
	}
	return t_config->p_Join(String(L"\n",1));
}
void c_AGKBuilder_ios::p_CreateMediaDir(String t_dir){
	t_dir=RealPath(t_dir);
	if(!m_syncData){
		bb_os_DeleteDir(t_dir,true);
	}
	CreateDir(t_dir);
	if(FileType(t_dir)!=2){
		bb_transcc_Die(String(L"Failed to create target project data dir: ",42)+t_dir);
	}
	String t_dataPath=bb_os_ExtractDir(bb_os_StripExt(m_tcc->m_opt_srcpath))+String(L"/media",6);
	if(FileType(t_dataPath)!=2){
		t_dataPath=String();
	}
	c_StringSet* t_udata=(new c_StringSet)->m_new();
	if((t_dataPath).Length()!=0){
		c_StringStack* t_srcs=(new c_StringStack)->m_new2();
		t_srcs->p_Push(t_dataPath);
		while(!t_srcs->p_IsEmpty()){
			String t_src=t_srcs->p_Pop();
			Array<String > t_=LoadDir(t_src);
			int t_2=0;
			while(t_2<t_.Length()){
				String t_f=t_[t_2];
				t_2=t_2+1;
				if(t_f.StartsWith(String(L".",1))){
					continue;
				}
				String t_p=t_src+String(L"/",1)+t_f;
				String t_r=t_p.Slice(t_dataPath.Length()+1);
				String t_t=t_dir+String(L"/",1)+t_r;
				int t_3=FileType(t_p);
				if(t_3==1){
					if(bb_transcc_MatchPath(t_r,m_DATA_FILES)){
						p_CCopyFile(t_p,t_t);
						t_udata->p_Insert(t_t);
						m_dataFiles->p_Set2(t_p,t_r);
					}
				}else{
					if(t_3==2){
						CreateDir(t_t);
						t_srcs->p_Push(t_p);
					}
				}
			}
		}
	}
	c_Enumerator* t_4=m_app->m_fileImports->p_ObjectEnumerator();
	while(t_4->p_HasNext()){
		String t_p2=t_4->p_NextObject();
		String t_r2=bb_os_StripDir(t_p2);
		String t_t2=t_dir+String(L"/",1)+t_r2;
		if(bb_transcc_MatchPath(t_r2,m_DATA_FILES)){
			p_CCopyFile(t_p2,t_t2);
			t_udata->p_Insert(t_t2);
			m_dataFiles->p_Set2(t_p2,t_r2);
		}
	}
	if((t_dataPath).Length()!=0){
		c_StringStack* t_dsts=(new c_StringStack)->m_new2();
		t_dsts->p_Push(t_dir);
		while(!t_dsts->p_IsEmpty()){
			String t_dst=t_dsts->p_Pop();
			Array<String > t_5=LoadDir(t_dst);
			int t_6=0;
			while(t_6<t_5.Length()){
				String t_f2=t_5[t_6];
				t_6=t_6+1;
				if(t_f2.StartsWith(String(L".",1))){
					continue;
				}
				String t_p3=t_dst+String(L"/",1)+t_f2;
				String t_r3=t_p3.Slice(t_dir.Length()+1);
				String t_t3=t_dataPath+String(L"/",1)+t_r3;
				int t_42=FileType(t_p3);
				if(t_42==1){
					if(!t_udata->p_Contains(t_p3)){
						DeleteFile(t_p3);
					}
				}else{
					if(t_42==2){
						if(FileType(t_t3)==2){
							t_dsts->p_Push(t_p3);
						}else{
							bb_os_DeleteDir(t_p3,true);
						}
					}
				}
			}
		}
	}
}
void c_AGKBuilder_ios::p_MakeXcode(){
	String t_sim_path=String(L"/Applications/Xcode.app/Contents/Developer/Applications/Simulator.app",69);
	if(m_tcc->m_opt_run==true){
		bbPrint(String(L"Starting iOS simulator...",25));
		p_Execute(String(L"open \"",6)+t_sim_path+String(L"\"",1),true);
	}
	String t_buildpath=String();
	t_buildpath=CurrentDir()+String(L"/AGKTemplate/apps/template_ios",30);
	bbPrint(String(L"buildpath= ",11)+t_buildpath);
	String t_template=LoadString(t_buildpath+String(L"/Classes/template.cpp",21));
	String t_templateh=LoadString(t_buildpath+String(L"/Classes/template.h",19));
	t_template=bb_transcc_ReplaceBlock(t_template,String(L"TRANSCODE",9),m_transCode,String(L"\n//",3));
	t_templateh=bb_transcc_ReplaceBlock(t_templateh,String(L"CONFIG",6),p_Config(),String(L"\n//",3));
	SaveString(t_template,t_buildpath+String(L"/Classes/template.cpp",21));
	SaveString(t_templateh,t_buildpath+String(L"/Classes/template.h",19));
	p_CreateMediaDir(t_buildpath+String(L"/media",6));
	if(m_tcc->m_opt_build){
		String t_src=t_buildpath+String(L"/build/",7)+m_casedConfig+String(L"/CerberusGame.app",17);
		ChangeDir(t_buildpath);
		p_Execute(String(L"xcodebuild -scheme agk_interpreter -configuration ",50)+m_casedConfig+String(L" -sdk iphonesimulator",21)+String(L" PRODUCT_NAME=CerberusGame",26)+String(L" BUILD_DIR=",11)+t_buildpath+String(L"/build",6),true);
		if(m_tcc->m_opt_run){
			bbPrint(String(L"Installing GerberusGame.app",27));
			p_Execute(String(L"xcrun simctl install booted \"",29)+t_src+String(L"\"",1),true);
			bbPrint(String(L"Running CerberusGame.app",24));
			p_Execute(String(L"xcrun simctl launch booted com.thegamecreators.AGKTemplate",58),true);
		}
	}
}
void c_AGKBuilder_ios::p_MakeTarget(){
	String t_2=HostOS();
	if(t_2==String(L"macos",5)){
		p_MakeXcode();
	}
}
void c_AGKBuilder_ios::mark(){
	c_Builder::mark();
}
c_StringMap3* bb_builders_Builders(c_TransCC* t_tcc){
	c_StringMap3* t_builders=(new c_StringMap3)->m_new();
	t_builders->p_Set3(String(L"android",7),((new c_AndroidBuilder)->m_new(t_tcc)));
	t_builders->p_Set3(String(L"android_ndk",11),((new c_AndroidNdkBuilder)->m_new(t_tcc)));
	t_builders->p_Set3(String(L"glfw",4),((new c_GlfwBuilder)->m_new(t_tcc)));
	t_builders->p_Set3(String(L"html5",5),((new c_Html5Builder)->m_new(t_tcc)));
	t_builders->p_Set3(String(L"ios",3),((new c_IosBuilder)->m_new(t_tcc)));
	t_builders->p_Set3(String(L"flash",5),((new c_FlashBuilder)->m_new(t_tcc)));
	t_builders->p_Set3(String(L"psm",3),((new c_PsmBuilder)->m_new(t_tcc)));
	t_builders->p_Set3(String(L"stdcpp",6),((new c_StdcppBuilder)->m_new(t_tcc)));
	t_builders->p_Set3(String(L"winrt",5),((new c_WinrtBuilder)->m_new(t_tcc)));
	t_builders->p_Set3(String(L"xna",3),((new c_XnaBuilder)->m_new(t_tcc)));
	t_builders->p_Set3(String(L"agk",3),((new c_AGKBuilder)->m_new(t_tcc)));
	t_builders->p_Set3(String(L"agk_ios",7),((new c_AGKBuilder_ios)->m_new(t_tcc)));
	return t_builders;
}
c_NodeEnumerator::c_NodeEnumerator(){
	m_node=0;
}
c_NodeEnumerator* c_NodeEnumerator::m_new(c_Node3* t_node){
	gc_assign(this->m_node,t_node);
	return this;
}
c_NodeEnumerator* c_NodeEnumerator::m_new2(){
	return this;
}
bool c_NodeEnumerator::p_HasNext(){
	return m_node!=0;
}
c_Node3* c_NodeEnumerator::p_NextObject(){
	c_Node3* t_t=m_node;
	gc_assign(m_node,m_node->p_NextNode());
	return t_t;
}
void c_NodeEnumerator::mark(){
	Object::mark();
	gc_mark_q(m_node);
}
c_List::c_List(){
	m__head=((new c_HeadNode)->m_new());
}
c_List* c_List::m_new(){
	return this;
}
c_Node4* c_List::p_AddLast(String t_data){
	return (new c_Node4)->m_new(m__head,m__head->m__pred,t_data);
}
c_List* c_List::m_new2(Array<String > t_data){
	Array<String > t_=t_data;
	int t_2=0;
	while(t_2<t_.Length()){
		String t_t=t_[t_2];
		t_2=t_2+1;
		p_AddLast(t_t);
	}
	return this;
}
bool c_List::p_IsEmpty(){
	return m__head->m__succ==m__head;
}
String c_List::p_RemoveFirst(){
	String t_data=m__head->m__succ->m__data;
	m__head->m__succ->p_Remove();
	return t_data;
}
bool c_List::p_Equals(String t_lhs,String t_rhs){
	return t_lhs==t_rhs;
}
c_Node4* c_List::p_Find(String t_value,c_Node4* t_start){
	while(t_start!=m__head){
		if(p_Equals(t_value,t_start->m__data)){
			return t_start;
		}
		t_start=t_start->m__succ;
	}
	return 0;
}
c_Node4* c_List::p_Find2(String t_value){
	return p_Find(t_value,m__head->m__succ);
}
void c_List::p_RemoveFirst2(String t_value){
	c_Node4* t_node=p_Find2(t_value);
	if((t_node)!=0){
		t_node->p_Remove();
	}
}
int c_List::p_Count(){
	int t_n=0;
	c_Node4* t_node=m__head->m__succ;
	while(t_node!=m__head){
		t_node=t_node->m__succ;
		t_n+=1;
	}
	return t_n;
}
c_Enumerator* c_List::p_ObjectEnumerator(){
	return (new c_Enumerator)->m_new(this);
}
Array<String > c_List::p_ToArray(){
	Array<String > t_arr=Array<String >(p_Count());
	int t_i=0;
	c_Enumerator* t_=this->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		String t_t=t_->p_NextObject();
		t_arr[t_i]=t_t;
		t_i+=1;
	}
	return t_arr;
}
String c_List::p_RemoveLast(){
	String t_data=m__head->m__pred->m__data;
	m__head->m__pred->p_Remove();
	return t_data;
}
c_Node4* c_List::p_FindLast(String t_value,c_Node4* t_start){
	while(t_start!=m__head){
		if(p_Equals(t_value,t_start->m__data)){
			return t_start;
		}
		t_start=t_start->m__pred;
	}
	return 0;
}
c_Node4* c_List::p_FindLast2(String t_value){
	return p_FindLast(t_value,m__head->m__pred);
}
void c_List::p_RemoveLast2(String t_value){
	c_Node4* t_node=p_FindLast2(t_value);
	if((t_node)!=0){
		t_node->p_Remove();
	}
}
void c_List::mark(){
	Object::mark();
	gc_mark_q(m__head);
}
c_StringList::c_StringList(){
}
c_StringList* c_StringList::m_new(Array<String > t_data){
	c_List::m_new2(t_data);
	return this;
}
c_StringList* c_StringList::m_new2(){
	c_List::m_new();
	return this;
}
bool c_StringList::p_Equals(String t_lhs,String t_rhs){
	return t_lhs==t_rhs;
}
void c_StringList::mark(){
	c_List::mark();
}
c_Node4::c_Node4(){
	m__succ=0;
	m__pred=0;
	m__data=String();
}
c_Node4* c_Node4::m_new(c_Node4* t_succ,c_Node4* t_pred,String t_data){
	gc_assign(m__succ,t_succ);
	gc_assign(m__pred,t_pred);
	gc_assign(m__succ->m__pred,this);
	gc_assign(m__pred->m__succ,this);
	m__data=t_data;
	return this;
}
c_Node4* c_Node4::m_new2(){
	return this;
}
int c_Node4::p_Remove(){
	gc_assign(m__succ->m__pred,m__pred);
	gc_assign(m__pred->m__succ,m__succ);
	return 0;
}
void c_Node4::mark(){
	Object::mark();
	gc_mark_q(m__succ);
	gc_mark_q(m__pred);
}
c_HeadNode::c_HeadNode(){
}
c_HeadNode* c_HeadNode::m_new(){
	c_Node4::m_new2();
	gc_assign(m__succ,(this));
	gc_assign(m__pred,(this));
	return this;
}
void c_HeadNode::mark(){
	c_Node4::mark();
}
c_Enumerator::c_Enumerator(){
	m__list=0;
	m__curr=0;
}
c_Enumerator* c_Enumerator::m_new(c_List* t_list){
	gc_assign(m__list,t_list);
	gc_assign(m__curr,t_list->m__head->m__succ);
	return this;
}
c_Enumerator* c_Enumerator::m_new2(){
	return this;
}
bool c_Enumerator::p_HasNext(){
	while(m__curr->m__succ->m__pred!=m__curr){
		gc_assign(m__curr,m__curr->m__succ);
	}
	return m__curr!=m__list->m__head;
}
String c_Enumerator::p_NextObject(){
	String t_data=m__curr->m__data;
	gc_assign(m__curr,m__curr->m__succ);
	return t_data;
}
void c_Enumerator::mark(){
	Object::mark();
	gc_mark_q(m__list);
	gc_mark_q(m__curr);
}
Array<String > bb_os_LoadDir(String t_path,bool t_recursive,bool t_hidden){
	c_StringList* t_dirs=(new c_StringList)->m_new2();
	c_StringList* t_files=(new c_StringList)->m_new2();
	t_dirs->p_AddLast(String());
	while(!t_dirs->p_IsEmpty()){
		String t_dir=t_dirs->p_RemoveFirst();
		Array<String > t_=LoadDir(t_path+String(L"/",1)+t_dir);
		int t_2=0;
		while(t_2<t_.Length()){
			String t_f=t_[t_2];
			t_2=t_2+1;
			if(!t_hidden && t_f.StartsWith(String(L".",1))){
				continue;
			}
			if((t_dir).Length()!=0){
				t_f=t_dir+String(L"/",1)+t_f;
			}
			int t_1=FileType(t_path+String(L"/",1)+t_f);
			if(t_1==1){
				t_files->p_AddLast(t_f);
			}else{
				if(t_1==2){
					if(t_recursive){
						t_dirs->p_AddLast(t_f);
					}else{
						t_files->p_AddLast(t_f);
					}
				}
			}
		}
	}
	return t_files->p_ToArray();
}
c_Stack2::c_Stack2(){
	m_data=Array<c_ConfigScope* >();
	m_length=0;
}
c_Stack2* c_Stack2::m_new(){
	return this;
}
c_Stack2* c_Stack2::m_new2(Array<c_ConfigScope* > t_data){
	gc_assign(this->m_data,t_data.Slice(0));
	this->m_length=t_data.Length();
	return this;
}
void c_Stack2::p_Push4(c_ConfigScope* t_value){
	if(m_length==m_data.Length()){
		gc_assign(m_data,m_data.Resize(m_length*2+10));
	}
	gc_assign(m_data[m_length],t_value);
	m_length+=1;
}
void c_Stack2::p_Push5(Array<c_ConfigScope* > t_values,int t_offset,int t_count){
	for(int t_i=0;t_i<t_count;t_i=t_i+1){
		p_Push4(t_values[t_offset+t_i]);
	}
}
void c_Stack2::p_Push6(Array<c_ConfigScope* > t_values,int t_offset){
	p_Push5(t_values,t_offset,t_values.Length()-t_offset);
}
c_ConfigScope* c_Stack2::m_NIL;
c_ConfigScope* c_Stack2::p_Pop(){
	m_length-=1;
	c_ConfigScope* t_v=m_data[m_length];
	gc_assign(m_data[m_length],m_NIL);
	return t_v;
}
void c_Stack2::mark(){
	Object::mark();
	gc_mark_q(m_data);
}
c_Stack2* bb_config__cfgScopeStack;
void bb_config_PushConfigScope(){
	bb_config__cfgScopeStack->p_Push4(bb_config__cfgScope);
	gc_assign(bb_config__cfgScope,(new c_ConfigScope)->m_new());
}
c_ModuleDecl::c_ModuleDecl(){
	m_rmodpath=String();
	m_filepath=String();
	m_modpath=String();
	m_imported=(new c_StringMap5)->m_new();
	m_friends=(new c_StringSet)->m_new();
	m_pubImported=(new c_StringMap5)->m_new();
}
c_ModuleDecl* c_ModuleDecl::m_new(String t_ident,int t_attrs,String t_munged,String t_modpath,String t_filepath,c_AppDecl* t_app){
	c_ScopeDecl::m_new();
	this->m_ident=t_ident;
	this->m_attrs=t_attrs;
	this->m_munged=t_munged;
	this->m_modpath=t_modpath;
	this->m_rmodpath=t_modpath;
	this->m_filepath=t_filepath;
	if(t_modpath.Contains(String(L".",1))){
		Array<String > t_bits=t_modpath.Split(String(L".",1));
		int t_n=t_bits.Length();
		if(t_n>1 && t_bits[t_n-2]==t_bits[t_n-1]){
			this->m_rmodpath=bb_os_StripExt(t_modpath);
		}
	}
	m_imported->p_Set5(t_filepath,this);
	t_app->p_InsertModule(this);
	return this;
}
c_ModuleDecl* c_ModuleDecl::m_new2(){
	c_ScopeDecl::m_new();
	return this;
}
int c_ModuleDecl::p_IsStrict(){
	return (((m_attrs&1)!=0)?1:0);
}
int c_ModuleDecl::p_ImportModule(String t_modpath,int t_attrs){
	String t_cdir=bb_os_ExtractDir(this->m_filepath);
	String t_dir=String();
	String t_filepath=String();
	String t_mpath=t_modpath.Replace(String(L".",1),String(L"/",1));
	Array<String > t_=bb_config_ENV_MODPATH.Split(String(L";",1));
	int t_2=0;
	while(t_2<t_.Length()){
		t_dir=t_[t_2];
		t_2=t_2+1;
		if(!((t_dir).Length()!=0)){
			continue;
		}
		if(t_dir==String(L".",1)){
			t_filepath=t_cdir+String(L"/",1)+t_mpath;
		}else{
			t_filepath=RealPath(t_dir)+String(L"/",1)+t_mpath;
		}
		String t_filepath2=bb_os_StripExt(t_filepath)+String(L"/",1)+bb_os_StripDir(t_filepath);
		String t_usedExtension=String();
		if(FileType(t_filepath+bb_parser_FILE_EXT)==1 || FileType(t_filepath2+bb_parser_FILE_EXT)==1){
			t_usedExtension=bb_parser_FILE_EXT;
		}else{
			if(FileType(t_filepath+bb_parser_FILE_EXT_OLD)==1 || FileType(t_filepath2+bb_parser_FILE_EXT_OLD)==1){
				t_usedExtension=bb_parser_FILE_EXT_OLD;
			}
		}
		t_filepath=t_filepath+t_usedExtension;
		t_filepath2=t_filepath2+t_usedExtension;
		if(FileType(t_filepath)==1){
			if(FileType(t_filepath2)!=1){
				break;
			}
			bb_config_Err(String(L"Duplicate module file: '",24)+t_filepath+String(L"' and '",7)+t_filepath2+String(L"'.",2));
		}else{
			if(FileType(t_filepath)==1){
				if(FileType(t_filepath2)!=1){
					break;
				}
				bb_config_Err(String(L"Duplicate module file: '",24)+t_filepath+String(L"' and '",7)+t_filepath2+String(L"'.",2));
			}
		}
		t_filepath=t_filepath2;
		if(FileType(t_filepath)==1){
			if(t_modpath.Contains(String(L".",1))){
				t_modpath=t_modpath+(String(L".",1)+bb_os_ExtractExt(t_modpath));
			}else{
				t_modpath=t_modpath+(String(L".",1)+t_modpath);
			}
			break;
		}
		t_filepath=String();
	}
	if(t_dir==String(L".",1) && this->m_modpath.Contains(String(L".",1))){
		t_modpath=bb_os_StripExt(this->m_modpath)+String(L".",1)+t_modpath;
	}
	c_AppDecl* t_app=dynamic_cast<c_AppDecl*>(m_scope);
	c_ModuleDecl* t_mdecl=t_app->m_imported->p_Get(t_filepath);
	if(((t_mdecl)!=0) && t_mdecl->m_modpath!=t_modpath){
		bbPrint(String(L"Modpath error - import=",23)+t_modpath+String(L", existing=",11)+t_mdecl->m_modpath);
	}
	if(this->m_imported->p_Contains(t_filepath)){
		return 0;
	}
	if(!((t_mdecl)!=0)){
		t_mdecl=bb_parser_ParseModule(t_modpath,t_filepath,t_app);
	}
	this->m_imported->p_Insert3(t_mdecl->m_filepath,t_mdecl);
	if(!((t_attrs&512)!=0)){
		this->m_pubImported->p_Insert3(t_mdecl->m_filepath,t_mdecl);
	}
	this->p_InsertDecl((new c_AliasDecl)->m_new(t_mdecl->m_ident,t_attrs,(t_mdecl)));
	return 0;
}
int c_ModuleDecl::p_SemantAll(){
	c_Enumerator2* t_=p_Decls()->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_Decl* t_decl=t_->p_NextObject();
		if((dynamic_cast<c_AliasDecl*>(t_decl))!=0){
			continue;
		}
		c_ClassDecl* t_cdecl=dynamic_cast<c_ClassDecl*>(t_decl);
		if((t_cdecl)!=0){
			if((t_cdecl->m_args).Length()!=0){
				c_Enumerator4* t_2=t_cdecl->m_instances->p_ObjectEnumerator();
				while(t_2->p_HasNext()){
					c_ClassDecl* t_inst=t_2->p_NextObject();
					c_Enumerator2* t_3=t_inst->p_Decls()->p_ObjectEnumerator();
					while(t_3->p_HasNext()){
						c_Decl* t_decl2=t_3->p_NextObject();
						if((dynamic_cast<c_AliasDecl*>(t_decl2))!=0){
							continue;
						}
						t_decl2->p_Semant();
					}
				}
			}else{
				t_decl->p_Semant();
				c_Enumerator2* t_4=t_cdecl->p_Decls()->p_ObjectEnumerator();
				while(t_4->p_HasNext()){
					c_Decl* t_decl3=t_4->p_NextObject();
					if((dynamic_cast<c_AliasDecl*>(t_decl3))!=0){
						continue;
					}
					t_decl3->p_Semant();
				}
			}
		}else{
			t_decl->p_Semant();
		}
	}
	m_attrs|=2;
	return 0;
}
String c_ModuleDecl::p_ToString(){
	return String(L"Module ",7)+m_modpath;
}
Object* c_ModuleDecl::p_GetDecl2(String t_ident){
	return c_ScopeDecl::p_GetDecl(t_ident);
}
Object* c_ModuleDecl::p_GetDecl(String t_ident){
	c_List9* t_todo=(new c_List9)->m_new();
	c_StringMap5* t_done=(new c_StringMap5)->m_new();
	t_todo->p_AddLast9(this);
	t_done->p_Insert3(m_filepath,this);
	Object* t_decl=0;
	String t_declmod=String();
	while(!t_todo->p_IsEmpty()){
		c_ModuleDecl* t_mdecl=t_todo->p_RemoveLast();
		Object* t_tdecl=t_mdecl->p_GetDecl2(t_ident);
		if(((t_tdecl)!=0) && ((bb_decl__env)!=0)){
			c_Decl* t_ddecl=dynamic_cast<c_Decl*>(t_tdecl);
			if(((t_ddecl)!=0) && !((t_ddecl->p_CheckAccess())!=0)){
				t_tdecl=0;
			}
			c_FuncDeclList* t_flist=dynamic_cast<c_FuncDeclList*>(t_tdecl);
			if((t_flist)!=0){
				bool t_pub=false;
				c_Enumerator3* t_=t_flist->p_ObjectEnumerator();
				while(t_->p_HasNext()){
					c_FuncDecl* t_fdecl=t_->p_NextObject();
					if(!((t_fdecl->p_CheckAccess())!=0)){
						continue;
					}
					t_pub=true;
					break;
				}
				if(!t_pub){
					t_tdecl=0;
				}
			}
		}
		if(((t_tdecl)!=0) && t_tdecl!=t_decl){
			if(t_mdecl==this){
				return t_tdecl;
			}
			if((t_decl)!=0){
				bb_config_Err(String(L"Duplicate identifier '",22)+t_ident+String(L"' found in module '",19)+t_declmod+String(L"' and module '",14)+t_mdecl->m_ident+String(L"'.",2));
			}
			t_decl=t_tdecl;
			t_declmod=t_mdecl->m_ident;
		}
		if(!((bb_decl__env)!=0)){
			break;
		}
		c_StringMap5* t_imps=t_mdecl->m_imported;
		if(t_mdecl!=bb_decl__env->p_ModuleScope()){
			t_imps=t_mdecl->m_pubImported;
		}
		c_ValueEnumerator* t_2=t_imps->p_Values()->p_ObjectEnumerator();
		while(t_2->p_HasNext()){
			c_ModuleDecl* t_mdecl2=t_2->p_NextObject();
			if(!t_done->p_Contains(t_mdecl2->m_filepath)){
				t_todo->p_AddLast9(t_mdecl2);
				t_done->p_Insert3(t_mdecl2->m_filepath,t_mdecl2);
			}
		}
	}
	return t_decl;
}
int c_ModuleDecl::p_OnSemant(){
	return 0;
}
void c_ModuleDecl::mark(){
	c_ScopeDecl::mark();
	gc_mark_q(m_imported);
	gc_mark_q(m_friends);
	gc_mark_q(m_pubImported);
}
c_ScopeDecl* bb_config_GetConfigScope(){
	return (bb_config__cfgScope);
}
c_ScopeDecl* bb_decl__env;
c_List2::c_List2(){
	m__head=((new c_HeadNode2)->m_new());
}
c_List2* c_List2::m_new(){
	return this;
}
c_Node5* c_List2::p_AddLast2(c_ScopeDecl* t_data){
	return (new c_Node5)->m_new(m__head,m__head->m__pred,t_data);
}
c_List2* c_List2::m_new2(Array<c_ScopeDecl* > t_data){
	Array<c_ScopeDecl* > t_=t_data;
	int t_2=0;
	while(t_2<t_.Length()){
		c_ScopeDecl* t_t=t_[t_2];
		t_2=t_2+1;
		p_AddLast2(t_t);
	}
	return this;
}
bool c_List2::p_IsEmpty(){
	return m__head->m__succ==m__head;
}
c_ScopeDecl* c_List2::p_RemoveLast(){
	c_ScopeDecl* t_data=m__head->m__pred->m__data;
	m__head->m__pred->p_Remove();
	return t_data;
}
bool c_List2::p_Equals2(c_ScopeDecl* t_lhs,c_ScopeDecl* t_rhs){
	return t_lhs==t_rhs;
}
c_Node5* c_List2::p_FindLast3(c_ScopeDecl* t_value,c_Node5* t_start){
	while(t_start!=m__head){
		if(p_Equals2(t_value,t_start->m__data)){
			return t_start;
		}
		t_start=t_start->m__pred;
	}
	return 0;
}
c_Node5* c_List2::p_FindLast4(c_ScopeDecl* t_value){
	return p_FindLast3(t_value,m__head->m__pred);
}
void c_List2::p_RemoveLast3(c_ScopeDecl* t_value){
	c_Node5* t_node=p_FindLast4(t_value);
	if((t_node)!=0){
		t_node->p_Remove();
	}
}
void c_List2::mark(){
	Object::mark();
	gc_mark_q(m__head);
}
c_Node5::c_Node5(){
	m__succ=0;
	m__pred=0;
	m__data=0;
}
c_Node5* c_Node5::m_new(c_Node5* t_succ,c_Node5* t_pred,c_ScopeDecl* t_data){
	gc_assign(m__succ,t_succ);
	gc_assign(m__pred,t_pred);
	gc_assign(m__succ->m__pred,this);
	gc_assign(m__pred->m__succ,this);
	gc_assign(m__data,t_data);
	return this;
}
c_Node5* c_Node5::m_new2(){
	return this;
}
int c_Node5::p_Remove(){
	gc_assign(m__succ->m__pred,m__pred);
	gc_assign(m__pred->m__succ,m__succ);
	return 0;
}
void c_Node5::mark(){
	Object::mark();
	gc_mark_q(m__succ);
	gc_mark_q(m__pred);
	gc_mark_q(m__data);
}
c_HeadNode2::c_HeadNode2(){
}
c_HeadNode2* c_HeadNode2::m_new(){
	c_Node5::m_new2();
	gc_assign(m__succ,(this));
	gc_assign(m__pred,(this));
	return this;
}
void c_HeadNode2::mark(){
	c_Node5::mark();
}
c_List2* bb_decl__envStack;
int bb_decl_PushEnv(c_ScopeDecl* t_env){
	bb_decl__envStack->p_AddLast2(bb_decl__env);
	gc_assign(bb_decl__env,t_env);
	return 0;
}
c_Toker::c_Toker(){
	m__path=String();
	m__line=0;
	m__source=String();
	m__length=0;
	m__toke=String();
	m__tokeType=0;
	m__tokePos=0;
}
c_StringSet* c_Toker::m__keywords;
c_StringSet* c_Toker::m__symbols;
int c_Toker::p__init(){
	if((m__keywords)!=0){
		return 0;
	}
	gc_assign(m__keywords,(new c_StringSet)->m_new());
	Array<String > t_=String(L"void strict public private protected friend property bool int float string array object mod continue exit include import module extern new self super eachin true false null not extends abstract final select case default const local global field method function class and or shl shr end if then else elseif endif while wend repeat until forever for to step next return interface implements inline alias try catch throw throwable enumerate",437).Split(String(L" ",1));
	int t_2=0;
	while(t_2<t_.Length()){
		String t_t=t_[t_2];
		t_2=t_2+1;
		m__keywords->p_Insert(t_t);
	}
	gc_assign(m__symbols,(new c_StringSet)->m_new());
	m__symbols->p_Insert(String(L"..",2));
	m__symbols->p_Insert(String(L":=",2));
	m__symbols->p_Insert(String(L"*=",2));
	m__symbols->p_Insert(String(L"/=",2));
	m__symbols->p_Insert(String(L"+=",2));
	m__symbols->p_Insert(String(L"-=",2));
	m__symbols->p_Insert(String(L"|=",2));
	m__symbols->p_Insert(String(L"&=",2));
	m__symbols->p_Insert(String(L"~=",2));
	return 0;
}
c_Toker* c_Toker::m_new(String t_path,String t_source){
	p__init();
	m__path=t_path;
	m__line=1;
	m__source=t_source;
	m__length=m__source.Length();
	m__toke=String();
	m__tokeType=0;
	m__tokePos=0;
	return this;
}
c_Toker* c_Toker::m_new2(c_Toker* t_toker){
	p__init();
	m__path=t_toker->m__path;
	m__line=t_toker->m__line;
	m__source=t_toker->m__source;
	m__length=m__source.Length();
	m__toke=t_toker->m__toke;
	m__tokeType=t_toker->m__tokeType;
	m__tokePos=t_toker->m__tokePos;
	return this;
}
c_Toker* c_Toker::m_new3(){
	return this;
}
int c_Toker::p_TCHR(int t_i){
	t_i+=m__tokePos;
	if(t_i<m__length){
		return (int)m__source[t_i];
	}
	return 0;
}
String c_Toker::p_TSTR(int t_i){
	t_i+=m__tokePos;
	if(t_i<m__length){
		return m__source.Slice(t_i,t_i+1);
	}
	return String();
}
String c_Toker::p_NextToke(){
	m__toke=String();
	if(m__tokePos==m__length){
		m__tokeType=0;
		return m__toke;
	}
	int t_chr=p_TCHR(0);
	String t_str=p_TSTR(0);
	int t_start=m__tokePos;
	m__tokePos+=1;
	if(t_str==String(L"\n",1)){
		m__tokeType=8;
		m__line+=1;
	}else{
		if((bb_config_IsSpace(t_chr))!=0){
			m__tokeType=1;
			while(m__tokePos<m__length && ((bb_config_IsSpace(p_TCHR(0)))!=0) && p_TSTR(0)!=String(L"\n",1)){
				m__tokePos+=1;
			}
		}else{
			if(t_str==String(L"_",1) || ((bb_config_IsAlpha(t_chr))!=0)){
				m__tokeType=2;
				while(m__tokePos<m__length){
					int t_chr2=(int)m__source[m__tokePos];
					if(t_chr2!=95 && !((bb_config_IsAlpha(t_chr2))!=0) && !((bb_config_IsDigit(t_chr2))!=0)){
						break;
					}
					m__tokePos+=1;
				}
				m__toke=m__source.Slice(t_start,m__tokePos);
				if(m__keywords->p_Contains(m__toke.ToLower())){
					m__tokeType=3;
				}
			}else{
				if(((bb_config_IsDigit(t_chr))!=0) || t_str==String(L".",1) && ((bb_config_IsDigit(p_TCHR(0)))!=0)){
					m__tokeType=4;
					if(t_str==String(L".",1)){
						m__tokeType=5;
					}
					while((bb_config_IsDigit(p_TCHR(0)))!=0){
						m__tokePos+=1;
					}
					if(m__tokeType==4 && p_TSTR(0)==String(L".",1) && ((bb_config_IsDigit(p_TCHR(1)))!=0)){
						m__tokeType=5;
						m__tokePos+=2;
						while((bb_config_IsDigit(p_TCHR(0)))!=0){
							m__tokePos+=1;
						}
					}
					if(p_TSTR(0).ToLower()==String(L"e",1)){
						m__tokeType=5;
						m__tokePos+=1;
						if(p_TSTR(0)==String(L"+",1) || p_TSTR(0)==String(L"-",1)){
							m__tokePos+=1;
						}
						while((bb_config_IsDigit(p_TCHR(0)))!=0){
							m__tokePos+=1;
						}
					}
				}else{
					if(t_str==String(L"%",1) && ((bb_config_IsBinDigit(p_TCHR(0)))!=0)){
						m__tokeType=4;
						m__tokePos+=1;
						while((bb_config_IsBinDigit(p_TCHR(0)))!=0){
							m__tokePos+=1;
						}
					}else{
						if(t_str==String(L"$",1) && ((bb_config_IsHexDigit(p_TCHR(0)))!=0)){
							m__tokeType=4;
							m__tokePos+=1;
							while((bb_config_IsHexDigit(p_TCHR(0)))!=0){
								m__tokePos+=1;
							}
						}else{
							if(t_str==String(L"\"",1)){
								m__tokeType=6;
								while(m__tokePos<m__length && p_TSTR(0)!=String(L"\"",1)){
									m__tokePos+=1;
								}
								if(m__tokePos<m__length){
									m__tokePos+=1;
								}else{
									m__tokeType=7;
								}
							}else{
								if(t_str==String(L"`",1)){
									m__tokeType=4;
									while(m__tokePos<m__length && p_TSTR(0)!=String(L"`",1)){
										m__tokePos+=1;
									}
									if(m__tokePos<m__length){
										m__tokePos+=1;
									}else{
										m__tokeType=10;
									}
								}else{
									if(t_str==String(L"'",1)){
										m__tokeType=9;
										while(m__tokePos<m__length && p_TSTR(0)!=String(L"\n",1)){
											m__tokePos+=1;
										}
										if(m__tokePos<m__length){
											m__tokePos+=1;
											m__line+=1;
										}
									}else{
										if(t_str==String(L"[",1)){
											m__tokeType=8;
											int t_i=0;
											while(m__tokePos+t_i<m__length){
												if(p_TSTR(t_i)==String(L"]",1)){
													m__tokePos+=t_i+1;
													break;
												}
												if(p_TSTR(t_i)==String(L"\n",1) || !((bb_config_IsSpace(p_TCHR(t_i)))!=0)){
													break;
												}
												t_i+=1;
											}
										}else{
											m__tokeType=8;
											if(m__symbols->p_Contains(m__source.Slice(m__tokePos-1,m__tokePos+1))){
												m__tokePos+=1;
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	if(!((m__toke).Length()!=0)){
		m__toke=m__source.Slice(t_start,m__tokePos);
	}
	return m__toke;
}
String c_Toker::p_Toke(){
	return m__toke;
}
int c_Toker::p_TokeType(){
	return m__tokeType;
}
String c_Toker::p_Path(){
	return m__path;
}
int c_Toker::p_Line(){
	return m__line;
}
int c_Toker::p_SkipSpace(){
	while(m__tokeType==1){
		p_NextToke();
	}
	return 0;
}
void c_Toker::mark(){
	Object::mark();
}
c_Set::c_Set(){
	m_map=0;
}
c_Set* c_Set::m_new(c_Map4* t_map){
	gc_assign(this->m_map,t_map);
	return this;
}
c_Set* c_Set::m_new2(){
	return this;
}
int c_Set::p_Insert(String t_value){
	m_map->p_Insert2(t_value,0);
	return 0;
}
bool c_Set::p_Contains(String t_value){
	return m_map->p_Contains(t_value);
}
void c_Set::mark(){
	Object::mark();
	gc_mark_q(m_map);
}
c_StringSet::c_StringSet(){
}
c_StringSet* c_StringSet::m_new(){
	c_Set::m_new((new c_StringMap4)->m_new());
	return this;
}
void c_StringSet::mark(){
	c_Set::mark();
}
c_Map4::c_Map4(){
	m_root=0;
}
c_Map4* c_Map4::m_new(){
	return this;
}
int c_Map4::p_RotateLeft4(c_Node6* t_node){
	c_Node6* t_child=t_node->m_right;
	gc_assign(t_node->m_right,t_child->m_left);
	if((t_child->m_left)!=0){
		gc_assign(t_child->m_left->m_parent,t_node);
	}
	gc_assign(t_child->m_parent,t_node->m_parent);
	if((t_node->m_parent)!=0){
		if(t_node==t_node->m_parent->m_left){
			gc_assign(t_node->m_parent->m_left,t_child);
		}else{
			gc_assign(t_node->m_parent->m_right,t_child);
		}
	}else{
		gc_assign(m_root,t_child);
	}
	gc_assign(t_child->m_left,t_node);
	gc_assign(t_node->m_parent,t_child);
	return 0;
}
int c_Map4::p_RotateRight4(c_Node6* t_node){
	c_Node6* t_child=t_node->m_left;
	gc_assign(t_node->m_left,t_child->m_right);
	if((t_child->m_right)!=0){
		gc_assign(t_child->m_right->m_parent,t_node);
	}
	gc_assign(t_child->m_parent,t_node->m_parent);
	if((t_node->m_parent)!=0){
		if(t_node==t_node->m_parent->m_right){
			gc_assign(t_node->m_parent->m_right,t_child);
		}else{
			gc_assign(t_node->m_parent->m_left,t_child);
		}
	}else{
		gc_assign(m_root,t_child);
	}
	gc_assign(t_child->m_right,t_node);
	gc_assign(t_node->m_parent,t_child);
	return 0;
}
int c_Map4::p_InsertFixup4(c_Node6* t_node){
	while(((t_node->m_parent)!=0) && t_node->m_parent->m_color==-1 && ((t_node->m_parent->m_parent)!=0)){
		if(t_node->m_parent==t_node->m_parent->m_parent->m_left){
			c_Node6* t_uncle=t_node->m_parent->m_parent->m_right;
			if(((t_uncle)!=0) && t_uncle->m_color==-1){
				t_node->m_parent->m_color=1;
				t_uncle->m_color=1;
				t_uncle->m_parent->m_color=-1;
				t_node=t_uncle->m_parent;
			}else{
				if(t_node==t_node->m_parent->m_right){
					t_node=t_node->m_parent;
					p_RotateLeft4(t_node);
				}
				t_node->m_parent->m_color=1;
				t_node->m_parent->m_parent->m_color=-1;
				p_RotateRight4(t_node->m_parent->m_parent);
			}
		}else{
			c_Node6* t_uncle2=t_node->m_parent->m_parent->m_left;
			if(((t_uncle2)!=0) && t_uncle2->m_color==-1){
				t_node->m_parent->m_color=1;
				t_uncle2->m_color=1;
				t_uncle2->m_parent->m_color=-1;
				t_node=t_uncle2->m_parent;
			}else{
				if(t_node==t_node->m_parent->m_left){
					t_node=t_node->m_parent;
					p_RotateRight4(t_node);
				}
				t_node->m_parent->m_color=1;
				t_node->m_parent->m_parent->m_color=-1;
				p_RotateLeft4(t_node->m_parent->m_parent);
			}
		}
	}
	m_root->m_color=1;
	return 0;
}
bool c_Map4::p_Set4(String t_key,Object* t_value){
	c_Node6* t_node=m_root;
	c_Node6* t_parent=0;
	int t_cmp=0;
	while((t_node)!=0){
		t_parent=t_node;
		t_cmp=p_Compare(t_key,t_node->m_key);
		if(t_cmp>0){
			t_node=t_node->m_right;
		}else{
			if(t_cmp<0){
				t_node=t_node->m_left;
			}else{
				gc_assign(t_node->m_value,t_value);
				return false;
			}
		}
	}
	t_node=(new c_Node6)->m_new(t_key,t_value,-1,t_parent);
	if((t_parent)!=0){
		if(t_cmp>0){
			gc_assign(t_parent->m_right,t_node);
		}else{
			gc_assign(t_parent->m_left,t_node);
		}
		p_InsertFixup4(t_node);
	}else{
		gc_assign(m_root,t_node);
	}
	return true;
}
bool c_Map4::p_Insert2(String t_key,Object* t_value){
	return p_Set4(t_key,t_value);
}
c_Node6* c_Map4::p_FindNode(String t_key){
	c_Node6* t_node=m_root;
	while((t_node)!=0){
		int t_cmp=p_Compare(t_key,t_node->m_key);
		if(t_cmp>0){
			t_node=t_node->m_right;
		}else{
			if(t_cmp<0){
				t_node=t_node->m_left;
			}else{
				return t_node;
			}
		}
	}
	return t_node;
}
bool c_Map4::p_Contains(String t_key){
	return p_FindNode(t_key)!=0;
}
Object* c_Map4::p_Get(String t_key){
	c_Node6* t_node=p_FindNode(t_key);
	if((t_node)!=0){
		return t_node->m_value;
	}
	return 0;
}
void c_Map4::mark(){
	Object::mark();
	gc_mark_q(m_root);
}
c_StringMap4::c_StringMap4(){
}
c_StringMap4* c_StringMap4::m_new(){
	c_Map4::m_new();
	return this;
}
int c_StringMap4::p_Compare(String t_lhs,String t_rhs){
	return t_lhs.Compare(t_rhs);
}
void c_StringMap4::mark(){
	c_Map4::mark();
}
c_Node6::c_Node6(){
	m_key=String();
	m_right=0;
	m_left=0;
	m_value=0;
	m_color=0;
	m_parent=0;
}
c_Node6* c_Node6::m_new(String t_key,Object* t_value,int t_color,c_Node6* t_parent){
	this->m_key=t_key;
	gc_assign(this->m_value,t_value);
	this->m_color=t_color;
	gc_assign(this->m_parent,t_parent);
	return this;
}
c_Node6* c_Node6::m_new2(){
	return this;
}
void c_Node6::mark(){
	Object::mark();
	gc_mark_q(m_right);
	gc_mark_q(m_left);
	gc_mark_q(m_value);
	gc_mark_q(m_parent);
}
int bb_config_IsSpace(int t_ch){
	return ((t_ch<=32)?1:0);
}
int bb_config_IsAlpha(int t_ch){
	return ((t_ch>=65 && t_ch<=90 || t_ch>=97 && t_ch<=122)?1:0);
}
int bb_config_IsDigit(int t_ch){
	return ((t_ch>=48 && t_ch<=57)?1:0);
}
int bb_config_IsBinDigit(int t_ch){
	return ((t_ch==48 || t_ch==49)?1:0);
}
int bb_config_IsHexDigit(int t_ch){
	return ((t_ch>=48 && t_ch<=57 || t_ch>=65 && t_ch<=70 || t_ch>=97 && t_ch<=102)?1:0);
}
String bb_config_ENV_MODPATH;
String bb_os_StripExt(String t_path){
	int t_i=t_path.FindLast(String(L".",1));
	if(t_i!=-1 && t_path.Find(String(L"/",1),t_i+1)==-1 && t_path.Find(String(L"\\",1),t_i+1)==-1){
		return t_path.Slice(0,t_i);
	}
	return t_path;
}
String bb_os_StripDir(String t_path){
	int t_i=t_path.FindLast(String(L"/",1));
	if(t_i==-1){
		t_i=t_path.FindLast(String(L"\\",1));
	}
	if(t_i!=-1){
		return t_path.Slice(t_i+1);
	}
	return t_path;
}
String bb_parser_FILE_EXT;
String bb_parser_FILE_EXT_OLD;
int bb_config_Err(String t_err){
	bbPrint(bb_config__errInfo+String(L" : Error : ",11)+t_err);
	ExitApp(-1);
	return 0;
}
String bb_os_ExtractExt(String t_path){
	int t_i=t_path.FindLast(String(L".",1));
	if(t_i!=-1 && t_path.Find(String(L"/",1),t_i+1)==-1 && t_path.Find(String(L"\\",1),t_i+1)==-1){
		return t_path.Slice(t_i+1);
	}
	return String();
}
c_AppDecl::c_AppDecl(){
	m_imported=(new c_StringMap5)->m_new();
	m_mainModule=0;
	m_fileImports=(new c_StringList)->m_new2();
	m_allSemantedDecls=(new c_List3)->m_new();
	m_semantedGlobals=(new c_List8)->m_new();
	m_semantedClasses=(new c_List6)->m_new();
	m_mainFunc=0;
}
int c_AppDecl::p_InsertModule(c_ModuleDecl* t_mdecl){
	gc_assign(t_mdecl->m_scope,(this));
	m_imported->p_Insert3(t_mdecl->m_filepath,t_mdecl);
	if(!((m_mainModule)!=0)){
		gc_assign(m_mainModule,t_mdecl);
	}
	return 0;
}
c_AppDecl* c_AppDecl::m_new(){
	c_ScopeDecl::m_new();
	return this;
}
int c_AppDecl::p_FinalizeClasses(){
	bb_decl__env=0;
	do{
		int t_more=0;
		c_Enumerator4* t_=m_semantedClasses->p_ObjectEnumerator();
		while(t_->p_HasNext()){
			c_ClassDecl* t_cdecl=t_->p_NextObject();
			t_more+=t_cdecl->p_UpdateLiveMethods();
		}
		if(!((t_more)!=0)){
			break;
		}
	}while(!(false));
	c_Enumerator4* t_2=m_semantedClasses->p_ObjectEnumerator();
	while(t_2->p_HasNext()){
		c_ClassDecl* t_cdecl2=t_2->p_NextObject();
		t_cdecl2->p_FinalizeClass();
	}
	return 0;
}
int c_AppDecl::p_OnSemant(){
	bb_decl__env=0;
	gc_assign(m_mainFunc,m_mainModule->p_FindFuncDecl(String(L"Main",4),Array<c_Expr* >(),0));
	if(!((m_mainFunc)!=0)){
		bb_config_Err(String(L"Function 'Main' not found.",26));
	}
	if(!((dynamic_cast<c_IntType*>(m_mainFunc->m_retType))!=0) || ((m_mainFunc->m_argDecls.Length())!=0)){
		bb_config_Err(String(L"Main function must be of type Main:Int()",40));
	}
	p_FinalizeClasses();
	return 0;
}
void c_AppDecl::mark(){
	c_ScopeDecl::mark();
	gc_mark_q(m_imported);
	gc_mark_q(m_mainModule);
	gc_mark_q(m_fileImports);
	gc_mark_q(m_allSemantedDecls);
	gc_mark_q(m_semantedGlobals);
	gc_mark_q(m_semantedClasses);
	gc_mark_q(m_mainFunc);
}
c_Map5::c_Map5(){
	m_root=0;
}
c_Map5* c_Map5::m_new(){
	return this;
}
c_Node7* c_Map5::p_FindNode(String t_key){
	c_Node7* t_node=m_root;
	while((t_node)!=0){
		int t_cmp=p_Compare(t_key,t_node->m_key);
		if(t_cmp>0){
			t_node=t_node->m_right;
		}else{
			if(t_cmp<0){
				t_node=t_node->m_left;
			}else{
				return t_node;
			}
		}
	}
	return t_node;
}
c_ModuleDecl* c_Map5::p_Get(String t_key){
	c_Node7* t_node=p_FindNode(t_key);
	if((t_node)!=0){
		return t_node->m_value;
	}
	return 0;
}
bool c_Map5::p_Contains(String t_key){
	return p_FindNode(t_key)!=0;
}
int c_Map5::p_RotateLeft5(c_Node7* t_node){
	c_Node7* t_child=t_node->m_right;
	gc_assign(t_node->m_right,t_child->m_left);
	if((t_child->m_left)!=0){
		gc_assign(t_child->m_left->m_parent,t_node);
	}
	gc_assign(t_child->m_parent,t_node->m_parent);
	if((t_node->m_parent)!=0){
		if(t_node==t_node->m_parent->m_left){
			gc_assign(t_node->m_parent->m_left,t_child);
		}else{
			gc_assign(t_node->m_parent->m_right,t_child);
		}
	}else{
		gc_assign(m_root,t_child);
	}
	gc_assign(t_child->m_left,t_node);
	gc_assign(t_node->m_parent,t_child);
	return 0;
}
int c_Map5::p_RotateRight5(c_Node7* t_node){
	c_Node7* t_child=t_node->m_left;
	gc_assign(t_node->m_left,t_child->m_right);
	if((t_child->m_right)!=0){
		gc_assign(t_child->m_right->m_parent,t_node);
	}
	gc_assign(t_child->m_parent,t_node->m_parent);
	if((t_node->m_parent)!=0){
		if(t_node==t_node->m_parent->m_right){
			gc_assign(t_node->m_parent->m_right,t_child);
		}else{
			gc_assign(t_node->m_parent->m_left,t_child);
		}
	}else{
		gc_assign(m_root,t_child);
	}
	gc_assign(t_child->m_right,t_node);
	gc_assign(t_node->m_parent,t_child);
	return 0;
}
int c_Map5::p_InsertFixup5(c_Node7* t_node){
	while(((t_node->m_parent)!=0) && t_node->m_parent->m_color==-1 && ((t_node->m_parent->m_parent)!=0)){
		if(t_node->m_parent==t_node->m_parent->m_parent->m_left){
			c_Node7* t_uncle=t_node->m_parent->m_parent->m_right;
			if(((t_uncle)!=0) && t_uncle->m_color==-1){
				t_node->m_parent->m_color=1;
				t_uncle->m_color=1;
				t_uncle->m_parent->m_color=-1;
				t_node=t_uncle->m_parent;
			}else{
				if(t_node==t_node->m_parent->m_right){
					t_node=t_node->m_parent;
					p_RotateLeft5(t_node);
				}
				t_node->m_parent->m_color=1;
				t_node->m_parent->m_parent->m_color=-1;
				p_RotateRight5(t_node->m_parent->m_parent);
			}
		}else{
			c_Node7* t_uncle2=t_node->m_parent->m_parent->m_left;
			if(((t_uncle2)!=0) && t_uncle2->m_color==-1){
				t_node->m_parent->m_color=1;
				t_uncle2->m_color=1;
				t_uncle2->m_parent->m_color=-1;
				t_node=t_uncle2->m_parent;
			}else{
				if(t_node==t_node->m_parent->m_left){
					t_node=t_node->m_parent;
					p_RotateRight5(t_node);
				}
				t_node->m_parent->m_color=1;
				t_node->m_parent->m_parent->m_color=-1;
				p_RotateLeft5(t_node->m_parent->m_parent);
			}
		}
	}
	m_root->m_color=1;
	return 0;
}
bool c_Map5::p_Set5(String t_key,c_ModuleDecl* t_value){
	c_Node7* t_node=m_root;
	c_Node7* t_parent=0;
	int t_cmp=0;
	while((t_node)!=0){
		t_parent=t_node;
		t_cmp=p_Compare(t_key,t_node->m_key);
		if(t_cmp>0){
			t_node=t_node->m_right;
		}else{
			if(t_cmp<0){
				t_node=t_node->m_left;
			}else{
				gc_assign(t_node->m_value,t_value);
				return false;
			}
		}
	}
	t_node=(new c_Node7)->m_new(t_key,t_value,-1,t_parent);
	if((t_parent)!=0){
		if(t_cmp>0){
			gc_assign(t_parent->m_right,t_node);
		}else{
			gc_assign(t_parent->m_left,t_node);
		}
		p_InsertFixup5(t_node);
	}else{
		gc_assign(m_root,t_node);
	}
	return true;
}
bool c_Map5::p_Insert3(String t_key,c_ModuleDecl* t_value){
	return p_Set5(t_key,t_value);
}
c_MapValues* c_Map5::p_Values(){
	return (new c_MapValues)->m_new(this);
}
c_Node7* c_Map5::p_FirstNode(){
	if(!((m_root)!=0)){
		return 0;
	}
	c_Node7* t_node=m_root;
	while((t_node->m_left)!=0){
		t_node=t_node->m_left;
	}
	return t_node;
}
void c_Map5::mark(){
	Object::mark();
	gc_mark_q(m_root);
}
c_StringMap5::c_StringMap5(){
}
c_StringMap5* c_StringMap5::m_new(){
	c_Map5::m_new();
	return this;
}
int c_StringMap5::p_Compare(String t_lhs,String t_rhs){
	return t_lhs.Compare(t_rhs);
}
void c_StringMap5::mark(){
	c_Map5::mark();
}
c_Node7::c_Node7(){
	m_key=String();
	m_right=0;
	m_left=0;
	m_value=0;
	m_color=0;
	m_parent=0;
}
c_Node7* c_Node7::m_new(String t_key,c_ModuleDecl* t_value,int t_color,c_Node7* t_parent){
	this->m_key=t_key;
	gc_assign(this->m_value,t_value);
	this->m_color=t_color;
	gc_assign(this->m_parent,t_parent);
	return this;
}
c_Node7* c_Node7::m_new2(){
	return this;
}
c_Node7* c_Node7::p_NextNode(){
	c_Node7* t_node=0;
	if((m_right)!=0){
		t_node=m_right;
		while((t_node->m_left)!=0){
			t_node=t_node->m_left;
		}
		return t_node;
	}
	t_node=this;
	c_Node7* t_parent=this->m_parent;
	while(((t_parent)!=0) && t_node==t_parent->m_right){
		t_node=t_parent;
		t_parent=t_parent->m_parent;
	}
	return t_parent;
}
void c_Node7::mark(){
	Object::mark();
	gc_mark_q(m_right);
	gc_mark_q(m_left);
	gc_mark_q(m_value);
	gc_mark_q(m_parent);
}
c_Parser::c_Parser(){
	m__toke=String();
	m__toker=0;
	m__app=0;
	m__module=0;
	m__defattrs=0;
	m__tokeType=0;
	m__block=0;
	m__blockStack=(new c_List7)->m_new();
	m__errStack=(new c_StringList)->m_new2();
	m__selTmpId=0;
}
int c_Parser::p_SetErr(){
	if((m__toker->p_Path()).Length()!=0){
		bb_config__errInfo=m__toker->p_Path()+String(L"<",1)+String(m__toker->p_Line())+String(L">",1);
	}
	return 0;
}
int c_Parser::p_CParse(String t_toke){
	if(m__toke!=t_toke){
		return 0;
	}
	p_NextToke();
	return 1;
}
int c_Parser::p_SkipEols(){
	while((p_CParse(String(L"\n",1)))!=0){
	}
	p_SetErr();
	return 0;
}
String c_Parser::p_NextToke(){
	String t_toke=m__toke;
	do{
		m__toke=m__toker->p_NextToke();
		m__tokeType=m__toker->p_TokeType();
	}while(!(m__tokeType!=1));
	int t_2=m__tokeType;
	if(t_2==3){
		m__toke=m__toke.ToLower();
	}else{
		if(t_2==8){
			if((int)m__toke[0]==91 && (int)m__toke[m__toke.Length()-1]==93){
				m__toke=String(L"[]",2);
			}
		}
	}
	if(t_toke==String(L",",1)){
		p_SkipEols();
	}
	return m__toke;
}
c_Parser* c_Parser::m_new(c_Toker* t_toker,c_AppDecl* t_app,c_ModuleDecl* t_mdecl,int t_defattrs){
	m__toke=String(L"\n",1);
	gc_assign(m__toker,t_toker);
	gc_assign(m__app,t_app);
	gc_assign(m__module,t_mdecl);
	m__defattrs=t_defattrs;
	p_SetErr();
	p_NextToke();
	return this;
}
c_Parser* c_Parser::m_new2(){
	return this;
}
String c_Parser::p_ParseStringLit(){
	if(m__tokeType!=6){
		bb_config_Err(String(L"Expecting string literal.",25));
	}
	String t_str=bb_config_Dequote(m__toke,String(L"cerberus",8));
	p_NextToke();
	return t_str;
}
String c_Parser::p_RealPath(String t_path){
	String t_popDir=CurrentDir();
	ChangeDir(bb_os_ExtractDir(m__toker->p_Path()));
	t_path=RealPath(t_path);
	ChangeDir(t_popDir);
	return t_path;
}
int c_Parser::p_ImportFile(String t_filepath){
	if((bb_config_ENV_SAFEMODE)!=0){
		if(m__app->m_mainModule==m__module){
			bb_config_Err(String(L"Import of external files not permitted in safe mode.",52));
		}
	}
	t_filepath=p_RealPath(t_filepath);
	if(FileType(t_filepath)!=1){
		bb_config_Err(String(L"File '",6)+t_filepath+String(L"' not found.",12));
	}
	m__app->m_fileImports->p_AddLast(t_filepath);
	return 0;
}
String c_Parser::p_ParseIdent(){
	String t_3=m__toke;
	if(t_3==String(L"@",1)){
		p_NextToke();
	}else{
		if(t_3==String(L"object",6) || t_3==String(L"throwable",9)){
		}else{
			if(m__tokeType!=2){
				bb_config_Err(String(L"Syntax error - expecting identifier.",36));
			}
		}
	}
	String t_id=m__toke;
	p_NextToke();
	return t_id;
}
String c_Parser::p_ParseModPath(){
	String t_path=p_ParseIdent();
	while((p_CParse(String(L".",1)))!=0){
		t_path=t_path+(String(L".",1)+p_ParseIdent());
	}
	return t_path;
}
int c_Parser::p_ImportModule(String t_modpath,int t_attrs){
	return 0;
}
int c_Parser::p_Parse(String t_toke){
	if(!((p_CParse(t_toke))!=0)){
		bb_config_Err(String(L"Syntax error - expecting '",26)+t_toke+String(L"'.",2));
	}
	return 0;
}
c_Type* c_Parser::p_CParsePrimitiveType(){
	if((p_CParse(String(L"void",4)))!=0){
		return (c_Type::m_voidType);
	}
	if((p_CParse(String(L"bool",4)))!=0){
		return (c_Type::m_boolType);
	}
	if((p_CParse(String(L"int",3)))!=0){
		return (c_Type::m_intType);
	}
	if((p_CParse(String(L"float",5)))!=0){
		return (c_Type::m_floatType);
	}
	if((p_CParse(String(L"string",6)))!=0){
		return (c_Type::m_stringType);
	}
	if((p_CParse(String(L"object",6)))!=0){
		return (c_Type::m_objectType);
	}
	if((p_CParse(String(L"throwable",9)))!=0){
		return (c_Type::m_throwableType);
	}
	return 0;
}
c_IdentType* c_Parser::p_ParseIdentType(){
	String t_id=p_ParseIdent();
	if((p_CParse(String(L".",1)))!=0){
		t_id=t_id+(String(L".",1)+p_ParseIdent());
	}
	c_Stack3* t_args=(new c_Stack3)->m_new();
	if((p_CParse(String(L"<",1)))!=0){
		do{
			c_Type* t_arg=p_ParseType();
			while((p_CParse(String(L"[]",2)))!=0){
				t_arg=(t_arg->p_ArrayOf());
			}
			t_args->p_Push7(t_arg);
		}while(!(!((p_CParse(String(L",",1)))!=0)));
		p_Parse(String(L">",1));
	}
	return (new c_IdentType)->m_new(t_id,t_args->p_ToArray());
}
c_Type* c_Parser::p_ParseType(){
	c_Type* t_ty=p_CParsePrimitiveType();
	if((t_ty)!=0){
		return t_ty;
	}
	return (p_ParseIdentType());
}
c_Type* c_Parser::p_ParseDeclType(){
	c_Type* t_ty=0;
	String t_4=m__toke;
	if(t_4==String(L"?",1)){
		p_NextToke();
		t_ty=(c_Type::m_boolType);
	}else{
		if(t_4==String(L"%",1)){
			p_NextToke();
			t_ty=(c_Type::m_intType);
		}else{
			if(t_4==String(L"#",1)){
				p_NextToke();
				t_ty=(c_Type::m_floatType);
			}else{
				if(t_4==String(L"$",1)){
					p_NextToke();
					t_ty=(c_Type::m_stringType);
				}else{
					if(t_4==String(L":",1)){
						p_NextToke();
						t_ty=p_ParseType();
					}else{
						if((m__module->p_IsStrict())!=0){
							bb_config_Err(String(L"Illegal type expression.",24));
						}
						t_ty=(c_Type::m_intType);
					}
				}
			}
		}
	}
	while((p_CParse(String(L"[]",2)))!=0){
		t_ty=(t_ty->p_ArrayOf());
	}
	return t_ty;
}
c_ArrayExpr* c_Parser::p_ParseArrayExpr(){
	p_Parse(String(L"[",1));
	c_Stack4* t_args=(new c_Stack4)->m_new();
	do{
		t_args->p_Push10(p_ParseExpr());
	}while(!(!((p_CParse(String(L",",1)))!=0)));
	p_Parse(String(L"]",1));
	return (new c_ArrayExpr)->m_new(t_args->p_ToArray());
}
int c_Parser::p_AtEos(){
	return ((m__toke==String() || m__toke==String(L";",1) || m__toke==String(L"\n",1) || m__toke==String(L"else",4))?1:0);
}
Array<c_Expr* > c_Parser::p_ParseArgs2(int t_stmt){
	Array<c_Expr* > t_args=Array<c_Expr* >();
	if((t_stmt)!=0){
		if((p_AtEos())!=0){
			return t_args;
		}
	}else{
		if(m__toke!=String(L"(",1)){
			return t_args;
		}
	}
	int t_nargs=0;
	int t_eat=0;
	if(m__toke==String(L"(",1)){
		if((t_stmt)!=0){
			c_Toker* t_toker=(new c_Toker)->m_new2(m__toker);
			int t_bra=1;
			do{
				t_toker->p_NextToke();
				t_toker->p_SkipSpace();
				String t_5=t_toker->p_Toke().ToLower();
				if(t_5==String() || t_5==String(L"else",4)){
					bb_config_Err(String(L"Parenthesis mismatch error.",27));
				}else{
					if(t_5==String(L"(",1) || t_5==String(L"[",1)){
						t_bra+=1;
					}else{
						if(t_5==String(L"]",1) || t_5==String(L")",1)){
							t_bra-=1;
							if((t_bra)!=0){
								continue;
							}
							t_toker->p_NextToke();
							t_toker->p_SkipSpace();
							String t_6=t_toker->p_Toke().ToLower();
							if(t_6==String(L".",1) || t_6==String(L"(",1) || t_6==String(L"[",1) || t_6==String() || t_6==String(L";",1) || t_6==String(L"\n",1) || t_6==String(L"else",4)){
								t_eat=1;
							}
							break;
						}else{
							if(t_5==String(L",",1)){
								if(t_bra!=1){
									continue;
								}
								t_eat=1;
								break;
							}
						}
					}
				}
			}while(!(false));
		}else{
			t_eat=1;
		}
		if(((t_eat)!=0) && p_NextToke()==String(L")",1)){
			p_NextToke();
			return t_args;
		}
	}
	do{
		c_Expr* t_arg=0;
		if(((m__toke).Length()!=0) && m__toke!=String(L",",1)){
			t_arg=p_ParseExpr();
		}
		if(t_args.Length()==t_nargs){
			t_args=t_args.Resize(t_nargs+10);
		}
		gc_assign(t_args[t_nargs],t_arg);
		t_nargs+=1;
	}while(!(!((p_CParse(String(L",",1)))!=0)));
	t_args=t_args.Slice(0,t_nargs);
	if((t_eat)!=0){
		p_Parse(String(L")",1));
	}
	return t_args;
}
c_IdentType* c_Parser::p_CParseIdentType(bool t_inner){
	if(m__tokeType!=2){
		return 0;
	}
	String t_id=p_ParseIdent();
	if((p_CParse(String(L".",1)))!=0){
		if(m__tokeType!=2){
			return 0;
		}
		t_id=t_id+(String(L".",1)+p_ParseIdent());
	}
	if(!((p_CParse(String(L"<",1)))!=0)){
		if(t_inner){
			return (new c_IdentType)->m_new(t_id,Array<c_Type* >());
		}
		return 0;
	}
	c_Stack3* t_args=(new c_Stack3)->m_new();
	do{
		c_Type* t_arg=p_CParsePrimitiveType();
		if(!((t_arg)!=0)){
			t_arg=(p_CParseIdentType(true));
			if(!((t_arg)!=0)){
				return 0;
			}
		}
		while((p_CParse(String(L"[]",2)))!=0){
			t_arg=(t_arg->p_ArrayOf());
		}
		t_args->p_Push7(t_arg);
	}while(!(!((p_CParse(String(L",",1)))!=0)));
	if(!((p_CParse(String(L">",1)))!=0)){
		return 0;
	}
	return (new c_IdentType)->m_new(t_id,t_args->p_ToArray());
}
c_Expr* c_Parser::p_ParsePrimaryExpr(int t_stmt){
	c_Expr* t_expr=0;
	String t_7=m__toke;
	if(t_7==String(L"(",1)){
		p_NextToke();
		t_expr=p_ParseExpr();
		p_Parse(String(L")",1));
	}else{
		if(t_7==String(L"[",1)){
			t_expr=(p_ParseArrayExpr());
		}else{
			if(t_7==String(L"[]",2)){
				p_NextToke();
				t_expr=((new c_ConstExpr)->m_new((c_Type::m_emptyArrayType),String()));
			}else{
				if(t_7==String(L".",1)){
					t_expr=((new c_ScopeExpr)->m_new(m__module));
				}else{
					if(t_7==String(L"new",3)){
						p_NextToke();
						c_Type* t_ty=p_ParseType();
						if((p_CParse(String(L"[",1)))!=0){
							c_Expr* t_len=p_ParseExpr();
							p_Parse(String(L"]",1));
							while((p_CParse(String(L"[]",2)))!=0){
								t_ty=(t_ty->p_ArrayOf());
							}
							t_expr=((new c_NewArrayExpr)->m_new(t_ty,t_len));
						}else{
							t_expr=((new c_NewObjectExpr)->m_new(t_ty,p_ParseArgs2(t_stmt)));
						}
					}else{
						if(t_7==String(L"null",4)){
							p_NextToke();
							t_expr=((new c_ConstExpr)->m_new((c_Type::m_nullObjectType),String()));
						}else{
							if(t_7==String(L"true",4)){
								p_NextToke();
								t_expr=((new c_ConstExpr)->m_new((c_Type::m_boolType),String(L"1",1)));
							}else{
								if(t_7==String(L"false",5)){
									p_NextToke();
									t_expr=((new c_ConstExpr)->m_new((c_Type::m_boolType),String()));
								}else{
									if(t_7==String(L"bool",4) || t_7==String(L"int",3) || t_7==String(L"float",5) || t_7==String(L"string",6) || t_7==String(L"object",6) || t_7==String(L"throwable",9)){
										String t_id=m__toke;
										c_Type* t_ty2=p_ParseType();
										if(((p_CParse(String(L"(",1)))!=0)){
											t_expr=p_ParseExpr();
											p_Parse(String(L")",1));
											t_expr=((new c_CastExpr)->m_new(t_ty2,t_expr,1));
										}else{
											t_expr=((new c_IdentExpr)->m_new(t_id,0));
										}
									}else{
										if(t_7==String(L"self",4)){
											p_NextToke();
											t_expr=((new c_SelfExpr)->m_new());
										}else{
											if(t_7==String(L"super",5)){
												p_NextToke();
												p_Parse(String(L".",1));
												p_SkipEols();
												if(m__toke==String(L"new",3)){
													p_NextToke();
													c_FuncDecl* t_func=dynamic_cast<c_FuncDecl*>(m__block);
													if(!((t_func)!=0) || !((t_stmt)!=0) || !t_func->p_IsCtor() || !t_func->m_stmts->p_IsEmpty()){
														bb_config_Err(String(L"Call to Super.new must be first statement in a constructor.",59));
													}
													t_expr=((new c_InvokeSuperExpr)->m_new(String(L"new",3),p_ParseArgs2(t_stmt)));
													t_func->m_attrs|=8;
												}else{
													String t_id2=p_ParseIdent();
													t_expr=((new c_InvokeSuperExpr)->m_new(t_id2,p_ParseArgs2(t_stmt)));
												}
											}else{
												int t_8=m__tokeType;
												if(t_8==2){
													c_Toker* t_toker=(new c_Toker)->m_new2(m__toker);
													c_IdentType* t_ty3=p_CParseIdentType(false);
													if((t_ty3)!=0){
														t_expr=((new c_IdentTypeExpr)->m_new(t_ty3));
													}else{
														gc_assign(m__toker,t_toker);
														m__toke=m__toker->p_Toke();
														m__tokeType=m__toker->p_TokeType();
														t_expr=((new c_IdentExpr)->m_new(p_ParseIdent(),0));
													}
												}else{
													if(t_8==4){
														t_expr=((new c_ConstExpr)->m_new((c_Type::m_intType),m__toke));
														p_NextToke();
													}else{
														if(t_8==5){
															t_expr=((new c_ConstExpr)->m_new((c_Type::m_floatType),m__toke));
															p_NextToke();
														}else{
															if(t_8==6){
																t_expr=((new c_ConstExpr)->m_new((c_Type::m_stringType),bb_config_Dequote(m__toke,String(L"cerberus",8))));
																p_NextToke();
															}else{
																bb_config_Err(String(L"Syntax error - unexpected token '",33)+m__toke+String(L"'",1));
															}
														}
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	do{
		String t_9=m__toke;
		if(t_9==String(L".",1)){
			p_NextToke();
			p_SkipEols();
			String t_id3=p_ParseIdent();
			t_expr=((new c_IdentExpr)->m_new(t_id3,t_expr));
		}else{
			if(t_9==String(L"(",1)){
				t_expr=((new c_FuncCallExpr)->m_new(t_expr,p_ParseArgs2(t_stmt)));
			}else{
				if(t_9==String(L"[",1)){
					p_NextToke();
					if((p_CParse(String(L"..",2)))!=0){
						if(m__toke==String(L"]",1)){
							t_expr=((new c_SliceExpr)->m_new(t_expr,0,0));
						}else{
							t_expr=((new c_SliceExpr)->m_new(t_expr,0,p_ParseExpr()));
						}
					}else{
						c_Expr* t_from=p_ParseExpr();
						if((p_CParse(String(L"..",2)))!=0){
							if(m__toke==String(L"]",1)){
								t_expr=((new c_SliceExpr)->m_new(t_expr,t_from,0));
							}else{
								t_expr=((new c_SliceExpr)->m_new(t_expr,t_from,p_ParseExpr()));
							}
						}else{
							t_expr=((new c_IndexExpr)->m_new(t_expr,t_from));
						}
					}
					p_Parse(String(L"]",1));
				}else{
					return t_expr;
				}
			}
		}
	}while(!(false));
}
c_Expr* c_Parser::p_ParseUnaryExpr(){
	p_SkipEols();
	String t_op=m__toke;
	String t_10=t_op;
	if(t_10==String(L"+",1) || t_10==String(L"-",1) || t_10==String(L"~",1) || t_10==String(L"not",3)){
		p_NextToke();
		c_Expr* t_expr=p_ParseUnaryExpr();
		return ((new c_UnaryExpr)->m_new(t_op,t_expr));
	}
	return p_ParsePrimaryExpr(0);
}
c_Expr* c_Parser::p_ParseMulDivExpr(){
	c_Expr* t_expr=p_ParseUnaryExpr();
	do{
		String t_op=m__toke;
		String t_11=t_op;
		if(t_11==String(L"*",1) || t_11==String(L"/",1) || t_11==String(L"mod",3) || t_11==String(L"shl",3) || t_11==String(L"shr",3)){
			p_NextToke();
			c_Expr* t_rhs=p_ParseUnaryExpr();
			t_expr=((new c_BinaryMathExpr)->m_new(t_op,t_expr,t_rhs));
		}else{
			return t_expr;
		}
	}while(!(false));
}
c_Expr* c_Parser::p_ParseAddSubExpr(){
	c_Expr* t_expr=p_ParseMulDivExpr();
	do{
		String t_op=m__toke;
		String t_12=t_op;
		if(t_12==String(L"+",1) || t_12==String(L"-",1)){
			p_NextToke();
			c_Expr* t_rhs=p_ParseMulDivExpr();
			t_expr=((new c_BinaryMathExpr)->m_new(t_op,t_expr,t_rhs));
		}else{
			return t_expr;
		}
	}while(!(false));
}
c_Expr* c_Parser::p_ParseBitandExpr(){
	c_Expr* t_expr=p_ParseAddSubExpr();
	do{
		String t_op=m__toke;
		String t_13=t_op;
		if(t_13==String(L"&",1) || t_13==String(L"~",1)){
			p_NextToke();
			c_Expr* t_rhs=p_ParseAddSubExpr();
			t_expr=((new c_BinaryMathExpr)->m_new(t_op,t_expr,t_rhs));
		}else{
			return t_expr;
		}
	}while(!(false));
}
c_Expr* c_Parser::p_ParseBitorExpr(){
	c_Expr* t_expr=p_ParseBitandExpr();
	do{
		String t_op=m__toke;
		String t_14=t_op;
		if(t_14==String(L"|",1)){
			p_NextToke();
			c_Expr* t_rhs=p_ParseBitandExpr();
			t_expr=((new c_BinaryMathExpr)->m_new(t_op,t_expr,t_rhs));
		}else{
			return t_expr;
		}
	}while(!(false));
}
c_Expr* c_Parser::p_ParseCompareExpr(){
	c_Expr* t_expr=p_ParseBitorExpr();
	do{
		String t_op=m__toke;
		String t_15=t_op;
		if(t_15==String(L"=",1) || t_15==String(L"<",1) || t_15==String(L">",1) || t_15==String(L"<=",2) || t_15==String(L">=",2) || t_15==String(L"<>",2)){
			p_NextToke();
			if(t_op==String(L">",1) && m__toke==String(L"=",1)){
				t_op=t_op+m__toke;
				p_NextToke();
			}else{
				if(t_op==String(L"<",1) && (m__toke==String(L"=",1) || m__toke==String(L">",1))){
					t_op=t_op+m__toke;
					p_NextToke();
				}
			}
			c_Expr* t_rhs=p_ParseBitorExpr();
			t_expr=((new c_BinaryCompareExpr)->m_new(t_op,t_expr,t_rhs));
		}else{
			return t_expr;
		}
	}while(!(false));
}
c_Expr* c_Parser::p_ParseAndExpr(){
	c_Expr* t_expr=p_ParseCompareExpr();
	do{
		String t_op=m__toke;
		if(t_op==String(L"and",3)){
			p_NextToke();
			c_Expr* t_rhs=p_ParseCompareExpr();
			t_expr=((new c_BinaryLogicExpr)->m_new(t_op,t_expr,t_rhs));
		}else{
			return t_expr;
		}
	}while(!(false));
}
c_Expr* c_Parser::p_ParseOrExpr(){
	c_Expr* t_expr=p_ParseAndExpr();
	do{
		String t_op=m__toke;
		if(t_op==String(L"or",2)){
			p_NextToke();
			c_Expr* t_rhs=p_ParseAndExpr();
			t_expr=((new c_BinaryLogicExpr)->m_new(t_op,t_expr,t_rhs));
		}else{
			return t_expr;
		}
	}while(!(false));
}
c_Expr* c_Parser::p_ParseExpr(){
	return p_ParseOrExpr();
}
c_Decl* c_Parser::p_ParseDecl(String t_toke,int t_attrs){
	p_SetErr();
	String t_id=p_ParseIdent();
	c_Type* t_ty=0;
	c_Expr* t_init=0;
	if((t_attrs&256)!=0){
		t_ty=p_ParseDeclType();
	}else{
		if((p_CParse(String(L":=",2)))!=0){
			t_init=p_ParseExpr();
		}else{
			t_ty=p_ParseDeclType();
			if((p_CParse(String(L"=",1)))!=0){
				t_init=p_ParseExpr();
			}else{
				if((p_CParse(String(L"[",1)))!=0){
					c_Expr* t_len=p_ParseExpr();
					p_Parse(String(L"]",1));
					while((p_CParse(String(L"[]",2)))!=0){
						t_ty=(t_ty->p_ArrayOf());
					}
					t_init=((new c_NewArrayExpr)->m_new(t_ty,t_len));
					t_ty=(t_ty->p_ArrayOf());
				}else{
					if(t_toke!=String(L"const",5)){
						t_init=((new c_ConstExpr)->m_new(t_ty,String()));
					}else{
						bb_config_Err(String(L"Constants must be initialized.",30));
					}
				}
			}
		}
	}
	c_ValDecl* t_decl=0;
	String t_21=t_toke;
	if(t_21==String(L"global",6)){
		t_decl=((new c_GlobalDecl)->m_new(t_id,t_attrs,t_ty,t_init));
	}else{
		if(t_21==String(L"field",5)){
			t_decl=((new c_FieldDecl)->m_new(t_id,t_attrs,t_ty,t_init));
		}else{
			if(t_21==String(L"const",5)){
				t_decl=((new c_ConstDecl)->m_new(t_id,t_attrs,t_ty,t_init));
			}else{
				if(t_21==String(L"local",5)){
					t_decl=((new c_LocalDecl)->m_new(t_id,t_attrs,t_ty,t_init));
				}
			}
		}
	}
	if(((t_decl->p_IsExtern())!=0) || ((p_CParse(String(L"extern",6)))!=0)){
		t_decl->m_munged=t_decl->m_ident;
		if((p_CParse(String(L"=",1)))!=0){
			t_decl->m_munged=p_ParseStringLit();
		}
	}
	return (t_decl);
}
c_List3* c_Parser::p_ParseDecls(String t_toke,int t_attrs){
	if((t_toke).Length()!=0){
		p_Parse(t_toke);
	}
	c_List3* t_decls=(new c_List3)->m_new();
	do{
		c_Decl* t_decl=p_ParseDecl(t_toke,t_attrs);
		t_decls->p_AddLast3(t_decl);
		if(!((p_CParse(String(L",",1)))!=0)){
			return t_decls;
		}
	}while(!(false));
}
c_List3* c_Parser::p_ParseEnumStmt(int t_attrs,bool t_createDeclList){
	if((t_attrs&256)!=0){
		bb_config_Err(String(L"Enumerations cannot be extern.",30));
	}
	c_List3* t_declList=0;
	if(t_createDeclList){
		t_declList=(new c_List3)->m_new();
	}
	String t_toke=m__toke;
	p_NextToke();
	int t_i=0;
	bool t_isFirst=true;
	c_Expr* t_lastInit=0;
	p_SkipEols();
	do{
		String t_id=p_ParseIdent();
		c_Expr* t_init=0;
		c_Decl* t_decl=0;
		if(m__toke!=String(L"=",1)){
			if(t_lastInit==0){
				t_init=((new c_ConstExpr)->m_new((c_Type::m_intType),String(t_i)));
			}else{
				t_init=((new c_BinaryMathExpr)->m_new(String(L"+",1),t_lastInit->p_Copy(),((new c_ConstExpr)->m_new((c_Type::m_intType),String(t_i)))));
			}
			t_decl=((new c_ConstDecl)->m_new(t_id,t_attrs,(c_Type::m_intType),t_init));
		}else{
			p_Parse(String(L"=",1));
			t_init=p_ParseExpr();
			t_lastInit=t_init;
			t_i=0;
			t_decl=((new c_ConstDecl)->m_new(t_id,t_attrs,(c_Type::m_intType),t_init));
		}
		t_i+=1;
		if(t_createDeclList){
			t_declList->p_AddLast3(t_decl);
		}else{
			m__block->p_AddStmt((new c_DeclStmt)->m_new(t_decl));
		}
	}while(!(!((p_CParse(String(L",",1)))!=0)));
	return t_declList;
}
c_List3* c_Parser::p_ParseEnum(String t_toke,int t_attrs){
	return p_ParseEnumStmt(t_attrs,true);
}
int c_Parser::p_PushBlock(c_BlockDecl* t_block){
	m__blockStack->p_AddLast7(m__block);
	m__errStack->p_AddLast(bb_config__errInfo);
	gc_assign(m__block,t_block);
	return 0;
}
int c_Parser::p_ParseDeclStmts(){
	String t_toke=m__toke;
	p_NextToke();
	do{
		c_Decl* t_decl=p_ParseDecl(t_toke,0);
		m__block->p_AddStmt((new c_DeclStmt)->m_new(t_decl));
	}while(!(!((p_CParse(String(L",",1)))!=0)));
	return 0;
}
int c_Parser::p_ParseReturnStmt(){
	p_Parse(String(L"return",6));
	c_Expr* t_expr=0;
	if(!((p_AtEos())!=0)){
		t_expr=p_ParseExpr();
	}
	m__block->p_AddStmt((new c_ReturnStmt)->m_new(t_expr));
	return 0;
}
int c_Parser::p_ParseExitStmt(){
	p_Parse(String(L"exit",4));
	m__block->p_AddStmt((new c_BreakStmt)->m_new());
	return 0;
}
int c_Parser::p_ParseContinueStmt(){
	p_Parse(String(L"continue",8));
	m__block->p_AddStmt((new c_ContinueStmt)->m_new());
	return 0;
}
int c_Parser::p_PopBlock(){
	gc_assign(m__block,m__blockStack->p_RemoveLast());
	bb_config__errInfo=m__errStack->p_RemoveLast();
	return 0;
}
int c_Parser::p_ParseIfStmt(String t_term){
	p_CParse(String(L"if",2));
	c_Expr* t_expr=p_ParseExpr();
	p_CParse(String(L"then",4));
	c_BlockDecl* t_thenBlock=(new c_BlockDecl)->m_new(m__block);
	c_BlockDecl* t_elseBlock=(new c_BlockDecl)->m_new(m__block);
	int t_eatTerm=0;
	if(!((t_term).Length()!=0)){
		if(m__toke==String(L"\n",1)){
			t_term=String(L"end",3);
		}else{
			t_term=String(L"\n",1);
		}
		t_eatTerm=1;
	}
	p_PushBlock(t_thenBlock);
	while(m__toke!=t_term){
		String t_16=m__toke;
		if(t_16==String(L"endif",5)){
			if(t_term==String(L"end",3)){
				break;
			}
			bb_config_Err(String(L"Syntax error - expecting 'End'.",31));
		}else{
			if(t_16==String(L"else",4) || t_16==String(L"elseif",6)){
				int t_elif=((m__toke==String(L"elseif",6))?1:0);
				p_NextToke();
				if(m__block==t_elseBlock){
					bb_config_Err(String(L"If statement can only have one 'else' block.",44));
				}
				p_PopBlock();
				p_PushBlock(t_elseBlock);
				if(((t_elif)!=0) || m__toke==String(L"if",2)){
					p_ParseIfStmt(t_term);
				}
			}else{
				p_ParseStmt();
			}
		}
	}
	p_PopBlock();
	if((t_eatTerm)!=0){
		p_NextToke();
		if(t_term==String(L"end",3)){
			p_CParse(String(L"if",2));
		}
	}
	c_IfStmt* t_stmt=(new c_IfStmt)->m_new(t_expr,t_thenBlock,t_elseBlock);
	m__block->p_AddStmt(t_stmt);
	return 0;
}
int c_Parser::p_ParseWhileStmt(){
	p_Parse(String(L"while",5));
	c_Expr* t_expr=p_ParseExpr();
	c_BlockDecl* t_block=(new c_BlockDecl)->m_new(m__block);
	p_PushBlock(t_block);
	while(!((p_CParse(String(L"wend",4)))!=0)){
		if((p_CParse(String(L"end",3)))!=0){
			p_CParse(String(L"while",5));
			break;
		}
		p_ParseStmt();
	}
	p_PopBlock();
	c_WhileStmt* t_stmt=(new c_WhileStmt)->m_new(t_expr,t_block);
	m__block->p_AddStmt(t_stmt);
	return 0;
}
int c_Parser::p_PushErr(){
	m__errStack->p_AddLast(bb_config__errInfo);
	return 0;
}
int c_Parser::p_PopErr(){
	bb_config__errInfo=m__errStack->p_RemoveLast();
	return 0;
}
int c_Parser::p_ParseRepeatStmt(){
	p_Parse(String(L"repeat",6));
	c_BlockDecl* t_block=(new c_BlockDecl)->m_new(m__block);
	p_PushBlock(t_block);
	while(m__toke!=String(L"until",5) && m__toke!=String(L"forever",7)){
		p_ParseStmt();
	}
	p_PopBlock();
	c_Expr* t_expr=0;
	if((p_CParse(String(L"until",5)))!=0){
		p_PushErr();
		t_expr=p_ParseExpr();
		p_PopErr();
	}else{
		p_Parse(String(L"forever",7));
		t_expr=((new c_ConstExpr)->m_new((c_Type::m_boolType),String()));
	}
	c_RepeatStmt* t_stmt=(new c_RepeatStmt)->m_new(t_block,t_expr);
	m__block->p_AddStmt(t_stmt);
	return 0;
}
int c_Parser::p_ParseForStmt(){
	p_Parse(String(L"for",3));
	String t_varid=String();
	c_Type* t_varty=0;
	int t_varlocal=0;
	if((p_CParse(String(L"local",5)))!=0){
		t_varlocal=1;
		t_varid=p_ParseIdent();
		if(!((p_CParse(String(L":=",2)))!=0)){
			t_varty=p_ParseDeclType();
			p_Parse(String(L"=",1));
		}
	}else{
		t_varlocal=0;
		t_varid=p_ParseIdent();
		p_Parse(String(L"=",1));
	}
	if((p_CParse(String(L"eachin",6)))!=0){
		c_Expr* t_expr=p_ParseExpr();
		c_BlockDecl* t_block=(new c_BlockDecl)->m_new(m__block);
		p_PushBlock(t_block);
		while(!((p_CParse(String(L"next",4)))!=0)){
			if((p_CParse(String(L"end",3)))!=0){
				p_CParse(String(L"for",3));
				break;
			}
			p_ParseStmt();
		}
		if(m__tokeType==2 && p_ParseIdent()!=t_varid){
			bb_config_Err(String(L"Next variable name does not match For variable name",51));
		}
		p_PopBlock();
		c_ForEachinStmt* t_stmt=(new c_ForEachinStmt)->m_new(t_varid,t_varty,t_varlocal,t_expr,t_block);
		m__block->p_AddStmt(t_stmt);
		return 0;
	}
	c_Expr* t_from=p_ParseExpr();
	String t_op=String();
	if((p_CParse(String(L"to",2)))!=0){
		t_op=String(L"<=",2);
	}else{
		if((p_CParse(String(L"until",5)))!=0){
			t_op=String(L"<",1);
		}else{
			bb_config_Err(String(L"Expecting 'To' or 'Until'.",26));
		}
	}
	c_Expr* t_term=p_ParseExpr();
	c_Expr* t_stp=0;
	if((p_CParse(String(L"step",4)))!=0){
		t_stp=p_ParseExpr();
	}else{
		t_stp=((new c_ConstExpr)->m_new((c_Type::m_intType),String(L"1",1)));
	}
	c_Stmt* t_init=0;
	c_Expr* t_expr2=0;
	c_Stmt* t_incr=0;
	if((t_varlocal)!=0){
		c_LocalDecl* t_indexVar=(new c_LocalDecl)->m_new(t_varid,0,t_varty,t_from);
		t_init=((new c_DeclStmt)->m_new(t_indexVar));
	}else{
		t_init=((new c_AssignStmt)->m_new(String(L"=",1),((new c_IdentExpr)->m_new(t_varid,0)),t_from));
	}
	t_expr2=((new c_BinaryCompareExpr)->m_new(t_op,((new c_IdentExpr)->m_new(t_varid,0)),t_term));
	t_incr=((new c_AssignStmt)->m_new(String(L"=",1),((new c_IdentExpr)->m_new(t_varid,0)),((new c_BinaryMathExpr)->m_new(String(L"+",1),((new c_IdentExpr)->m_new(t_varid,0)),t_stp))));
	c_BlockDecl* t_block2=(new c_BlockDecl)->m_new(m__block);
	p_PushBlock(t_block2);
	while(!((p_CParse(String(L"next",4)))!=0)){
		if((p_CParse(String(L"end",3)))!=0){
			p_CParse(String(L"for",3));
			break;
		}
		p_ParseStmt();
	}
	if(m__tokeType==2 && p_ParseIdent()!=t_varid){
		bb_config_Err(String(L"Next variable name does not match For variable name",51));
	}
	p_PopBlock();
	c_ForStmt* t_stmt2=(new c_ForStmt)->m_new(t_init,t_expr2,t_incr,t_block2);
	m__block->p_AddStmt(t_stmt2);
	return 0;
}
int c_Parser::p_ParseSelectStmt(){
	p_Parse(String(L"select",6));
	c_Expr* t_expr=p_ParseExpr();
	c_BlockDecl* t_block=m__block;
	m__selTmpId+=1;
	String t_tmpId=String(m__selTmpId);
	t_block->p_AddStmt((new c_DeclStmt)->m_new2(t_tmpId,0,t_expr));
	c_IdentExpr* t_tmpExpr=(new c_IdentExpr)->m_new(t_tmpId,0);
	while(m__toke!=String(L"end",3) && m__toke!=String(L"default",7)){
		p_SetErr();
		String t_17=m__toke;
		if(t_17==String(L"\n",1)){
			p_NextToke();
		}else{
			if(t_17==String(L"case",4)){
				p_NextToke();
				c_Expr* t_comp=0;
				do{
					c_Expr* t_expr2=((new c_IdentExpr)->m_new(t_tmpId,0));
					t_expr2=((new c_BinaryCompareExpr)->m_new(String(L"=",1),t_expr2,p_ParseExpr()));
					if((t_comp)!=0){
						t_comp=((new c_BinaryLogicExpr)->m_new(String(L"or",2),t_comp,t_expr2));
					}else{
						t_comp=t_expr2;
					}
				}while(!(!((p_CParse(String(L",",1)))!=0)));
				c_BlockDecl* t_thenBlock=(new c_BlockDecl)->m_new(m__block);
				c_BlockDecl* t_elseBlock=(new c_BlockDecl)->m_new(m__block);
				c_IfStmt* t_ifstmt=(new c_IfStmt)->m_new(t_comp,t_thenBlock,t_elseBlock);
				t_block->p_AddStmt(t_ifstmt);
				t_block=t_ifstmt->m_thenBlock;
				p_PushBlock(t_block);
				while(m__toke!=String(L"case",4) && m__toke!=String(L"default",7) && m__toke!=String(L"end",3)){
					p_ParseStmt();
				}
				p_PopBlock();
				t_block=t_elseBlock;
			}else{
				bb_config_Err(String(L"Syntax error - expecting 'Case', 'Default' or 'End'.",52));
			}
		}
	}
	if(m__toke==String(L"default",7)){
		p_NextToke();
		p_PushBlock(t_block);
		while(m__toke!=String(L"end",3)){
			p_SetErr();
			String t_18=m__toke;
			if(t_18==String(L"case",4)){
				bb_config_Err(String(L"Case can not appear after default.",34));
			}else{
				if(t_18==String(L"default",7)){
					bb_config_Err(String(L"Select statement can have only one default block.",49));
				}
			}
			p_ParseStmt();
		}
		p_PopBlock();
	}
	p_SetErr();
	p_Parse(String(L"end",3));
	p_CParse(String(L"select",6));
	return 0;
}
int c_Parser::p_ParseTryStmt(){
	p_Parse(String(L"try",3));
	c_BlockDecl* t_block=(new c_BlockDecl)->m_new(m__block);
	c_Stack7* t_catches=(new c_Stack7)->m_new();
	p_PushBlock(t_block);
	while(m__toke!=String(L"end",3)){
		if((p_CParse(String(L"catch",5)))!=0){
			String t_id=p_ParseIdent();
			p_Parse(String(L":",1));
			c_Type* t_ty=p_ParseType();
			c_LocalDecl* t_init=(new c_LocalDecl)->m_new(t_id,0,t_ty,0);
			c_BlockDecl* t_block2=(new c_BlockDecl)->m_new(m__block);
			t_catches->p_Push19((new c_CatchStmt)->m_new(t_init,t_block2));
			p_PopBlock();
			p_PushBlock(t_block2);
		}else{
			p_ParseStmt();
		}
	}
	if(!((t_catches->p_Length2())!=0)){
		bb_config_Err(String(L"Try block must have at least one catch block",44));
	}
	p_PopBlock();
	p_NextToke();
	p_CParse(String(L"try",3));
	m__block->p_AddStmt((new c_TryStmt)->m_new(t_block,t_catches->p_ToArray()));
	return 0;
}
int c_Parser::p_ParseThrowStmt(){
	p_Parse(String(L"throw",5));
	m__block->p_AddStmt((new c_ThrowStmt)->m_new(p_ParseExpr()));
	return 0;
}
int c_Parser::p_ParseStmt(){
	p_SetErr();
	String t_19=m__toke;
	if(t_19==String(L";",1) || t_19==String(L"\n",1)){
		p_NextToke();
	}else{
		if(t_19==String(L"const",5) || t_19==String(L"local",5)){
			p_ParseDeclStmts();
		}else{
			if(t_19==String(L"enumerate",9)){
				p_ParseEnumStmt(0,false);
			}else{
				if(t_19==String(L"return",6)){
					p_ParseReturnStmt();
				}else{
					if(t_19==String(L"exit",4)){
						p_ParseExitStmt();
					}else{
						if(t_19==String(L"continue",8)){
							p_ParseContinueStmt();
						}else{
							if(t_19==String(L"if",2)){
								p_ParseIfStmt(String());
							}else{
								if(t_19==String(L"while",5)){
									p_ParseWhileStmt();
								}else{
									if(t_19==String(L"repeat",6)){
										p_ParseRepeatStmt();
									}else{
										if(t_19==String(L"for",3)){
											p_ParseForStmt();
										}else{
											if(t_19==String(L"select",6)){
												p_ParseSelectStmt();
											}else{
												if(t_19==String(L"try",3)){
													p_ParseTryStmt();
												}else{
													if(t_19==String(L"throw",5)){
														p_ParseThrowStmt();
													}else{
														c_Expr* t_expr=p_ParsePrimaryExpr(1);
														String t_20=m__toke;
														if(t_20==String(L"=",1) || t_20==String(L"*=",2) || t_20==String(L"/=",2) || t_20==String(L"+=",2) || t_20==String(L"-=",2) || t_20==String(L"&=",2) || t_20==String(L"|=",2) || t_20==String(L"~=",2) || t_20==String(L"mod",3) || t_20==String(L"shl",3) || t_20==String(L"shr",3)){
															if(((dynamic_cast<c_IdentExpr*>(t_expr))!=0) || ((dynamic_cast<c_IndexExpr*>(t_expr))!=0)){
																String t_op=m__toke;
																p_NextToke();
																if(!t_op.EndsWith(String(L"=",1))){
																	p_Parse(String(L"=",1));
																	t_op=t_op+String(L"=",1);
																}
																m__block->p_AddStmt((new c_AssignStmt)->m_new(t_op,t_expr,p_ParseExpr()));
															}else{
																bb_config_Err(String(L"Assignment operator '",21)+m__toke+String(L"' cannot be used this way.",26));
															}
															return 0;
														}
														if((dynamic_cast<c_IdentExpr*>(t_expr))!=0){
															t_expr=((new c_FuncCallExpr)->m_new(t_expr,p_ParseArgs2(1)));
														}else{
															if(((dynamic_cast<c_FuncCallExpr*>(t_expr))!=0) || ((dynamic_cast<c_InvokeSuperExpr*>(t_expr))!=0) || ((dynamic_cast<c_NewObjectExpr*>(t_expr))!=0)){
															}else{
																bb_config_Err(String(L"Expression cannot be used as a statement.",41));
															}
														}
														m__block->p_AddStmt((new c_ExprStmt)->m_new(t_expr));
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	return 0;
}
c_FuncDecl* c_Parser::p_ParseFuncDecl(int t_attrs){
	p_SetErr();
	if((p_CParse(String(L"method",6)))!=0){
		t_attrs|=1;
	}else{
		if(!((p_CParse(String(L"function",8)))!=0)){
			bb_config_InternalErr(String(L"Internal error",14));
		}
	}
	t_attrs|=m__defattrs;
	String t_id=String();
	c_Type* t_ty=0;
	if((t_attrs&1)!=0){
		if(m__toke==String(L"new",3)){
			if((t_attrs&256)!=0){
				bb_config_Err(String(L"Extern classes cannot have constructors.",40));
			}
			t_id=m__toke;
			p_NextToke();
			t_attrs|=2;
			t_attrs&=-2;
		}else{
			t_id=p_ParseIdent();
			t_ty=p_ParseDeclType();
		}
	}else{
		t_id=p_ParseIdent();
		t_ty=p_ParseDeclType();
	}
	c_Stack6* t_args=(new c_Stack6)->m_new();
	p_Parse(String(L"(",1));
	p_SkipEols();
	if(m__toke!=String(L")",1)){
		do{
			String t_id2=p_ParseIdent();
			c_Type* t_ty2=p_ParseDeclType();
			c_Expr* t_init=0;
			if((p_CParse(String(L"=",1)))!=0){
				t_init=p_ParseExpr();
			}
			t_args->p_Push16((new c_ArgDecl)->m_new(t_id2,0,t_ty2,t_init));
			if(m__toke==String(L")",1)){
				break;
			}
			p_Parse(String(L",",1));
		}while(!(false));
	}
	p_Parse(String(L")",1));
	do{
		if((p_CParse(String(L"final",5)))!=0){
			if(!((t_attrs&1)!=0)){
				bb_config_Err(String(L"Functions cannot be final.",26));
			}
			if((t_attrs&2048)!=0){
				bb_config_Err(String(L"Duplicate method attribute.",27));
			}
			if((t_attrs&1024)!=0){
				bb_config_Err(String(L"Methods cannot be both final and abstract.",42));
			}
			t_attrs|=2048;
		}else{
			if((p_CParse(String(L"abstract",8)))!=0){
				if(!((t_attrs&1)!=0)){
					bb_config_Err(String(L"Functions cannot be abstract.",29));
				}
				if((t_attrs&1024)!=0){
					bb_config_Err(String(L"Duplicate method attribute.",27));
				}
				if((t_attrs&2048)!=0){
					bb_config_Err(String(L"Methods cannot be both final and abstract.",42));
				}
				t_attrs|=1024;
			}else{
				if((p_CParse(String(L"property",8)))!=0){
					if(!((t_attrs&1)!=0)){
						bb_config_Err(String(L"Functions cannot be properties.",31));
					}
					if((t_attrs&4)!=0){
						bb_config_Err(String(L"Duplicate method attribute.",27));
					}
					t_attrs|=4;
				}else{
					break;
				}
			}
		}
	}while(!(false));
	c_FuncDecl* t_funcDecl=(new c_FuncDecl)->m_new(t_id,t_attrs,t_ty,t_args->p_ToArray());
	if(((t_funcDecl->p_IsExtern())!=0) || ((p_CParse(String(L"extern",6)))!=0)){
		t_funcDecl->m_munged=t_funcDecl->m_ident;
		if((p_CParse(String(L"=",1)))!=0){
			t_funcDecl->m_munged=p_ParseStringLit();
			if(t_funcDecl->m_munged==String(L"$resize",7)){
				gc_assign(t_funcDecl->m_retType,(c_Type::m_emptyArrayType));
			}
		}
	}
	if(((t_funcDecl->p_IsExtern())!=0) || ((t_funcDecl->p_IsAbstract())!=0)){
		return t_funcDecl;
	}
	p_PushBlock(t_funcDecl);
	while(m__toke!=String(L"end",3)){
		p_ParseStmt();
	}
	p_PopBlock();
	p_NextToke();
	if((t_attrs&3)!=0){
		p_CParse(String(L"method",6));
	}else{
		p_CParse(String(L"function",8));
	}
	return t_funcDecl;
}
c_ClassDecl* c_Parser::p_ParseClassDecl(int t_attrs){
	p_SetErr();
	String t_toke=m__toke;
	if((p_CParse(String(L"interface",9)))!=0){
		if((t_attrs&256)!=0){
			bb_config_Err(String(L"Interfaces cannot be extern.",28));
		}
		t_attrs|=5120;
	}else{
		if(!((p_CParse(String(L"class",5)))!=0)){
			bb_config_InternalErr(String(L"Internal error",14));
		}
	}
	String t_id=p_ParseIdent();
	c_StringStack* t_args=(new c_StringStack)->m_new2();
	c_IdentType* t_superTy=c_Type::m_objectType;
	c_Stack5* t_imps=(new c_Stack5)->m_new();
	if((p_CParse(String(L"<",1)))!=0){
		if((t_attrs&256)!=0){
			bb_config_Err(String(L"Extern classes cannot be generic.",33));
		}
		do{
			t_args->p_Push(p_ParseIdent());
		}while(!(!((p_CParse(String(L",",1)))!=0)));
		p_Parse(String(L">",1));
	}
	if((p_CParse(String(L"extends",7)))!=0){
		if((p_CParse(String(L"null",4)))!=0){
			if((t_attrs&4096)!=0){
				bb_config_Err(String(L"Interfaces cannot extend null",29));
			}
			if(!((t_attrs&256)!=0)){
				bb_config_Err(String(L"Only extern objects can extend null.",36));
			}
			t_superTy=0;
		}else{
			if((t_attrs&4096)!=0){
				do{
					t_imps->p_Push13(p_ParseIdentType());
				}while(!(!((p_CParse(String(L",",1)))!=0)));
				t_superTy=c_Type::m_objectType;
			}else{
				t_superTy=p_ParseIdentType();
			}
		}
	}
	if((p_CParse(String(L"implements",10)))!=0){
		if((t_attrs&256)!=0){
			bb_config_Err(String(L"Implements cannot be used with external classes.",48));
		}
		if((t_attrs&4096)!=0){
			bb_config_Err(String(L"Implements cannot be used with interfaces.",42));
		}
		do{
			t_imps->p_Push13(p_ParseIdentType());
		}while(!(!((p_CParse(String(L",",1)))!=0)));
	}
	do{
		if((p_CParse(String(L"final",5)))!=0){
			if((t_attrs&4096)!=0){
				bb_config_Err(String(L"Interfaces cannot be final.",27));
			}
			if((t_attrs&2048)!=0){
				bb_config_Err(String(L"Duplicate class attribute.",26));
			}
			if((t_attrs&1024)!=0){
				bb_config_Err(String(L"Classes cannot be both final and abstract.",42));
			}
			t_attrs|=2048;
		}else{
			if((p_CParse(String(L"abstract",8)))!=0){
				if((t_attrs&4096)!=0){
					bb_config_Err(String(L"Interfaces cannot be abstract.",30));
				}
				if((t_attrs&1024)!=0){
					bb_config_Err(String(L"Duplicate class attribute.",26));
				}
				if((t_attrs&2048)!=0){
					bb_config_Err(String(L"Classes cannot be both final and abstract.",42));
				}
				t_attrs|=1024;
			}else{
				break;
			}
		}
	}while(!(false));
	c_ClassDecl* t_classDecl=(new c_ClassDecl)->m_new(t_id,t_attrs,t_args->p_ToArray(),t_superTy,t_imps->p_ToArray());
	if(((t_classDecl->p_IsExtern())!=0) || ((p_CParse(String(L"extern",6)))!=0)){
		t_classDecl->m_munged=t_classDecl->m_ident;
		if((p_CParse(String(L"=",1)))!=0){
			t_classDecl->m_munged=p_ParseStringLit();
		}
	}
	int t_decl_attrs=t_attrs&256;
	int t_func_attrs=0;
	if((t_attrs&4096)!=0){
		t_func_attrs|=1024;
	}
	do{
		p_SkipEols();
		String t_22=m__toke;
		if(t_22==String(L"end",3)){
			p_NextToke();
			break;
		}else{
			if(t_22==String(L"public",6)){
				p_NextToke();
				t_decl_attrs&=-16897;
			}else{
				if(t_22==String(L"private",7)){
					p_NextToke();
					t_decl_attrs&=-16897;
					t_decl_attrs|=512;
				}else{
					if(t_22==String(L"protected",9)){
						p_NextToke();
						t_decl_attrs&=-16897;
						t_decl_attrs|=16384;
					}else{
						if(t_22==String(L"const",5) || t_22==String(L"global",6) || t_22==String(L"field",5)){
							if(((t_attrs&4096)!=0) && m__toke!=String(L"const",5)){
								bb_config_Err(String(L"Interfaces can only contain constants and methods.",50));
							}
							t_classDecl->p_InsertDecls(p_ParseDecls(m__toke,t_decl_attrs));
						}else{
							if(t_22==String(L"enumerate",9)){
								t_classDecl->p_InsertDecls(p_ParseEnum(m__toke,t_decl_attrs));
							}else{
								if(t_22==String(L"method",6)){
									t_classDecl->p_InsertDecl(p_ParseFuncDecl(t_decl_attrs|t_func_attrs));
								}else{
									if(t_22==String(L"function",8)){
										if((t_attrs&4096)!=0){
											bb_config_Err(String(L"Interfaces can only contain constants and methods.",50));
										}
										t_classDecl->p_InsertDecl(p_ParseFuncDecl(t_decl_attrs|t_func_attrs));
									}else{
										bb_config_Err(String(L"Syntax error - expecting class member declaration.",50));
									}
								}
							}
						}
					}
				}
			}
		}
	}while(!(false));
	if((t_toke).Length()!=0){
		p_CParse(t_toke);
	}
	return t_classDecl;
}
int c_Parser::p_ParseMain(){
	p_SkipEols();
	if((p_CParse(String(L"strict",6)))!=0){
		m__module->m_attrs|=1;
	}
	int t_attrs=0;
	while((m__toke).Length()!=0){
		p_SetErr();
		String t_23=m__toke;
		if(t_23==String(L"\n",1)){
			p_NextToke();
		}else{
			if(t_23==String(L"public",6)){
				p_NextToke();
				t_attrs=0;
			}else{
				if(t_23==String(L"private",7)){
					p_NextToke();
					t_attrs=512;
				}else{
					if(t_23==String(L"protected",9)){
						bb_config_Err(String(L"Protected may only be used within classes.",42));
					}else{
						if(t_23==String(L"import",6)){
							p_NextToke();
							if(m__tokeType==6){
								p_ImportFile(bb_config_EvalConfigTags(p_ParseStringLit()));
							}else{
								p_ImportModule(p_ParseModPath(),t_attrs);
							}
						}else{
							if(t_23==String(L"friend",6)){
								p_NextToke();
								String t_modpath=p_ParseModPath();
								m__module->m_friends->p_Insert(t_modpath);
							}else{
								if(t_23==String(L"alias",5)){
									p_NextToke();
									do{
										String t_ident=p_ParseIdent();
										p_Parse(String(L"=",1));
										Object* t_decl=0;
										String t_24=m__toke;
										if(t_24==String(L"int",3)){
											t_decl=(c_Type::m_intType);
										}else{
											if(t_24==String(L"float",5)){
												t_decl=(c_Type::m_floatType);
											}else{
												if(t_24==String(L"string",6)){
													t_decl=(c_Type::m_stringType);
												}
											}
										}
										if((t_decl)!=0){
											m__module->p_InsertDecl((new c_AliasDecl)->m_new(t_ident,t_attrs,t_decl));
											p_NextToke();
											continue;
										}
										c_ScopeDecl* t_scope=(m__module);
										bb_decl_PushEnv(m__module);
										do{
											String t_id=p_ParseIdent();
											t_decl=t_scope->p_FindDecl(t_id);
											if(!((t_decl)!=0)){
												bb_config_Err(String(L"Identifier '",12)+t_id+String(L"' not found.",12));
											}
											if(!((p_CParse(String(L".",1)))!=0)){
												break;
											}
											t_scope=dynamic_cast<c_ScopeDecl*>(t_decl);
											if(!((t_scope)!=0) || ((dynamic_cast<c_FuncDecl*>(t_scope))!=0)){
												bb_config_Err(String(L"Invalid scope '",15)+t_id+String(L"'.",2));
											}
										}while(!(false));
										bb_decl_PopEnv();
										m__module->p_InsertDecl((new c_AliasDecl)->m_new(t_ident,t_attrs,t_decl));
									}while(!(!((p_CParse(String(L",",1)))!=0)));
								}else{
									break;
								}
							}
						}
					}
				}
			}
		}
	}
	while((m__toke).Length()!=0){
		p_SetErr();
		String t_25=m__toke;
		if(t_25==String(L"\n",1)){
			p_NextToke();
		}else{
			if(t_25==String(L"public",6)){
				p_NextToke();
				t_attrs=0;
			}else{
				if(t_25==String(L"private",7)){
					p_NextToke();
					t_attrs=512;
				}else{
					if(t_25==String(L"extern",6)){
						if((bb_config_ENV_SAFEMODE)!=0){
							if(m__app->m_mainModule==m__module){
								bb_config_Err(String(L"Extern not permitted in safe mode.",34));
							}
						}
						p_NextToke();
						t_attrs=256;
						if((p_CParse(String(L"private",7)))!=0){
							t_attrs|=512;
						}
					}else{
						if(t_25==String(L"const",5) || t_25==String(L"global",6)){
							m__module->p_InsertDecls(p_ParseDecls(m__toke,t_attrs));
						}else{
							if(t_25==String(L"enumerate",9)){
								m__module->p_InsertDecls(p_ParseEnum(m__toke,t_attrs));
							}else{
								if(t_25==String(L"class",5)){
									m__module->p_InsertDecl(p_ParseClassDecl(t_attrs));
								}else{
									if(t_25==String(L"interface",9)){
										m__module->p_InsertDecl(p_ParseClassDecl(t_attrs));
									}else{
										if(t_25==String(L"function",8)){
											m__module->p_InsertDecl(p_ParseFuncDecl(t_attrs));
										}else{
											bb_config_Err(String(L"Syntax error - expecting declaration.",37));
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	bb_config__errInfo=String();
	return 0;
}
void c_Parser::mark(){
	Object::mark();
	gc_mark_q(m__toker);
	gc_mark_q(m__app);
	gc_mark_q(m__module);
	gc_mark_q(m__block);
	gc_mark_q(m__blockStack);
	gc_mark_q(m__errStack);
}
int bb_config_InternalErr(String t_err){
	bbPrint(bb_config__errInfo+String(L" : ",3)+t_err);
	bbError(bb_config__errInfo+String(L" : ",3)+t_err);
	return 0;
}
int bb_config_StringToInt(String t_str,int t_base){
	int t_i=0;
	int t_l=t_str.Length();
	while(t_i<t_l && (int)t_str[t_i]<=32){
		t_i+=1;
	}
	bool t_neg=false;
	if(t_i<t_l && ((int)t_str[t_i]==43 || (int)t_str[t_i]==45)){
		t_neg=(int)t_str[t_i]==45;
		t_i+=1;
	}
	int t_n=0;
	while(t_i<t_l){
		int t_c=(int)t_str[t_i];
		int t_t=0;
		if(t_c>=48 && t_c<58){
			t_t=t_c-48;
		}else{
			if(t_c>=65 && t_c<=90){
				t_t=t_c-55;
			}else{
				if(t_c>=97 && t_c<=122){
					t_t=t_c-87;
				}else{
					break;
				}
			}
		}
		if(t_t>=t_base){
			break;
		}
		t_n=t_n*t_base+t_t;
		t_i+=1;
	}
	if(t_neg){
		t_n=-t_n;
	}
	return t_n;
}
String bb_config_Dequote(String t_str,String t_lang){
	String t_4=t_lang;
	if(t_4==String(L"cerberus",8)){
		if(t_str.Length()<2 || !(t_str.StartsWith(String(L"\"",1)) && t_str.EndsWith(String(L"\"",1))) && !(t_str.StartsWith(String(L"`",1)) && t_str.EndsWith(String(L"`",1)))){
			bb_config_InternalErr(String(L"Internal error",14));
		}
		bool t_isChar=t_str.StartsWith(String(L"`",1));
		t_str=t_str.Slice(1,-1);
		int t_i=0;
		do{
			t_i=t_str.Find(String(L"~",1),t_i);
			if(t_i==-1){
				break;
			}
			if(t_i+1>=t_str.Length()){
				bb_config_Err(String(L"Invalid escape sequence in string",33));
			}
			String t_ch=t_str.Slice(t_i+1,t_i+2);
			String t_5=t_ch;
			if(t_5==String(L"~",1)){
				t_ch=String(L"~",1);
			}else{
				if(t_5==String(L"q",1)){
					t_ch=String(L"\"",1);
				}else{
					if(t_5==String(L"n",1)){
						t_ch=String(L"\n",1);
					}else{
						if(t_5==String(L"r",1)){
							t_ch=String(L"\r",1);
						}else{
							if(t_5==String(L"t",1)){
								t_ch=String(L"\t",1);
							}else{
								if(t_5==String(L"g",1)){
									t_ch=String(L"`",1);
								}else{
									if(t_5==String(L"u",1)){
										String t_t=t_str.Slice(t_i+2,t_i+6);
										if(t_t.Length()!=4){
											bb_config_Err(String(L"Invalid unicode hex value in string",35));
										}
										for(int t_j=0;t_j<4;t_j=t_j+1){
											if(!((bb_config_IsHexDigit((int)t_t[t_j]))!=0)){
												bb_config_Err(String(L"Invalid unicode hex digit in string",35));
											}
										}
										t_str=t_str.Slice(0,t_i)+String((Char)(bb_config_StringToInt(t_t,16)),1)+t_str.Slice(t_i+6);
										t_i+=1;
										continue;
									}else{
										if(t_5==String(L"0",1)){
											t_ch=String((Char)(0),1);
										}else{
											bb_config_Err(String(L"Invalid escape character in string: '",37)+t_ch+String(L"'",1));
										}
									}
								}
							}
						}
					}
				}
			}
			t_str=t_str.Slice(0,t_i)+t_ch+t_str.Slice(t_i+2);
			t_i+=t_ch.Length();
		}while(!(false));
		if(t_isChar && t_str.Length()>1){
			bb_config_InternalErr(String(L"Internal error",14));
		}
		return t_str;
	}else{
		if(t_4==String(L"monkey",6)){
			if(t_str.Length()<2 || !(t_str.StartsWith(String(L"\"",1)) && t_str.EndsWith(String(L"\"",1))) && !(t_str.StartsWith(String(L"`",1)) && t_str.EndsWith(String(L"`",1)))){
				bb_config_InternalErr(String(L"Internal error",14));
			}
			bool t_isChar2=t_str.StartsWith(String(L"`",1));
			t_str=t_str.Slice(1,-1);
			int t_i2=0;
			do{
				t_i2=t_str.Find(String(L"~",1),t_i2);
				if(t_i2==-1){
					break;
				}
				if(t_i2+1>=t_str.Length()){
					bb_config_Err(String(L"Invalid escape sequence in string",33));
				}
				String t_ch2=t_str.Slice(t_i2+1,t_i2+2);
				String t_6=t_ch2;
				if(t_6==String(L"~",1)){
					t_ch2=String(L"~",1);
				}else{
					if(t_6==String(L"q",1)){
						t_ch2=String(L"\"",1);
					}else{
						if(t_6==String(L"n",1)){
							t_ch2=String(L"\n",1);
						}else{
							if(t_6==String(L"r",1)){
								t_ch2=String(L"\r",1);
							}else{
								if(t_6==String(L"t",1)){
									t_ch2=String(L"\t",1);
								}else{
									if(t_6==String(L"g",1)){
										t_ch2=String(L"`",1);
									}else{
										if(t_6==String(L"u",1)){
											String t_t2=t_str.Slice(t_i2+2,t_i2+6);
											if(t_t2.Length()!=4){
												bb_config_Err(String(L"Invalid unicode hex value in string",35));
											}
											for(int t_j2=0;t_j2<4;t_j2=t_j2+1){
												if(!((bb_config_IsHexDigit((int)t_t2[t_j2]))!=0)){
													bb_config_Err(String(L"Invalid unicode hex digit in string",35));
												}
											}
											t_str=t_str.Slice(0,t_i2)+String((Char)(bb_config_StringToInt(t_t2,16)),1)+t_str.Slice(t_i2+6);
											t_i2+=1;
											continue;
										}else{
											if(t_6==String(L"0",1)){
												t_ch2=String((Char)(0),1);
											}else{
												bb_config_Err(String(L"Invalid escape character in string: '",37)+t_ch2+String(L"'",1));
											}
										}
									}
								}
							}
						}
					}
				}
				t_str=t_str.Slice(0,t_i2)+t_ch2+t_str.Slice(t_i2+2);
				t_i2+=t_ch2.Length();
			}while(!(false));
			if(t_isChar2 && t_str.Length()>1){
				bb_config_InternalErr(String(L"Internal error",14));
			}
			return t_str;
		}
	}
	bb_config_InternalErr(String(L"Internal error",14));
	return String();
}
String bb_config_EvalConfigTags(String t_cfg){
	int t_i=0;
	do{
		t_i=t_cfg.Find(String(L"${",2),0);
		if(t_i==-1){
			return t_cfg;
		}
		int t_e=t_cfg.Find(String(L"}",1),t_i+2);
		if(t_e==-1){
			return t_cfg;
		}
		String t_key=t_cfg.Slice(t_i+2,t_e);
		String t_val=bb_config__cfgScope->m_vars->p_Get(t_key);
		t_cfg=t_cfg.Slice(0,t_i)+t_val+t_cfg.Slice(t_e+1);
		t_i+=t_val.Length();
	}while(!(false));
}
int bb_config_ENV_SAFEMODE;
c_NumericType::c_NumericType(){
}
c_NumericType* c_NumericType::m_new(){
	c_Type::m_new();
	return this;
}
void c_NumericType::mark(){
	c_Type::mark();
}
c_IntType::c_IntType(){
}
c_IntType* c_IntType::m_new(){
	c_NumericType::m_new();
	return this;
}
int c_IntType::p_EqualsType(c_Type* t_ty){
	return ((dynamic_cast<c_IntType*>(t_ty)!=0)?1:0);
}
int c_IntType::p_ExtendsType(c_Type* t_ty){
	if((dynamic_cast<c_ObjectType*>(t_ty))!=0){
		c_Expr* t_expr=((new c_ConstExpr)->m_new((this),String()))->p_Semant();
		c_Expr* t_[]={t_expr};
		c_FuncDecl* t_ctor=t_ty->p_GetClass()->p_FindFuncDecl(String(L"new",3),Array<c_Expr* >(t_,1),1);
		return ((((t_ctor)!=0) && t_ctor->p_IsCtor())?1:0);
	}
	return ((dynamic_cast<c_NumericType*>(t_ty)!=0 || dynamic_cast<c_StringType*>(t_ty)!=0)?1:0);
}
c_ClassDecl* c_IntType::p_GetClass(){
	return dynamic_cast<c_ClassDecl*>(bb_decl__env->p_FindDecl(String(L"int",3)));
}
String c_IntType::p_ToString(){
	return String(L"Int",3);
}
void c_IntType::mark(){
	c_NumericType::mark();
}
c_FloatType::c_FloatType(){
}
c_FloatType* c_FloatType::m_new(){
	c_NumericType::m_new();
	return this;
}
int c_FloatType::p_EqualsType(c_Type* t_ty){
	return ((dynamic_cast<c_FloatType*>(t_ty)!=0)?1:0);
}
int c_FloatType::p_ExtendsType(c_Type* t_ty){
	if((dynamic_cast<c_ObjectType*>(t_ty))!=0){
		c_Expr* t_expr=((new c_ConstExpr)->m_new((this),String()))->p_Semant();
		c_Expr* t_[]={t_expr};
		c_FuncDecl* t_ctor=t_ty->p_GetClass()->p_FindFuncDecl(String(L"new",3),Array<c_Expr* >(t_,1),1);
		return ((((t_ctor)!=0) && t_ctor->p_IsCtor())?1:0);
	}
	return ((dynamic_cast<c_NumericType*>(t_ty)!=0 || dynamic_cast<c_StringType*>(t_ty)!=0)?1:0);
}
c_ClassDecl* c_FloatType::p_GetClass(){
	return dynamic_cast<c_ClassDecl*>(bb_decl__env->p_FindDecl(String(L"float",5)));
}
String c_FloatType::p_ToString(){
	return String(L"Float",5);
}
void c_FloatType::mark(){
	c_NumericType::mark();
}
c_AliasDecl::c_AliasDecl(){
	m_decl=0;
}
c_AliasDecl* c_AliasDecl::m_new(String t_ident,int t_attrs,Object* t_decl){
	c_Decl::m_new();
	this->m_ident=t_ident;
	this->m_attrs=t_attrs;
	gc_assign(this->m_decl,t_decl);
	return this;
}
c_AliasDecl* c_AliasDecl::m_new2(){
	c_Decl::m_new();
	return this;
}
c_Decl* c_AliasDecl::p_OnCopy(){
	return ((new c_AliasDecl)->m_new(m_ident,m_attrs,m_decl));
}
int c_AliasDecl::p_OnSemant(){
	return 0;
}
void c_AliasDecl::mark(){
	c_Decl::mark();
	gc_mark_q(m_decl);
}
c_List3::c_List3(){
	m__head=((new c_HeadNode3)->m_new());
}
c_List3* c_List3::m_new(){
	return this;
}
c_Node8* c_List3::p_AddLast3(c_Decl* t_data){
	return (new c_Node8)->m_new(m__head,m__head->m__pred,t_data);
}
c_List3* c_List3::m_new2(Array<c_Decl* > t_data){
	Array<c_Decl* > t_=t_data;
	int t_2=0;
	while(t_2<t_.Length()){
		c_Decl* t_t=t_[t_2];
		t_2=t_2+1;
		p_AddLast3(t_t);
	}
	return this;
}
c_Enumerator2* c_List3::p_ObjectEnumerator(){
	return (new c_Enumerator2)->m_new(this);
}
int c_List3::p_Count(){
	int t_n=0;
	c_Node8* t_node=m__head->m__succ;
	while(t_node!=m__head){
		t_node=t_node->m__succ;
		t_n+=1;
	}
	return t_n;
}
void c_List3::mark(){
	Object::mark();
	gc_mark_q(m__head);
}
c_Node8::c_Node8(){
	m__succ=0;
	m__pred=0;
	m__data=0;
}
c_Node8* c_Node8::m_new(c_Node8* t_succ,c_Node8* t_pred,c_Decl* t_data){
	gc_assign(m__succ,t_succ);
	gc_assign(m__pred,t_pred);
	gc_assign(m__succ->m__pred,this);
	gc_assign(m__pred->m__succ,this);
	gc_assign(m__data,t_data);
	return this;
}
c_Node8* c_Node8::m_new2(){
	return this;
}
void c_Node8::mark(){
	Object::mark();
	gc_mark_q(m__succ);
	gc_mark_q(m__pred);
	gc_mark_q(m__data);
}
c_HeadNode3::c_HeadNode3(){
}
c_HeadNode3* c_HeadNode3::m_new(){
	c_Node8::m_new2();
	gc_assign(m__succ,(this));
	gc_assign(m__pred,(this));
	return this;
}
void c_HeadNode3::mark(){
	c_Node8::mark();
}
c_BlockDecl::c_BlockDecl(){
	m_stmts=(new c_List5)->m_new();
}
int c_BlockDecl::p_AddStmt(c_Stmt* t_stmt){
	m_stmts->p_AddLast5(t_stmt);
	return 0;
}
c_BlockDecl* c_BlockDecl::m_new(c_ScopeDecl* t_scope){
	c_ScopeDecl::m_new();
	gc_assign(this->m_scope,t_scope);
	return this;
}
c_BlockDecl* c_BlockDecl::m_new2(){
	c_ScopeDecl::m_new();
	return this;
}
c_Decl* c_BlockDecl::p_OnCopy(){
	c_BlockDecl* t_t=(new c_BlockDecl)->m_new2();
	c_Enumerator5* t_=m_stmts->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_Stmt* t_stmt=t_->p_NextObject();
		t_t->p_AddStmt(t_stmt->p_Copy2(t_t));
	}
	return (t_t);
}
int c_BlockDecl::p_OnSemant(){
	bb_decl_PushEnv(this);
	c_Enumerator5* t_=m_stmts->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_Stmt* t_stmt=t_->p_NextObject();
		t_stmt->p_Semant();
	}
	bb_decl_PopEnv();
	return 0;
}
c_BlockDecl* c_BlockDecl::p_CopyBlock(c_ScopeDecl* t_scope){
	c_BlockDecl* t_t=dynamic_cast<c_BlockDecl*>(p_Copy());
	gc_assign(t_t->m_scope,t_scope);
	return t_t;
}
void c_BlockDecl::mark(){
	c_ScopeDecl::mark();
	gc_mark_q(m_stmts);
}
c_FuncDecl::c_FuncDecl(){
	m_retType=0;
	m_argDecls=Array<c_ArgDecl* >();
	m_overrides=0;
}
bool c_FuncDecl::p_IsCtor(){
	return (m_attrs&2)!=0;
}
c_FuncDecl* c_FuncDecl::m_new(String t_ident,int t_attrs,c_Type* t_retType,Array<c_ArgDecl* > t_argDecls){
	c_BlockDecl::m_new2();
	this->m_ident=t_ident;
	this->m_attrs=t_attrs;
	gc_assign(this->m_retType,t_retType);
	gc_assign(this->m_argDecls,t_argDecls);
	return this;
}
c_FuncDecl* c_FuncDecl::m_new2(){
	c_BlockDecl::m_new2();
	return this;
}
bool c_FuncDecl::p_IsMethod(){
	return (m_attrs&1)!=0;
}
String c_FuncDecl::p_ToString(){
	String t_t=String();
	Array<c_ArgDecl* > t_=m_argDecls;
	int t_2=0;
	while(t_2<t_.Length()){
		c_ArgDecl* t_decl=t_[t_2];
		t_2=t_2+1;
		if((t_t).Length()!=0){
			t_t=t_t+String(L",",1);
		}
		t_t=t_t+t_decl->p_ToString();
	}
	String t_q=String();
	if(p_IsCtor()){
		t_q=String(L"Method ",7)+c_Decl::p_ToString();
	}else{
		if(p_IsMethod()){
			t_q=String(L"Method ",7);
		}else{
			t_q=String(L"Function ",9);
		}
		t_q=t_q+(c_Decl::p_ToString()+String(L":",1));
		t_q=t_q+m_retType->p_ToString();
	}
	return t_q+String(L"(",1)+t_t+String(L")",1);
}
bool c_FuncDecl::p_EqualsArgs(c_FuncDecl* t_decl){
	if(m_argDecls.Length()!=t_decl->m_argDecls.Length()){
		return false;
	}
	for(int t_i=0;t_i<m_argDecls.Length();t_i=t_i+1){
		if(!((m_argDecls[t_i]->m_type->p_EqualsType(t_decl->m_argDecls[t_i]->m_type))!=0)){
			return false;
		}
	}
	return true;
}
bool c_FuncDecl::p_EqualsFunc(c_FuncDecl* t_decl){
	return ((m_retType->p_EqualsType(t_decl->m_retType))!=0) && p_EqualsArgs(t_decl);
}
c_Decl* c_FuncDecl::p_OnCopy(){
	Array<c_ArgDecl* > t_args=m_argDecls.Slice(0);
	for(int t_i=0;t_i<t_args.Length();t_i=t_i+1){
		gc_assign(t_args[t_i],dynamic_cast<c_ArgDecl*>(t_args[t_i]->p_Copy()));
	}
	c_FuncDecl* t_t=(new c_FuncDecl)->m_new(m_ident,m_attrs,m_retType,t_args);
	c_Enumerator5* t_=m_stmts->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_Stmt* t_stmt=t_->p_NextObject();
		t_t->p_AddStmt(t_stmt->p_Copy2(t_t));
	}
	return (t_t);
}
int c_FuncDecl::p_OnSemant(){
	c_ClassDecl* t_cdecl=p_ClassScope();
	c_ClassDecl* t_sclass=0;
	if((t_cdecl)!=0){
		t_sclass=t_cdecl->m_superClass;
	}
	if(p_IsCtor()){
		gc_assign(m_retType,(t_cdecl->m_objectType));
	}else{
		gc_assign(m_retType,m_retType->p_Semant());
	}
	Array<c_ArgDecl* > t_=m_argDecls;
	int t_2=0;
	while(t_2<t_.Length()){
		c_ArgDecl* t_arg=t_[t_2];
		t_2=t_2+1;
		p_InsertDecl(t_arg);
		t_arg->p_Semant();
	}
	c_Enumerator3* t_3=m_scope->p_SemantedFuncs(m_ident)->p_ObjectEnumerator();
	while(t_3->p_HasNext()){
		c_FuncDecl* t_decl=t_3->p_NextObject();
		if(t_decl!=this && p_EqualsArgs(t_decl)){
			bb_config_Err(String(L"Duplicate declaration ",22)+p_ToString());
		}
	}
	if(p_IsCtor() && !((m_attrs&8)!=0)){
		if((t_sclass->p_FindFuncDecl(String(L"new",3),Array<c_Expr* >(),0))!=0){
			c_InvokeSuperExpr* t_expr=(new c_InvokeSuperExpr)->m_new(String(L"new",3),Array<c_Expr* >());
			m_stmts->p_AddFirst((new c_ExprStmt)->m_new(t_expr));
		}
	}
	if(((t_sclass)!=0) && p_IsMethod()){
		while((t_sclass)!=0){
			int t_found=0;
			c_Enumerator3* t_4=t_sclass->p_MethodDecls(m_ident)->p_ObjectEnumerator();
			while(t_4->p_HasNext()){
				c_FuncDecl* t_decl2=t_4->p_NextObject();
				t_found=1;
				t_decl2->p_Semant();
				if(p_EqualsFunc(t_decl2)){
					gc_assign(m_overrides,t_decl2);
					t_decl2->m_attrs|=16;
					break;
				}
			}
			if((t_found)!=0){
				if(!((m_overrides)!=0)){
					bb_config_Err(String(L"Overriding method does not match any overridden method.",55));
				}
				if((m_overrides->p_IsFinal())!=0){
					bb_config_Err(String(L"Cannot override final method.",29));
				}
				if((m_overrides->m_munged).Length()!=0){
					if(((m_munged).Length()!=0) && m_munged!=m_overrides->m_munged){
						bb_config_InternalErr(String(L"Internal error",14));
					}
					m_munged=m_overrides->m_munged;
				}
				break;
			}
			t_sclass=t_sclass->m_superClass;
		}
	}
	m_attrs|=1048576;
	c_BlockDecl::p_OnSemant();
	return 0;
}
bool c_FuncDecl::p_IsStatic(){
	return (m_attrs&3)==0;
}
bool c_FuncDecl::p_IsProperty(){
	return (m_attrs&4)!=0;
}
bool c_FuncDecl::p_IsVirtual(){
	return (m_attrs&1040)!=0;
}
void c_FuncDecl::mark(){
	c_BlockDecl::mark();
	gc_mark_q(m_retType);
	gc_mark_q(m_argDecls);
	gc_mark_q(m_overrides);
}
c_List4::c_List4(){
	m__head=((new c_HeadNode4)->m_new());
}
c_List4* c_List4::m_new(){
	return this;
}
c_Node9* c_List4::p_AddLast4(c_FuncDecl* t_data){
	return (new c_Node9)->m_new(m__head,m__head->m__pred,t_data);
}
c_List4* c_List4::m_new2(Array<c_FuncDecl* > t_data){
	Array<c_FuncDecl* > t_=t_data;
	int t_2=0;
	while(t_2<t_.Length()){
		c_FuncDecl* t_t=t_[t_2];
		t_2=t_2+1;
		p_AddLast4(t_t);
	}
	return this;
}
c_Enumerator3* c_List4::p_ObjectEnumerator(){
	return (new c_Enumerator3)->m_new(this);
}
void c_List4::mark(){
	Object::mark();
	gc_mark_q(m__head);
}
c_FuncDeclList::c_FuncDeclList(){
}
c_FuncDeclList* c_FuncDeclList::m_new(){
	c_List4::m_new();
	return this;
}
void c_FuncDeclList::mark(){
	c_List4::mark();
}
c_Node9::c_Node9(){
	m__succ=0;
	m__pred=0;
	m__data=0;
}
c_Node9* c_Node9::m_new(c_Node9* t_succ,c_Node9* t_pred,c_FuncDecl* t_data){
	gc_assign(m__succ,t_succ);
	gc_assign(m__pred,t_pred);
	gc_assign(m__succ->m__pred,this);
	gc_assign(m__pred->m__succ,this);
	gc_assign(m__data,t_data);
	return this;
}
c_Node9* c_Node9::m_new2(){
	return this;
}
void c_Node9::mark(){
	Object::mark();
	gc_mark_q(m__succ);
	gc_mark_q(m__pred);
	gc_mark_q(m__data);
}
c_HeadNode4::c_HeadNode4(){
}
c_HeadNode4* c_HeadNode4::m_new(){
	c_Node9::m_new2();
	gc_assign(m__succ,(this));
	gc_assign(m__pred,(this));
	return this;
}
void c_HeadNode4::mark(){
	c_Node9::mark();
}
c_ClassDecl::c_ClassDecl(){
	m_superClass=0;
	m_args=Array<String >();
	m_superTy=0;
	m_impltys=Array<c_IdentType* >();
	m_objectType=0;
	m_instances=0;
	m_instanceof=0;
	m_instArgs=Array<c_Type* >();
	m_implmentsAll=Array<c_ClassDecl* >();
	m_implments=Array<c_ClassDecl* >();
}
c_ClassDecl* c_ClassDecl::m_new(String t_ident,int t_attrs,Array<String > t_args,c_IdentType* t_superTy,Array<c_IdentType* > t_impls){
	c_ScopeDecl::m_new();
	this->m_ident=t_ident;
	this->m_attrs=t_attrs;
	gc_assign(this->m_args,t_args);
	gc_assign(this->m_superTy,t_superTy);
	gc_assign(this->m_impltys,t_impls);
	gc_assign(this->m_objectType,(new c_ObjectType)->m_new(this));
	if((t_args).Length()!=0){
		gc_assign(m_instances,(new c_List6)->m_new());
	}
	return this;
}
c_ClassDecl* c_ClassDecl::m_new2(){
	c_ScopeDecl::m_new();
	return this;
}
int c_ClassDecl::p_IsInterface(){
	return (((m_attrs&4096)!=0)?1:0);
}
String c_ClassDecl::p_ToString(){
	String t_t=String();
	if((m_args).Length()!=0){
		t_t=String(L",",1).Join(m_args);
	}else{
		if((m_instArgs).Length()!=0){
			Array<c_Type* > t_=m_instArgs;
			int t_2=0;
			while(t_2<t_.Length()){
				c_Type* t_arg=t_[t_2];
				t_2=t_2+1;
				if((t_t).Length()!=0){
					t_t=t_t+String(L",",1);
				}
				t_t=t_t+t_arg->p_ToString();
			}
		}
	}
	if((t_t).Length()!=0){
		t_t=String(L"<",1)+t_t+String(L">",1);
	}
	return m_ident+t_t;
}
c_FuncDecl* c_ClassDecl::p_FindFuncDecl2(String t_ident,Array<c_Expr* > t_args,int t_explicit){
	return c_ScopeDecl::p_FindFuncDecl(t_ident,t_args,t_explicit);
}
c_FuncDecl* c_ClassDecl::p_FindFuncDecl(String t_ident,Array<c_Expr* > t_args,int t_explicit){
	if(!((p_IsInterface())!=0)){
		return p_FindFuncDecl2(t_ident,t_args,t_explicit);
	}
	c_FuncDecl* t_fdecl=p_FindFuncDecl2(t_ident,t_args,1);
	Array<c_ClassDecl* > t_=m_implmentsAll;
	int t_2=0;
	while(t_2<t_.Length()){
		c_ClassDecl* t_iface=t_[t_2];
		t_2=t_2+1;
		c_FuncDecl* t_decl=t_iface->p_FindFuncDecl2(t_ident,t_args,1);
		if(!((t_decl)!=0)){
			continue;
		}
		if((t_fdecl)!=0){
			if(t_fdecl->p_EqualsFunc(t_decl)){
				continue;
			}
			bb_config_Err(String(L"Unable to determine overload to use: ",37)+t_fdecl->p_ToString()+String(L" or ",4)+t_decl->p_ToString()+String(L".",1));
		}
		t_fdecl=t_decl;
	}
	if(((t_fdecl)!=0) || ((t_explicit)!=0)){
		return t_fdecl;
	}
	t_fdecl=p_FindFuncDecl2(t_ident,t_args,0);
	Array<c_ClassDecl* > t_3=m_implmentsAll;
	int t_4=0;
	while(t_4<t_3.Length()){
		c_ClassDecl* t_iface2=t_3[t_4];
		t_4=t_4+1;
		c_FuncDecl* t_decl2=t_iface2->p_FindFuncDecl2(t_ident,t_args,0);
		if(!((t_decl2)!=0)){
			continue;
		}
		if((t_fdecl)!=0){
			if(t_fdecl->p_EqualsFunc(t_decl2)){
				continue;
			}
			bb_config_Err(String(L"Unable to determine overload to use: ",37)+t_fdecl->p_ToString()+String(L" or ",4)+t_decl2->p_ToString()+String(L".",1));
		}
		t_fdecl=t_decl2;
	}
	return t_fdecl;
}
int c_ClassDecl::p_ExtendsObject(){
	return (((m_attrs&2)!=0)?1:0);
}
c_ClassDecl* c_ClassDecl::p_GenClassInstance(Array<c_Type* > t_instArgs){
	if((m_instanceof)!=0){
		bb_config_InternalErr(String(L"Internal error",14));
	}
	if(!((t_instArgs).Length()!=0)){
		if(!((m_args).Length()!=0)){
			return this;
		}
		c_Enumerator4* t_=m_instances->p_ObjectEnumerator();
		while(t_->p_HasNext()){
			c_ClassDecl* t_inst=t_->p_NextObject();
			if(bb_decl__env->p_ClassScope()==t_inst){
				return t_inst;
			}
		}
	}
	if(m_args.Length()!=t_instArgs.Length()){
		bb_config_Err(String(L"Wrong number of type arguments for class ",41)+p_ToString());
	}
	c_Enumerator4* t_2=m_instances->p_ObjectEnumerator();
	while(t_2->p_HasNext()){
		c_ClassDecl* t_inst2=t_2->p_NextObject();
		int t_equal=1;
		for(int t_i=0;t_i<m_args.Length();t_i=t_i+1){
			if(!((t_inst2->m_instArgs[t_i]->p_EqualsType(t_instArgs[t_i]))!=0)){
				t_equal=0;
				break;
			}
		}
		if((t_equal)!=0){
			return t_inst2;
		}
	}
	c_ClassDecl* t_inst3=(new c_ClassDecl)->m_new(m_ident,m_attrs,Array<String >(),m_superTy,m_impltys);
	t_inst3->m_attrs&=-1048577;
	t_inst3->m_munged=m_munged;
	t_inst3->m_errInfo=m_errInfo;
	gc_assign(t_inst3->m_scope,m_scope);
	gc_assign(t_inst3->m_instanceof,this);
	gc_assign(t_inst3->m_instArgs,t_instArgs);
	m_instances->p_AddLast6(t_inst3);
	for(int t_i2=0;t_i2<m_args.Length();t_i2=t_i2+1){
		t_inst3->p_InsertDecl((new c_AliasDecl)->m_new(m_args[t_i2],0,(t_instArgs[t_i2])));
	}
	c_Enumerator2* t_3=m_decls->p_ObjectEnumerator();
	while(t_3->p_HasNext()){
		c_Decl* t_decl=t_3->p_NextObject();
		t_inst3->p_InsertDecl(t_decl->p_Copy());
	}
	return t_inst3;
}
int c_ClassDecl::p_IsFinalized(){
	return (((m_attrs&4)!=0)?1:0);
}
int c_ClassDecl::p_UpdateLiveMethods(){
	if((p_IsFinalized())!=0){
		return 0;
	}
	if((p_IsInterface())!=0){
		return 0;
	}
	if(!((m_superClass)!=0)){
		return 0;
	}
	int t_n=0;
	c_Enumerator3* t_=p_MethodDecls(String())->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_FuncDecl* t_decl=t_->p_NextObject();
		if((t_decl->p_IsSemanted())!=0){
			continue;
		}
		int t_live=0;
		c_List4* t_unsem=(new c_List4)->m_new();
		t_unsem->p_AddLast4(t_decl);
		c_ClassDecl* t_sclass=m_superClass;
		while((t_sclass)!=0){
			c_Enumerator3* t_2=t_sclass->p_MethodDecls(t_decl->m_ident)->p_ObjectEnumerator();
			while(t_2->p_HasNext()){
				c_FuncDecl* t_decl2=t_2->p_NextObject();
				if((t_decl2->p_IsSemanted())!=0){
					t_live=1;
				}else{
					t_unsem->p_AddLast4(t_decl2);
					if((t_decl2->p_IsExtern())!=0){
						t_live=1;
					}
					if((t_decl2->p_IsSemanted())!=0){
						t_live=1;
					}
				}
			}
			t_sclass=t_sclass->m_superClass;
		}
		if(!((t_live)!=0)){
			c_ClassDecl* t_cdecl=this;
			while((t_cdecl)!=0){
				Array<c_ClassDecl* > t_3=t_cdecl->m_implmentsAll;
				int t_4=0;
				while(t_4<t_3.Length()){
					c_ClassDecl* t_iface=t_3[t_4];
					t_4=t_4+1;
					c_Enumerator3* t_5=t_iface->p_MethodDecls(t_decl->m_ident)->p_ObjectEnumerator();
					while(t_5->p_HasNext()){
						c_FuncDecl* t_decl22=t_5->p_NextObject();
						if((t_decl22->p_IsSemanted())!=0){
							t_live=1;
						}else{
							t_unsem->p_AddLast4(t_decl22);
							if((t_decl22->p_IsExtern())!=0){
								t_live=1;
							}
							if((t_decl22->p_IsSemanted())!=0){
								t_live=1;
							}
						}
					}
				}
				t_cdecl=t_cdecl->m_superClass;
			}
		}
		if(!((t_live)!=0)){
			continue;
		}
		c_Enumerator3* t_6=t_unsem->p_ObjectEnumerator();
		while(t_6->p_HasNext()){
			c_FuncDecl* t_decl3=t_6->p_NextObject();
			t_decl3->p_Semant();
			t_n+=1;
		}
	}
	return t_n;
}
int c_ClassDecl::p_IsInstanced(){
	return (((m_attrs&1)!=0)?1:0);
}
int c_ClassDecl::p_FinalizeClass(){
	if((p_IsFinalized())!=0){
		return 0;
	}
	m_attrs|=4;
	if((p_IsInterface())!=0){
		return 0;
	}
	bb_config_PushErr(m_errInfo);
	c_Enumerator2* t_=p_Semanted()->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_Decl* t_decl=t_->p_NextObject();
		c_FieldDecl* t_fdecl=dynamic_cast<c_FieldDecl*>(t_decl);
		if(!((t_fdecl)!=0)){
			continue;
		}
		c_ClassDecl* t_cdecl=m_superClass;
		while((t_cdecl)!=0){
			c_Enumerator2* t_2=t_cdecl->p_Semanted()->p_ObjectEnumerator();
			while(t_2->p_HasNext()){
				c_Decl* t_decl2=t_2->p_NextObject();
				if(t_decl2->m_ident==t_fdecl->m_ident){
					bb_config__errInfo=t_fdecl->m_errInfo;
					bb_config_Err(String(L"Field '",7)+t_fdecl->m_ident+String(L"' in class ",11)+p_ToString()+String(L" overrides existing declaration in class ",41)+t_cdecl->p_ToString());
				}
			}
			t_cdecl=t_cdecl->m_superClass;
		}
	}
	if((p_IsAbstract())!=0){
		if((p_IsInstanced())!=0){
			bb_config_Err(String(L"Can't create instance of abstract class ",40)+p_ToString()+String(L".",1));
		}
	}else{
		c_ClassDecl* t_cdecl2=this;
		c_List4* t_impls=(new c_List4)->m_new();
		while(((t_cdecl2)!=0) && !((p_IsAbstract())!=0)){
			c_Enumerator3* t_3=t_cdecl2->p_SemantedMethods(String())->p_ObjectEnumerator();
			while(t_3->p_HasNext()){
				c_FuncDecl* t_decl3=t_3->p_NextObject();
				if((t_decl3->p_IsAbstract())!=0){
					int t_found=0;
					c_Enumerator3* t_4=t_impls->p_ObjectEnumerator();
					while(t_4->p_HasNext()){
						c_FuncDecl* t_decl22=t_4->p_NextObject();
						if(t_decl3->m_ident==t_decl22->m_ident && t_decl3->p_EqualsFunc(t_decl22)){
							t_found=1;
							break;
						}
					}
					if(!((t_found)!=0)){
						if((p_IsInstanced())!=0){
							bb_config_Err(String(L"Can't create instance of class ",31)+p_ToString()+String(L" due to abstract method ",24)+t_decl3->p_ToString()+String(L".",1));
						}
						m_attrs|=1024;
						break;
					}
				}else{
					t_impls->p_AddLast4(t_decl3);
				}
			}
			t_cdecl2=t_cdecl2->m_superClass;
		}
	}
	Array<c_ClassDecl* > t_5=m_implmentsAll;
	int t_6=0;
	while(t_6<t_5.Length()){
		c_ClassDecl* t_iface=t_5[t_6];
		t_6=t_6+1;
		c_ClassDecl* t_cdecl3=m_superClass;
		bool t_found2=false;
		while((t_cdecl3)!=0){
			Array<c_ClassDecl* > t_7=t_cdecl3->m_implmentsAll;
			int t_8=0;
			while(t_8<t_7.Length()){
				c_ClassDecl* t_iface2=t_7[t_8];
				t_8=t_8+1;
				if(t_iface!=t_iface2){
					continue;
				}
				t_found2=true;
				break;
			}
			if(t_found2){
				break;
			}
			t_cdecl3=t_cdecl3->m_superClass;
		}
		if(t_found2){
			continue;
		}
		c_Enumerator3* t_9=t_iface->p_SemantedMethods(String())->p_ObjectEnumerator();
		while(t_9->p_HasNext()){
			c_FuncDecl* t_decl4=t_9->p_NextObject();
			bool t_found3=false;
			c_Enumerator3* t_10=p_SemantedMethods(t_decl4->m_ident)->p_ObjectEnumerator();
			while(t_10->p_HasNext()){
				c_FuncDecl* t_decl23=t_10->p_NextObject();
				if(t_decl4->p_EqualsFunc(t_decl23)){
					if((t_decl23->m_munged).Length()!=0){
						bb_config_Err(String(L"Extern methods cannot be used to implement interface methods.",61));
					}
					t_found3=true;
				}
			}
			if(!t_found3){
				bb_config_Err(t_decl4->p_ToString()+String(L" must be implemented by class ",30)+p_ToString());
			}
		}
	}
	bb_config_PopErr();
	return 0;
}
c_Decl* c_ClassDecl::p_OnCopy(){
	bb_config_InternalErr(String(L"Internal error",14));
	return 0;
}
Object* c_ClassDecl::p_GetDecl2(String t_ident){
	return c_ScopeDecl::p_GetDecl(t_ident);
}
Object* c_ClassDecl::p_GetDecl(String t_ident){
	c_ClassDecl* t_cdecl=this;
	while((t_cdecl)!=0){
		Object* t_decl=t_cdecl->p_GetDecl2(t_ident);
		if((t_decl)!=0){
			return t_decl;
		}
		t_cdecl=t_cdecl->m_superClass;
	}
	return 0;
}
c_ClassDecl* c_ClassDecl::m_nullObjectClass;
int c_ClassDecl::p_IsThrowable(){
	return (((m_attrs&8192)!=0)?1:0);
}
int c_ClassDecl::p_OnSemant(){
	if((m_args).Length()!=0){
		return 0;
	}
	bb_decl_PushEnv(this);
	if((m_superTy)!=0){
		gc_assign(m_superClass,m_superTy->p_SemantClass());
		if((m_superClass->p_IsFinal())!=0){
			bb_config_Err(String(L"Cannot extend final class.",26));
		}
		if((m_superClass->p_IsInterface())!=0){
			bb_config_Err(String(L"Cannot extend an interface.",27));
		}
		if(m_munged==String(L"ThrowableObject",15) || ((m_superClass->p_IsThrowable())!=0)){
			m_attrs|=8192;
		}
		if((m_superClass->p_ExtendsObject())!=0){
			m_attrs|=2;
		}
	}else{
		if(m_munged==String(L"Object",6)){
			m_attrs|=2;
		}
	}
	Array<c_ClassDecl* > t_impls=Array<c_ClassDecl* >(m_impltys.Length());
	c_Stack8* t_implsall=(new c_Stack8)->m_new();
	for(int t_i=0;t_i<m_impltys.Length();t_i=t_i+1){
		c_ClassDecl* t_cdecl=m_impltys[t_i]->p_SemantClass();
		if(!((t_cdecl->p_IsInterface())!=0)){
			bb_config_Err(t_cdecl->p_ToString()+String(L" is a class, not an interface.",30));
		}
		for(int t_j=0;t_j<t_i;t_j=t_j+1){
			if(t_impls[t_j]==t_cdecl){
				bb_config_Err(String(L"Duplicate interface ",20)+t_cdecl->p_ToString()+String(L".",1));
			}
		}
		gc_assign(t_impls[t_i],t_cdecl);
		t_implsall->p_Push22(t_cdecl);
		Array<c_ClassDecl* > t_=t_cdecl->m_implmentsAll;
		int t_2=0;
		while(t_2<t_.Length()){
			c_ClassDecl* t_tdecl=t_[t_2];
			t_2=t_2+1;
			t_implsall->p_Push22(t_tdecl);
		}
	}
	gc_assign(m_implmentsAll,Array<c_ClassDecl* >(t_implsall->p_Length2()));
	for(int t_i2=0;t_i2<t_implsall->p_Length2();t_i2=t_i2+1){
		gc_assign(m_implmentsAll[t_i2],t_implsall->p_Get2(t_i2));
	}
	gc_assign(m_implments,t_impls);
	bb_decl_PopEnv();
	if(!((p_IsAbstract())!=0)){
		c_Enumerator2* t_3=m_decls->p_ObjectEnumerator();
		while(t_3->p_HasNext()){
			c_Decl* t_decl=t_3->p_NextObject();
			c_FuncDecl* t_fdecl=dynamic_cast<c_FuncDecl*>(t_decl);
			if(((t_fdecl)!=0) && ((t_fdecl->p_IsAbstract())!=0)){
				m_attrs|=1024;
				break;
			}
		}
	}
	if(!((p_IsExtern())!=0) && !((p_IsInterface())!=0)){
		c_FuncDecl* t_fdecl2=0;
		c_Enumerator3* t_4=p_FuncDecls(String())->p_ObjectEnumerator();
		while(t_4->p_HasNext()){
			c_FuncDecl* t_decl2=t_4->p_NextObject();
			if(!t_decl2->p_IsCtor()){
				continue;
			}
			int t_nargs=0;
			Array<c_ArgDecl* > t_5=t_decl2->m_argDecls;
			int t_6=0;
			while(t_6<t_5.Length()){
				c_ArgDecl* t_arg=t_5[t_6];
				t_6=t_6+1;
				if(!((t_arg->m_init)!=0)){
					t_nargs+=1;
				}
			}
			if((t_nargs)!=0){
				continue;
			}
			t_fdecl2=t_decl2;
			break;
		}
		if(!((t_fdecl2)!=0)){
			t_fdecl2=(new c_FuncDecl)->m_new(String(L"new",3),2,(m_objectType),Array<c_ArgDecl* >());
			t_fdecl2->p_AddStmt((new c_ReturnStmt)->m_new(0));
			p_InsertDecl(t_fdecl2);
		}
	}
	p_AppScope()->m_semantedClasses->p_AddLast6(this);
	return 0;
}
int c_ClassDecl::p_ExtendsClass(c_ClassDecl* t_cdecl){
	if(this==m_nullObjectClass){
		return 1;
	}
	c_ClassDecl* t_tdecl=this;
	while((t_tdecl)!=0){
		if(t_tdecl==t_cdecl){
			return 1;
		}
		if((t_cdecl->p_IsInterface())!=0){
			Array<c_ClassDecl* > t_=t_tdecl->m_implmentsAll;
			int t_2=0;
			while(t_2<t_.Length()){
				c_ClassDecl* t_iface=t_[t_2];
				t_2=t_2+1;
				if(t_iface==t_cdecl){
					return 1;
				}
			}
		}
		t_tdecl=t_tdecl->m_superClass;
	}
	return 0;
}
void c_ClassDecl::mark(){
	c_ScopeDecl::mark();
	gc_mark_q(m_superClass);
	gc_mark_q(m_args);
	gc_mark_q(m_superTy);
	gc_mark_q(m_impltys);
	gc_mark_q(m_objectType);
	gc_mark_q(m_instances);
	gc_mark_q(m_instanceof);
	gc_mark_q(m_instArgs);
	gc_mark_q(m_implmentsAll);
	gc_mark_q(m_implments);
}
int bb_decl_PopEnv(){
	if(bb_decl__envStack->p_IsEmpty()){
		bb_config_InternalErr(String(L"Internal error",14));
	}
	gc_assign(bb_decl__env,bb_decl__envStack->p_RemoveLast());
	return 0;
}
c_VoidType::c_VoidType(){
}
c_VoidType* c_VoidType::m_new(){
	c_Type::m_new();
	return this;
}
int c_VoidType::p_EqualsType(c_Type* t_ty){
	return ((dynamic_cast<c_VoidType*>(t_ty)!=0)?1:0);
}
String c_VoidType::p_ToString(){
	return String(L"Void",4);
}
void c_VoidType::mark(){
	c_Type::mark();
}
c_IdentType::c_IdentType(){
	m_ident=String();
	m_args=Array<c_Type* >();
}
c_IdentType* c_IdentType::m_new(String t_ident,Array<c_Type* > t_args){
	c_Type::m_new();
	this->m_ident=t_ident;
	gc_assign(this->m_args,t_args);
	return this;
}
c_IdentType* c_IdentType::m_new2(){
	c_Type::m_new();
	return this;
}
c_Type* c_IdentType::p_Semant(){
	if(!((m_ident).Length()!=0)){
		return (c_ClassDecl::m_nullObjectClass->m_objectType);
	}
	Array<c_Type* > t_targs=Array<c_Type* >(m_args.Length());
	for(int t_i=0;t_i<m_args.Length();t_i=t_i+1){
		gc_assign(t_targs[t_i],m_args[t_i]->p_Semant());
	}
	String t_tyid=String();
	c_Type* t_type=0;
	int t_i2=m_ident.Find(String(L".",1),0);
	if(t_i2==-1){
		t_tyid=m_ident;
		t_type=bb_decl__env->p_FindType(t_tyid,t_targs);
	}else{
		String t_modid=m_ident.Slice(0,t_i2);
		c_ModuleDecl* t_mdecl=bb_decl__env->p_FindModuleDecl(t_modid);
		if(!((t_mdecl)!=0)){
			bb_config_Err(String(L"Module '",8)+t_modid+String(L"' not found",11));
		}
		t_tyid=m_ident.Slice(t_i2+1);
		t_type=t_mdecl->p_FindType(t_tyid,t_targs);
	}
	if(!((t_type)!=0)){
		bb_config_Err(String(L"Type '",6)+t_tyid+String(L"' not found",11));
	}
	return t_type;
}
c_ClassDecl* c_IdentType::p_SemantClass(){
	c_ObjectType* t_type=dynamic_cast<c_ObjectType*>(p_Semant());
	if(!((t_type)!=0)){
		bb_config_Err(String(L"Type is not a class",19));
	}
	return t_type->m_classDecl;
}
int c_IdentType::p_EqualsType(c_Type* t_ty){
	bb_config_InternalErr(String(L"Internal error",14));
	return 0;
}
int c_IdentType::p_ExtendsType(c_Type* t_ty){
	bb_config_InternalErr(String(L"Internal error",14));
	return 0;
}
String c_IdentType::p_ToString(){
	String t_t=String();
	Array<c_Type* > t_=m_args;
	int t_2=0;
	while(t_2<t_.Length()){
		c_Type* t_arg=t_[t_2];
		t_2=t_2+1;
		if((t_t).Length()!=0){
			t_t=t_t+String(L",",1);
		}
		t_t=t_t+t_arg->p_ToString();
	}
	if((t_t).Length()!=0){
		return String(L"$",1)+m_ident+String(L"<",1)+t_t.Replace(String(L"$",1),String())+String(L">",1);
	}
	return String(L"$",1)+m_ident;
}
void c_IdentType::mark(){
	c_Type::mark();
	gc_mark_q(m_args);
}
c_Stack3::c_Stack3(){
	m_data=Array<c_Type* >();
	m_length=0;
}
c_Stack3* c_Stack3::m_new(){
	return this;
}
c_Stack3* c_Stack3::m_new2(Array<c_Type* > t_data){
	gc_assign(this->m_data,t_data.Slice(0));
	this->m_length=t_data.Length();
	return this;
}
void c_Stack3::p_Push7(c_Type* t_value){
	if(m_length==m_data.Length()){
		gc_assign(m_data,m_data.Resize(m_length*2+10));
	}
	gc_assign(m_data[m_length],t_value);
	m_length+=1;
}
void c_Stack3::p_Push8(Array<c_Type* > t_values,int t_offset,int t_count){
	for(int t_i=0;t_i<t_count;t_i=t_i+1){
		p_Push7(t_values[t_offset+t_i]);
	}
}
void c_Stack3::p_Push9(Array<c_Type* > t_values,int t_offset){
	p_Push8(t_values,t_offset,t_values.Length()-t_offset);
}
Array<c_Type* > c_Stack3::p_ToArray(){
	Array<c_Type* > t_t=Array<c_Type* >(m_length);
	for(int t_i=0;t_i<m_length;t_i=t_i+1){
		gc_assign(t_t[t_i],m_data[t_i]);
	}
	return t_t;
}
void c_Stack3::mark(){
	Object::mark();
	gc_mark_q(m_data);
}
c_ArrayType::c_ArrayType(){
	m_elemType=0;
}
c_ArrayType* c_ArrayType::m_new(c_Type* t_elemType){
	c_Type::m_new();
	gc_assign(this->m_elemType,t_elemType);
	return this;
}
c_ArrayType* c_ArrayType::m_new2(){
	c_Type::m_new();
	return this;
}
int c_ArrayType::p_EqualsType(c_Type* t_ty){
	c_ArrayType* t_arrayType=dynamic_cast<c_ArrayType*>(t_ty);
	return ((((t_arrayType)!=0) && ((m_elemType->p_EqualsType(t_arrayType->m_elemType))!=0))?1:0);
}
int c_ArrayType::p_ExtendsType(c_Type* t_ty){
	c_ArrayType* t_arrayType=dynamic_cast<c_ArrayType*>(t_ty);
	return ((((t_arrayType)!=0) && (((dynamic_cast<c_VoidType*>(m_elemType))!=0) || ((m_elemType->p_EqualsType(t_arrayType->m_elemType))!=0)))?1:0);
}
c_Type* c_ArrayType::p_Semant(){
	c_Type* t_ty=m_elemType->p_Semant();
	if(t_ty!=m_elemType){
		return ((new c_ArrayType)->m_new(t_ty));
	}
	return (this);
}
c_ClassDecl* c_ArrayType::p_GetClass(){
	return dynamic_cast<c_ClassDecl*>(bb_decl__env->p_FindDecl(String(L"array",5)));
}
String c_ArrayType::p_ToString(){
	return m_elemType->p_ToString()+String(L"[]",2);
}
void c_ArrayType::mark(){
	c_Type::mark();
	gc_mark_q(m_elemType);
}
c_UnaryExpr::c_UnaryExpr(){
	m_op=String();
	m_expr=0;
}
c_UnaryExpr* c_UnaryExpr::m_new(String t_op,c_Expr* t_expr){
	c_Expr::m_new();
	this->m_op=t_op;
	gc_assign(this->m_expr,t_expr);
	return this;
}
c_UnaryExpr* c_UnaryExpr::m_new2(){
	c_Expr::m_new();
	return this;
}
c_Expr* c_UnaryExpr::p_Copy(){
	return ((new c_UnaryExpr)->m_new(m_op,p_CopyExpr(m_expr)));
}
c_Expr* c_UnaryExpr::p_Semant(){
	if((m_exprType)!=0){
		return (this);
	}
	String t_1=m_op;
	if(t_1==String(L"+",1) || t_1==String(L"-",1)){
		gc_assign(m_expr,m_expr->p_Semant());
		if(!((dynamic_cast<c_NumericType*>(m_expr->m_exprType))!=0)){
			bb_config_Err(m_expr->p_ToString()+String(L" must be numeric for use with unary operator '",46)+m_op+String(L"'",1));
		}
		gc_assign(m_exprType,m_expr->m_exprType);
	}else{
		if(t_1==String(L"~",1)){
			gc_assign(m_expr,m_expr->p_Semant2((c_Type::m_intType),0));
			gc_assign(m_exprType,(c_Type::m_intType));
		}else{
			if(t_1==String(L"not",3)){
				gc_assign(m_expr,m_expr->p_Semant2((c_Type::m_boolType),1));
				gc_assign(m_exprType,(c_Type::m_boolType));
			}else{
				bb_config_InternalErr(String(L"Internal error",14));
			}
		}
	}
	if((dynamic_cast<c_ConstExpr*>(m_expr))!=0){
		return p_EvalConst();
	}
	return (this);
}
String c_UnaryExpr::p_Eval(){
	String t_val=m_expr->p_Eval();
	String t_2=m_op;
	if(t_2==String(L"~",1)){
		return String(~(t_val).ToInt());
	}else{
		if(t_2==String(L"+",1)){
			return t_val;
		}else{
			if(t_2==String(L"-",1)){
				if(t_val.StartsWith(String(L"-",1))){
					return t_val.Slice(1);
				}
				return String(L"-",1)+t_val;
			}else{
				if(t_2==String(L"not",3)){
					if((t_val).Length()!=0){
						return String();
					}
					return String(L"1",1);
				}
			}
		}
	}
	bb_config_InternalErr(String(L"Internal error",14));
	return String();
}
String c_UnaryExpr::p_Trans(){
	return bb_translator__trans->p_TransUnaryExpr(this);
}
void c_UnaryExpr::mark(){
	c_Expr::mark();
	gc_mark_q(m_expr);
}
c_ArrayExpr::c_ArrayExpr(){
	m_exprs=Array<c_Expr* >();
}
c_ArrayExpr* c_ArrayExpr::m_new(Array<c_Expr* > t_exprs){
	c_Expr::m_new();
	gc_assign(this->m_exprs,t_exprs);
	return this;
}
c_ArrayExpr* c_ArrayExpr::m_new2(){
	c_Expr::m_new();
	return this;
}
c_Expr* c_ArrayExpr::p_Copy(){
	return ((new c_ArrayExpr)->m_new(p_CopyArgs(m_exprs)));
}
c_Expr* c_ArrayExpr::p_Semant(){
	if((m_exprType)!=0){
		return (this);
	}
	gc_assign(m_exprs[0],m_exprs[0]->p_Semant());
	c_Type* t_ty=m_exprs[0]->m_exprType;
	for(int t_i=1;t_i<m_exprs.Length();t_i=t_i+1){
		gc_assign(m_exprs[t_i],m_exprs[t_i]->p_Semant());
		t_ty=p_BalanceTypes(t_ty,m_exprs[t_i]->m_exprType);
	}
	for(int t_i2=0;t_i2<m_exprs.Length();t_i2=t_i2+1){
		gc_assign(m_exprs[t_i2],m_exprs[t_i2]->p_Cast(t_ty,0));
	}
	gc_assign(m_exprType,(t_ty->p_ArrayOf()));
	return (this);
}
String c_ArrayExpr::p_Trans(){
	return bb_translator__trans->p_TransArrayExpr(this);
}
void c_ArrayExpr::mark(){
	c_Expr::mark();
	gc_mark_q(m_exprs);
}
c_Stack4::c_Stack4(){
	m_data=Array<c_Expr* >();
	m_length=0;
}
c_Stack4* c_Stack4::m_new(){
	return this;
}
c_Stack4* c_Stack4::m_new2(Array<c_Expr* > t_data){
	gc_assign(this->m_data,t_data.Slice(0));
	this->m_length=t_data.Length();
	return this;
}
void c_Stack4::p_Push10(c_Expr* t_value){
	if(m_length==m_data.Length()){
		gc_assign(m_data,m_data.Resize(m_length*2+10));
	}
	gc_assign(m_data[m_length],t_value);
	m_length+=1;
}
void c_Stack4::p_Push11(Array<c_Expr* > t_values,int t_offset,int t_count){
	for(int t_i=0;t_i<t_count;t_i=t_i+1){
		p_Push10(t_values[t_offset+t_i]);
	}
}
void c_Stack4::p_Push12(Array<c_Expr* > t_values,int t_offset){
	p_Push11(t_values,t_offset,t_values.Length()-t_offset);
}
Array<c_Expr* > c_Stack4::p_ToArray(){
	Array<c_Expr* > t_t=Array<c_Expr* >(m_length);
	for(int t_i=0;t_i<m_length;t_i=t_i+1){
		gc_assign(t_t[t_i],m_data[t_i]);
	}
	return t_t;
}
void c_Stack4::mark(){
	Object::mark();
	gc_mark_q(m_data);
}
c_ConstExpr::c_ConstExpr(){
	m_ty=0;
	m_value=String();
}
c_ConstExpr* c_ConstExpr::m_new(c_Type* t_ty,String t_value){
	c_Expr::m_new();
	if((dynamic_cast<c_IntType*>(t_ty))!=0){
		if(t_value.StartsWith(String(L"%",1))){
			t_value=String(bb_config_StringToInt(t_value.Slice(1),2));
		}else{
			if(t_value.StartsWith(String(L"$",1))){
				t_value=String(bb_config_StringToInt(t_value.Slice(1),16));
			}else{
				if(t_value.StartsWith(String(L"`",1)) && t_value.EndsWith(String(L"`",1))){
					t_value=bb_config_Dequote(t_value,String(L"cerberus",8));
					if(t_value.Length()<1){
						t_value=String(L"0",1);
					}else{
						t_value=String((int)t_value[0]);
					}
				}else{
					while(t_value.Length()>1 && t_value.StartsWith(String(L"0",1))){
						t_value=t_value.Slice(1);
					}
				}
			}
		}
	}else{
		if((dynamic_cast<c_FloatType*>(t_ty))!=0){
			if(!(t_value.Contains(String(L"e",1)) || t_value.Contains(String(L"E",1)) || t_value.Contains(String(L".",1)))){
				t_value=t_value+String(L".0",2);
			}
		}
	}
	gc_assign(this->m_ty,t_ty);
	this->m_value=t_value;
	return this;
}
c_ConstExpr* c_ConstExpr::m_new2(){
	c_Expr::m_new();
	return this;
}
c_Expr* c_ConstExpr::p_Semant(){
	if((m_exprType)!=0){
		return (this);
	}
	gc_assign(m_exprType,m_ty->p_Semant());
	return (this);
}
c_Expr* c_ConstExpr::p_Copy(){
	return ((new c_ConstExpr)->m_new(m_ty,m_value));
}
String c_ConstExpr::p_ToString(){
	return String(L"ConstExpr(\"",11)+m_value+String(L"\")",2);
}
String c_ConstExpr::p_Eval(){
	return m_value;
}
c_Expr* c_ConstExpr::p_EvalConst(){
	return (this);
}
bool c_ConstExpr::p_SideEffects(){
	return false;
}
String c_ConstExpr::p_Trans(){
	return bb_translator__trans->p_TransConstExpr(this);
}
void c_ConstExpr::mark(){
	c_Expr::mark();
	gc_mark_q(m_ty);
}
c_ScopeExpr::c_ScopeExpr(){
	m_scope=0;
}
c_ScopeExpr* c_ScopeExpr::m_new(c_ScopeDecl* t_scope){
	c_Expr::m_new();
	gc_assign(this->m_scope,t_scope);
	return this;
}
c_ScopeExpr* c_ScopeExpr::m_new2(){
	c_Expr::m_new();
	return this;
}
c_Expr* c_ScopeExpr::p_Copy(){
	return (this);
}
String c_ScopeExpr::p_ToString(){
	bbPrint(String(L"ScopeExpr(",10)+m_scope->p_ToString()+String(L")",1));
	return String();
}
c_Expr* c_ScopeExpr::p_Semant(){
	bb_config_InternalErr(String(L"Internal error",14));
	return 0;
}
c_ScopeDecl* c_ScopeExpr::p_SemantScope(){
	return m_scope;
}
void c_ScopeExpr::mark(){
	c_Expr::mark();
	gc_mark_q(m_scope);
}
c_NewArrayExpr::c_NewArrayExpr(){
	m_ty=0;
	m_expr=0;
}
c_NewArrayExpr* c_NewArrayExpr::m_new(c_Type* t_ty,c_Expr* t_expr){
	c_Expr::m_new();
	gc_assign(this->m_ty,t_ty);
	gc_assign(this->m_expr,t_expr);
	return this;
}
c_NewArrayExpr* c_NewArrayExpr::m_new2(){
	c_Expr::m_new();
	return this;
}
c_Expr* c_NewArrayExpr::p_Copy(){
	if((m_exprType)!=0){
		bb_config_InternalErr(String(L"Internal error",14));
	}
	return ((new c_NewArrayExpr)->m_new(m_ty,p_CopyExpr(m_expr)));
}
c_Expr* c_NewArrayExpr::p_Semant(){
	if((m_exprType)!=0){
		return (this);
	}
	gc_assign(m_ty,m_ty->p_Semant());
	gc_assign(m_exprType,(m_ty->p_ArrayOf()));
	gc_assign(m_expr,m_expr->p_Semant2((c_Type::m_intType),0));
	return (this);
}
String c_NewArrayExpr::p_Trans(){
	return bb_translator__trans->p_TransNewArrayExpr(this);
}
void c_NewArrayExpr::mark(){
	c_Expr::mark();
	gc_mark_q(m_ty);
	gc_mark_q(m_expr);
}
c_NewObjectExpr::c_NewObjectExpr(){
	m_ty=0;
	m_args=Array<c_Expr* >();
	m_classDecl=0;
	m_ctor=0;
}
c_NewObjectExpr* c_NewObjectExpr::m_new(c_Type* t_ty,Array<c_Expr* > t_args){
	c_Expr::m_new();
	gc_assign(this->m_ty,t_ty);
	gc_assign(this->m_args,t_args);
	return this;
}
c_NewObjectExpr* c_NewObjectExpr::m_new2(){
	c_Expr::m_new();
	return this;
}
c_Expr* c_NewObjectExpr::p_Semant(){
	if((m_exprType)!=0){
		return (this);
	}
	gc_assign(m_ty,m_ty->p_Semant());
	gc_assign(m_args,p_SemantArgs(m_args));
	c_ObjectType* t_objTy=dynamic_cast<c_ObjectType*>(m_ty);
	if(!((t_objTy)!=0)){
		bb_config_Err(String(L"Expression is not a class.",26));
	}
	gc_assign(m_classDecl,t_objTy->m_classDecl);
	if((m_classDecl->p_IsInterface())!=0){
		bb_config_Err(String(L"Cannot create instance of an interface.",39));
	}
	if((m_classDecl->p_IsAbstract())!=0){
		bb_config_Err(String(L"Cannot create instance of an abstract class.",44));
	}
	if(((m_classDecl->m_args).Length()!=0) && !((m_classDecl->m_instanceof)!=0)){
		bb_config_Err(String(L"Cannot create instance of a generic class.",42));
	}
	if((m_classDecl->p_IsExtern())!=0){
		if((m_args).Length()!=0){
			bb_config_Err(String(L"No suitable constructor found for class ",40)+m_classDecl->p_ToString()+String(L".",1));
		}
	}else{
		gc_assign(m_ctor,m_classDecl->p_FindFuncDecl(String(L"new",3),m_args,0));
		if(!((m_ctor)!=0)){
			bb_config_Err(String(L"No suitable constructor found for class ",40)+m_classDecl->p_ToString()+String(L".",1));
		}
		gc_assign(m_args,p_CastArgs(m_args,m_ctor));
	}
	m_classDecl->m_attrs|=1;
	gc_assign(m_exprType,m_ty);
	return (this);
}
c_Expr* c_NewObjectExpr::p_Copy(){
	return ((new c_NewObjectExpr)->m_new(m_ty,p_CopyArgs(m_args)));
}
String c_NewObjectExpr::p_Trans(){
	return bb_translator__trans->p_TransNewObjectExpr(this);
}
void c_NewObjectExpr::mark(){
	c_Expr::mark();
	gc_mark_q(m_ty);
	gc_mark_q(m_args);
	gc_mark_q(m_classDecl);
	gc_mark_q(m_ctor);
}
c_CastExpr::c_CastExpr(){
	m_ty=0;
	m_expr=0;
	m_flags=0;
}
c_CastExpr* c_CastExpr::m_new(c_Type* t_ty,c_Expr* t_expr,int t_flags){
	c_Expr::m_new();
	gc_assign(this->m_ty,t_ty);
	gc_assign(this->m_expr,t_expr);
	this->m_flags=t_flags;
	return this;
}
c_CastExpr* c_CastExpr::m_new2(){
	c_Expr::m_new();
	return this;
}
c_Expr* c_CastExpr::p_Semant(){
	if((m_exprType)!=0){
		return (this);
	}
	gc_assign(m_ty,m_ty->p_Semant());
	gc_assign(m_expr,m_expr->p_Semant());
	c_Type* t_src=m_expr->m_exprType;
	if((t_src->p_EqualsType(m_ty))!=0){
		return m_expr;
	}
	if((t_src->p_ExtendsType(m_ty))!=0){
		if(((dynamic_cast<c_ArrayType*>(t_src))!=0) && ((dynamic_cast<c_VoidType*>(dynamic_cast<c_ArrayType*>(t_src)->m_elemType))!=0)){
			return ((new c_ConstExpr)->m_new(m_ty,String()))->p_Semant();
		}
		if(((dynamic_cast<c_ObjectType*>(m_ty))!=0) && !((dynamic_cast<c_ObjectType*>(t_src))!=0)){
			c_Expr* t_[]={m_expr};
			gc_assign(m_expr,((new c_NewObjectExpr)->m_new(m_ty,Array<c_Expr* >(t_,1)))->p_Semant());
		}else{
			if(((dynamic_cast<c_ObjectType*>(t_src))!=0) && !((dynamic_cast<c_ObjectType*>(m_ty))!=0)){
				String t_op=String();
				if((dynamic_cast<c_BoolType*>(m_ty))!=0){
					t_op=String(L"ToBool",6);
				}else{
					if((dynamic_cast<c_IntType*>(m_ty))!=0){
						t_op=String(L"ToInt",5);
					}else{
						if((dynamic_cast<c_FloatType*>(m_ty))!=0){
							t_op=String(L"ToFloat",7);
						}else{
							if((dynamic_cast<c_StringType*>(m_ty))!=0){
								t_op=String(L"ToString",8);
							}else{
								bb_config_InternalErr(String(L"Internal error",14));
							}
						}
					}
				}
				c_FuncDecl* t_fdecl=t_src->p_GetClass()->p_FindFuncDecl(t_op,Array<c_Expr* >(),0);
				gc_assign(m_expr,((new c_InvokeMemberExpr)->m_new(m_expr,t_fdecl,Array<c_Expr* >()))->p_Semant());
			}
		}
		gc_assign(m_exprType,m_ty);
	}else{
		if((dynamic_cast<c_BoolType*>(m_ty))!=0){
			if((dynamic_cast<c_VoidType*>(t_src))!=0){
				bb_config_Err(String(L"Cannot convert from Void to Bool.",33));
			}
			if((m_flags&1)!=0){
				gc_assign(m_exprType,m_ty);
			}
		}else{
			if((m_ty->p_ExtendsType(t_src))!=0){
				if((m_flags&1)!=0){
					if(dynamic_cast<c_ObjectType*>(m_ty)!=0==(dynamic_cast<c_ObjectType*>(t_src)!=0)){
						gc_assign(m_exprType,m_ty);
					}
				}
			}else{
				if(((dynamic_cast<c_ObjectType*>(m_ty))!=0) && ((dynamic_cast<c_ObjectType*>(t_src))!=0)){
					if((m_flags&1)!=0){
						if(((t_src->p_GetClass()->p_IsInterface())!=0) || ((m_ty->p_GetClass()->p_IsInterface())!=0)){
							gc_assign(m_exprType,m_ty);
						}
					}
				}
			}
		}
	}
	if(!((m_exprType)!=0)){
		bb_config_Err(String(L"Cannot convert from ",20)+t_src->p_ToString()+String(L" to ",4)+m_ty->p_ToString()+String(L".",1));
	}
	if((dynamic_cast<c_ConstExpr*>(m_expr))!=0){
		return p_EvalConst();
	}
	return (this);
}
c_Expr* c_CastExpr::p_Copy(){
	return ((new c_CastExpr)->m_new(m_ty,p_CopyExpr(m_expr),m_flags));
}
String c_CastExpr::p_Eval(){
	String t_val=m_expr->p_Eval();
	if((dynamic_cast<c_BoolType*>(m_exprType))!=0){
		if((dynamic_cast<c_IntType*>(m_expr->m_exprType))!=0){
			if(((t_val).ToInt())!=0){
				return String(L"1",1);
			}
			return String();
		}else{
			if((dynamic_cast<c_FloatType*>(m_expr->m_exprType))!=0){
				if(((t_val).ToFloat())!=0){
					return String(L"1",1);
				}
				return String();
			}else{
				if((dynamic_cast<c_StringType*>(m_expr->m_exprType))!=0){
					if((t_val).Length()!=0){
						return String(L"1",1);
					}
					return String();
				}
			}
		}
	}else{
		if((dynamic_cast<c_IntType*>(m_exprType))!=0){
			if((dynamic_cast<c_BoolType*>(m_expr->m_exprType))!=0){
				if((t_val).Length()!=0){
					return String(L"1",1);
				}
				return String(L"0",1);
			}
			return String((t_val).ToInt());
		}else{
			if((dynamic_cast<c_FloatType*>(m_exprType))!=0){
				return String((t_val).ToFloat());
			}else{
				if((dynamic_cast<c_StringType*>(m_exprType))!=0){
					return t_val;
				}
			}
		}
	}
	if(!((t_val).Length()!=0)){
		return t_val;
	}
	return c_Expr::p_Eval();
}
String c_CastExpr::p_Trans(){
	return bb_translator__trans->p_TransCastExpr(this);
}
void c_CastExpr::mark(){
	c_Expr::mark();
	gc_mark_q(m_ty);
	gc_mark_q(m_expr);
}
c_IdentExpr::c_IdentExpr(){
	m_ident=String();
	m_expr=0;
	m_scope=0;
	m_static=false;
}
c_IdentExpr* c_IdentExpr::m_new(String t_ident,c_Expr* t_expr){
	c_Expr::m_new();
	this->m_ident=t_ident;
	gc_assign(this->m_expr,t_expr);
	return this;
}
c_IdentExpr* c_IdentExpr::m_new2(){
	c_Expr::m_new();
	return this;
}
c_Expr* c_IdentExpr::p_Copy(){
	return ((new c_IdentExpr)->m_new(m_ident,p_CopyExpr(m_expr)));
}
String c_IdentExpr::p_ToString(){
	String t_t=String(L"IdentExpr(\"",11)+m_ident+String(L"\"",1);
	if((m_expr)!=0){
		t_t=t_t+(String(L",",1)+m_expr->p_ToString());
	}
	return t_t+String(L")",1);
}
int c_IdentExpr::p__Semant(){
	if((m_scope)!=0){
		return 0;
	}
	if((m_expr)!=0){
		gc_assign(m_scope,m_expr->p_SemantScope());
		if((m_scope)!=0){
			m_static=true;
		}else{
			gc_assign(m_expr,m_expr->p_Semant());
			gc_assign(m_scope,(m_expr->m_exprType->p_GetClass()));
			if(!((m_scope)!=0)){
				bb_config_Err(String(L"Expression has no scope",23));
			}
		}
	}else{
		gc_assign(m_scope,bb_decl__env);
		m_static=bb_decl__env->p_FuncScope()==0 || bb_decl__env->p_FuncScope()->p_IsStatic();
	}
	return 0;
}
int c_IdentExpr::p_IdentErr(){
	String t_close=String();
	c_Enumerator2* t_=m_scope->p_Decls()->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_Decl* t_decl=t_->p_NextObject();
		if(m_ident.ToLower()==t_decl->m_ident.ToLower()){
			t_close=t_decl->m_ident;
		}
	}
	if(((t_close).Length()!=0) && m_ident!=t_close){
		bb_config_Err(String(L"Identifier '",12)+m_ident+String(L"' not found - perhaps you meant '",33)+t_close+String(L"'?",2));
	}
	bb_config_Err(String(L"Identifier '",12)+m_ident+String(L"' not found.",12));
	return 0;
}
c_Expr* c_IdentExpr::p_SemantSet(String t_op,c_Expr* t_rhs){
	p__Semant();
	c_ValDecl* t_vdecl=m_scope->p_FindValDecl(m_ident);
	if((t_vdecl)!=0){
		if((dynamic_cast<c_ConstDecl*>(t_vdecl))!=0){
			if((t_rhs)!=0){
				bb_config_Err(String(L"Constant '",10)+m_ident+String(L"' cannot be modified.",21));
			}
			c_ConstExpr* t_cexpr=(new c_ConstExpr)->m_new(t_vdecl->m_type,dynamic_cast<c_ConstDecl*>(t_vdecl)->m_value);
			if(!m_static && (((dynamic_cast<c_InvokeExpr*>(m_expr))!=0) || ((dynamic_cast<c_InvokeMemberExpr*>(m_expr))!=0))){
				return ((new c_StmtExpr)->m_new(((new c_ExprStmt)->m_new(m_expr)),(t_cexpr)))->p_Semant();
			}
			return t_cexpr->p_Semant();
		}else{
			if((dynamic_cast<c_FieldDecl*>(t_vdecl))!=0){
				if(m_static){
					bb_config_Err(String(L"Field '",7)+m_ident+String(L"' cannot be accessed from here.",31));
				}
				if((m_expr)!=0){
					return ((new c_MemberVarExpr)->m_new(m_expr,dynamic_cast<c_VarDecl*>(t_vdecl)))->p_Semant();
				}
			}
		}
		return ((new c_VarExpr)->m_new(dynamic_cast<c_VarDecl*>(t_vdecl)))->p_Semant();
	}
	if(((t_op).Length()!=0) && t_op!=String(L"=",1)){
		c_FuncDecl* t_fdecl=m_scope->p_FindFuncDecl(m_ident,Array<c_Expr* >(),0);
		if(!((t_fdecl)!=0)){
			p_IdentErr();
		}
		if(((bb_decl__env->p_ModuleScope()->p_IsStrict())!=0) && !t_fdecl->p_IsProperty()){
			bb_config_Err(String(L"Identifier '",12)+m_ident+String(L"' cannot be used in this way.",29));
		}
		c_Expr* t_lhs=0;
		if(t_fdecl->p_IsStatic() || m_scope==bb_decl__env && !bb_decl__env->p_FuncScope()->p_IsStatic()){
			t_lhs=((new c_InvokeExpr)->m_new(t_fdecl,Array<c_Expr* >()));
		}else{
			if((m_expr)!=0){
				c_LocalDecl* t_tmp=(new c_LocalDecl)->m_new(String(),0,0,m_expr);
				t_lhs=((new c_InvokeMemberExpr)->m_new(((new c_VarExpr)->m_new(t_tmp)),t_fdecl,Array<c_Expr* >()));
				t_lhs=((new c_StmtExpr)->m_new(((new c_DeclStmt)->m_new(t_tmp)),t_lhs));
			}else{
				return 0;
			}
		}
		String t_bop=t_op.Slice(0,1);
		String t_1=t_bop;
		if(t_1==String(L"*",1) || t_1==String(L"/",1) || t_1==String(L"shl",3) || t_1==String(L"shr",3) || t_1==String(L"+",1) || t_1==String(L"-",1) || t_1==String(L"&",1) || t_1==String(L"|",1) || t_1==String(L"~",1)){
			t_rhs=((new c_BinaryMathExpr)->m_new(t_bop,t_lhs,t_rhs));
		}else{
			bb_config_InternalErr(String(L"Internal error",14));
		}
		t_rhs=t_rhs->p_Semant();
	}
	Array<c_Expr* > t_args=Array<c_Expr* >();
	if((t_rhs)!=0){
		c_Expr* t_[]={t_rhs};
		t_args=Array<c_Expr* >(t_,1);
	}
	c_FuncDecl* t_fdecl2=m_scope->p_FindFuncDecl(m_ident,t_args,0);
	if((t_fdecl2)!=0){
		if(((bb_decl__env->p_ModuleScope()->p_IsStrict())!=0) && !t_fdecl2->p_IsProperty()){
			bb_config_Err(String(L"Identifier '",12)+m_ident+String(L"' cannot be used in this way.",29));
		}
		if(!t_fdecl2->p_IsStatic()){
			if(m_static){
				bb_config_Err(String(L"Method '",8)+m_ident+String(L"' cannot be accessed from here.",31));
			}
			if((m_expr)!=0){
				return ((new c_InvokeMemberExpr)->m_new(m_expr,t_fdecl2,t_args))->p_Semant();
			}
		}
		return ((new c_InvokeExpr)->m_new(t_fdecl2,t_args))->p_Semant();
	}
	p_IdentErr();
	return 0;
}
c_Expr* c_IdentExpr::p_Semant(){
	return p_SemantSet(String(),0);
}
c_ScopeDecl* c_IdentExpr::p_SemantScope(){
	p__Semant();
	return m_scope->p_FindScopeDecl(m_ident);
}
c_Expr* c_IdentExpr::p_SemantFunc(Array<c_Expr* > t_args){
	p__Semant();
	c_FuncDecl* t_fdecl=m_scope->p_FindFuncDecl(m_ident,t_args,0);
	if((t_fdecl)!=0){
		if(!t_fdecl->p_IsStatic()){
			if(m_static){
				bb_config_Err(String(L"Method '",8)+m_ident+String(L"' cannot be accessed from here.",31));
			}
			if((m_expr)!=0){
				return ((new c_InvokeMemberExpr)->m_new(m_expr,t_fdecl,t_args))->p_Semant();
			}
		}
		return ((new c_InvokeExpr)->m_new(t_fdecl,t_args))->p_Semant();
	}
	c_Type* t_type=m_scope->p_FindType(m_ident,Array<c_Type* >());
	if((t_type)!=0){
		if(t_args.Length()==1 && ((t_args[0])!=0)){
			return t_args[0]->p_Cast(t_type,1);
		}
		bb_config_Err(String(L"Illegal number of arguments for type conversion",47));
	}
	p_IdentErr();
	return 0;
}
void c_IdentExpr::mark(){
	c_Expr::mark();
	gc_mark_q(m_expr);
	gc_mark_q(m_scope);
}
c_SelfExpr::c_SelfExpr(){
}
c_SelfExpr* c_SelfExpr::m_new(){
	c_Expr::m_new();
	return this;
}
c_Expr* c_SelfExpr::p_Copy(){
	return ((new c_SelfExpr)->m_new());
}
c_Expr* c_SelfExpr::p_Semant(){
	if((m_exprType)!=0){
		return (this);
	}
	if((bb_decl__env->p_FuncScope())!=0){
		if(bb_decl__env->p_FuncScope()->p_IsStatic()){
			bb_config_Err(String(L"Illegal use of Self within static scope.",40));
		}
	}else{
		bb_config_Err(String(L"Self cannot be used here.",25));
	}
	gc_assign(m_exprType,(bb_decl__env->p_ClassScope()->m_objectType));
	return (this);
}
bool c_SelfExpr::p_SideEffects(){
	return false;
}
String c_SelfExpr::p_Trans(){
	return bb_translator__trans->p_TransSelfExpr(this);
}
void c_SelfExpr::mark(){
	c_Expr::mark();
}
c_Stmt::c_Stmt(){
	m_errInfo=String();
}
c_Stmt* c_Stmt::m_new(){
	m_errInfo=bb_config__errInfo;
	return this;
}
c_Stmt* c_Stmt::p_Copy2(c_ScopeDecl* t_scope){
	c_Stmt* t_t=p_OnCopy2(t_scope);
	t_t->m_errInfo=m_errInfo;
	return t_t;
}
int c_Stmt::p_Semant(){
	bb_config_PushErr(m_errInfo);
	p_OnSemant();
	bb_config_PopErr();
	return 0;
}
void c_Stmt::mark(){
	Object::mark();
}
c_List5::c_List5(){
	m__head=((new c_HeadNode5)->m_new());
}
c_List5* c_List5::m_new(){
	return this;
}
c_Node10* c_List5::p_AddLast5(c_Stmt* t_data){
	return (new c_Node10)->m_new(m__head,m__head->m__pred,t_data);
}
c_List5* c_List5::m_new2(Array<c_Stmt* > t_data){
	Array<c_Stmt* > t_=t_data;
	int t_2=0;
	while(t_2<t_.Length()){
		c_Stmt* t_t=t_[t_2];
		t_2=t_2+1;
		p_AddLast5(t_t);
	}
	return this;
}
bool c_List5::p_IsEmpty(){
	return m__head->m__succ==m__head;
}
c_Enumerator5* c_List5::p_ObjectEnumerator(){
	return (new c_Enumerator5)->m_new(this);
}
c_Node10* c_List5::p_AddFirst(c_Stmt* t_data){
	return (new c_Node10)->m_new(m__head->m__succ,m__head,t_data);
}
void c_List5::mark(){
	Object::mark();
	gc_mark_q(m__head);
}
c_Node10::c_Node10(){
	m__succ=0;
	m__pred=0;
	m__data=0;
}
c_Node10* c_Node10::m_new(c_Node10* t_succ,c_Node10* t_pred,c_Stmt* t_data){
	gc_assign(m__succ,t_succ);
	gc_assign(m__pred,t_pred);
	gc_assign(m__succ->m__pred,this);
	gc_assign(m__pred->m__succ,this);
	gc_assign(m__data,t_data);
	return this;
}
c_Node10* c_Node10::m_new2(){
	return this;
}
void c_Node10::mark(){
	Object::mark();
	gc_mark_q(m__succ);
	gc_mark_q(m__pred);
	gc_mark_q(m__data);
}
c_HeadNode5::c_HeadNode5(){
}
c_HeadNode5* c_HeadNode5::m_new(){
	c_Node10::m_new2();
	gc_assign(m__succ,(this));
	gc_assign(m__pred,(this));
	return this;
}
void c_HeadNode5::mark(){
	c_Node10::mark();
}
c_InvokeSuperExpr::c_InvokeSuperExpr(){
	m_ident=String();
	m_args=Array<c_Expr* >();
	m_funcDecl=0;
}
c_InvokeSuperExpr* c_InvokeSuperExpr::m_new(String t_ident,Array<c_Expr* > t_args){
	c_Expr::m_new();
	this->m_ident=t_ident;
	gc_assign(this->m_args,t_args);
	return this;
}
c_InvokeSuperExpr* c_InvokeSuperExpr::m_new2(){
	c_Expr::m_new();
	return this;
}
c_Expr* c_InvokeSuperExpr::p_Copy(){
	return ((new c_InvokeSuperExpr)->m_new(m_ident,p_CopyArgs(m_args)));
}
c_Expr* c_InvokeSuperExpr::p_Semant(){
	if((m_exprType)!=0){
		return (this);
	}
	if(bb_decl__env->p_FuncScope()->p_IsStatic()){
		bb_config_Err(String(L"Illegal use of Super.",21));
	}
	c_ClassDecl* t_classScope=bb_decl__env->p_ClassScope();
	c_ClassDecl* t_superClass=t_classScope->m_superClass;
	if(!((t_superClass)!=0)){
		bb_config_Err(String(L"Class has no super class.",25));
	}
	gc_assign(m_args,p_SemantArgs(m_args));
	gc_assign(m_funcDecl,t_superClass->p_FindFuncDecl(m_ident,m_args,0));
	if(!((m_funcDecl)!=0)){
		bb_config_Err(String(L"Can't find superclass method '",30)+m_ident+String(L"'.",2));
	}
	if((m_funcDecl->p_IsAbstract())!=0){
		bb_config_Err(String(L"Can't invoke abstract superclass method '",41)+m_ident+String(L"'.",2));
	}
	gc_assign(m_args,p_CastArgs(m_args,m_funcDecl));
	gc_assign(m_exprType,m_funcDecl->m_retType);
	return (this);
}
String c_InvokeSuperExpr::p_Trans(){
	return bb_translator__trans->p_TransInvokeSuperExpr(this);
}
void c_InvokeSuperExpr::mark(){
	c_Expr::mark();
	gc_mark_q(m_args);
	gc_mark_q(m_funcDecl);
}
c_IdentTypeExpr::c_IdentTypeExpr(){
	m_cdecl=0;
}
c_IdentTypeExpr* c_IdentTypeExpr::m_new(c_Type* t_type){
	c_Expr::m_new();
	gc_assign(this->m_exprType,t_type);
	return this;
}
c_IdentTypeExpr* c_IdentTypeExpr::m_new2(){
	c_Expr::m_new();
	return this;
}
c_Expr* c_IdentTypeExpr::p_Copy(){
	return ((new c_IdentTypeExpr)->m_new(m_exprType));
}
int c_IdentTypeExpr::p__Semant(){
	if((m_cdecl)!=0){
		return 0;
	}
	gc_assign(m_exprType,m_exprType->p_Semant());
	gc_assign(m_cdecl,m_exprType->p_GetClass());
	if(!((m_cdecl)!=0)){
		bb_config_InternalErr(String(L"Internal error",14));
	}
	return 0;
}
c_Expr* c_IdentTypeExpr::p_Semant(){
	p__Semant();
	bb_config_Err(String(L"Expression can't be used in this way",36));
	return 0;
}
c_ScopeDecl* c_IdentTypeExpr::p_SemantScope(){
	p__Semant();
	return (m_cdecl);
}
c_Expr* c_IdentTypeExpr::p_SemantFunc(Array<c_Expr* > t_args){
	p__Semant();
	if(t_args.Length()==1 && ((t_args[0])!=0)){
		return t_args[0]->p_Cast((m_cdecl->m_objectType),1);
	}
	bb_config_Err(String(L"Illegal number of arguments for type conversion",47));
	return 0;
}
void c_IdentTypeExpr::mark(){
	c_Expr::mark();
	gc_mark_q(m_cdecl);
}
c_FuncCallExpr::c_FuncCallExpr(){
	m_expr=0;
	m_args=Array<c_Expr* >();
}
c_FuncCallExpr* c_FuncCallExpr::m_new(c_Expr* t_expr,Array<c_Expr* > t_args){
	c_Expr::m_new();
	gc_assign(this->m_expr,t_expr);
	gc_assign(this->m_args,t_args);
	return this;
}
c_FuncCallExpr* c_FuncCallExpr::m_new2(){
	c_Expr::m_new();
	return this;
}
c_Expr* c_FuncCallExpr::p_Copy(){
	return ((new c_FuncCallExpr)->m_new(p_CopyExpr(m_expr),p_CopyArgs(m_args)));
}
String c_FuncCallExpr::p_ToString(){
	String t_t=String(L"FuncCallExpr(",13)+m_expr->p_ToString();
	Array<c_Expr* > t_=m_args;
	int t_2=0;
	while(t_2<t_.Length()){
		c_Expr* t_arg=t_[t_2];
		t_2=t_2+1;
		t_t=t_t+(String(L",",1)+t_arg->p_ToString());
	}
	return t_t+String(L")",1);
}
c_Expr* c_FuncCallExpr::p_Semant(){
	gc_assign(m_args,p_SemantArgs(m_args));
	return m_expr->p_SemantFunc(m_args);
}
void c_FuncCallExpr::mark(){
	c_Expr::mark();
	gc_mark_q(m_expr);
	gc_mark_q(m_args);
}
c_SliceExpr::c_SliceExpr(){
	m_expr=0;
	m_from=0;
	m_term=0;
}
c_SliceExpr* c_SliceExpr::m_new(c_Expr* t_expr,c_Expr* t_from,c_Expr* t_term){
	c_Expr::m_new();
	gc_assign(this->m_expr,t_expr);
	gc_assign(this->m_from,t_from);
	gc_assign(this->m_term,t_term);
	return this;
}
c_SliceExpr* c_SliceExpr::m_new2(){
	c_Expr::m_new();
	return this;
}
c_Expr* c_SliceExpr::p_Copy(){
	return ((new c_SliceExpr)->m_new(p_CopyExpr(m_expr),p_CopyExpr(m_from),p_CopyExpr(m_term)));
}
c_Expr* c_SliceExpr::p_Semant(){
	if((m_exprType)!=0){
		return (this);
	}
	gc_assign(m_expr,m_expr->p_Semant());
	if(((dynamic_cast<c_ArrayType*>(m_expr->m_exprType))!=0) || ((dynamic_cast<c_StringType*>(m_expr->m_exprType))!=0)){
		if((m_from)!=0){
			gc_assign(m_from,m_from->p_Semant2((c_Type::m_intType),0));
		}
		if((m_term)!=0){
			gc_assign(m_term,m_term->p_Semant2((c_Type::m_intType),0));
		}
		gc_assign(m_exprType,m_expr->m_exprType);
	}else{
		bb_config_Err(String(L"Slices can only be used on strings or arrays.",45));
	}
	return (this);
}
String c_SliceExpr::p_Eval(){
	int t_from=(this->m_from->p_Eval()).ToInt();
	int t_term=(this->m_term->p_Eval()).ToInt();
	if((dynamic_cast<c_StringType*>(m_expr->m_exprType))!=0){
		return m_expr->p_Eval().Slice(t_from,t_term);
	}else{
		if((dynamic_cast<c_ArrayType*>(m_expr->m_exprType))!=0){
			bb_config_Err(String(L"TODO!",5));
		}
	}
	return String();
}
String c_SliceExpr::p_Trans(){
	return bb_translator__trans->p_TransSliceExpr(this);
}
void c_SliceExpr::mark(){
	c_Expr::mark();
	gc_mark_q(m_expr);
	gc_mark_q(m_from);
	gc_mark_q(m_term);
}
c_IndexExpr::c_IndexExpr(){
	m_expr=0;
	m_index=0;
}
c_IndexExpr* c_IndexExpr::m_new(c_Expr* t_expr,c_Expr* t_index){
	c_Expr::m_new();
	gc_assign(this->m_expr,t_expr);
	gc_assign(this->m_index,t_index);
	return this;
}
c_IndexExpr* c_IndexExpr::m_new2(){
	c_Expr::m_new();
	return this;
}
c_Expr* c_IndexExpr::p_Copy(){
	return ((new c_IndexExpr)->m_new(p_CopyExpr(m_expr),p_CopyExpr(m_index)));
}
c_Expr* c_IndexExpr::p_Semant(){
	if((m_exprType)!=0){
		return (this);
	}
	gc_assign(m_expr,m_expr->p_Semant());
	gc_assign(m_index,m_index->p_Semant2((c_Type::m_intType),0));
	if((dynamic_cast<c_StringType*>(m_expr->m_exprType))!=0){
		gc_assign(m_exprType,(c_Type::m_intType));
	}else{
		if((dynamic_cast<c_ArrayType*>(m_expr->m_exprType))!=0){
			gc_assign(m_exprType,dynamic_cast<c_ArrayType*>(m_expr->m_exprType)->m_elemType);
		}else{
			bb_config_Err(String(L"Only strings and arrays may be indexed.",39));
		}
	}
	if(((dynamic_cast<c_StringType*>(m_expr->m_exprType))!=0) && ((dynamic_cast<c_ConstExpr*>(m_expr))!=0) && ((dynamic_cast<c_ConstExpr*>(m_index))!=0)){
		return p_EvalConst();
	}
	return (this);
}
String c_IndexExpr::p_Eval(){
	if((dynamic_cast<c_StringType*>(m_expr->m_exprType))!=0){
		String t_str=m_expr->p_Eval();
		int t_idx=(m_index->p_Eval()).ToInt();
		if(t_idx<0 || t_idx>=t_str.Length()){
			bb_config_Err(String(L"String index out of range.",26));
		}
		return String((int)t_str[t_idx]);
	}
	bb_config_InternalErr(String(L"Internal error",14));
	return String();
}
c_Expr* c_IndexExpr::p_SemantSet(String t_op,c_Expr* t_rhs){
	p_Semant();
	if((dynamic_cast<c_StringType*>(m_expr->m_exprType))!=0){
		bb_config_Err(String(L"Strings are read only.",22));
	}
	return (this);
}
bool c_IndexExpr::p_SideEffects(){
	return m_expr->p_SideEffects() || m_index->p_SideEffects();
}
String c_IndexExpr::p_Trans(){
	return bb_translator__trans->p_TransIndexExpr(this);
}
String c_IndexExpr::p_TransVar(){
	return bb_translator__trans->p_TransIndexExpr(this);
}
void c_IndexExpr::mark(){
	c_Expr::mark();
	gc_mark_q(m_expr);
	gc_mark_q(m_index);
}
c_BinaryExpr::c_BinaryExpr(){
	m_op=String();
	m_lhs=0;
	m_rhs=0;
}
c_BinaryExpr* c_BinaryExpr::m_new(String t_op,c_Expr* t_lhs,c_Expr* t_rhs){
	c_Expr::m_new();
	this->m_op=t_op;
	gc_assign(this->m_lhs,t_lhs);
	gc_assign(this->m_rhs,t_rhs);
	return this;
}
c_BinaryExpr* c_BinaryExpr::m_new2(){
	c_Expr::m_new();
	return this;
}
String c_BinaryExpr::p_Trans(){
	return bb_translator__trans->p_TransBinaryExpr(this);
}
void c_BinaryExpr::mark(){
	c_Expr::mark();
	gc_mark_q(m_lhs);
	gc_mark_q(m_rhs);
}
c_BinaryMathExpr::c_BinaryMathExpr(){
}
c_BinaryMathExpr* c_BinaryMathExpr::m_new(String t_op,c_Expr* t_lhs,c_Expr* t_rhs){
	c_BinaryExpr::m_new2();
	this->m_op=t_op;
	gc_assign(this->m_lhs,t_lhs);
	gc_assign(this->m_rhs,t_rhs);
	return this;
}
c_BinaryMathExpr* c_BinaryMathExpr::m_new2(){
	c_BinaryExpr::m_new2();
	return this;
}
c_Expr* c_BinaryMathExpr::p_Copy(){
	return ((new c_BinaryMathExpr)->m_new(m_op,p_CopyExpr(m_lhs),p_CopyExpr(m_rhs)));
}
c_Expr* c_BinaryMathExpr::p_Semant(){
	if((m_exprType)!=0){
		return (this);
	}
	gc_assign(m_lhs,m_lhs->p_Semant());
	gc_assign(m_rhs,m_rhs->p_Semant());
	String t_3=m_op;
	if(t_3==String(L"&",1) || t_3==String(L"~",1) || t_3==String(L"|",1) || t_3==String(L"shl",3) || t_3==String(L"shr",3)){
		gc_assign(m_exprType,(c_Type::m_intType));
	}else{
		gc_assign(m_exprType,p_BalanceTypes(m_lhs->m_exprType,m_rhs->m_exprType));
		if((dynamic_cast<c_StringType*>(m_exprType))!=0){
			if(m_op!=String(L"+",1)){
				bb_config_Err(String(L"Illegal string operator.",24));
			}
		}else{
			if(!((dynamic_cast<c_NumericType*>(m_exprType))!=0)){
				bb_config_Err(String(L"Illegal expression type.",24));
			}
		}
	}
	gc_assign(m_lhs,m_lhs->p_Cast(m_exprType,0));
	gc_assign(m_rhs,m_rhs->p_Cast(m_exprType,0));
	if(((dynamic_cast<c_ConstExpr*>(m_lhs))!=0) && ((dynamic_cast<c_ConstExpr*>(m_rhs))!=0)){
		return p_EvalConst();
	}
	return (this);
}
String c_BinaryMathExpr::p_Eval(){
	String t_lhs=this->m_lhs->p_Eval();
	String t_rhs=this->m_rhs->p_Eval();
	if((dynamic_cast<c_IntType*>(m_exprType))!=0){
		int t_x=(t_lhs).ToInt();
		int t_y=(t_rhs).ToInt();
		String t_4=m_op;
		if(t_4==String(L"/",1)){
			if(!((t_y)!=0)){
				bb_config_Err(String(L"Divide by zero error.",21));
			}
			return String(t_x/t_y);
		}else{
			if(t_4==String(L"*",1)){
				return String(t_x*t_y);
			}else{
				if(t_4==String(L"mod",3)){
					if(!((t_y)!=0)){
						bb_config_Err(String(L"Divide by zero error.",21));
					}
					return String(t_x % t_y);
				}else{
					if(t_4==String(L"shl",3)){
						return String(t_x<<t_y);
					}else{
						if(t_4==String(L"shr",3)){
							return String(t_x>>t_y);
						}else{
							if(t_4==String(L"+",1)){
								return String(t_x+t_y);
							}else{
								if(t_4==String(L"-",1)){
									return String(t_x-t_y);
								}else{
									if(t_4==String(L"&",1)){
										return String(t_x&t_y);
									}else{
										if(t_4==String(L"~",1)){
											return String(t_x^t_y);
										}else{
											if(t_4==String(L"|",1)){
												return String(t_x|t_y);
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}else{
		if((dynamic_cast<c_FloatType*>(m_exprType))!=0){
			Float t_x2=(t_lhs).ToFloat();
			Float t_y2=(t_rhs).ToFloat();
			String t_5=m_op;
			if(t_5==String(L"/",1)){
				if(!((t_y2)!=0)){
					bb_config_Err(String(L"Divide by zero error.",21));
				}
				return String(t_x2/t_y2);
			}else{
				if(t_5==String(L"*",1)){
					return String(t_x2*t_y2);
				}else{
					if(t_5==String(L"mod",3)){
						if(!((t_y2)!=0)){
							bb_config_Err(String(L"Divide by zero error.",21));
						}
						return String((Float)fmod(t_x2,t_y2));
					}else{
						if(t_5==String(L"+",1)){
							return String(t_x2+t_y2);
						}else{
							if(t_5==String(L"-",1)){
								return String(t_x2-t_y2);
							}
						}
					}
				}
			}
		}else{
			if((dynamic_cast<c_StringType*>(m_exprType))!=0){
				String t_6=m_op;
				if(t_6==String(L"+",1)){
					return t_lhs+t_rhs;
				}
			}
		}
	}
	bb_config_InternalErr(String(L"Internal error",14));
	return String();
}
void c_BinaryMathExpr::mark(){
	c_BinaryExpr::mark();
}
c_BinaryCompareExpr::c_BinaryCompareExpr(){
	m_ty=0;
}
c_BinaryCompareExpr* c_BinaryCompareExpr::m_new(String t_op,c_Expr* t_lhs,c_Expr* t_rhs){
	c_BinaryExpr::m_new2();
	this->m_op=t_op;
	gc_assign(this->m_lhs,t_lhs);
	gc_assign(this->m_rhs,t_rhs);
	return this;
}
c_BinaryCompareExpr* c_BinaryCompareExpr::m_new2(){
	c_BinaryExpr::m_new2();
	return this;
}
c_Expr* c_BinaryCompareExpr::p_Copy(){
	return ((new c_BinaryCompareExpr)->m_new(m_op,p_CopyExpr(m_lhs),p_CopyExpr(m_rhs)));
}
c_Expr* c_BinaryCompareExpr::p_Semant(){
	if((m_exprType)!=0){
		return (this);
	}
	gc_assign(m_lhs,m_lhs->p_Semant());
	gc_assign(m_rhs,m_rhs->p_Semant());
	gc_assign(m_ty,p_BalanceTypes(m_lhs->m_exprType,m_rhs->m_exprType));
	if((dynamic_cast<c_ArrayType*>(m_ty))!=0){
		bb_config_Err(String(L"Arrays cannot be compared.",26));
	}
	if(((dynamic_cast<c_BoolType*>(m_ty))!=0) && m_op!=String(L"=",1) && m_op!=String(L"<>",2)){
		bb_config_Err(String(L"Bools can only be compared for equality.",40));
	}
	if(((dynamic_cast<c_ObjectType*>(m_ty))!=0) && m_op!=String(L"=",1) && m_op!=String(L"<>",2)){
		bb_config_Err(String(L"Objects can only be compared for equality.",42));
	}
	gc_assign(m_lhs,m_lhs->p_Cast(m_ty,0));
	gc_assign(m_rhs,m_rhs->p_Cast(m_ty,0));
	gc_assign(m_exprType,(c_Type::m_boolType));
	if(((dynamic_cast<c_ConstExpr*>(m_lhs))!=0) && ((dynamic_cast<c_ConstExpr*>(m_rhs))!=0)){
		return p_EvalConst();
	}
	return (this);
}
String c_BinaryCompareExpr::p_Eval(){
	int t_r=-1;
	if((dynamic_cast<c_BoolType*>(m_ty))!=0){
		String t_lhs=this->m_lhs->p_Eval();
		String t_rhs=this->m_rhs->p_Eval();
		String t_7=m_op;
		if(t_7==String(L"=",1)){
			t_r=((t_lhs==t_rhs)?1:0);
		}else{
			if(t_7==String(L"<>",2)){
				t_r=((t_lhs!=t_rhs)?1:0);
			}
		}
	}else{
		if((dynamic_cast<c_IntType*>(m_ty))!=0){
			int t_lhs2=(this->m_lhs->p_Eval()).ToInt();
			int t_rhs2=(this->m_rhs->p_Eval()).ToInt();
			String t_8=m_op;
			if(t_8==String(L"=",1)){
				t_r=((t_lhs2==t_rhs2)?1:0);
			}else{
				if(t_8==String(L"<>",2)){
					t_r=((t_lhs2!=t_rhs2)?1:0);
				}else{
					if(t_8==String(L"<",1)){
						t_r=((t_lhs2<t_rhs2)?1:0);
					}else{
						if(t_8==String(L"<=",2)){
							t_r=((t_lhs2<=t_rhs2)?1:0);
						}else{
							if(t_8==String(L">",1)){
								t_r=((t_lhs2>t_rhs2)?1:0);
							}else{
								if(t_8==String(L">=",2)){
									t_r=((t_lhs2>=t_rhs2)?1:0);
								}
							}
						}
					}
				}
			}
		}else{
			if((dynamic_cast<c_FloatType*>(m_ty))!=0){
				Float t_lhs3=(this->m_lhs->p_Eval()).ToFloat();
				Float t_rhs3=(this->m_rhs->p_Eval()).ToFloat();
				String t_9=m_op;
				if(t_9==String(L"=",1)){
					t_r=((t_lhs3==t_rhs3)?1:0);
				}else{
					if(t_9==String(L"<>",2)){
						t_r=((t_lhs3!=t_rhs3)?1:0);
					}else{
						if(t_9==String(L"<",1)){
							t_r=((t_lhs3<t_rhs3)?1:0);
						}else{
							if(t_9==String(L"<=",2)){
								t_r=((t_lhs3<=t_rhs3)?1:0);
							}else{
								if(t_9==String(L">",1)){
									t_r=((t_lhs3>t_rhs3)?1:0);
								}else{
									if(t_9==String(L">=",2)){
										t_r=((t_lhs3>=t_rhs3)?1:0);
									}
								}
							}
						}
					}
				}
			}else{
				if((dynamic_cast<c_StringType*>(m_ty))!=0){
					String t_lhs4=this->m_lhs->p_Eval();
					String t_rhs4=this->m_rhs->p_Eval();
					String t_10=m_op;
					if(t_10==String(L"=",1)){
						t_r=((t_lhs4==t_rhs4)?1:0);
					}else{
						if(t_10==String(L"<>",2)){
							t_r=((t_lhs4!=t_rhs4)?1:0);
						}else{
							if(t_10==String(L"<",1)){
								t_r=((t_lhs4<t_rhs4)?1:0);
							}else{
								if(t_10==String(L"<=",2)){
									t_r=((t_lhs4<=t_rhs4)?1:0);
								}else{
									if(t_10==String(L">",1)){
										t_r=((t_lhs4>t_rhs4)?1:0);
									}else{
										if(t_10==String(L">=",2)){
											t_r=((t_lhs4>=t_rhs4)?1:0);
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	if(t_r==1){
		return String(L"1",1);
	}
	if(t_r==0){
		return String();
	}
	bb_config_InternalErr(String(L"Internal error",14));
	return String();
}
void c_BinaryCompareExpr::mark(){
	c_BinaryExpr::mark();
	gc_mark_q(m_ty);
}
c_BinaryLogicExpr::c_BinaryLogicExpr(){
}
c_BinaryLogicExpr* c_BinaryLogicExpr::m_new(String t_op,c_Expr* t_lhs,c_Expr* t_rhs){
	c_BinaryExpr::m_new2();
	this->m_op=t_op;
	gc_assign(this->m_lhs,t_lhs);
	gc_assign(this->m_rhs,t_rhs);
	return this;
}
c_BinaryLogicExpr* c_BinaryLogicExpr::m_new2(){
	c_BinaryExpr::m_new2();
	return this;
}
c_Expr* c_BinaryLogicExpr::p_Copy(){
	return ((new c_BinaryLogicExpr)->m_new(m_op,p_CopyExpr(m_lhs),p_CopyExpr(m_rhs)));
}
c_Expr* c_BinaryLogicExpr::p_Semant(){
	if((m_exprType)!=0){
		return (this);
	}
	gc_assign(m_lhs,m_lhs->p_Semant2((c_Type::m_boolType),1));
	gc_assign(m_rhs,m_rhs->p_Semant2((c_Type::m_boolType),1));
	gc_assign(m_exprType,(c_Type::m_boolType));
	if(((dynamic_cast<c_ConstExpr*>(m_lhs))!=0) && ((dynamic_cast<c_ConstExpr*>(m_rhs))!=0)){
		return p_EvalConst();
	}
	return (this);
}
String c_BinaryLogicExpr::p_Eval(){
	String t_11=m_op;
	if(t_11==String(L"and",3)){
		if(((m_lhs->p_Eval()).Length()!=0) && ((m_rhs->p_Eval()).Length()!=0)){
			return String(L"1",1);
		}else{
			return String();
		}
	}else{
		if(t_11==String(L"or",2)){
			if(((m_lhs->p_Eval()).Length()!=0) || ((m_rhs->p_Eval()).Length()!=0)){
				return String(L"1",1);
			}else{
				return String();
			}
		}
	}
	bb_config_InternalErr(String(L"Internal error",14));
	return String();
}
void c_BinaryLogicExpr::mark(){
	c_BinaryExpr::mark();
}
c_VarDecl::c_VarDecl(){
}
c_VarDecl* c_VarDecl::m_new(){
	c_ValDecl::m_new();
	return this;
}
void c_VarDecl::mark(){
	c_ValDecl::mark();
}
c_GlobalDecl::c_GlobalDecl(){
}
c_GlobalDecl* c_GlobalDecl::m_new(String t_ident,int t_attrs,c_Type* t_type,c_Expr* t_init){
	c_VarDecl::m_new();
	this->m_ident=t_ident;
	this->m_attrs=t_attrs;
	gc_assign(this->m_type,t_type);
	gc_assign(this->m_init,t_init);
	return this;
}
c_GlobalDecl* c_GlobalDecl::m_new2(){
	c_VarDecl::m_new();
	return this;
}
String c_GlobalDecl::p_ToString(){
	return String(L"Global ",7)+c_ValDecl::p_ToString();
}
c_Decl* c_GlobalDecl::p_OnCopy(){
	return ((new c_GlobalDecl)->m_new(m_ident,m_attrs,m_type,p_CopyInit()));
}
void c_GlobalDecl::mark(){
	c_VarDecl::mark();
}
c_FieldDecl::c_FieldDecl(){
}
c_FieldDecl* c_FieldDecl::m_new(String t_ident,int t_attrs,c_Type* t_type,c_Expr* t_init){
	c_VarDecl::m_new();
	this->m_ident=t_ident;
	this->m_attrs=t_attrs;
	gc_assign(this->m_type,t_type);
	gc_assign(this->m_init,t_init);
	return this;
}
c_FieldDecl* c_FieldDecl::m_new2(){
	c_VarDecl::m_new();
	return this;
}
String c_FieldDecl::p_ToString(){
	return String(L"Field ",6)+c_ValDecl::p_ToString();
}
c_Decl* c_FieldDecl::p_OnCopy(){
	return ((new c_FieldDecl)->m_new(m_ident,m_attrs,m_type,p_CopyInit()));
}
void c_FieldDecl::mark(){
	c_VarDecl::mark();
}
c_LocalDecl::c_LocalDecl(){
}
c_LocalDecl* c_LocalDecl::m_new(String t_ident,int t_attrs,c_Type* t_type,c_Expr* t_init){
	c_VarDecl::m_new();
	this->m_ident=t_ident;
	this->m_attrs=t_attrs;
	gc_assign(this->m_type,t_type);
	gc_assign(this->m_init,t_init);
	return this;
}
c_LocalDecl* c_LocalDecl::m_new2(){
	c_VarDecl::m_new();
	return this;
}
String c_LocalDecl::p_ToString(){
	return String(L"Local ",6)+c_ValDecl::p_ToString();
}
c_Decl* c_LocalDecl::p_OnCopy(){
	return ((new c_LocalDecl)->m_new(m_ident,m_attrs,m_type,p_CopyInit()));
}
void c_LocalDecl::mark(){
	c_VarDecl::mark();
}
c_Enumerator2::c_Enumerator2(){
	m__list=0;
	m__curr=0;
}
c_Enumerator2* c_Enumerator2::m_new(c_List3* t_list){
	gc_assign(m__list,t_list);
	gc_assign(m__curr,t_list->m__head->m__succ);
	return this;
}
c_Enumerator2* c_Enumerator2::m_new2(){
	return this;
}
bool c_Enumerator2::p_HasNext(){
	while(m__curr->m__succ->m__pred!=m__curr){
		gc_assign(m__curr,m__curr->m__succ);
	}
	return m__curr!=m__list->m__head;
}
c_Decl* c_Enumerator2::p_NextObject(){
	c_Decl* t_data=m__curr->m__data;
	gc_assign(m__curr,m__curr->m__succ);
	return t_data;
}
void c_Enumerator2::mark(){
	Object::mark();
	gc_mark_q(m__list);
	gc_mark_q(m__curr);
}
c_DeclStmt::c_DeclStmt(){
	m_decl=0;
}
c_DeclStmt* c_DeclStmt::m_new(c_Decl* t_decl){
	c_Stmt::m_new();
	gc_assign(this->m_decl,t_decl);
	return this;
}
c_DeclStmt* c_DeclStmt::m_new2(String t_id,c_Type* t_ty,c_Expr* t_init){
	c_Stmt::m_new();
	gc_assign(this->m_decl,((new c_LocalDecl)->m_new(t_id,0,t_ty,t_init)));
	return this;
}
c_DeclStmt* c_DeclStmt::m_new3(){
	c_Stmt::m_new();
	return this;
}
c_Stmt* c_DeclStmt::p_OnCopy2(c_ScopeDecl* t_scope){
	return ((new c_DeclStmt)->m_new(m_decl->p_Copy()));
}
int c_DeclStmt::p_OnSemant(){
	m_decl->p_Semant();
	bb_decl__env->p_InsertDecl(m_decl);
	return 0;
}
String c_DeclStmt::p_Trans(){
	return bb_translator__trans->p_TransDeclStmt(this);
}
void c_DeclStmt::mark(){
	c_Stmt::mark();
	gc_mark_q(m_decl);
}
c_Stack5::c_Stack5(){
	m_data=Array<c_IdentType* >();
	m_length=0;
}
c_Stack5* c_Stack5::m_new(){
	return this;
}
c_Stack5* c_Stack5::m_new2(Array<c_IdentType* > t_data){
	gc_assign(this->m_data,t_data.Slice(0));
	this->m_length=t_data.Length();
	return this;
}
void c_Stack5::p_Push13(c_IdentType* t_value){
	if(m_length==m_data.Length()){
		gc_assign(m_data,m_data.Resize(m_length*2+10));
	}
	gc_assign(m_data[m_length],t_value);
	m_length+=1;
}
void c_Stack5::p_Push14(Array<c_IdentType* > t_values,int t_offset,int t_count){
	for(int t_i=0;t_i<t_count;t_i=t_i+1){
		p_Push13(t_values[t_offset+t_i]);
	}
}
void c_Stack5::p_Push15(Array<c_IdentType* > t_values,int t_offset){
	p_Push14(t_values,t_offset,t_values.Length()-t_offset);
}
Array<c_IdentType* > c_Stack5::p_ToArray(){
	Array<c_IdentType* > t_t=Array<c_IdentType* >(m_length);
	for(int t_i=0;t_i<m_length;t_i=t_i+1){
		gc_assign(t_t[t_i],m_data[t_i]);
	}
	return t_t;
}
void c_Stack5::mark(){
	Object::mark();
	gc_mark_q(m_data);
}
c_ObjectType::c_ObjectType(){
	m_classDecl=0;
}
c_ObjectType* c_ObjectType::m_new(c_ClassDecl* t_classDecl){
	c_Type::m_new();
	gc_assign(this->m_classDecl,t_classDecl);
	return this;
}
c_ObjectType* c_ObjectType::m_new2(){
	c_Type::m_new();
	return this;
}
int c_ObjectType::p_EqualsType(c_Type* t_ty){
	c_ObjectType* t_objty=dynamic_cast<c_ObjectType*>(t_ty);
	return ((((t_objty)!=0) && m_classDecl==t_objty->m_classDecl)?1:0);
}
c_ClassDecl* c_ObjectType::p_GetClass(){
	return m_classDecl;
}
int c_ObjectType::p_ExtendsType(c_Type* t_ty){
	c_ObjectType* t_objty=dynamic_cast<c_ObjectType*>(t_ty);
	if((t_objty)!=0){
		return m_classDecl->p_ExtendsClass(t_objty->m_classDecl);
	}
	String t_op=String();
	if((dynamic_cast<c_BoolType*>(t_ty))!=0){
		t_op=String(L"ToBool",6);
	}else{
		if((dynamic_cast<c_IntType*>(t_ty))!=0){
			t_op=String(L"ToInt",5);
		}else{
			if((dynamic_cast<c_FloatType*>(t_ty))!=0){
				t_op=String(L"ToFloat",7);
			}else{
				if((dynamic_cast<c_StringType*>(t_ty))!=0){
					t_op=String(L"ToString",8);
				}else{
					return 0;
				}
			}
		}
	}
	c_FuncDecl* t_fdecl=p_GetClass()->p_FindFuncDecl(t_op,Array<c_Expr* >(),1);
	return ((((t_fdecl)!=0) && t_fdecl->p_IsMethod() && ((t_fdecl->m_retType->p_EqualsType(t_ty))!=0))?1:0);
}
String c_ObjectType::p_ToString(){
	return m_classDecl->p_ToString();
}
void c_ObjectType::mark(){
	c_Type::mark();
	gc_mark_q(m_classDecl);
}
c_List6::c_List6(){
	m__head=((new c_HeadNode6)->m_new());
}
c_List6* c_List6::m_new(){
	return this;
}
c_Node11* c_List6::p_AddLast6(c_ClassDecl* t_data){
	return (new c_Node11)->m_new(m__head,m__head->m__pred,t_data);
}
c_List6* c_List6::m_new2(Array<c_ClassDecl* > t_data){
	Array<c_ClassDecl* > t_=t_data;
	int t_2=0;
	while(t_2<t_.Length()){
		c_ClassDecl* t_t=t_[t_2];
		t_2=t_2+1;
		p_AddLast6(t_t);
	}
	return this;
}
c_Enumerator4* c_List6::p_ObjectEnumerator(){
	return (new c_Enumerator4)->m_new(this);
}
void c_List6::mark(){
	Object::mark();
	gc_mark_q(m__head);
}
c_Node11::c_Node11(){
	m__succ=0;
	m__pred=0;
	m__data=0;
}
c_Node11* c_Node11::m_new(c_Node11* t_succ,c_Node11* t_pred,c_ClassDecl* t_data){
	gc_assign(m__succ,t_succ);
	gc_assign(m__pred,t_pred);
	gc_assign(m__succ->m__pred,this);
	gc_assign(m__pred->m__succ,this);
	gc_assign(m__data,t_data);
	return this;
}
c_Node11* c_Node11::m_new2(){
	return this;
}
void c_Node11::mark(){
	Object::mark();
	gc_mark_q(m__succ);
	gc_mark_q(m__pred);
	gc_mark_q(m__data);
}
c_HeadNode6::c_HeadNode6(){
}
c_HeadNode6* c_HeadNode6::m_new(){
	c_Node11::m_new2();
	gc_assign(m__succ,(this));
	gc_assign(m__pred,(this));
	return this;
}
void c_HeadNode6::mark(){
	c_Node11::mark();
}
c_ArgDecl::c_ArgDecl(){
}
c_ArgDecl* c_ArgDecl::m_new(String t_ident,int t_attrs,c_Type* t_type,c_Expr* t_init){
	c_LocalDecl::m_new2();
	this->m_ident=t_ident;
	this->m_attrs=t_attrs;
	gc_assign(this->m_type,t_type);
	gc_assign(this->m_init,t_init);
	return this;
}
c_ArgDecl* c_ArgDecl::m_new2(){
	c_LocalDecl::m_new2();
	return this;
}
String c_ArgDecl::p_ToString(){
	return c_LocalDecl::p_ToString();
}
c_Decl* c_ArgDecl::p_OnCopy(){
	return ((new c_ArgDecl)->m_new(m_ident,m_attrs,m_type,p_CopyInit()));
}
void c_ArgDecl::mark(){
	c_LocalDecl::mark();
}
c_Stack6::c_Stack6(){
	m_data=Array<c_ArgDecl* >();
	m_length=0;
}
c_Stack6* c_Stack6::m_new(){
	return this;
}
c_Stack6* c_Stack6::m_new2(Array<c_ArgDecl* > t_data){
	gc_assign(this->m_data,t_data.Slice(0));
	this->m_length=t_data.Length();
	return this;
}
void c_Stack6::p_Push16(c_ArgDecl* t_value){
	if(m_length==m_data.Length()){
		gc_assign(m_data,m_data.Resize(m_length*2+10));
	}
	gc_assign(m_data[m_length],t_value);
	m_length+=1;
}
void c_Stack6::p_Push17(Array<c_ArgDecl* > t_values,int t_offset,int t_count){
	for(int t_i=0;t_i<t_count;t_i=t_i+1){
		p_Push16(t_values[t_offset+t_i]);
	}
}
void c_Stack6::p_Push18(Array<c_ArgDecl* > t_values,int t_offset){
	p_Push17(t_values,t_offset,t_values.Length()-t_offset);
}
Array<c_ArgDecl* > c_Stack6::p_ToArray(){
	Array<c_ArgDecl* > t_t=Array<c_ArgDecl* >(m_length);
	for(int t_i=0;t_i<m_length;t_i=t_i+1){
		gc_assign(t_t[t_i],m_data[t_i]);
	}
	return t_t;
}
void c_Stack6::mark(){
	Object::mark();
	gc_mark_q(m_data);
}
c_List7::c_List7(){
	m__head=((new c_HeadNode7)->m_new());
}
c_List7* c_List7::m_new(){
	return this;
}
c_Node12* c_List7::p_AddLast7(c_BlockDecl* t_data){
	return (new c_Node12)->m_new(m__head,m__head->m__pred,t_data);
}
c_List7* c_List7::m_new2(Array<c_BlockDecl* > t_data){
	Array<c_BlockDecl* > t_=t_data;
	int t_2=0;
	while(t_2<t_.Length()){
		c_BlockDecl* t_t=t_[t_2];
		t_2=t_2+1;
		p_AddLast7(t_t);
	}
	return this;
}
c_BlockDecl* c_List7::p_RemoveLast(){
	c_BlockDecl* t_data=m__head->m__pred->m__data;
	m__head->m__pred->p_Remove();
	return t_data;
}
bool c_List7::p_Equals3(c_BlockDecl* t_lhs,c_BlockDecl* t_rhs){
	return t_lhs==t_rhs;
}
c_Node12* c_List7::p_FindLast5(c_BlockDecl* t_value,c_Node12* t_start){
	while(t_start!=m__head){
		if(p_Equals3(t_value,t_start->m__data)){
			return t_start;
		}
		t_start=t_start->m__pred;
	}
	return 0;
}
c_Node12* c_List7::p_FindLast6(c_BlockDecl* t_value){
	return p_FindLast5(t_value,m__head->m__pred);
}
void c_List7::p_RemoveLast4(c_BlockDecl* t_value){
	c_Node12* t_node=p_FindLast6(t_value);
	if((t_node)!=0){
		t_node->p_Remove();
	}
}
void c_List7::mark(){
	Object::mark();
	gc_mark_q(m__head);
}
c_Node12::c_Node12(){
	m__succ=0;
	m__pred=0;
	m__data=0;
}
c_Node12* c_Node12::m_new(c_Node12* t_succ,c_Node12* t_pred,c_BlockDecl* t_data){
	gc_assign(m__succ,t_succ);
	gc_assign(m__pred,t_pred);
	gc_assign(m__succ->m__pred,this);
	gc_assign(m__pred->m__succ,this);
	gc_assign(m__data,t_data);
	return this;
}
c_Node12* c_Node12::m_new2(){
	return this;
}
int c_Node12::p_Remove(){
	gc_assign(m__succ->m__pred,m__pred);
	gc_assign(m__pred->m__succ,m__succ);
	return 0;
}
void c_Node12::mark(){
	Object::mark();
	gc_mark_q(m__succ);
	gc_mark_q(m__pred);
	gc_mark_q(m__data);
}
c_HeadNode7::c_HeadNode7(){
}
c_HeadNode7* c_HeadNode7::m_new(){
	c_Node12::m_new2();
	gc_assign(m__succ,(this));
	gc_assign(m__pred,(this));
	return this;
}
void c_HeadNode7::mark(){
	c_Node12::mark();
}
c_ReturnStmt::c_ReturnStmt(){
	m_expr=0;
}
c_ReturnStmt* c_ReturnStmt::m_new(c_Expr* t_expr){
	c_Stmt::m_new();
	gc_assign(this->m_expr,t_expr);
	return this;
}
c_ReturnStmt* c_ReturnStmt::m_new2(){
	c_Stmt::m_new();
	return this;
}
c_Stmt* c_ReturnStmt::p_OnCopy2(c_ScopeDecl* t_scope){
	if((m_expr)!=0){
		return ((new c_ReturnStmt)->m_new(m_expr->p_Copy()));
	}
	return ((new c_ReturnStmt)->m_new(0));
}
int c_ReturnStmt::p_OnSemant(){
	c_FuncDecl* t_fdecl=bb_decl__env->p_FuncScope();
	if((m_expr)!=0){
		if(t_fdecl->p_IsCtor()){
			bb_config_Err(String(L"Constructors may not return a value.",36));
		}
		if((dynamic_cast<c_VoidType*>(t_fdecl->m_retType))!=0){
			bb_config_Err(String(L"Void functions may not return a value.",38));
		}
		gc_assign(m_expr,m_expr->p_Semant2(t_fdecl->m_retType,0));
	}else{
		if(t_fdecl->p_IsCtor()){
			gc_assign(m_expr,((new c_SelfExpr)->m_new())->p_Semant());
		}else{
			if(!((dynamic_cast<c_VoidType*>(t_fdecl->m_retType))!=0)){
				if((bb_decl__env->p_ModuleScope()->p_IsStrict())!=0){
					bb_config_Err(String(L"Missing return expression.",26));
				}
				gc_assign(m_expr,((new c_ConstExpr)->m_new(t_fdecl->m_retType,String()))->p_Semant());
			}
		}
	}
	return 0;
}
String c_ReturnStmt::p_Trans(){
	return bb_translator__trans->p_TransReturnStmt(this);
}
void c_ReturnStmt::mark(){
	c_Stmt::mark();
	gc_mark_q(m_expr);
}
c_BreakStmt::c_BreakStmt(){
}
c_BreakStmt* c_BreakStmt::m_new(){
	c_Stmt::m_new();
	return this;
}
c_Stmt* c_BreakStmt::p_OnCopy2(c_ScopeDecl* t_scope){
	return ((new c_BreakStmt)->m_new());
}
int c_BreakStmt::p_OnSemant(){
	if(!((bb_decl__loopnest)!=0)){
		bb_config_Err(String(L"Exit statement must appear inside a loop.",41));
	}
	return 0;
}
String c_BreakStmt::p_Trans(){
	return bb_translator__trans->p_TransBreakStmt(this);
}
void c_BreakStmt::mark(){
	c_Stmt::mark();
}
c_ContinueStmt::c_ContinueStmt(){
}
c_ContinueStmt* c_ContinueStmt::m_new(){
	c_Stmt::m_new();
	return this;
}
c_Stmt* c_ContinueStmt::p_OnCopy2(c_ScopeDecl* t_scope){
	return ((new c_ContinueStmt)->m_new());
}
int c_ContinueStmt::p_OnSemant(){
	if(!((bb_decl__loopnest)!=0)){
		bb_config_Err(String(L"Continue statement must appear inside a loop.",45));
	}
	return 0;
}
String c_ContinueStmt::p_Trans(){
	return bb_translator__trans->p_TransContinueStmt(this);
}
void c_ContinueStmt::mark(){
	c_Stmt::mark();
}
c_IfStmt::c_IfStmt(){
	m_expr=0;
	m_thenBlock=0;
	m_elseBlock=0;
}
c_IfStmt* c_IfStmt::m_new(c_Expr* t_expr,c_BlockDecl* t_thenBlock,c_BlockDecl* t_elseBlock){
	c_Stmt::m_new();
	gc_assign(this->m_expr,t_expr);
	gc_assign(this->m_thenBlock,t_thenBlock);
	gc_assign(this->m_elseBlock,t_elseBlock);
	return this;
}
c_IfStmt* c_IfStmt::m_new2(){
	c_Stmt::m_new();
	return this;
}
c_Stmt* c_IfStmt::p_OnCopy2(c_ScopeDecl* t_scope){
	return ((new c_IfStmt)->m_new(m_expr->p_Copy(),m_thenBlock->p_CopyBlock(t_scope),m_elseBlock->p_CopyBlock(t_scope)));
}
int c_IfStmt::p_OnSemant(){
	gc_assign(m_expr,m_expr->p_Semant2((c_Type::m_boolType),1));
	m_thenBlock->p_Semant();
	m_elseBlock->p_Semant();
	return 0;
}
String c_IfStmt::p_Trans(){
	return bb_translator__trans->p_TransIfStmt(this);
}
void c_IfStmt::mark(){
	c_Stmt::mark();
	gc_mark_q(m_expr);
	gc_mark_q(m_thenBlock);
	gc_mark_q(m_elseBlock);
}
c_WhileStmt::c_WhileStmt(){
	m_expr=0;
	m_block=0;
}
c_WhileStmt* c_WhileStmt::m_new(c_Expr* t_expr,c_BlockDecl* t_block){
	c_Stmt::m_new();
	gc_assign(this->m_expr,t_expr);
	gc_assign(this->m_block,t_block);
	return this;
}
c_WhileStmt* c_WhileStmt::m_new2(){
	c_Stmt::m_new();
	return this;
}
c_Stmt* c_WhileStmt::p_OnCopy2(c_ScopeDecl* t_scope){
	return ((new c_WhileStmt)->m_new(m_expr->p_Copy(),m_block->p_CopyBlock(t_scope)));
}
int c_WhileStmt::p_OnSemant(){
	gc_assign(m_expr,m_expr->p_Semant2((c_Type::m_boolType),1));
	bb_decl__loopnest+=1;
	m_block->p_Semant();
	bb_decl__loopnest-=1;
	return 0;
}
String c_WhileStmt::p_Trans(){
	return bb_translator__trans->p_TransWhileStmt(this);
}
void c_WhileStmt::mark(){
	c_Stmt::mark();
	gc_mark_q(m_expr);
	gc_mark_q(m_block);
}
c_RepeatStmt::c_RepeatStmt(){
	m_block=0;
	m_expr=0;
}
c_RepeatStmt* c_RepeatStmt::m_new(c_BlockDecl* t_block,c_Expr* t_expr){
	c_Stmt::m_new();
	gc_assign(this->m_block,t_block);
	gc_assign(this->m_expr,t_expr);
	return this;
}
c_RepeatStmt* c_RepeatStmt::m_new2(){
	c_Stmt::m_new();
	return this;
}
c_Stmt* c_RepeatStmt::p_OnCopy2(c_ScopeDecl* t_scope){
	return ((new c_RepeatStmt)->m_new(m_block->p_CopyBlock(t_scope),m_expr->p_Copy()));
}
int c_RepeatStmt::p_OnSemant(){
	bb_decl__loopnest+=1;
	m_block->p_Semant();
	bb_decl__loopnest-=1;
	gc_assign(m_expr,m_expr->p_Semant2((c_Type::m_boolType),1));
	return 0;
}
String c_RepeatStmt::p_Trans(){
	return bb_translator__trans->p_TransRepeatStmt(this);
}
void c_RepeatStmt::mark(){
	c_Stmt::mark();
	gc_mark_q(m_block);
	gc_mark_q(m_expr);
}
c_ForEachinStmt::c_ForEachinStmt(){
	m_varid=String();
	m_varty=0;
	m_varlocal=0;
	m_expr=0;
	m_block=0;
}
c_ForEachinStmt* c_ForEachinStmt::m_new(String t_varid,c_Type* t_varty,int t_varlocal,c_Expr* t_expr,c_BlockDecl* t_block){
	c_Stmt::m_new();
	this->m_varid=t_varid;
	gc_assign(this->m_varty,t_varty);
	this->m_varlocal=t_varlocal;
	gc_assign(this->m_expr,t_expr);
	gc_assign(this->m_block,t_block);
	return this;
}
c_ForEachinStmt* c_ForEachinStmt::m_new2(){
	c_Stmt::m_new();
	return this;
}
c_Stmt* c_ForEachinStmt::p_OnCopy2(c_ScopeDecl* t_scope){
	return ((new c_ForEachinStmt)->m_new(m_varid,m_varty,m_varlocal,m_expr->p_Copy(),m_block->p_CopyBlock(t_scope)));
}
int c_ForEachinStmt::p_OnSemant(){
	gc_assign(m_expr,m_expr->p_Semant());
	if(((dynamic_cast<c_ArrayType*>(m_expr->m_exprType))!=0) || ((dynamic_cast<c_StringType*>(m_expr->m_exprType))!=0)){
		c_LocalDecl* t_exprTmp=(new c_LocalDecl)->m_new(String(),0,0,m_expr);
		c_LocalDecl* t_indexTmp=(new c_LocalDecl)->m_new(String(),0,0,((new c_ConstExpr)->m_new((c_Type::m_intType),String(L"0",1))));
		c_Expr* t_lenExpr=((new c_IdentExpr)->m_new(String(L"Length",6),((new c_VarExpr)->m_new(t_exprTmp))));
		c_Expr* t_cmpExpr=((new c_BinaryCompareExpr)->m_new(String(L"<",1),((new c_VarExpr)->m_new(t_indexTmp)),t_lenExpr));
		c_Expr* t_indexExpr=((new c_IndexExpr)->m_new(((new c_VarExpr)->m_new(t_exprTmp)),((new c_VarExpr)->m_new(t_indexTmp))));
		c_Expr* t_addExpr=((new c_BinaryMathExpr)->m_new(String(L"+",1),((new c_VarExpr)->m_new(t_indexTmp)),((new c_ConstExpr)->m_new((c_Type::m_intType),String(L"1",1)))));
		m_block->m_stmts->p_AddFirst((new c_AssignStmt)->m_new(String(L"=",1),((new c_VarExpr)->m_new(t_indexTmp)),t_addExpr));
		if((m_varlocal)!=0){
			c_LocalDecl* t_varTmp=(new c_LocalDecl)->m_new(m_varid,0,m_varty,t_indexExpr);
			m_block->m_stmts->p_AddFirst((new c_DeclStmt)->m_new(t_varTmp));
		}else{
			m_block->m_stmts->p_AddFirst((new c_AssignStmt)->m_new(String(L"=",1),((new c_IdentExpr)->m_new(m_varid,0)),t_indexExpr));
		}
		c_WhileStmt* t_whileStmt=(new c_WhileStmt)->m_new(t_cmpExpr,m_block);
		gc_assign(m_block,(new c_BlockDecl)->m_new(m_block->m_scope));
		m_block->p_AddStmt((new c_DeclStmt)->m_new(t_exprTmp));
		m_block->p_AddStmt((new c_DeclStmt)->m_new(t_indexTmp));
		m_block->p_AddStmt(t_whileStmt);
	}else{
		if((dynamic_cast<c_ObjectType*>(m_expr->m_exprType))!=0){
			c_Expr* t_enumerInit=((new c_FuncCallExpr)->m_new(((new c_IdentExpr)->m_new(String(L"ObjectEnumerator",16),m_expr)),Array<c_Expr* >()));
			c_LocalDecl* t_enumerTmp=(new c_LocalDecl)->m_new(String(),0,0,t_enumerInit);
			c_Expr* t_hasNextExpr=((new c_FuncCallExpr)->m_new(((new c_IdentExpr)->m_new(String(L"HasNext",7),((new c_VarExpr)->m_new(t_enumerTmp)))),Array<c_Expr* >()));
			c_Expr* t_nextObjExpr=((new c_FuncCallExpr)->m_new(((new c_IdentExpr)->m_new(String(L"NextObject",10),((new c_VarExpr)->m_new(t_enumerTmp)))),Array<c_Expr* >()));
			if((m_varlocal)!=0){
				c_LocalDecl* t_varTmp2=(new c_LocalDecl)->m_new(m_varid,0,m_varty,t_nextObjExpr);
				m_block->m_stmts->p_AddFirst((new c_DeclStmt)->m_new(t_varTmp2));
			}else{
				m_block->m_stmts->p_AddFirst((new c_AssignStmt)->m_new(String(L"=",1),((new c_IdentExpr)->m_new(m_varid,0)),t_nextObjExpr));
			}
			c_WhileStmt* t_whileStmt2=(new c_WhileStmt)->m_new(t_hasNextExpr,m_block);
			gc_assign(m_block,(new c_BlockDecl)->m_new(m_block->m_scope));
			m_block->p_AddStmt((new c_DeclStmt)->m_new(t_enumerTmp));
			m_block->p_AddStmt(t_whileStmt2);
		}else{
			bb_config_Err(String(L"Expression cannot be used with For Each.",40));
		}
	}
	m_block->p_Semant();
	return 0;
}
String c_ForEachinStmt::p_Trans(){
	return bb_translator__trans->p_TransBlock(m_block);
}
void c_ForEachinStmt::mark(){
	c_Stmt::mark();
	gc_mark_q(m_varty);
	gc_mark_q(m_expr);
	gc_mark_q(m_block);
}
c_AssignStmt::c_AssignStmt(){
	m_op=String();
	m_lhs=0;
	m_rhs=0;
	m_tmp1=0;
	m_tmp2=0;
}
c_AssignStmt* c_AssignStmt::m_new(String t_op,c_Expr* t_lhs,c_Expr* t_rhs){
	c_Stmt::m_new();
	this->m_op=t_op;
	gc_assign(this->m_lhs,t_lhs);
	gc_assign(this->m_rhs,t_rhs);
	return this;
}
c_AssignStmt* c_AssignStmt::m_new2(){
	c_Stmt::m_new();
	return this;
}
c_Stmt* c_AssignStmt::p_OnCopy2(c_ScopeDecl* t_scope){
	return ((new c_AssignStmt)->m_new(m_op,m_lhs->p_Copy(),m_rhs->p_Copy()));
}
int c_AssignStmt::p_FixSideEffects(){
	c_MemberVarExpr* t_e1=dynamic_cast<c_MemberVarExpr*>(m_lhs);
	if((t_e1)!=0){
		if(t_e1->m_expr->p_SideEffects()){
			gc_assign(m_tmp1,(new c_LocalDecl)->m_new(String(),0,t_e1->m_expr->m_exprType,t_e1->m_expr));
			m_tmp1->p_Semant();
			gc_assign(m_lhs,((new c_MemberVarExpr)->m_new(((new c_VarExpr)->m_new(m_tmp1)),t_e1->m_decl)));
		}
	}
	c_IndexExpr* t_e2=dynamic_cast<c_IndexExpr*>(m_lhs);
	if((t_e2)!=0){
		c_Expr* t_expr=t_e2->m_expr;
		c_Expr* t_index=t_e2->m_index;
		if(t_expr->p_SideEffects() || t_index->p_SideEffects()){
			if(t_expr->p_SideEffects()){
				gc_assign(m_tmp1,(new c_LocalDecl)->m_new(String(),0,t_expr->m_exprType,t_expr));
				m_tmp1->p_Semant();
				t_expr=((new c_VarExpr)->m_new(m_tmp1));
			}
			if(t_index->p_SideEffects()){
				gc_assign(m_tmp2,(new c_LocalDecl)->m_new(String(),0,t_index->m_exprType,t_index));
				m_tmp2->p_Semant();
				t_index=((new c_VarExpr)->m_new(m_tmp2));
			}
			gc_assign(m_lhs,((new c_IndexExpr)->m_new(t_expr,t_index))->p_Semant());
		}
	}
	return 0;
}
int c_AssignStmt::p_OnSemant(){
	gc_assign(m_rhs,m_rhs->p_Semant());
	gc_assign(m_lhs,m_lhs->p_SemantSet(m_op,m_rhs));
	if(((dynamic_cast<c_InvokeExpr*>(m_lhs))!=0) || ((dynamic_cast<c_InvokeMemberExpr*>(m_lhs))!=0)){
		m_rhs=0;
		return 0;
	}
	bool t_kludge=true;
	String t_1=m_op;
	if(t_1==String(L"=",1)){
		gc_assign(m_rhs,m_rhs->p_Cast(m_lhs->m_exprType,0));
		t_kludge=false;
	}else{
		if(t_1==String(L"*=",2) || t_1==String(L"/=",2) || t_1==String(L"+=",2) || t_1==String(L"-=",2)){
			if(((dynamic_cast<c_NumericType*>(m_lhs->m_exprType))!=0) && ((m_lhs->m_exprType->p_EqualsType(m_rhs->m_exprType))!=0)){
				t_kludge=false;
				if(bb_config_ENV_LANG==String(L"js",2)){
					if(m_op==String(L"/=",2) && ((dynamic_cast<c_IntType*>(m_lhs->m_exprType))!=0)){
						t_kludge=true;
					}
				}
			}
		}else{
			if(t_1==String(L"&=",2) || t_1==String(L"|=",2) || t_1==String(L"~=",2) || t_1==String(L"shl=",4) || t_1==String(L"shr=",4) || t_1==String(L"mod=",4)){
				if(((dynamic_cast<c_IntType*>(m_lhs->m_exprType))!=0) && ((m_lhs->m_exprType->p_EqualsType(m_rhs->m_exprType))!=0)){
					t_kludge=false;
				}
			}else{
				bb_config_InternalErr(String(L"Internal error",14));
			}
		}
	}
	if(bb_config_ENV_LANG==String()){
		t_kludge=true;
	}
	if(t_kludge){
		p_FixSideEffects();
		gc_assign(m_rhs,((new c_BinaryMathExpr)->m_new(m_op.Slice(0,-1),m_lhs,m_rhs))->p_Semant()->p_Cast(m_lhs->m_exprType,0));
		m_op=String(L"=",1);
	}
	return 0;
}
String c_AssignStmt::p_Trans(){
	bb_config__errInfo=m_errInfo;
	return bb_translator__trans->p_TransAssignStmt(this);
}
void c_AssignStmt::mark(){
	c_Stmt::mark();
	gc_mark_q(m_lhs);
	gc_mark_q(m_rhs);
	gc_mark_q(m_tmp1);
	gc_mark_q(m_tmp2);
}
c_ForStmt::c_ForStmt(){
	m_init=0;
	m_expr=0;
	m_incr=0;
	m_block=0;
}
c_ForStmt* c_ForStmt::m_new(c_Stmt* t_init,c_Expr* t_expr,c_Stmt* t_incr,c_BlockDecl* t_block){
	c_Stmt::m_new();
	gc_assign(this->m_init,t_init);
	gc_assign(this->m_expr,t_expr);
	gc_assign(this->m_incr,t_incr);
	gc_assign(this->m_block,t_block);
	return this;
}
c_ForStmt* c_ForStmt::m_new2(){
	c_Stmt::m_new();
	return this;
}
c_Stmt* c_ForStmt::p_OnCopy2(c_ScopeDecl* t_scope){
	return ((new c_ForStmt)->m_new(m_init->p_Copy2(t_scope),m_expr->p_Copy(),m_incr->p_Copy2(t_scope),m_block->p_CopyBlock(t_scope)));
}
int c_ForStmt::p_OnSemant(){
	bb_decl_PushEnv(m_block);
	m_init->p_Semant();
	gc_assign(m_expr,m_expr->p_Semant());
	bb_decl__loopnest+=1;
	m_block->p_Semant();
	bb_decl__loopnest-=1;
	m_incr->p_Semant();
	bb_decl_PopEnv();
	c_AssignStmt* t_assop=dynamic_cast<c_AssignStmt*>(m_incr);
	c_BinaryExpr* t_addop=dynamic_cast<c_BinaryExpr*>(t_assop->m_rhs);
	if(!((t_addop)!=0)){
		bb_config_Err(String(L"Invalid step expression",23));
	}
	String t_stpval=t_addop->m_rhs->p_Eval();
	if(t_stpval.StartsWith(String(L"-",1))){
		c_BinaryExpr* t_bexpr=dynamic_cast<c_BinaryExpr*>(m_expr);
		String t_2=t_bexpr->m_op;
		if(t_2==String(L"<",1)){
			t_bexpr->m_op=String(L">",1);
		}else{
			if(t_2==String(L"<=",2)){
				t_bexpr->m_op=String(L">=",2);
			}
		}
	}
	return 0;
}
String c_ForStmt::p_Trans(){
	return bb_translator__trans->p_TransForStmt(this);
}
void c_ForStmt::mark(){
	c_Stmt::mark();
	gc_mark_q(m_init);
	gc_mark_q(m_expr);
	gc_mark_q(m_incr);
	gc_mark_q(m_block);
}
c_CatchStmt::c_CatchStmt(){
	m_init=0;
	m_block=0;
}
c_CatchStmt* c_CatchStmt::m_new(c_LocalDecl* t_init,c_BlockDecl* t_block){
	c_Stmt::m_new();
	gc_assign(this->m_init,t_init);
	gc_assign(this->m_block,t_block);
	return this;
}
c_CatchStmt* c_CatchStmt::m_new2(){
	c_Stmt::m_new();
	return this;
}
c_Stmt* c_CatchStmt::p_OnCopy2(c_ScopeDecl* t_scope){
	return ((new c_CatchStmt)->m_new(dynamic_cast<c_LocalDecl*>(m_init->p_Copy()),m_block->p_CopyBlock(t_scope)));
}
int c_CatchStmt::p_OnSemant(){
	m_init->p_Semant();
	if(!((dynamic_cast<c_ObjectType*>(m_init->m_type))!=0)){
		bb_config_Err(String(L"Variable type must extend Throwable",35));
	}
	if(!((m_init->m_type->p_GetClass()->p_IsThrowable())!=0)){
		bb_config_Err(String(L"Variable type must extend Throwable",35));
	}
	m_block->p_InsertDecl(m_init);
	m_block->p_Semant();
	return 0;
}
String c_CatchStmt::p_Trans(){
	return String();
}
void c_CatchStmt::mark(){
	c_Stmt::mark();
	gc_mark_q(m_init);
	gc_mark_q(m_block);
}
c_Stack7::c_Stack7(){
	m_data=Array<c_CatchStmt* >();
	m_length=0;
}
c_Stack7* c_Stack7::m_new(){
	return this;
}
c_Stack7* c_Stack7::m_new2(Array<c_CatchStmt* > t_data){
	gc_assign(this->m_data,t_data.Slice(0));
	this->m_length=t_data.Length();
	return this;
}
void c_Stack7::p_Push19(c_CatchStmt* t_value){
	if(m_length==m_data.Length()){
		gc_assign(m_data,m_data.Resize(m_length*2+10));
	}
	gc_assign(m_data[m_length],t_value);
	m_length+=1;
}
void c_Stack7::p_Push20(Array<c_CatchStmt* > t_values,int t_offset,int t_count){
	for(int t_i=0;t_i<t_count;t_i=t_i+1){
		p_Push19(t_values[t_offset+t_i]);
	}
}
void c_Stack7::p_Push21(Array<c_CatchStmt* > t_values,int t_offset){
	p_Push20(t_values,t_offset,t_values.Length()-t_offset);
}
c_CatchStmt* c_Stack7::m_NIL;
void c_Stack7::p_Length(int t_newlength){
	if(t_newlength<m_length){
		for(int t_i=t_newlength;t_i<m_length;t_i=t_i+1){
			gc_assign(m_data[t_i],m_NIL);
		}
	}else{
		if(t_newlength>m_data.Length()){
			gc_assign(m_data,m_data.Resize(bb_math_Max(m_length*2+10,t_newlength)));
		}
	}
	m_length=t_newlength;
}
int c_Stack7::p_Length2(){
	return m_length;
}
Array<c_CatchStmt* > c_Stack7::p_ToArray(){
	Array<c_CatchStmt* > t_t=Array<c_CatchStmt* >(m_length);
	for(int t_i=0;t_i<m_length;t_i=t_i+1){
		gc_assign(t_t[t_i],m_data[t_i]);
	}
	return t_t;
}
void c_Stack7::mark(){
	Object::mark();
	gc_mark_q(m_data);
}
int bb_math_Max(int t_x,int t_y){
	if(t_x>t_y){
		return t_x;
	}
	return t_y;
}
Float bb_math_Max2(Float t_x,Float t_y){
	if(t_x>t_y){
		return t_x;
	}
	return t_y;
}
c_TryStmt::c_TryStmt(){
	m_block=0;
	m_catches=Array<c_CatchStmt* >();
}
c_TryStmt* c_TryStmt::m_new(c_BlockDecl* t_block,Array<c_CatchStmt* > t_catches){
	c_Stmt::m_new();
	gc_assign(this->m_block,t_block);
	gc_assign(this->m_catches,t_catches);
	return this;
}
c_TryStmt* c_TryStmt::m_new2(){
	c_Stmt::m_new();
	return this;
}
c_Stmt* c_TryStmt::p_OnCopy2(c_ScopeDecl* t_scope){
	Array<c_CatchStmt* > t_tcatches=this->m_catches.Slice(0);
	for(int t_i=0;t_i<t_tcatches.Length();t_i=t_i+1){
		gc_assign(t_tcatches[t_i],dynamic_cast<c_CatchStmt*>(t_tcatches[t_i]->p_Copy2(t_scope)));
	}
	return ((new c_TryStmt)->m_new(m_block->p_CopyBlock(t_scope),t_tcatches));
}
int c_TryStmt::p_OnSemant(){
	m_block->p_Semant();
	for(int t_i=0;t_i<m_catches.Length();t_i=t_i+1){
		m_catches[t_i]->p_Semant();
		for(int t_j=0;t_j<t_i;t_j=t_j+1){
			if((m_catches[t_i]->m_init->m_type->p_ExtendsType(m_catches[t_j]->m_init->m_type))!=0){
				bb_config_PushErr(m_catches[t_i]->m_errInfo);
				bb_config_Err(String(L"Catch variable class extends earlier catch variable class",57));
			}
		}
	}
	return 0;
}
String c_TryStmt::p_Trans(){
	return bb_translator__trans->p_TransTryStmt(this);
}
void c_TryStmt::mark(){
	c_Stmt::mark();
	gc_mark_q(m_block);
	gc_mark_q(m_catches);
}
c_ThrowStmt::c_ThrowStmt(){
	m_expr=0;
}
c_ThrowStmt* c_ThrowStmt::m_new(c_Expr* t_expr){
	c_Stmt::m_new();
	gc_assign(this->m_expr,t_expr);
	return this;
}
c_ThrowStmt* c_ThrowStmt::m_new2(){
	c_Stmt::m_new();
	return this;
}
c_Stmt* c_ThrowStmt::p_OnCopy2(c_ScopeDecl* t_scope){
	return ((new c_ThrowStmt)->m_new(m_expr->p_Copy()));
}
int c_ThrowStmt::p_OnSemant(){
	gc_assign(m_expr,m_expr->p_Semant());
	if(!((dynamic_cast<c_ObjectType*>(m_expr->m_exprType))!=0)){
		bb_config_Err(String(L"Expression type must extend Throwable",37));
	}
	if(!((m_expr->m_exprType->p_GetClass()->p_IsThrowable())!=0)){
		bb_config_Err(String(L"Expression type must extend Throwable",37));
	}
	return 0;
}
String c_ThrowStmt::p_Trans(){
	return bb_translator__trans->p_TransThrowStmt(this);
}
void c_ThrowStmt::mark(){
	c_Stmt::mark();
	gc_mark_q(m_expr);
}
c_ExprStmt::c_ExprStmt(){
	m_expr=0;
}
c_ExprStmt* c_ExprStmt::m_new(c_Expr* t_expr){
	c_Stmt::m_new();
	gc_assign(this->m_expr,t_expr);
	return this;
}
c_ExprStmt* c_ExprStmt::m_new2(){
	c_Stmt::m_new();
	return this;
}
c_Stmt* c_ExprStmt::p_OnCopy2(c_ScopeDecl* t_scope){
	return ((new c_ExprStmt)->m_new(m_expr->p_Copy()));
}
int c_ExprStmt::p_OnSemant(){
	gc_assign(m_expr,m_expr->p_Semant());
	if(!((m_expr)!=0)){
		bb_config_InternalErr(String(L"Internal error",14));
	}
	return 0;
}
String c_ExprStmt::p_Trans(){
	return bb_translator__trans->p_TransExprStmt(this);
}
void c_ExprStmt::mark(){
	c_Stmt::mark();
	gc_mark_q(m_expr);
}
c_ModuleDecl* bb_parser_ParseModule(String t_modpath,String t_filepath,c_AppDecl* t_app){
	String t_ident=t_modpath;
	if(t_ident.Contains(String(L".",1))){
		t_ident=bb_os_ExtractExt(t_ident);
	}
	c_ModuleDecl* t_mdecl=(new c_ModuleDecl)->m_new(t_ident,0,String(),t_modpath,t_filepath,t_app);
	t_mdecl->p_ImportModule(String(L"cerberus",8),0);
	String t_source=bb_preprocessor_PreProcess(t_filepath,t_mdecl);
	c_Toker* t_toker=(new c_Toker)->m_new(t_filepath,t_source);
	c_Parser* t_parser=(new c_Parser)->m_new(t_toker,t_app,t_mdecl,0);
	t_parser->p_ParseMain();
	return t_parser->m__module;
}
c_Enumerator3::c_Enumerator3(){
	m__list=0;
	m__curr=0;
}
c_Enumerator3* c_Enumerator3::m_new(c_List4* t_list){
	gc_assign(m__list,t_list);
	gc_assign(m__curr,t_list->m__head->m__succ);
	return this;
}
c_Enumerator3* c_Enumerator3::m_new2(){
	return this;
}
bool c_Enumerator3::p_HasNext(){
	while(m__curr->m__succ->m__pred!=m__curr){
		gc_assign(m__curr,m__curr->m__succ);
	}
	return m__curr!=m__list->m__head;
}
c_FuncDecl* c_Enumerator3::p_NextObject(){
	c_FuncDecl* t_data=m__curr->m__data;
	gc_assign(m__curr,m__curr->m__succ);
	return t_data;
}
void c_Enumerator3::mark(){
	Object::mark();
	gc_mark_q(m__list);
	gc_mark_q(m__curr);
}
c_StringList* bb_config__errStack;
int bb_config_PushErr(String t_errInfo){
	bb_config__errStack->p_AddLast(bb_config__errInfo);
	bb_config__errInfo=t_errInfo;
	return 0;
}
c_List8::c_List8(){
	m__head=((new c_HeadNode8)->m_new());
}
c_List8* c_List8::m_new(){
	return this;
}
c_Node13* c_List8::p_AddLast8(c_GlobalDecl* t_data){
	return (new c_Node13)->m_new(m__head,m__head->m__pred,t_data);
}
c_List8* c_List8::m_new2(Array<c_GlobalDecl* > t_data){
	Array<c_GlobalDecl* > t_=t_data;
	int t_2=0;
	while(t_2<t_.Length()){
		c_GlobalDecl* t_t=t_[t_2];
		t_2=t_2+1;
		p_AddLast8(t_t);
	}
	return this;
}
c_Enumerator6* c_List8::p_ObjectEnumerator(){
	return (new c_Enumerator6)->m_new(this);
}
void c_List8::mark(){
	Object::mark();
	gc_mark_q(m__head);
}
c_Node13::c_Node13(){
	m__succ=0;
	m__pred=0;
	m__data=0;
}
c_Node13* c_Node13::m_new(c_Node13* t_succ,c_Node13* t_pred,c_GlobalDecl* t_data){
	gc_assign(m__succ,t_succ);
	gc_assign(m__pred,t_pred);
	gc_assign(m__succ->m__pred,this);
	gc_assign(m__pred->m__succ,this);
	gc_assign(m__data,t_data);
	return this;
}
c_Node13* c_Node13::m_new2(){
	return this;
}
void c_Node13::mark(){
	Object::mark();
	gc_mark_q(m__succ);
	gc_mark_q(m__pred);
	gc_mark_q(m__data);
}
c_HeadNode8::c_HeadNode8(){
}
c_HeadNode8* c_HeadNode8::m_new(){
	c_Node13::m_new2();
	gc_assign(m__succ,(this));
	gc_assign(m__pred,(this));
	return this;
}
void c_HeadNode8::mark(){
	c_Node13::mark();
}
int bb_config_PopErr(){
	bb_config__errInfo=bb_config__errStack->p_RemoveLast();
	return 0;
}
c_InvokeMemberExpr::c_InvokeMemberExpr(){
	m_expr=0;
	m_decl=0;
	m_args=Array<c_Expr* >();
	m_isResize=0;
}
c_InvokeMemberExpr* c_InvokeMemberExpr::m_new(c_Expr* t_expr,c_FuncDecl* t_decl,Array<c_Expr* > t_args){
	c_Expr::m_new();
	gc_assign(this->m_expr,t_expr);
	gc_assign(this->m_decl,t_decl);
	gc_assign(this->m_args,t_args);
	return this;
}
c_InvokeMemberExpr* c_InvokeMemberExpr::m_new2(){
	c_Expr::m_new();
	return this;
}
c_Expr* c_InvokeMemberExpr::p_Semant(){
	if((m_exprType)!=0){
		return (this);
	}
	gc_assign(m_exprType,m_decl->m_retType);
	gc_assign(m_args,p_CastArgs(m_args,m_decl));
	if(((dynamic_cast<c_ArrayType*>(m_exprType))!=0) && ((dynamic_cast<c_VoidType*>(dynamic_cast<c_ArrayType*>(m_exprType)->m_elemType))!=0)){
		m_isResize=1;
		gc_assign(m_exprType,m_expr->m_exprType);
	}
	return (this);
}
String c_InvokeMemberExpr::p_ToString(){
	String t_t=String(L"InvokeMemberExpr(",17)+m_expr->p_ToString()+String(L",",1)+m_decl->p_ToString();
	Array<c_Expr* > t_=m_args;
	int t_2=0;
	while(t_2<t_.Length()){
		c_Expr* t_arg=t_[t_2];
		t_2=t_2+1;
		t_t=t_t+(String(L",",1)+t_arg->p_ToString());
	}
	return t_t+String(L")",1);
}
String c_InvokeMemberExpr::p_Trans(){
	if((m_isResize)!=0){
		return bb_translator__trans->p_TransInvokeMemberExpr(this);
	}
	return bb_translator__trans->p_TransInvokeMemberExpr(this);
}
String c_InvokeMemberExpr::p_TransStmt(){
	return bb_translator__trans->p_TransInvokeMemberExpr(this);
}
void c_InvokeMemberExpr::mark(){
	c_Expr::mark();
	gc_mark_q(m_expr);
	gc_mark_q(m_decl);
	gc_mark_q(m_args);
}
c_Expr* bb_preprocessor_EvalExpr(c_Toker* t_toker){
	c_StringStack* t_buf=(new c_StringStack)->m_new2();
	while(((t_toker->p_Toke()).Length()!=0) && t_toker->p_Toke()!=String(L"\n",1) && t_toker->p_TokeType()!=9){
		t_buf->p_Push(t_toker->p_Toke());
		t_toker->p_NextToke();
	}
	String t_source=t_buf->p_Join(String());
	t_toker=(new c_Toker)->m_new(String(),t_source);
	c_Parser* t_parser=(new c_Parser)->m_new(t_toker,0,0,0);
	c_Expr* t_expr=t_parser->p_ParseExpr()->p_Semant();
	return t_expr;
}
bool bb_preprocessor_EvalBool(c_Toker* t_toker){
	c_Expr* t_expr=bb_preprocessor_EvalExpr(t_toker);
	if(!((dynamic_cast<c_BoolType*>(t_expr->m_exprType))!=0)){
		t_expr=t_expr->p_Cast((c_Type::m_boolType),1);
	}
	if((t_expr->p_Eval()).Length()!=0){
		return true;
	}
	return false;
}
String bb_preprocessor_EvalText(c_Toker* t_toker){
	c_Expr* t_expr=bb_preprocessor_EvalExpr(t_toker);
	String t_val=t_expr->p_Eval();
	if((dynamic_cast<c_StringType*>(t_expr->m_exprType))!=0){
		return bb_config_EvalConfigTags(t_val);
	}
	if((dynamic_cast<c_BoolType*>(t_expr->m_exprType))!=0){
		if((t_val).Length()!=0){
			return String(L"True",4);
		}
		return String(L"False",5);
	}
	return t_val;
}
c_StringMap2* bb_config_GetConfigVars(){
	return bb_config__cfgScope->m_vars;
}
c_Type* bb_config_GetConfigVarType(String t_key){
	c_ConstDecl* t_decl=bb_config__cfgScope->m_cdecls->p_Get(t_key);
	if((t_decl)!=0){
		return t_decl->m_type;
	}
	return 0;
}
String bb_preprocessor_PreProcess(String t_path,c_ModuleDecl* t_mdecl){
	int t_cnest=0;
	int t_ifnest=0;
	int t_line=0;
	c_StringStack* t_source=(new c_StringStack)->m_new2();
	bb_decl_PushEnv(bb_config_GetConfigScope());
	String t_p_cd=bb_config_GetConfigVar(String(L"CD",2));
	String t_p_modpath=bb_config_GetConfigVar(String(L"MODPATH",7));
	bb_config_SetConfigVar2(String(L"CD",2),bb_os_ExtractDir(RealPath(t_path)));
	if((t_mdecl)!=0){
		bb_config_SetConfigVar2(String(L"MODPATH",7),t_mdecl->m_rmodpath);
	}else{
		bb_config_SetConfigVar2(String(L"MODPATH",7),String());
	}
	c_Toker* t_toker=(new c_Toker)->m_new(t_path,LoadString(t_path));
	t_toker->p_NextToke();
	int t_attrs=0;
	do{
		if((t_line)!=0){
			t_source->p_Push(String(L"\n",1));
			while(((t_toker->p_Toke()).Length()!=0) && t_toker->p_Toke()!=String(L"\n",1) && t_toker->p_TokeType()!=9){
				t_toker->p_NextToke();
			}
			if(!((t_toker->p_Toke()).Length()!=0)){
				break;
			}
			t_toker->p_NextToke();
		}
		t_line+=1;
		bb_config__errInfo=t_toker->p_Path()+String(L"<",1)+String(t_toker->p_Line())+String(L">",1);
		if(t_toker->p_TokeType()==1){
			t_toker->p_NextToke();
		}
		if(t_toker->p_Toke()!=String(L"#",1)){
			if(t_cnest==t_ifnest){
				String t_line2=String();
				while(((t_toker->p_Toke()).Length()!=0) && t_toker->p_Toke()!=String(L"\n",1) && t_toker->p_TokeType()!=9){
					String t_toke=t_toker->p_Toke();
					t_toker->p_NextToke();
					if((t_mdecl)!=0){
						String t_1=t_toke.ToLower();
						if(t_1==String(L"public",6)){
							t_attrs=0;
						}else{
							if(t_1==String(L"private",7)){
								t_attrs=512;
							}else{
								if(t_1==String(L"import",6)){
									while(t_toker->p_TokeType()==1){
										t_toke=t_toke+t_toker->p_Toke();
										t_toker->p_NextToke();
									}
									if(t_toker->p_TokeType()==2){
										String t_modpath=t_toker->p_Toke();
										while(t_toker->p_NextToke()==String(L".",1)){
											t_modpath=t_modpath+String(L".",1);
											t_toker->p_NextToke();
											if(t_toker->p_TokeType()!=2){
												break;
											}
											t_modpath=t_modpath+t_toker->p_Toke();
										}
										t_toke=t_toke+t_modpath;
										t_mdecl->p_ImportModule(t_modpath,t_attrs);
									}
								}
							}
						}
					}
					t_line2=t_line2+t_toke;
				}
				if((t_line2).Length()!=0){
					t_source->p_Push(t_line2);
				}
			}
			continue;
		}
		String t_toke2=t_toker->p_NextToke();
		if(t_toker->p_TokeType()==1){
			t_toke2=t_toker->p_NextToke();
		}
		String t_stm=t_toke2.ToLower();
		int t_ty=t_toker->p_TokeType();
		t_toker->p_NextToke();
		if(t_stm==String(L"end",3) || t_stm==String(L"else",4)){
			if(t_toker->p_TokeType()==1){
				t_toker->p_NextToke();
			}
			if(t_toker->p_Toke().ToLower()==String(L"if",2)){
				t_toker->p_NextToke();
				t_stm=t_stm+String(L"if",2);
			}
		}
		String t_2=t_stm;
		if(t_2==String(L"rem",3)){
			t_ifnest+=1;
		}else{
			if(t_2==String(L"if",2)){
				t_ifnest+=1;
				if(t_cnest==t_ifnest-1){
					if(bb_preprocessor_EvalBool(t_toker)){
						t_cnest=t_ifnest;
					}
				}
			}else{
				if(t_2==String(L"else",4)){
					if(!((t_ifnest)!=0)){
						bb_config_Err(String(L"#Else without #If",17));
					}
					if(t_cnest==t_ifnest){
						t_cnest|=65536;
					}else{
						if(t_cnest==t_ifnest-1){
							t_cnest=t_ifnest;
						}
					}
				}else{
					if(t_2==String(L"elseif",6)){
						if(!((t_ifnest)!=0)){
							bb_config_Err(String(L"#ElseIf without #If",19));
						}
						if(t_cnest==t_ifnest){
							t_cnest|=65536;
						}else{
							if(t_cnest==t_ifnest-1){
								if(bb_preprocessor_EvalBool(t_toker)){
									t_cnest=t_ifnest;
								}
							}
						}
					}else{
						if(t_2==String(L"end",3) || t_2==String(L"endif",5)){
							if(!((t_ifnest)!=0)){
								bb_config_Err(String(L"#End without #If or #Rem",24));
							}
							t_ifnest-=1;
							if(t_ifnest<(t_cnest&65535)){
								t_cnest=t_ifnest;
							}
						}else{
							if(t_2==String(L"print",5)){
								if(t_cnest==t_ifnest){
									bbPrint(bb_preprocessor_EvalText(t_toker));
								}
							}else{
								if(t_2==String(L"error",5)){
									if(t_cnest==t_ifnest){
										bb_config_Err(bb_preprocessor_EvalText(t_toker));
									}
								}else{
									if(t_cnest==t_ifnest){
										if(t_ty==2){
											if(t_toker->p_TokeType()==1){
												t_toker->p_NextToke();
											}
											String t_op=t_toker->p_Toke();
											String t_3=t_op;
											if(t_3==String(L"=",1) || t_3==String(L"+=",2)){
												String t_4=t_toke2;
												if(t_4==String(L"HOST",4) || t_4==String(L"LANG",4) || t_4==String(L"CONFIG",6) || t_4==String(L"TARGET",6) || t_4==String(L"SAFEMODE",8)){
													bb_config_Err(String(L"App config var '",16)+t_toke2+String(L"' cannot be modified",20));
												}
												t_toker->p_NextToke();
												String t_5=t_op;
												if(t_5==String(L"=",1)){
													c_Expr* t_expr=bb_preprocessor_EvalExpr(t_toker);
													String t_val=t_expr->p_Eval();
													if(!bb_config_GetConfigVars()->p_Contains(t_toke2)){
														if((dynamic_cast<c_StringType*>(t_expr->m_exprType))!=0){
															t_val=bb_config_EvalConfigTags(t_val);
														}
														bb_config_SetConfigVar(t_toke2,t_val,t_expr->m_exprType);
													}
												}else{
													if(t_5==String(L"+=",2)){
														String t_val2=bb_preprocessor_EvalText(t_toker);
														String t_var=bb_config_GetConfigVar(t_toke2);
														if((dynamic_cast<c_BoolType*>(bb_config_GetConfigVarType(t_toke2)))!=0){
															if(t_var==String(L"1",1)){
																t_var=String(L"True",4);
															}else{
																t_var=String(L"False",5);
															}
														}
														if(((t_var).Length()!=0) && !t_val2.StartsWith(String(L";",1))){
															t_val2=String(L";",1)+t_val2;
														}
														bb_config_SetConfigVar2(t_toke2,t_var+t_val2);
													}
												}
											}else{
												bb_config_Err(String(L"Expecting assignment operator.",30));
											}
										}else{
											bb_config_Err(String(L"Unrecognized preprocessor directive '",37)+t_toke2+String(L"'",1));
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}while(!(false));
	bb_config_SetConfigVar2(String(L"MODPATH",7),t_p_modpath);
	bb_config_SetConfigVar2(String(L"CD",2),t_p_cd);
	bb_decl_PopEnv();
	return t_source->p_Join(String());
}
c_Target::c_Target(){
	m_dir=String();
	m_name=String();
	m_system=String();
	m_builder=0;
}
c_Target* c_Target::m_new(String t_dir,String t_name,String t_system,c_Builder* t_builder){
	this->m_dir=t_dir;
	this->m_name=t_name;
	this->m_system=t_system;
	gc_assign(this->m_builder,t_builder);
	return this;
}
c_Target* c_Target::m_new2(){
	return this;
}
void c_Target::mark(){
	Object::mark();
	gc_mark_q(m_builder);
}
c_Map6::c_Map6(){
	m_root=0;
}
c_Map6* c_Map6::m_new(){
	return this;
}
int c_Map6::p_RotateLeft6(c_Node14* t_node){
	c_Node14* t_child=t_node->m_right;
	gc_assign(t_node->m_right,t_child->m_left);
	if((t_child->m_left)!=0){
		gc_assign(t_child->m_left->m_parent,t_node);
	}
	gc_assign(t_child->m_parent,t_node->m_parent);
	if((t_node->m_parent)!=0){
		if(t_node==t_node->m_parent->m_left){
			gc_assign(t_node->m_parent->m_left,t_child);
		}else{
			gc_assign(t_node->m_parent->m_right,t_child);
		}
	}else{
		gc_assign(m_root,t_child);
	}
	gc_assign(t_child->m_left,t_node);
	gc_assign(t_node->m_parent,t_child);
	return 0;
}
int c_Map6::p_RotateRight6(c_Node14* t_node){
	c_Node14* t_child=t_node->m_left;
	gc_assign(t_node->m_left,t_child->m_right);
	if((t_child->m_right)!=0){
		gc_assign(t_child->m_right->m_parent,t_node);
	}
	gc_assign(t_child->m_parent,t_node->m_parent);
	if((t_node->m_parent)!=0){
		if(t_node==t_node->m_parent->m_right){
			gc_assign(t_node->m_parent->m_right,t_child);
		}else{
			gc_assign(t_node->m_parent->m_left,t_child);
		}
	}else{
		gc_assign(m_root,t_child);
	}
	gc_assign(t_child->m_right,t_node);
	gc_assign(t_node->m_parent,t_child);
	return 0;
}
int c_Map6::p_InsertFixup6(c_Node14* t_node){
	while(((t_node->m_parent)!=0) && t_node->m_parent->m_color==-1 && ((t_node->m_parent->m_parent)!=0)){
		if(t_node->m_parent==t_node->m_parent->m_parent->m_left){
			c_Node14* t_uncle=t_node->m_parent->m_parent->m_right;
			if(((t_uncle)!=0) && t_uncle->m_color==-1){
				t_node->m_parent->m_color=1;
				t_uncle->m_color=1;
				t_uncle->m_parent->m_color=-1;
				t_node=t_uncle->m_parent;
			}else{
				if(t_node==t_node->m_parent->m_right){
					t_node=t_node->m_parent;
					p_RotateLeft6(t_node);
				}
				t_node->m_parent->m_color=1;
				t_node->m_parent->m_parent->m_color=-1;
				p_RotateRight6(t_node->m_parent->m_parent);
			}
		}else{
			c_Node14* t_uncle2=t_node->m_parent->m_parent->m_left;
			if(((t_uncle2)!=0) && t_uncle2->m_color==-1){
				t_node->m_parent->m_color=1;
				t_uncle2->m_color=1;
				t_uncle2->m_parent->m_color=-1;
				t_node=t_uncle2->m_parent;
			}else{
				if(t_node==t_node->m_parent->m_left){
					t_node=t_node->m_parent;
					p_RotateRight6(t_node);
				}
				t_node->m_parent->m_color=1;
				t_node->m_parent->m_parent->m_color=-1;
				p_RotateLeft6(t_node->m_parent->m_parent);
			}
		}
	}
	m_root->m_color=1;
	return 0;
}
bool c_Map6::p_Set6(String t_key,c_Target* t_value){
	c_Node14* t_node=m_root;
	c_Node14* t_parent=0;
	int t_cmp=0;
	while((t_node)!=0){
		t_parent=t_node;
		t_cmp=p_Compare(t_key,t_node->m_key);
		if(t_cmp>0){
			t_node=t_node->m_right;
		}else{
			if(t_cmp<0){
				t_node=t_node->m_left;
			}else{
				gc_assign(t_node->m_value,t_value);
				return false;
			}
		}
	}
	t_node=(new c_Node14)->m_new(t_key,t_value,-1,t_parent);
	if((t_parent)!=0){
		if(t_cmp>0){
			gc_assign(t_parent->m_right,t_node);
		}else{
			gc_assign(t_parent->m_left,t_node);
		}
		p_InsertFixup6(t_node);
	}else{
		gc_assign(m_root,t_node);
	}
	return true;
}
c_Node14* c_Map6::p_FirstNode(){
	if(!((m_root)!=0)){
		return 0;
	}
	c_Node14* t_node=m_root;
	while((t_node->m_left)!=0){
		t_node=t_node->m_left;
	}
	return t_node;
}
c_NodeEnumerator2* c_Map6::p_ObjectEnumerator(){
	return (new c_NodeEnumerator2)->m_new(p_FirstNode());
}
c_Node14* c_Map6::p_FindNode(String t_key){
	c_Node14* t_node=m_root;
	while((t_node)!=0){
		int t_cmp=p_Compare(t_key,t_node->m_key);
		if(t_cmp>0){
			t_node=t_node->m_right;
		}else{
			if(t_cmp<0){
				t_node=t_node->m_left;
			}else{
				return t_node;
			}
		}
	}
	return t_node;
}
c_Target* c_Map6::p_Get(String t_key){
	c_Node14* t_node=p_FindNode(t_key);
	if((t_node)!=0){
		return t_node->m_value;
	}
	return 0;
}
void c_Map6::mark(){
	Object::mark();
	gc_mark_q(m_root);
}
c_StringMap6::c_StringMap6(){
}
c_StringMap6* c_StringMap6::m_new(){
	c_Map6::m_new();
	return this;
}
int c_StringMap6::p_Compare(String t_lhs,String t_rhs){
	return t_lhs.Compare(t_rhs);
}
void c_StringMap6::mark(){
	c_Map6::mark();
}
c_Node14::c_Node14(){
	m_key=String();
	m_right=0;
	m_left=0;
	m_value=0;
	m_color=0;
	m_parent=0;
}
c_Node14* c_Node14::m_new(String t_key,c_Target* t_value,int t_color,c_Node14* t_parent){
	this->m_key=t_key;
	gc_assign(this->m_value,t_value);
	this->m_color=t_color;
	gc_assign(this->m_parent,t_parent);
	return this;
}
c_Node14* c_Node14::m_new2(){
	return this;
}
c_Node14* c_Node14::p_NextNode(){
	c_Node14* t_node=0;
	if((m_right)!=0){
		t_node=m_right;
		while((t_node->m_left)!=0){
			t_node=t_node->m_left;
		}
		return t_node;
	}
	t_node=this;
	c_Node14* t_parent=this->m_parent;
	while(((t_parent)!=0) && t_node==t_parent->m_right){
		t_node=t_parent;
		t_parent=t_parent->m_parent;
	}
	return t_parent;
}
String c_Node14::p_Key(){
	return m_key;
}
void c_Node14::mark(){
	Object::mark();
	gc_mark_q(m_right);
	gc_mark_q(m_left);
	gc_mark_q(m_value);
	gc_mark_q(m_parent);
}
void bb_config_PopConfigScope(){
	gc_assign(bb_config__cfgScope,bb_config__cfgScopeStack->p_Pop());
}
c_NodeEnumerator2::c_NodeEnumerator2(){
	m_node=0;
}
c_NodeEnumerator2* c_NodeEnumerator2::m_new(c_Node14* t_node){
	gc_assign(this->m_node,t_node);
	return this;
}
c_NodeEnumerator2* c_NodeEnumerator2::m_new2(){
	return this;
}
bool c_NodeEnumerator2::p_HasNext(){
	return m_node!=0;
}
c_Node14* c_NodeEnumerator2::p_NextObject(){
	c_Node14* t_t=m_node;
	gc_assign(m_node,m_node->p_NextNode());
	return t_t;
}
void c_NodeEnumerator2::mark(){
	Object::mark();
	gc_mark_q(m_node);
}
String bb_config_ENV_HOST;
String bb_config_ENV_CONFIG;
String bb_config_ENV_TARGET;
String bb_config_ENV_LANG;
String bb_os_StripAll(String t_path){
	return bb_os_StripDir(bb_os_StripExt(t_path));
}
c_AppDecl* bb_parser_ParseApp(String t_filepath){
	bb_config__errInfo=t_filepath+String(L"<1>",3);
	c_AppDecl* t_app=(new c_AppDecl)->m_new();
	String t_modpath=bb_os_StripAll(t_filepath);
	bb_parser_ParseModule(t_modpath,t_filepath,t_app);
	return t_app;
}
c_Reflector::c_Reflector(){
	m_debug=false;
	m_refmod=0;
	m_langmod=0;
	m_boxesmod=0;
	m_munged=(new c_StringMap7)->m_new();
	m_modexprs=(new c_StringMap2)->m_new();
	m_refmods=(new c_StringSet)->m_new();
	m_classdecls=(new c_Stack8)->m_new();
	m_classids=(new c_StringMap7)->m_new();
	m_output=(new c_StringStack)->m_new2();
}
c_Reflector* c_Reflector::m_new(){
	return this;
}
bool c_Reflector::m_MatchPath(String t_text,String t_pattern){
	Array<String > t_alts=t_pattern.Split(String(L"|",1));
	Array<String > t_=t_alts;
	int t_2=0;
	while(t_2<t_.Length()){
		String t_alt=t_[t_2];
		t_2=t_2+1;
		if(!((t_alt).Length()!=0)){
			continue;
		}
		Array<String > t_bits=t_alt.Split(String(L"*",1));
		if(t_bits.Length()==1){
			if(t_bits[0]==t_text){
				return true;
			}
			continue;
		}
		if(!t_text.StartsWith(t_bits[0])){
			continue;
		}
		int t_i=t_bits[0].Length();
		for(int t_j=1;t_j<t_bits.Length()-1;t_j=t_j+1){
			String t_bit=t_bits[t_j];
			t_i=t_text.Find(t_bit,t_i);
			if(t_i==-1){
				break;
			}
			t_i+=t_bit.Length();
		}
		if(t_i!=-1 && t_text.Slice(t_i).EndsWith(t_bits[t_bits.Length()-1])){
			return true;
		}
	}
	return false;
}
String c_Reflector::p_Mung(String t_ident){
	if(m_debug){
		t_ident=String(L"R",1)+t_ident;
		t_ident=t_ident.Replace(String(L"_",1),String(L"_0",2));
		t_ident=t_ident.Replace(String(L"[",1),String(L"_1",2));
		t_ident=t_ident.Replace(String(L"]",1),String(L"_2",2));
		t_ident=t_ident.Replace(String(L"<",1),String(L"_3",2));
		t_ident=t_ident.Replace(String(L">",1),String(L"_4",2));
		t_ident=t_ident.Replace(String(L",",1),String(L"_5",2));
		t_ident=t_ident.Replace(String(L".",1),String(L"_",1));
	}else{
		t_ident=String(L"R",1);
	}
	if(m_munged->p_Contains(t_ident)){
		int t_n=m_munged->p_Get(t_ident);
		t_n+=1;
		m_munged->p_Set7(t_ident,t_n);
		t_ident=t_ident+String(t_n);
	}else{
		m_munged->p_Set7(t_ident,1);
	}
	return t_ident;
}
bool c_Reflector::p_ValidClass(c_ClassDecl* t_cdecl){
	if(t_cdecl->m_munged==String(L"Object",6)){
		return true;
	}
	if(t_cdecl->m_munged==String(L"ThrowableObject",15)){
		return true;
	}
	if(!((t_cdecl->p_ExtendsObject())!=0)){
		return false;
	}
	if(!m_refmods->p_Contains(t_cdecl->p_ModuleScope()->m_filepath)){
		return false;
	}
	Array<c_Type* > t_=t_cdecl->m_instArgs;
	int t_2=0;
	while(t_2<t_.Length()){
		c_Type* t_arg=t_[t_2];
		t_2=t_2+1;
		if(((dynamic_cast<c_ObjectType*>(t_arg))!=0) && !p_ValidClass(t_arg->p_GetClass())){
			return false;
		}
	}
	if((t_cdecl->m_superClass)!=0){
		return p_ValidClass(t_cdecl->m_superClass);
	}
	return true;
}
String c_Reflector::p_TypeExpr(c_Type* t_ty,bool t_path){
	if((dynamic_cast<c_VoidType*>(t_ty))!=0){
		return String(L"Void",4);
	}
	if((dynamic_cast<c_BoolType*>(t_ty))!=0){
		return String(L"Bool",4);
	}
	if((dynamic_cast<c_IntType*>(t_ty))!=0){
		return String(L"Int",3);
	}
	if((dynamic_cast<c_FloatType*>(t_ty))!=0){
		return String(L"Float",5);
	}
	if((dynamic_cast<c_StringType*>(t_ty))!=0){
		return String(L"String",6);
	}
	if((dynamic_cast<c_ArrayType*>(t_ty))!=0){
		return p_TypeExpr(dynamic_cast<c_ArrayType*>(t_ty)->m_elemType,t_path)+String(L"[]",2);
	}
	if((dynamic_cast<c_ObjectType*>(t_ty))!=0){
		return p_DeclExpr((t_ty->p_GetClass()),t_path);
	}
	bb_config_InternalErr(String(L"Internal error",14));
	return String();
}
String c_Reflector::p_DeclExpr(c_Decl* t_decl,bool t_path){
	if(t_path && ((dynamic_cast<c_ClassDecl*>(t_decl->m_scope))!=0)){
		return t_decl->m_ident;
	}
	c_ModuleDecl* t_mdecl=dynamic_cast<c_ModuleDecl*>(t_decl);
	if((t_mdecl)!=0){
		if(t_path){
			return t_mdecl->m_rmodpath;
		}
		String t_expr=m_modexprs->p_Get(t_mdecl->m_filepath);
		if(!((t_expr).Length()!=0)){
			bbPrint(String(L"REFLECTION ERROR",16));
			t_expr=p_Mung(t_mdecl->m_rmodpath);
			m_refmod->p_InsertDecl((new c_AliasDecl)->m_new(t_expr,0,(t_mdecl)));
			m_modexprs->p_Set2(t_mdecl->m_filepath,t_expr);
		}
		return t_expr;
	}
	c_ClassDecl* t_cdecl=dynamic_cast<c_ClassDecl*>(t_decl);
	if(((t_cdecl)!=0) && t_cdecl->m_munged==String(L"Object",6)){
		if(t_path){
			return String(L"cerberus.lang.Object",20);
		}
		return String(L"Object",6);
	}
	if(((t_cdecl)!=0) && t_cdecl->m_munged==String(L"ThrowableObject",15)){
		if(t_path){
			return String(L"cerberus.lang.Throwable",23);
		}
		return String(L"Throwable",9);
	}
	String t_ident=p_DeclExpr((t_decl->m_scope),t_path)+String(L".",1)+t_decl->m_ident;
	if(((t_cdecl)!=0) && ((t_cdecl->m_instArgs).Length()!=0)){
		String t_t=String();
		Array<c_Type* > t_=t_cdecl->m_instArgs;
		int t_2=0;
		while(t_2<t_.Length()){
			c_Type* t_arg=t_[t_2];
			t_2=t_2+1;
			if((t_t).Length()!=0){
				t_t=t_t+String(L",",1);
			}
			t_t=t_t+p_TypeExpr(t_arg,t_path);
		}
		t_ident=t_ident+(String(L"<",1)+t_t+String(L">",1));
	}
	return t_ident;
}
int c_Reflector::p_Emit(String t_t){
	m_output->p_Push(t_t);
	return 0;
}
bool c_Reflector::p_ValidType(c_Type* t_ty){
	if((dynamic_cast<c_ArrayType*>(t_ty))!=0){
		return p_ValidType(dynamic_cast<c_ArrayType*>(t_ty)->m_elemType);
	}
	if((dynamic_cast<c_ObjectType*>(t_ty))!=0){
		return p_ValidClass(t_ty->p_GetClass());
	}
	return true;
}
String c_Reflector::p_TypeInfo(c_Type* t_ty){
	if((dynamic_cast<c_VoidType*>(t_ty))!=0){
		return String(L"Null",4);
	}
	if((dynamic_cast<c_BoolType*>(t_ty))!=0){
		return String(L"_boolClass",10);
	}
	if((dynamic_cast<c_IntType*>(t_ty))!=0){
		return String(L"_intClass",9);
	}
	if((dynamic_cast<c_FloatType*>(t_ty))!=0){
		return String(L"_floatClass",11);
	}
	if((dynamic_cast<c_StringType*>(t_ty))!=0){
		return String(L"_stringClass",12);
	}
	if((dynamic_cast<c_ArrayType*>(t_ty))!=0){
		c_Type* t_elemType=dynamic_cast<c_ArrayType*>(t_ty)->m_elemType;
		String t_name=String(L"cerberus.boxes.ArrayObject<",27)+p_TypeExpr(t_elemType,true)+String(L">",1);
		if(m_classids->p_Contains(t_name)){
			return String(L"_classes[",9)+String(m_classids->p_Get(t_name))+String(L"]",1);
		}
		if(m_debug){
			bbPrint(String(L"Instantiating class: ",21)+t_name);
		}
		c_Type* t_[]={t_elemType};
		c_ClassDecl* t_cdecl=m_boxesmod->p_FindType(String(L"ArrayObject",11),Array<c_Type* >(t_,1))->p_GetClass();
		c_Enumerator2* t_2=t_cdecl->p_Decls()->p_ObjectEnumerator();
		while(t_2->p_HasNext()){
			c_Decl* t_decl=t_2->p_NextObject();
			if(!((dynamic_cast<c_AliasDecl*>(t_decl))!=0)){
				t_decl->p_Semant();
			}
		}
		int t_id=m_classdecls->p_Length2();
		m_classids->p_Set7(t_name,t_id);
		m_classdecls->p_Push22(t_cdecl);
		return String(L"_classes[",9)+String(t_id)+String(L"]",1);
	}
	if((dynamic_cast<c_ObjectType*>(t_ty))!=0){
		String t_name2=p_DeclExpr((t_ty->p_GetClass()),true);
		if(m_classids->p_Contains(t_name2)){
			return String(L"_classes[",9)+String(m_classids->p_Get(t_name2))+String(L"]",1);
		}
		return String(L"_unknownClass",13);
	}
	bb_config_InternalErr(String(L"Internal error",14));
	return String();
}
int c_Reflector::p_Attrs(c_Decl* t_decl){
	return t_decl->m_attrs>>8&255;
}
String c_Reflector::p_Box(c_Type* t_ty,String t_expr){
	if((dynamic_cast<c_VoidType*>(t_ty))!=0){
		return t_expr;
	}
	if((dynamic_cast<c_BoolType*>(t_ty))!=0){
		return String(L"New BoolObject(",15)+t_expr+String(L")",1);
	}
	if((dynamic_cast<c_IntType*>(t_ty))!=0){
		return String(L"New IntObject(",14)+t_expr+String(L")",1);
	}
	if((dynamic_cast<c_FloatType*>(t_ty))!=0){
		return String(L"New FloatObject(",16)+t_expr+String(L")",1);
	}
	if((dynamic_cast<c_StringType*>(t_ty))!=0){
		return String(L"New StringObject(",17)+t_expr+String(L")",1);
	}
	if((dynamic_cast<c_ArrayType*>(t_ty))!=0){
		return String(L"New ArrayObject<",16)+p_TypeExpr(dynamic_cast<c_ArrayType*>(t_ty)->m_elemType,false)+String(L">(",2)+t_expr+String(L")",1);
	}
	if((dynamic_cast<c_ObjectType*>(t_ty))!=0){
		return t_expr;
	}
	bb_config_InternalErr(String(L"Internal error",14));
	return String();
}
String c_Reflector::p_Emit2(c_ConstDecl* t_tdecl){
	if(!p_ValidType(t_tdecl->m_type)){
		return String();
	}
	String t_name=p_DeclExpr((t_tdecl),true);
	String t_expr=p_DeclExpr((t_tdecl),false);
	String t_type=p_TypeInfo(t_tdecl->m_type);
	return String(L"New ConstInfo(\"",15)+t_name+String(L"\",",2)+String(p_Attrs(t_tdecl))+String(L",",1)+t_type+String(L",",1)+p_Box(t_tdecl->m_type,t_expr)+String(L")",1);
}
String c_Reflector::p_Unbox(c_Type* t_ty,String t_expr){
	if((dynamic_cast<c_BoolType*>(t_ty))!=0){
		return String(L"BoolObject(",11)+t_expr+String(L").value",7);
	}
	if((dynamic_cast<c_IntType*>(t_ty))!=0){
		return String(L"IntObject(",10)+t_expr+String(L").value",7);
	}
	if((dynamic_cast<c_FloatType*>(t_ty))!=0){
		return String(L"FloatObject(",12)+t_expr+String(L").value",7);
	}
	if((dynamic_cast<c_StringType*>(t_ty))!=0){
		return String(L"StringObject(",13)+t_expr+String(L").value",7);
	}
	if((dynamic_cast<c_ArrayType*>(t_ty))!=0){
		return String(L"ArrayObject<",12)+p_TypeExpr(dynamic_cast<c_ArrayType*>(t_ty)->m_elemType,false)+String(L">(",2)+t_expr+String(L").value",7);
	}
	if((dynamic_cast<c_ObjectType*>(t_ty))!=0){
		return p_DeclExpr((t_ty->p_GetClass()),false)+String(L"(",1)+t_expr+String(L")",1);
	}
	bb_config_InternalErr(String(L"Internal error",14));
	return String();
}
String c_Reflector::p_Emit3(c_ClassDecl* t_cdecl){
	if((t_cdecl->m_args).Length()!=0){
		bb_config_InternalErr(String(L"Internal error",14));
	}
	String t_name=p_DeclExpr((t_cdecl),true);
	String t_expr=p_DeclExpr((t_cdecl),false);
	String t_ident=p_Mung(t_name);
	String t_sclass=String(L"Null",4);
	if((t_cdecl->m_superClass)!=0){
		t_sclass=p_TypeInfo(t_cdecl->m_superClass->m_objectType);
	}
	String t_ifaces=String();
	Array<c_ClassDecl* > t_=t_cdecl->m_implments;
	int t_2=0;
	while(t_2<t_.Length()){
		c_ClassDecl* t_idecl=t_[t_2];
		t_2=t_2+1;
		if((t_ifaces).Length()!=0){
			t_ifaces=t_ifaces+String(L",",1);
		}
		t_ifaces=t_ifaces+p_TypeInfo(t_idecl->m_objectType);
	}
	c_StringStack* t_consts=(new c_StringStack)->m_new2();
	c_StringStack* t_globals=(new c_StringStack)->m_new2();
	c_StringStack* t_fields=(new c_StringStack)->m_new2();
	c_StringStack* t_methods=(new c_StringStack)->m_new2();
	c_StringStack* t_functions=(new c_StringStack)->m_new2();
	c_StringStack* t_ctors=(new c_StringStack)->m_new2();
	c_Enumerator2* t_3=t_cdecl->p_Decls()->p_ObjectEnumerator();
	while(t_3->p_HasNext()){
		c_Decl* t_decl=t_3->p_NextObject();
		if((dynamic_cast<c_AliasDecl*>(t_decl))!=0){
			continue;
		}
		if(!((t_decl->p_IsSemanted())!=0)){
			continue;
		}
		c_ConstDecl* t_pdecl=dynamic_cast<c_ConstDecl*>(t_decl);
		if((t_pdecl)!=0){
			String t_p=p_Emit2(t_pdecl);
			if((t_p).Length()!=0){
				t_consts->p_Push(t_p);
			}
			continue;
		}
		c_GlobalDecl* t_gdecl=dynamic_cast<c_GlobalDecl*>(t_decl);
		if((t_gdecl)!=0){
			String t_g=p_Emit6(t_gdecl);
			if((t_g).Length()!=0){
				t_globals->p_Push(t_g);
			}
			continue;
		}
		c_FieldDecl* t_tdecl=dynamic_cast<c_FieldDecl*>(t_decl);
		if((t_tdecl)!=0){
			String t_f=p_Emit5(t_tdecl);
			if((t_f).Length()!=0){
				t_fields->p_Push(t_f);
			}
			continue;
		}
		c_FuncDecl* t_fdecl=dynamic_cast<c_FuncDecl*>(t_decl);
		if((t_fdecl)!=0){
			String t_f2=p_Emit4(t_fdecl);
			if((t_f2).Length()!=0){
				if(t_fdecl->p_IsCtor()){
					t_ctors->p_Push(t_f2);
				}else{
					if(t_fdecl->p_IsMethod()){
						t_methods->p_Push(t_f2);
					}else{
						t_functions->p_Push(t_f2);
					}
				}
			}
			continue;
		}
	}
	p_Emit(String(L"Class ",6)+t_ident+String(L" Extends ClassInfo",18));
	p_Emit(String(L" Method New()",13));
	p_Emit(String(L"  Super.New(\"",13)+t_name+String(L"\",",2)+String(p_Attrs(t_cdecl))+String(L",",1)+t_sclass+String(L",[",2)+t_ifaces+String(L"])",2));
	String t_1=t_name;
	if(t_1==String(L"cerberus.boxes.BoolObject",25)){
		p_Emit(String(L"  _boolClass=Self",17));
	}else{
		if(t_1==String(L"cerberus.boxes.IntObject",24)){
			p_Emit(String(L"  _intClass=Self",16));
		}else{
			if(t_1==String(L"cerberus.boxes.FloatObject",26)){
				p_Emit(String(L"  _floatClass=Self",18));
			}else{
				if(t_1==String(L"cerberus.boxes.StringObject",27)){
					p_Emit(String(L"  _stringClass=Self",19));
				}
			}
		}
	}
	p_Emit(String(L" End",4));
	if(t_name.StartsWith(String(L"cerberus.boxes.ArrayObject<",27))){
		c_Type* t_elemType=t_cdecl->m_instArgs[0];
		String t_elemExpr=p_TypeExpr(t_elemType,false);
		int t_i=t_elemExpr.Find(String(L"[]",2),0);
		if(t_i==-1){
			t_i=t_elemExpr.Length();
		}
		String t_ARRAY_PREFIX=m_modexprs->p_Get(m_boxesmod->m_filepath)+String(L".ArrayObject<",13);
		p_Emit(String(L" Method ElementType:ClassInfo() Property",40));
		p_Emit(String(L"  Return ",9)+p_TypeInfo(t_elemType));
		p_Emit(String(L" End",4));
		p_Emit(String(L" Method ArrayLength:Int(i:Object) Property",42));
		p_Emit(String(L"  Return ",9)+t_ARRAY_PREFIX+t_elemExpr+String(L">(i).value.Length",17));
		p_Emit(String(L" End",4));
		p_Emit(String(L" Method GetElement:Object(i:Object,e)",37));
		p_Emit(String(L"  Return ",9)+p_Box(t_elemType,t_ARRAY_PREFIX+t_elemExpr+String(L">(i).value[e]",13)));
		p_Emit(String(L" End",4));
		p_Emit(String(L" Method SetElement:Void(i:Object,e,v:Object)",44));
		p_Emit(String(L"  ",2)+t_ARRAY_PREFIX+t_elemExpr+String(L">(i).value[e]=",14)+p_Unbox(t_elemType,String(L"v",1)));
		p_Emit(String(L" End",4));
		p_Emit(String(L" Method NewArray:Object(l:Int)",30));
		p_Emit(String(L"  Return ",9)+p_Box((t_elemType->p_ArrayOf()),String(L"New ",4)+t_elemExpr.Slice(0,t_i)+String(L"[l]",3)+t_elemExpr.Slice(t_i)));
		p_Emit(String(L" End",4));
	}
	if(!((t_cdecl->p_IsAbstract())!=0) && !((t_cdecl->p_IsExtern())!=0)){
		p_Emit(String(L" Method NewInstance:Object()",28));
		p_Emit(String(L"  Return New ",13)+t_expr);
		p_Emit(String(L" End",4));
	}
	p_Emit(String(L" Method Init()",14));
	if((t_consts->p_Length2())!=0){
		p_Emit(String(L"  _consts=new ConstInfo[",24)+String(t_consts->p_Length2())+String(L"]",1));
		for(int t_i2=0;t_i2<t_consts->p_Length2();t_i2=t_i2+1){
			p_Emit(String(L"  _consts[",10)+String(t_i2)+String(L"]=",2)+t_consts->p_Get2(t_i2));
		}
	}
	if((t_globals->p_Length2())!=0){
		p_Emit(String(L"  _globals=new GlobalInfo[",26)+String(t_globals->p_Length2())+String(L"]",1));
		for(int t_i3=0;t_i3<t_globals->p_Length2();t_i3=t_i3+1){
			p_Emit(String(L"  _globals[",11)+String(t_i3)+String(L"]=New ",6)+t_globals->p_Get2(t_i3));
		}
	}
	if((t_fields->p_Length2())!=0){
		p_Emit(String(L"  _fields=New FieldInfo[",24)+String(t_fields->p_Length2())+String(L"]",1));
		for(int t_i4=0;t_i4<t_fields->p_Length2();t_i4=t_i4+1){
			p_Emit(String(L"  _fields[",10)+String(t_i4)+String(L"]=New ",6)+t_fields->p_Get2(t_i4));
		}
	}
	if((t_methods->p_Length2())!=0){
		p_Emit(String(L"  _methods=New MethodInfo[",26)+String(t_methods->p_Length2())+String(L"]",1));
		for(int t_i5=0;t_i5<t_methods->p_Length2();t_i5=t_i5+1){
			p_Emit(String(L"  _methods[",11)+String(t_i5)+String(L"]=New ",6)+t_methods->p_Get2(t_i5));
		}
	}
	if((t_functions->p_Length2())!=0){
		p_Emit(String(L"  _functions=New FunctionInfo[",30)+String(t_functions->p_Length2())+String(L"]",1));
		for(int t_i6=0;t_i6<t_functions->p_Length2();t_i6=t_i6+1){
			p_Emit(String(L"  _functions[",13)+String(t_i6)+String(L"]=New ",6)+t_functions->p_Get2(t_i6));
		}
	}
	if((t_ctors->p_Length2())!=0){
		p_Emit(String(L"  _ctors=New FunctionInfo[",26)+String(t_ctors->p_Length2())+String(L"]",1));
		for(int t_i7=0;t_i7<t_ctors->p_Length2();t_i7=t_i7+1){
			p_Emit(String(L"  _ctors[",9)+String(t_i7)+String(L"]=New ",6)+t_ctors->p_Get2(t_i7));
		}
	}
	p_Emit(String(L" InitR()",8));
	p_Emit(String(L" End",4));
	p_Emit(String(L"End",3));
	return t_ident;
}
String c_Reflector::p_Emit4(c_FuncDecl* t_fdecl){
	if(!p_ValidType(t_fdecl->m_retType)){
		return String();
	}
	Array<c_ArgDecl* > t_=t_fdecl->m_argDecls;
	int t_2=0;
	while(t_2<t_.Length()){
		c_ArgDecl* t_arg=t_[t_2];
		t_2=t_2+1;
		if(!p_ValidType(t_arg->m_type)){
			return String();
		}
	}
	String t_name=p_DeclExpr((t_fdecl),true);
	String t_expr=p_DeclExpr((t_fdecl),false);
	String t_ident=p_Mung(t_name);
	String t_rtype=p_TypeInfo(t_fdecl->m_retType);
	String t_base=String(L"FunctionInfo",12);
	if(t_fdecl->p_IsMethod()){
		String t_clas=p_DeclExpr((t_fdecl->p_ClassScope()),false);
		t_expr=t_clas+String(L"(i).",4)+t_fdecl->m_ident;
		t_base=String(L"MethodInfo",10);
	}
	Array<String > t_argtys=Array<String >(t_fdecl->m_argDecls.Length());
	for(int t_i=0;t_i<t_argtys.Length();t_i=t_i+1){
		t_argtys[t_i]=p_TypeInfo(t_fdecl->m_argDecls[t_i]->m_type);
	}
	p_Emit(String(L"Class ",6)+t_ident+String(L" Extends ",9)+t_base);
	p_Emit(String(L" Method New()",13));
	p_Emit(String(L"  Super.New(\"",13)+t_name+String(L"\",",2)+String(p_Attrs(t_fdecl))+String(L",",1)+t_rtype+String(L",[",2)+String(L",",1).Join(t_argtys)+String(L"])",2));
	p_Emit(String(L" End",4));
	if(t_fdecl->p_IsMethod()){
		p_Emit(String(L" Method Invoke:Object(i:Object,p:Object[])",42));
	}else{
		p_Emit(String(L" Method Invoke:Object(p:Object[])",33));
	}
	c_StringStack* t_args=(new c_StringStack)->m_new2();
	for(int t_i2=0;t_i2<t_fdecl->m_argDecls.Length();t_i2=t_i2+1){
		c_ArgDecl* t_arg2=t_fdecl->m_argDecls[t_i2];
		t_args->p_Push(p_Unbox(t_arg2->m_type,String(L"p[",2)+String(t_i2)+String(L"]",1)));
	}
	if(t_fdecl->p_IsCtor()){
		c_ClassDecl* t_cdecl=t_fdecl->p_ClassScope();
		if((t_cdecl->p_IsAbstract())!=0){
			p_Emit(String(L"  Return Null",13));
		}else{
			p_Emit(String(L"  Return New ",13)+p_DeclExpr((t_cdecl),false)+String(L"(",1)+t_args->p_Join(String(L",",1))+String(L")",1));
		}
	}else{
		if((dynamic_cast<c_VoidType*>(t_fdecl->m_retType))!=0){
			p_Emit(String(L"  ",2)+t_expr+String(L"(",1)+t_args->p_Join(String(L",",1))+String(L")",1));
		}else{
			p_Emit(String(L"  Return ",9)+p_Box(t_fdecl->m_retType,t_expr+String(L"(",1)+t_args->p_Join(String(L",",1))+String(L")",1)));
		}
	}
	p_Emit(String(L" End",4));
	p_Emit(String(L"End",3));
	return t_ident;
}
String c_Reflector::p_Emit5(c_FieldDecl* t_tdecl){
	if(!p_ValidType(t_tdecl->m_type)){
		return String();
	}
	String t_name=t_tdecl->m_ident;
	String t_ident=p_Mung(t_name);
	String t_type=p_TypeInfo(t_tdecl->m_type);
	String t_clas=p_DeclExpr((t_tdecl->p_ClassScope()),false);
	String t_expr=t_clas+String(L"(i).",4)+t_tdecl->m_ident;
	p_Emit(String(L"Class ",6)+t_ident+String(L" Extends FieldInfo",18));
	p_Emit(String(L" Method New()",13));
	p_Emit(String(L"  Super.New(\"",13)+t_name+String(L"\",",2)+String(p_Attrs(t_tdecl))+String(L",",1)+t_type+String(L")",1));
	p_Emit(String(L" End",4));
	p_Emit(String(L" Method GetValue:Object(i:Object)",33));
	p_Emit(String(L"  Return ",9)+p_Box(t_tdecl->m_type,t_expr));
	p_Emit(String(L" End",4));
	p_Emit(String(L" Method SetValue:Void(i:Object,v:Object)",40));
	p_Emit(String(L"  ",2)+t_expr+String(L"=",1)+p_Unbox(t_tdecl->m_type,String(L"v",1)));
	p_Emit(String(L" End",4));
	p_Emit(String(L"End",3));
	return t_ident;
}
String c_Reflector::p_Emit6(c_GlobalDecl* t_gdecl){
	if(!p_ValidType(t_gdecl->m_type)){
		return String();
	}
	String t_name=p_DeclExpr((t_gdecl),true);
	String t_expr=p_DeclExpr((t_gdecl),false);
	String t_ident=p_Mung(t_name);
	String t_type=p_TypeInfo(t_gdecl->m_type);
	p_Emit(String(L"Class ",6)+t_ident+String(L" Extends GlobalInfo",19));
	p_Emit(String(L" Method New()",13));
	p_Emit(String(L"  Super.New(\"",13)+t_name+String(L"\",",2)+String(p_Attrs(t_gdecl))+String(L",",1)+t_type+String(L")",1));
	p_Emit(String(L" End",4));
	p_Emit(String(L" Method GetValue:Object()",25));
	p_Emit(String(L"  Return ",9)+p_Box(t_gdecl->m_type,t_expr));
	p_Emit(String(L" End",4));
	p_Emit(String(L" Method SetValue:Void(v:Object)",31));
	p_Emit(String(L"  ",2)+t_expr+String(L"=",1)+p_Unbox(t_gdecl->m_type,String(L"v",1)));
	p_Emit(String(L" End",4));
	p_Emit(String(L"End",3));
	return t_ident;
}
int c_Reflector::p_Semant3(c_AppDecl* t_app){
	String t_filter=bb_config_GetConfigVar(String(L"REFLECTION_FILTER",17));
	if(!((t_filter).Length()!=0)){
		return 0;
	}
	t_filter=t_filter.Replace(String(L";",1),String(L"|",1));
	m_debug=bb_config_GetConfigVar(String(L"DEBUG_REFLECTION",16))==String(L"1",1);
	c_ValueEnumerator* t_=t_app->m_imported->p_Values()->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_ModuleDecl* t_mdecl=t_->p_NextObject();
		String t_path=t_mdecl->m_rmodpath;
		if(t_path==String(L"reflection",10)){
			gc_assign(m_refmod,t_mdecl);
		}else{
			if(t_path==String(L"cerberus.lang",13)){
				gc_assign(m_langmod,t_mdecl);
			}else{
				if(t_path==String(L"cerberus.boxes",14)){
					gc_assign(m_boxesmod,t_mdecl);
				}
			}
		}
	}
	if(!((m_refmod)!=0)){
		bbError(String(L"reflection module not found!",28));
	}
	if(m_debug){
		bbPrint(String(L"Semanting all",13));
	}
	c_ValueEnumerator* t_2=t_app->m_imported->p_Values()->p_ObjectEnumerator();
	while(t_2->p_HasNext()){
		c_ModuleDecl* t_mdecl2=t_2->p_NextObject();
		String t_path2=t_mdecl2->m_rmodpath;
		if(t_mdecl2!=m_boxesmod && t_mdecl2!=m_langmod && !m_MatchPath(t_path2,t_filter)){
			continue;
		}
		String t_expr=p_Mung(t_path2);
		m_refmod->p_InsertDecl((new c_AliasDecl)->m_new(t_expr,0,(t_mdecl2)));
		m_modexprs->p_Set2(t_mdecl2->m_filepath,t_expr);
		m_refmods->p_Insert(t_mdecl2->m_filepath);
		t_mdecl2->p_SemantAll();
	}
	do{
		int t_n=t_app->m_allSemantedDecls->p_Count();
		c_ValueEnumerator* t_3=t_app->m_imported->p_Values()->p_ObjectEnumerator();
		while(t_3->p_HasNext()){
			c_ModuleDecl* t_mdecl3=t_3->p_NextObject();
			if(!m_refmods->p_Contains(t_mdecl3->m_filepath)){
				continue;
			}
			t_mdecl3->p_SemantAll();
		}
		t_n=t_app->m_allSemantedDecls->p_Count()-t_n;
		if(!((t_n)!=0)){
			break;
		}
		if(m_debug){
			bbPrint(String(L"Semanting more: ",16)+String(t_n));
		}
	}while(!(false));
	c_Enumerator2* t_4=t_app->m_allSemantedDecls->p_ObjectEnumerator();
	while(t_4->p_HasNext()){
		c_Decl* t_decl=t_4->p_NextObject();
		if(!m_refmods->p_Contains(t_decl->p_ModuleScope()->m_filepath)){
			continue;
		}
		c_ClassDecl* t_cdecl=dynamic_cast<c_ClassDecl*>(t_decl);
		if(((t_cdecl)!=0) && p_ValidClass(t_cdecl)){
			m_classids->p_Set7(p_DeclExpr((t_cdecl),true),m_classdecls->p_Length2());
			m_classdecls->p_Push22(t_cdecl);
			continue;
		}
	}
	c_StringStack* t_classes=(new c_StringStack)->m_new2();
	c_StringStack* t_consts=(new c_StringStack)->m_new2();
	c_StringStack* t_globals=(new c_StringStack)->m_new2();
	c_StringStack* t_functions=(new c_StringStack)->m_new2();
	if(m_debug){
		bbPrint(String(L"Generating reflection info",26));
	}
	c_Enumerator2* t_5=t_app->m_allSemantedDecls->p_ObjectEnumerator();
	while(t_5->p_HasNext()){
		c_Decl* t_decl2=t_5->p_NextObject();
		if(!m_refmods->p_Contains(t_decl2->p_ModuleScope()->m_filepath)){
			continue;
		}
		c_ConstDecl* t_pdecl=dynamic_cast<c_ConstDecl*>(t_decl2);
		if((t_pdecl)!=0){
			String t_p=p_Emit2(t_pdecl);
			if((t_p).Length()!=0){
				t_consts->p_Push(t_p);
			}
			continue;
		}
		c_GlobalDecl* t_gdecl=dynamic_cast<c_GlobalDecl*>(t_decl2);
		if((t_gdecl)!=0){
			String t_g=p_Emit6(t_gdecl);
			if((t_g).Length()!=0){
				t_globals->p_Push(t_g);
			}
			continue;
		}
		c_FuncDecl* t_fdecl=dynamic_cast<c_FuncDecl*>(t_decl2);
		if((t_fdecl)!=0){
			String t_f=p_Emit4(t_fdecl);
			if((t_f).Length()!=0){
				t_functions->p_Push(t_f);
			}
			continue;
		}
	}
	if(m_debug){
		bbPrint(String(L"Finalizing classes",18));
	}
	t_app->p_FinalizeClasses();
	if(m_debug){
		bbPrint(String(L"Generating class reflection info",32));
	}
	for(int t_i=0;t_i<m_classdecls->p_Length2();t_i=t_i+1){
		t_classes->p_Push(p_Emit3(m_classdecls->p_Get2(t_i)));
	}
	p_Emit(String(L"Global _init:=__init()",22));
	p_Emit(String(L"Function __init()",17));
	if((t_classes->p_Length2())!=0){
		p_Emit(String(L" _classes=New ClassInfo[",24)+String(t_classes->p_Length2())+String(L"]",1));
		for(int t_i2=0;t_i2<t_classes->p_Length2();t_i2=t_i2+1){
			p_Emit(String(L" _classes[",10)+String(t_i2)+String(L"]=New ",6)+t_classes->p_Get2(t_i2));
		}
		for(int t_i3=0;t_i3<t_classes->p_Length2();t_i3=t_i3+1){
			p_Emit(String(L" _classes[",10)+String(t_i3)+String(L"].Init()",8));
		}
	}
	if((t_consts->p_Length2())!=0){
		p_Emit(String(L" _consts=new ConstInfo[",23)+String(t_consts->p_Length2())+String(L"]",1));
		for(int t_i4=0;t_i4<t_consts->p_Length2();t_i4=t_i4+1){
			p_Emit(String(L" _consts[",9)+String(t_i4)+String(L"]=",2)+t_consts->p_Get2(t_i4));
		}
	}
	if((t_globals->p_Length2())!=0){
		p_Emit(String(L" _globals=New GlobalInfo[",25)+String(t_globals->p_Length2())+String(L"]",1));
		for(int t_i5=0;t_i5<t_globals->p_Length2();t_i5=t_i5+1){
			p_Emit(String(L" _globals[",10)+String(t_i5)+String(L"]=New ",6)+t_globals->p_Get2(t_i5));
		}
	}
	if((t_functions->p_Length2())!=0){
		p_Emit(String(L" _functions=New FunctionInfo[",29)+String(t_functions->p_Length2())+String(L"]",1));
		for(int t_i6=0;t_i6<t_functions->p_Length2();t_i6=t_i6+1){
			p_Emit(String(L" _functions[",12)+String(t_i6)+String(L"]=New ",6)+t_functions->p_Get2(t_i6));
		}
	}
	p_Emit(String(L" _getClass=New __GetClass",25));
	p_Emit(String(L"End",3));
	p_Emit(String(L"Class __GetClass Extends _GetClass",34));
	p_Emit(String(L" Method GetClass:ClassInfo(o:Object)",36));
	for(int t_i7=t_classes->p_Length2()-1;t_i7>=0;t_i7=t_i7+-1){
		String t_expr2=p_DeclExpr((m_classdecls->p_Get2(t_i7)),false);
		p_Emit(String(L"  If ",5)+t_expr2+String(L"(o)<>Null Return _classes[",26)+String(t_i7)+String(L"]",1));
	}
	p_Emit(String(L"  Return _unknownClass",22));
	p_Emit(String(L" End",4));
	p_Emit(String(L"End",3));
	String t_source=m_output->p_Join(String(L"\n",1));
	int t_attrs=8388608;
	if(m_debug){
		bbPrint(String(L"Reflection source:\n",19)+t_source);
	}else{
		t_attrs|=4194304;
	}
	bb_parser_ParseSource(t_source,t_app,m_refmod,t_attrs);
	m_refmod->p_FindValDecl(String(L"_init",5));
	t_app->p_Semant();
	return 0;
}
void c_Reflector::mark(){
	Object::mark();
	gc_mark_q(m_refmod);
	gc_mark_q(m_langmod);
	gc_mark_q(m_boxesmod);
	gc_mark_q(m_munged);
	gc_mark_q(m_modexprs);
	gc_mark_q(m_refmods);
	gc_mark_q(m_classdecls);
	gc_mark_q(m_classids);
	gc_mark_q(m_output);
}
c_MapValues::c_MapValues(){
	m_map=0;
}
c_MapValues* c_MapValues::m_new(c_Map5* t_map){
	gc_assign(this->m_map,t_map);
	return this;
}
c_MapValues* c_MapValues::m_new2(){
	return this;
}
c_ValueEnumerator* c_MapValues::p_ObjectEnumerator(){
	return (new c_ValueEnumerator)->m_new(m_map->p_FirstNode());
}
void c_MapValues::mark(){
	Object::mark();
	gc_mark_q(m_map);
}
c_ValueEnumerator::c_ValueEnumerator(){
	m_node=0;
}
c_ValueEnumerator* c_ValueEnumerator::m_new(c_Node7* t_node){
	gc_assign(this->m_node,t_node);
	return this;
}
c_ValueEnumerator* c_ValueEnumerator::m_new2(){
	return this;
}
bool c_ValueEnumerator::p_HasNext(){
	return m_node!=0;
}
c_ModuleDecl* c_ValueEnumerator::p_NextObject(){
	c_Node7* t_t=m_node;
	gc_assign(m_node,m_node->p_NextNode());
	return t_t->m_value;
}
void c_ValueEnumerator::mark(){
	Object::mark();
	gc_mark_q(m_node);
}
c_Map7::c_Map7(){
	m_root=0;
}
c_Map7* c_Map7::m_new(){
	return this;
}
c_Node15* c_Map7::p_FindNode(String t_key){
	c_Node15* t_node=m_root;
	while((t_node)!=0){
		int t_cmp=p_Compare(t_key,t_node->m_key);
		if(t_cmp>0){
			t_node=t_node->m_right;
		}else{
			if(t_cmp<0){
				t_node=t_node->m_left;
			}else{
				return t_node;
			}
		}
	}
	return t_node;
}
bool c_Map7::p_Contains(String t_key){
	return p_FindNode(t_key)!=0;
}
int c_Map7::p_Get(String t_key){
	c_Node15* t_node=p_FindNode(t_key);
	if((t_node)!=0){
		return t_node->m_value;
	}
	return 0;
}
int c_Map7::p_RotateLeft7(c_Node15* t_node){
	c_Node15* t_child=t_node->m_right;
	gc_assign(t_node->m_right,t_child->m_left);
	if((t_child->m_left)!=0){
		gc_assign(t_child->m_left->m_parent,t_node);
	}
	gc_assign(t_child->m_parent,t_node->m_parent);
	if((t_node->m_parent)!=0){
		if(t_node==t_node->m_parent->m_left){
			gc_assign(t_node->m_parent->m_left,t_child);
		}else{
			gc_assign(t_node->m_parent->m_right,t_child);
		}
	}else{
		gc_assign(m_root,t_child);
	}
	gc_assign(t_child->m_left,t_node);
	gc_assign(t_node->m_parent,t_child);
	return 0;
}
int c_Map7::p_RotateRight7(c_Node15* t_node){
	c_Node15* t_child=t_node->m_left;
	gc_assign(t_node->m_left,t_child->m_right);
	if((t_child->m_right)!=0){
		gc_assign(t_child->m_right->m_parent,t_node);
	}
	gc_assign(t_child->m_parent,t_node->m_parent);
	if((t_node->m_parent)!=0){
		if(t_node==t_node->m_parent->m_right){
			gc_assign(t_node->m_parent->m_right,t_child);
		}else{
			gc_assign(t_node->m_parent->m_left,t_child);
		}
	}else{
		gc_assign(m_root,t_child);
	}
	gc_assign(t_child->m_right,t_node);
	gc_assign(t_node->m_parent,t_child);
	return 0;
}
int c_Map7::p_InsertFixup7(c_Node15* t_node){
	while(((t_node->m_parent)!=0) && t_node->m_parent->m_color==-1 && ((t_node->m_parent->m_parent)!=0)){
		if(t_node->m_parent==t_node->m_parent->m_parent->m_left){
			c_Node15* t_uncle=t_node->m_parent->m_parent->m_right;
			if(((t_uncle)!=0) && t_uncle->m_color==-1){
				t_node->m_parent->m_color=1;
				t_uncle->m_color=1;
				t_uncle->m_parent->m_color=-1;
				t_node=t_uncle->m_parent;
			}else{
				if(t_node==t_node->m_parent->m_right){
					t_node=t_node->m_parent;
					p_RotateLeft7(t_node);
				}
				t_node->m_parent->m_color=1;
				t_node->m_parent->m_parent->m_color=-1;
				p_RotateRight7(t_node->m_parent->m_parent);
			}
		}else{
			c_Node15* t_uncle2=t_node->m_parent->m_parent->m_left;
			if(((t_uncle2)!=0) && t_uncle2->m_color==-1){
				t_node->m_parent->m_color=1;
				t_uncle2->m_color=1;
				t_uncle2->m_parent->m_color=-1;
				t_node=t_uncle2->m_parent;
			}else{
				if(t_node==t_node->m_parent->m_left){
					t_node=t_node->m_parent;
					p_RotateRight7(t_node);
				}
				t_node->m_parent->m_color=1;
				t_node->m_parent->m_parent->m_color=-1;
				p_RotateLeft7(t_node->m_parent->m_parent);
			}
		}
	}
	m_root->m_color=1;
	return 0;
}
bool c_Map7::p_Set7(String t_key,int t_value){
	c_Node15* t_node=m_root;
	c_Node15* t_parent=0;
	int t_cmp=0;
	while((t_node)!=0){
		t_parent=t_node;
		t_cmp=p_Compare(t_key,t_node->m_key);
		if(t_cmp>0){
			t_node=t_node->m_right;
		}else{
			if(t_cmp<0){
				t_node=t_node->m_left;
			}else{
				t_node->m_value=t_value;
				return false;
			}
		}
	}
	t_node=(new c_Node15)->m_new(t_key,t_value,-1,t_parent);
	if((t_parent)!=0){
		if(t_cmp>0){
			gc_assign(t_parent->m_right,t_node);
		}else{
			gc_assign(t_parent->m_left,t_node);
		}
		p_InsertFixup7(t_node);
	}else{
		gc_assign(m_root,t_node);
	}
	return true;
}
void c_Map7::mark(){
	Object::mark();
	gc_mark_q(m_root);
}
c_StringMap7::c_StringMap7(){
}
c_StringMap7* c_StringMap7::m_new(){
	c_Map7::m_new();
	return this;
}
int c_StringMap7::p_Compare(String t_lhs,String t_rhs){
	return t_lhs.Compare(t_rhs);
}
void c_StringMap7::mark(){
	c_Map7::mark();
}
c_Node15::c_Node15(){
	m_key=String();
	m_right=0;
	m_left=0;
	m_value=0;
	m_color=0;
	m_parent=0;
}
c_Node15* c_Node15::m_new(String t_key,int t_value,int t_color,c_Node15* t_parent){
	this->m_key=t_key;
	this->m_value=t_value;
	this->m_color=t_color;
	gc_assign(this->m_parent,t_parent);
	return this;
}
c_Node15* c_Node15::m_new2(){
	return this;
}
void c_Node15::mark(){
	Object::mark();
	gc_mark_q(m_right);
	gc_mark_q(m_left);
	gc_mark_q(m_parent);
}
c_Enumerator4::c_Enumerator4(){
	m__list=0;
	m__curr=0;
}
c_Enumerator4* c_Enumerator4::m_new(c_List6* t_list){
	gc_assign(m__list,t_list);
	gc_assign(m__curr,t_list->m__head->m__succ);
	return this;
}
c_Enumerator4* c_Enumerator4::m_new2(){
	return this;
}
bool c_Enumerator4::p_HasNext(){
	while(m__curr->m__succ->m__pred!=m__curr){
		gc_assign(m__curr,m__curr->m__succ);
	}
	return m__curr!=m__list->m__head;
}
c_ClassDecl* c_Enumerator4::p_NextObject(){
	c_ClassDecl* t_data=m__curr->m__data;
	gc_assign(m__curr,m__curr->m__succ);
	return t_data;
}
void c_Enumerator4::mark(){
	Object::mark();
	gc_mark_q(m__list);
	gc_mark_q(m__curr);
}
c_Stack8::c_Stack8(){
	m_data=Array<c_ClassDecl* >();
	m_length=0;
}
c_Stack8* c_Stack8::m_new(){
	return this;
}
c_Stack8* c_Stack8::m_new2(Array<c_ClassDecl* > t_data){
	gc_assign(this->m_data,t_data.Slice(0));
	this->m_length=t_data.Length();
	return this;
}
c_ClassDecl* c_Stack8::m_NIL;
void c_Stack8::p_Length(int t_newlength){
	if(t_newlength<m_length){
		for(int t_i=t_newlength;t_i<m_length;t_i=t_i+1){
			gc_assign(m_data[t_i],m_NIL);
		}
	}else{
		if(t_newlength>m_data.Length()){
			gc_assign(m_data,m_data.Resize(bb_math_Max(m_length*2+10,t_newlength)));
		}
	}
	m_length=t_newlength;
}
int c_Stack8::p_Length2(){
	return m_length;
}
void c_Stack8::p_Push22(c_ClassDecl* t_value){
	if(m_length==m_data.Length()){
		gc_assign(m_data,m_data.Resize(m_length*2+10));
	}
	gc_assign(m_data[m_length],t_value);
	m_length+=1;
}
void c_Stack8::p_Push23(Array<c_ClassDecl* > t_values,int t_offset,int t_count){
	for(int t_i=0;t_i<t_count;t_i=t_i+1){
		p_Push22(t_values[t_offset+t_i]);
	}
}
void c_Stack8::p_Push24(Array<c_ClassDecl* > t_values,int t_offset){
	p_Push23(t_values,t_offset,t_values.Length()-t_offset);
}
c_ClassDecl* c_Stack8::p_Get2(int t_index){
	return m_data[t_index];
}
void c_Stack8::mark(){
	Object::mark();
	gc_mark_q(m_data);
}
int bb_parser_ParseSource(String t_source,c_AppDecl* t_app,c_ModuleDecl* t_mdecl,int t_defattrs){
	c_Toker* t_toker=(new c_Toker)->m_new(String(L"$SOURCE",7),t_source);
	c_Parser* t_parser=(new c_Parser)->m_new(t_toker,t_app,t_mdecl,t_defattrs);
	t_parser->p_ParseMain();
	return 0;
}
c_Translator::c_Translator(){
}
c_Translator* c_Translator::m_new(){
	return this;
}
void c_Translator::mark(){
	Object::mark();
}
c_Translator* bb_translator__trans;
int bb_os_DeleteDir(String t_path,bool t_recursive){
	if(!t_recursive){
		return DeleteDir(t_path);
	}
	int t_4=FileType(t_path);
	if(t_4==0){
		return 1;
	}else{
		if(t_4==1){
			return 0;
		}
	}
	Array<String > t_=LoadDir(t_path);
	int t_2=0;
	while(t_2<t_.Length()){
		String t_f=t_[t_2];
		t_2=t_2+1;
		if(t_f==String(L".",1) || t_f==String(L"..",2)){
			continue;
		}
		String t_fpath=t_path+String(L"/",1)+t_f;
		if(FileType(t_fpath)==2){
			if(!((bb_os_DeleteDir(t_fpath,true))!=0)){
				return 0;
			}
		}else{
			if(!((DeleteFile(t_fpath))!=0)){
				return 0;
			}
		}
	}
	return DeleteDir(t_path);
}
int bb_os_CopyDir(String t_srcpath,String t_dstpath,bool t_recursive,bool t_hidden){
	if(FileType(t_srcpath)!=2){
		return 0;
	}
	Array<String > t_files=LoadDir(t_srcpath);
	int t_2=FileType(t_dstpath);
	if(t_2==0){
		if(!((CreateDir(t_dstpath))!=0)){
			return 0;
		}
	}else{
		if(t_2==1){
			return 0;
		}
	}
	Array<String > t_=t_files;
	int t_3=0;
	while(t_3<t_.Length()){
		String t_f=t_[t_3];
		t_3=t_3+1;
		if(!t_hidden && t_f.StartsWith(String(L".",1))){
			continue;
		}
		String t_srcp=t_srcpath+String(L"/",1)+t_f;
		String t_dstp=t_dstpath+String(L"/",1)+t_f;
		int t_32=FileType(t_srcp);
		if(t_32==1){
			if(!((CopyFile(t_srcp,t_dstp))!=0)){
				return 0;
			}
		}else{
			if(t_32==2){
				if(t_recursive && !((bb_os_CopyDir(t_srcp,t_dstp,t_recursive,t_hidden))!=0)){
					return 0;
				}
			}
		}
	}
	return 1;
}
int bbMain(){
	c_TransCC* t_tcc=(new c_TransCC)->m_new();
	t_tcc->p_Run(AppArgs());
	return 0;
}
c_CTranslator::c_CTranslator(){
	m_funcMungs=(new c_StringMap8)->m_new();
	m_mungedFuncs=(new c_StringMap9)->m_new();
	m_mungedScopes=(new c_StringMap10)->m_new();
	m_indent=String();
	m_lines=(new c_StringStack)->m_new2();
	m_emitDebugInfo=false;
	m_unreachable=0;
	m_broken=0;
}
c_CTranslator* c_CTranslator::m_new(){
	c_Translator::m_new();
	return this;
}
int c_CTranslator::p_MungMethodDecl(c_FuncDecl* t_fdecl){
	if((t_fdecl->m_munged).Length()!=0){
		return 0;
	}
	if((t_fdecl->m_overrides)!=0){
		p_MungMethodDecl(t_fdecl->m_overrides);
		t_fdecl->m_munged=t_fdecl->m_overrides->m_munged;
		return 0;
	}
	c_FuncDeclList* t_funcs=m_funcMungs->p_Get(t_fdecl->m_ident);
	if((t_funcs)!=0){
		c_Enumerator3* t_=t_funcs->p_ObjectEnumerator();
		while(t_->p_HasNext()){
			c_FuncDecl* t_tdecl=t_->p_NextObject();
			if(t_fdecl->p_EqualsArgs(t_tdecl)){
				t_fdecl->m_munged=t_tdecl->m_munged;
				return 0;
			}
		}
	}else{
		t_funcs=(new c_FuncDeclList)->m_new();
		m_funcMungs->p_Set8(t_fdecl->m_ident,t_funcs);
	}
	String t_id=t_fdecl->m_ident;
	if(m_mungedFuncs->p_Contains(t_id)){
		int t_n=1;
		do{
			t_n+=1;
			t_id=t_fdecl->m_ident+String(t_n);
		}while(!(!m_mungedFuncs->p_Contains(t_id)));
	}
	m_mungedFuncs->p_Set9(t_id,t_fdecl);
	t_fdecl->m_munged=String(L"p_",2)+t_id;
	t_funcs->p_AddLast4(t_fdecl);
	return 0;
}
int c_CTranslator::p_MungDecl(c_Decl* t_decl){
	if((t_decl->m_munged).Length()!=0){
		return 0;
	}
	c_FuncDecl* t_fdecl=dynamic_cast<c_FuncDecl*>(t_decl);
	if(((t_fdecl)!=0) && t_fdecl->p_IsMethod()){
		return p_MungMethodDecl(t_fdecl);
	}
	String t_id=t_decl->m_ident;
	String t_munged=String();
	String t_scope=String();
	if((dynamic_cast<c_LocalDecl*>(t_decl))!=0){
		t_scope=String(L"$",1);
		t_munged=String(L"t_",2)+t_id;
	}else{
		if((dynamic_cast<c_ClassDecl*>(t_decl))!=0){
			t_scope=String();
			t_munged=String(L"c_",2)+t_id;
		}else{
			if((dynamic_cast<c_ModuleDecl*>(t_decl))!=0){
				t_scope=String();
				t_munged=String(L"bb_",3)+t_id;
			}else{
				if((dynamic_cast<c_ClassDecl*>(t_decl->m_scope))!=0){
					t_scope=t_decl->m_scope->m_munged;
					t_munged=String(L"m_",2)+t_id;
				}else{
					if((dynamic_cast<c_ModuleDecl*>(t_decl->m_scope))!=0){
						if(bb_config_ENV_LANG==String(L"cs",2) || bb_config_ENV_LANG==String(L"java",4)){
							t_scope=t_decl->m_scope->m_munged;
							t_munged=String(L"g_",2)+t_id;
						}else{
							t_scope=String();
							t_munged=t_decl->m_scope->m_munged+String(L"_",1)+t_id;
						}
					}else{
						bb_config_InternalErr(String(L"Internal error",14));
					}
				}
			}
		}
	}
	c_StringSet* t_set=m_mungedScopes->p_Get(t_scope);
	if((t_set)!=0){
		if(t_set->p_Contains(t_munged.ToLower())){
			int t_id2=1;
			do{
				t_id2+=1;
				String t_t=t_munged+String(t_id2);
				if(t_set->p_Contains(t_t.ToLower())){
					continue;
				}
				t_munged=t_t;
				break;
			}while(!(false));
		}
	}else{
		if(t_scope==String(L"$",1)){
			bbPrint(String(L"OOPS2",5));
			bb_config_InternalErr(String(L"Internal error",14));
		}
		t_set=(new c_StringSet)->m_new();
		m_mungedScopes->p_Set10(t_scope,t_set);
	}
	t_set->p_Insert(t_munged.ToLower());
	t_decl->m_munged=t_munged;
	return 0;
}
int c_CTranslator::p_Emit(String t_t){
	if(!((t_t).Length()!=0)){
		return 0;
	}
	if(t_t.StartsWith(String(L"}",1))){
		m_indent=m_indent.Slice(0,m_indent.Length()-1);
	}
	m_lines->p_Push(m_indent+t_t);
	if(t_t.EndsWith(String(L"{",1))){
		m_indent=m_indent+String(L"\t",1);
	}
	return 0;
}
int c_CTranslator::p_BeginLocalScope(){
	m_mungedScopes->p_Set10(String(L"$",1),(new c_StringSet)->m_new());
	return 0;
}
String c_CTranslator::p_Bra(String t_str){
	if(t_str.StartsWith(String(L"(",1)) && t_str.EndsWith(String(L")",1))){
		int t_n=1;
		for(int t_i=1;t_i<t_str.Length()-1;t_i=t_i+1){
			String t_1=t_str.Slice(t_i,t_i+1);
			if(t_1==String(L"(",1)){
				t_n+=1;
			}else{
				if(t_1==String(L")",1)){
					t_n-=1;
					if(!((t_n)!=0)){
						return String(L"(",1)+t_str+String(L")",1);
					}
				}
			}
		}
		if(t_n==1){
			return t_str;
		}
	}
	return String(L"(",1)+t_str+String(L")",1);
}
int c_CTranslator::p_EmitEnter(c_FuncDecl* t_func){
	return 0;
}
int c_CTranslator::p_EmitEnterBlock(){
	return 0;
}
int c_CTranslator::p_EmitSetErr(String t_errInfo){
	return 0;
}
String c_CTranslator::p_CreateLocal(c_Expr* t_expr){
	c_LocalDecl* t_tmp=(new c_LocalDecl)->m_new(String(),0,t_expr->m_exprType,t_expr);
	p_MungDecl(t_tmp);
	p_Emit(p_TransLocalDecl(t_tmp->m_munged,t_expr)+String(L";",1));
	return t_tmp->m_munged;
}
String c_CTranslator::p_TransExprNS(c_Expr* t_expr){
	if(!t_expr->p_SideEffects()){
		return t_expr->p_Trans();
	}
	return p_CreateLocal(t_expr);
}
int c_CTranslator::p_EmitLeave(){
	return 0;
}
int c_CTranslator::p_EmitLeaveBlock(){
	return 0;
}
int c_CTranslator::p_EmitBlock(c_BlockDecl* t_block,bool t_realBlock){
	bb_decl_PushEnv(t_block);
	c_FuncDecl* t_func=dynamic_cast<c_FuncDecl*>(t_block);
	if((t_func)!=0){
		m_emitDebugInfo=bb_config_ENV_CONFIG!=String(L"release",7);
		if((t_func->m_attrs&4194304)!=0){
			m_emitDebugInfo=false;
		}
		if(m_emitDebugInfo){
			p_EmitEnter(t_func);
		}
	}else{
		if(m_emitDebugInfo && t_realBlock){
			p_EmitEnterBlock();
		}
	}
	c_Stmt* t_lastStmt=0;
	c_Enumerator5* t_=t_block->m_stmts->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_Stmt* t_stmt=t_->p_NextObject();
		bb_config__errInfo=t_stmt->m_errInfo;
		if((m_unreachable)!=0){
			break;
		}
		t_lastStmt=t_stmt;
		if(m_emitDebugInfo){
			c_ReturnStmt* t_rs=dynamic_cast<c_ReturnStmt*>(t_stmt);
			if((t_rs)!=0){
				if((t_rs->m_expr)!=0){
					if((t_stmt->m_errInfo).Length()!=0){
						p_EmitSetErr(t_stmt->m_errInfo);
					}
					String t_t_expr=p_TransExprNS(t_rs->m_expr);
					p_EmitLeave();
					p_Emit(String(L"return ",7)+t_t_expr+String(L";",1));
				}else{
					p_EmitLeave();
					p_Emit(String(L"return;",7));
				}
				m_unreachable=1;
				continue;
			}
			if((t_stmt->m_errInfo).Length()!=0){
				p_EmitSetErr(t_stmt->m_errInfo);
			}
		}
		String t_t=t_stmt->p_Trans();
		if((t_t).Length()!=0){
			p_Emit(t_t+String(L";",1));
		}
	}
	bb_config__errInfo=String();
	int t_unr=m_unreachable;
	m_unreachable=0;
	if((t_unr)!=0){
		if(((t_func)!=0) && bb_config_ENV_LANG==String(L"as",2) && !((dynamic_cast<c_VoidType*>(t_func->m_retType))!=0)){
			if(!((dynamic_cast<c_ReturnStmt*>(t_lastStmt))!=0)){
				p_Emit(String(L"return ",7)+p_TransValue(t_func->m_retType,String())+String(L";",1));
			}
		}
	}else{
		if((t_func)!=0){
			if(m_emitDebugInfo){
				p_EmitLeave();
			}
			if(!((dynamic_cast<c_VoidType*>(t_func->m_retType))!=0)){
				if(t_func->p_IsCtor()){
					p_Emit(String(L"return this;",12));
				}else{
					if((t_func->p_ModuleScope()->p_IsStrict())!=0){
						bb_config__errInfo=t_func->m_errInfo;
						bb_config_Err(String(L"Missing return statement.",25));
					}
					p_Emit(String(L"return ",7)+p_TransValue(t_func->m_retType,String())+String(L";",1));
				}
			}
		}else{
			if(m_emitDebugInfo && t_realBlock){
				p_EmitLeaveBlock();
			}
		}
	}
	bb_decl_PopEnv();
	return t_unr;
}
int c_CTranslator::p_EndLocalScope(){
	m_mungedScopes->p_Set10(String(L"$",1),0);
	return 0;
}
String c_CTranslator::p_JoinLines(){
	String t_code=m_lines->p_Join(String(L"\n",1));
	m_lines->p_Clear();
	return t_code;
}
String c_CTranslator::p_Enquote(String t_str){
	return bb_config_Enquote(t_str,bb_config_ENV_LANG);
}
int c_CTranslator::p_BeginLoop(){
	return 0;
}
int c_CTranslator::p_EndLoop(){
	return 0;
}
int c_CTranslator::p_ExprPri(c_Expr* t_expr){
	if((dynamic_cast<c_NewObjectExpr*>(t_expr))!=0){
		return 3;
	}else{
		if((dynamic_cast<c_UnaryExpr*>(t_expr))!=0){
			String t_5=dynamic_cast<c_UnaryExpr*>(t_expr)->m_op;
			if(t_5==String(L"+",1) || t_5==String(L"-",1) || t_5==String(L"~",1) || t_5==String(L"not",3)){
				return 3;
			}
			bb_config_InternalErr(String(L"Internal error",14));
		}else{
			if((dynamic_cast<c_BinaryExpr*>(t_expr))!=0){
				String t_6=dynamic_cast<c_BinaryExpr*>(t_expr)->m_op;
				if(t_6==String(L"*",1) || t_6==String(L"/",1) || t_6==String(L"mod",3)){
					return 4;
				}else{
					if(t_6==String(L"+",1) || t_6==String(L"-",1)){
						return 5;
					}else{
						if(t_6==String(L"shl",3) || t_6==String(L"shr",3)){
							return 6;
						}else{
							if(t_6==String(L"<",1) || t_6==String(L"<=",2) || t_6==String(L">",1) || t_6==String(L">=",2)){
								return 7;
							}else{
								if(t_6==String(L"=",1) || t_6==String(L"<>",2)){
									return 8;
								}else{
									if(t_6==String(L"&",1)){
										return 9;
									}else{
										if(t_6==String(L"~",1)){
											return 10;
										}else{
											if(t_6==String(L"|",1)){
												return 11;
											}else{
												if(t_6==String(L"and",3)){
													return 12;
												}else{
													if(t_6==String(L"or",2)){
														return 13;
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
				bb_config_InternalErr(String(L"Internal error",14));
			}
		}
	}
	return 2;
}
String c_CTranslator::p_TransSubExpr(c_Expr* t_expr,int t_pri){
	String t_t_expr=t_expr->p_Trans();
	if(p_ExprPri(t_expr)>t_pri){
		t_t_expr=p_Bra(t_t_expr);
	}
	return t_t_expr;
}
String c_CTranslator::p_TransStmtExpr(c_StmtExpr* t_expr){
	String t_t=t_expr->m_stmt->p_Trans();
	if((t_t).Length()!=0){
		p_Emit(t_t+String(L";",1));
	}
	return t_expr->m_expr->p_Trans();
}
String c_CTranslator::p_TransVarExpr(c_VarExpr* t_expr){
	c_VarDecl* t_decl=t_expr->m_decl;
	if(t_decl->m_munged.StartsWith(String(L"$",1))){
		return p_TransIntrinsicExpr((t_decl),0,Array<c_Expr* >());
	}
	if((dynamic_cast<c_LocalDecl*>(t_decl))!=0){
		return t_decl->m_munged;
	}
	if((dynamic_cast<c_FieldDecl*>(t_decl))!=0){
		return p_TransField(dynamic_cast<c_FieldDecl*>(t_decl),0);
	}
	if((dynamic_cast<c_GlobalDecl*>(t_decl))!=0){
		return p_TransGlobal(dynamic_cast<c_GlobalDecl*>(t_decl));
	}
	bb_config_InternalErr(String(L"Internal error",14));
	return String();
}
String c_CTranslator::p_TransMemberVarExpr(c_MemberVarExpr* t_expr){
	c_VarDecl* t_decl=t_expr->m_decl;
	if(t_decl->m_munged.StartsWith(String(L"$",1))){
		return p_TransIntrinsicExpr((t_decl),t_expr->m_expr,Array<c_Expr* >());
	}
	if((dynamic_cast<c_FieldDecl*>(t_decl))!=0){
		return p_TransField(dynamic_cast<c_FieldDecl*>(t_decl),t_expr->m_expr);
	}
	bb_config_InternalErr(String(L"Internal error",14));
	return String();
}
String c_CTranslator::p_TransInvokeExpr(c_InvokeExpr* t_expr){
	c_FuncDecl* t_decl=t_expr->m_decl;
	String t_t=String();
	if(t_decl->m_munged.StartsWith(String(L"$",1))){
		return p_TransIntrinsicExpr((t_decl),0,t_expr->m_args);
	}
	if((t_decl)!=0){
		return p_TransFunc(t_decl,t_expr->m_args,0);
	}
	bb_config_InternalErr(String(L"Internal error",14));
	return String();
}
String c_CTranslator::p_TransInvokeMemberExpr(c_InvokeMemberExpr* t_expr){
	c_FuncDecl* t_decl=t_expr->m_decl;
	String t_t=String();
	if(t_decl->m_munged.StartsWith(String(L"$",1))){
		return p_TransIntrinsicExpr((t_decl),t_expr->m_expr,t_expr->m_args);
	}
	if((t_decl)!=0){
		return p_TransFunc(t_decl,t_expr->m_args,t_expr->m_expr);
	}
	bb_config_InternalErr(String(L"Internal error",14));
	return String();
}
String c_CTranslator::p_TransInvokeSuperExpr(c_InvokeSuperExpr* t_expr){
	c_FuncDecl* t_decl=t_expr->m_funcDecl;
	String t_t=String();
	if(t_decl->m_munged.StartsWith(String(L"$",1))){
		return p_TransIntrinsicExpr((t_decl),(t_expr),Array<c_Expr* >());
	}
	if((t_decl)!=0){
		return p_TransSuperFunc(t_decl,t_expr->m_args);
	}
	bb_config_InternalErr(String(L"Internal error",14));
	return String();
}
String c_CTranslator::p_TransExprStmt(c_ExprStmt* t_stmt){
	return t_stmt->m_expr->p_TransStmt();
}
String c_CTranslator::p_TransAssignOp(String t_op){
	String t_4=t_op;
	if(t_4==String(L"~=",2)){
		return String(L"^=",2);
	}else{
		if(t_4==String(L"mod=",4)){
			return String(L"%=",2);
		}else{
			if(t_4==String(L"shl=",4)){
				return String(L"<<=",3);
			}else{
				if(t_4==String(L"shr=",4)){
					return String(L">>=",3);
				}
			}
		}
	}
	return t_op;
}
String c_CTranslator::p_TransAssignStmt2(c_AssignStmt* t_stmt){
	return t_stmt->m_lhs->p_TransVar()+p_TransAssignOp(t_stmt->m_op)+t_stmt->m_rhs->p_Trans();
}
String c_CTranslator::p_TransAssignStmt(c_AssignStmt* t_stmt){
	if(!((t_stmt->m_rhs)!=0)){
		return t_stmt->m_lhs->p_Trans();
	}
	if((t_stmt->m_tmp1)!=0){
		p_MungDecl(t_stmt->m_tmp1);
		p_Emit(p_TransLocalDecl(t_stmt->m_tmp1->m_munged,t_stmt->m_tmp1->m_init)+String(L";",1));
	}
	if((t_stmt->m_tmp2)!=0){
		p_MungDecl(t_stmt->m_tmp2);
		p_Emit(p_TransLocalDecl(t_stmt->m_tmp2->m_munged,t_stmt->m_tmp2->m_init)+String(L";",1));
	}
	return p_TransAssignStmt2(t_stmt);
}
String c_CTranslator::p_TransReturnStmt(c_ReturnStmt* t_stmt){
	String t_t=String(L"return",6);
	if((t_stmt->m_expr)!=0){
		t_t=t_t+(String(L" ",1)+t_stmt->m_expr->p_Trans());
	}
	m_unreachable=1;
	return t_t;
}
String c_CTranslator::p_TransContinueStmt(c_ContinueStmt* t_stmt){
	m_unreachable=1;
	return String(L"continue",8);
}
String c_CTranslator::p_TransBreakStmt(c_BreakStmt* t_stmt){
	m_unreachable=1;
	m_broken+=1;
	return String(L"break",5);
}
String c_CTranslator::p_TransBlock(c_BlockDecl* t_block){
	p_EmitBlock(t_block,false);
	return String();
}
String c_CTranslator::p_TransDeclStmt(c_DeclStmt* t_stmt){
	c_LocalDecl* t_decl=dynamic_cast<c_LocalDecl*>(t_stmt->m_decl);
	if((t_decl)!=0){
		p_MungDecl(t_decl);
		return p_TransLocalDecl(t_decl->m_munged,t_decl->m_init);
	}
	c_ConstDecl* t_cdecl=dynamic_cast<c_ConstDecl*>(t_stmt->m_decl);
	if((t_cdecl)!=0){
		return String();
	}
	bb_config_InternalErr(String(L"Internal error",14));
	return String();
}
String c_CTranslator::p_TransIfStmt(c_IfStmt* t_stmt){
	if(((dynamic_cast<c_ConstExpr*>(t_stmt->m_expr))!=0) && bb_config_ENV_LANG!=String(L"java",4)){
		if((dynamic_cast<c_ConstExpr*>(t_stmt->m_expr)->m_value).Length()!=0){
			if(!t_stmt->m_thenBlock->m_stmts->p_IsEmpty()){
				p_Emit(String(L"if(true){",9));
				if((p_EmitBlock(t_stmt->m_thenBlock,true))!=0){
					m_unreachable=1;
				}
				p_Emit(String(L"}",1));
			}
		}else{
			if(!t_stmt->m_elseBlock->m_stmts->p_IsEmpty()){
				p_Emit(String(L"if(true){",9));
				if((p_EmitBlock(t_stmt->m_elseBlock,true))!=0){
					m_unreachable=1;
				}
				p_Emit(String(L"}",1));
			}
		}
	}else{
		if(!t_stmt->m_elseBlock->m_stmts->p_IsEmpty()){
			p_Emit(String(L"if",2)+p_Bra(t_stmt->m_expr->p_Trans())+String(L"{",1));
			int t_unr=p_EmitBlock(t_stmt->m_thenBlock,true);
			p_Emit(String(L"}else{",6));
			int t_unr2=p_EmitBlock(t_stmt->m_elseBlock,true);
			p_Emit(String(L"}",1));
			if(((t_unr)!=0) && ((t_unr2)!=0)){
				m_unreachable=1;
			}
		}else{
			p_Emit(String(L"if",2)+p_Bra(t_stmt->m_expr->p_Trans())+String(L"{",1));
			int t_unr3=p_EmitBlock(t_stmt->m_thenBlock,true);
			p_Emit(String(L"}",1));
		}
	}
	return String();
}
String c_CTranslator::p_TransWhileStmt(c_WhileStmt* t_stmt){
	int t_nbroken=m_broken;
	p_Emit(String(L"while",5)+p_Bra(t_stmt->m_expr->p_Trans())+String(L"{",1));
	p_BeginLoop();
	int t_unr=p_EmitBlock(t_stmt->m_block,true);
	p_EndLoop();
	p_Emit(String(L"}",1));
	if(m_broken==t_nbroken && ((dynamic_cast<c_ConstExpr*>(t_stmt->m_expr))!=0) && ((dynamic_cast<c_ConstExpr*>(t_stmt->m_expr)->m_value).Length()!=0)){
		m_unreachable=1;
	}
	m_broken=t_nbroken;
	return String();
}
String c_CTranslator::p_TransRepeatStmt(c_RepeatStmt* t_stmt){
	int t_nbroken=m_broken;
	p_Emit(String(L"do{",3));
	p_BeginLoop();
	int t_unr=p_EmitBlock(t_stmt->m_block,true);
	p_EndLoop();
	p_Emit(String(L"}while(!",8)+p_Bra(t_stmt->m_expr->p_Trans())+String(L");",2));
	if(m_broken==t_nbroken && ((dynamic_cast<c_ConstExpr*>(t_stmt->m_expr))!=0) && !((dynamic_cast<c_ConstExpr*>(t_stmt->m_expr)->m_value).Length()!=0)){
		m_unreachable=1;
	}
	m_broken=t_nbroken;
	return String();
}
String c_CTranslator::p_TransForStmt(c_ForStmt* t_stmt){
	int t_nbroken=m_broken;
	String t_init=t_stmt->m_init->p_Trans();
	String t_expr=t_stmt->m_expr->p_Trans();
	String t_incr=t_stmt->m_incr->p_Trans();
	p_Emit(String(L"for(",4)+t_init+String(L";",1)+t_expr+String(L";",1)+t_incr+String(L"){",2));
	p_BeginLoop();
	int t_unr=p_EmitBlock(t_stmt->m_block,true);
	p_EndLoop();
	p_Emit(String(L"}",1));
	if(m_broken==t_nbroken && ((dynamic_cast<c_ConstExpr*>(t_stmt->m_expr))!=0) && ((dynamic_cast<c_ConstExpr*>(t_stmt->m_expr)->m_value).Length()!=0)){
		m_unreachable=1;
	}
	m_broken=t_nbroken;
	return String();
}
String c_CTranslator::p_TransTryStmt(c_TryStmt* t_stmt){
	bb_config_Err(String(L"TODO!",5));
	return String();
}
String c_CTranslator::p_TransThrowStmt(c_ThrowStmt* t_stmt){
	m_unreachable=1;
	return String(L"throw ",6)+t_stmt->m_expr->p_Trans();
}
String c_CTranslator::p_TransUnaryOp(String t_op){
	String t_2=t_op;
	if(t_2==String(L"+",1)){
		return String(L"+",1);
	}else{
		if(t_2==String(L"-",1)){
			return String(L"-",1);
		}else{
			if(t_2==String(L"~",1)){
				return t_op;
			}else{
				if(t_2==String(L"not",3)){
					return String(L"!",1);
				}
			}
		}
	}
	bb_config_InternalErr(String(L"Internal error",14));
	return String();
}
String c_CTranslator::p_TransBinaryOp(String t_op,String t_rhs){
	String t_3=t_op;
	if(t_3==String(L"+",1) || t_3==String(L"-",1)){
		if(t_rhs.StartsWith(t_op)){
			return t_op+String(L" ",1);
		}
		return t_op;
	}else{
		if(t_3==String(L"*",1) || t_3==String(L"/",1)){
			return t_op;
		}else{
			if(t_3==String(L"shl",3)){
				return String(L"<<",2);
			}else{
				if(t_3==String(L"shr",3)){
					return String(L">>",2);
				}else{
					if(t_3==String(L"mod",3)){
						return String(L" % ",3);
					}else{
						if(t_3==String(L"and",3)){
							return String(L" && ",4);
						}else{
							if(t_3==String(L"or",2)){
								return String(L" || ",4);
							}else{
								if(t_3==String(L"=",1)){
									return String(L"==",2);
								}else{
									if(t_3==String(L"<>",2)){
										return String(L"!=",2);
									}else{
										if(t_3==String(L"<",1) || t_3==String(L"<=",2) || t_3==String(L">",1) || t_3==String(L">=",2)){
											return t_op;
										}else{
											if(t_3==String(L"&",1) || t_3==String(L"|",1)){
												return t_op;
											}else{
												if(t_3==String(L"~",1)){
													return String(L"^",1);
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	bb_config_InternalErr(String(L"Internal error",14));
	return String();
}
void c_CTranslator::mark(){
	c_Translator::mark();
	gc_mark_q(m_funcMungs);
	gc_mark_q(m_mungedFuncs);
	gc_mark_q(m_mungedScopes);
	gc_mark_q(m_lines);
}
c_JavaTranslator::c_JavaTranslator(){
	m_langutil=false;
	m_unsafe=0;
}
c_JavaTranslator* c_JavaTranslator::m_new(){
	c_CTranslator::m_new();
	return this;
}
String c_JavaTranslator::p_TransType(c_Type* t_ty){
	if((dynamic_cast<c_VoidType*>(t_ty))!=0){
		return String(L"void",4);
	}
	if((dynamic_cast<c_BoolType*>(t_ty))!=0){
		return String(L"boolean",7);
	}
	if((dynamic_cast<c_IntType*>(t_ty))!=0){
		return String(L"int",3);
	}
	if((dynamic_cast<c_FloatType*>(t_ty))!=0){
		return String(L"float",5);
	}
	if((dynamic_cast<c_StringType*>(t_ty))!=0){
		return String(L"String",6);
	}
	if((dynamic_cast<c_ArrayType*>(t_ty))!=0){
		return p_TransType(dynamic_cast<c_ArrayType*>(t_ty)->m_elemType)+String(L"[]",2);
	}
	if((dynamic_cast<c_ObjectType*>(t_ty))!=0){
		return t_ty->p_GetClass()->m_munged;
	}
	bb_config_InternalErr(String(L"Internal error",14));
	return String();
}
int c_JavaTranslator::p_EmitFuncDecl(c_FuncDecl* t_decl){
	m_unsafe=((t_decl->m_ident.EndsWith(String(L"__UNSAFE__",10)))?1:0);
	p_BeginLocalScope();
	String t_args=String();
	Array<c_ArgDecl* > t_=t_decl->m_argDecls;
	int t_2=0;
	while(t_2<t_.Length()){
		c_ArgDecl* t_arg=t_[t_2];
		t_2=t_2+1;
		p_MungDecl(t_arg);
		if((t_args).Length()!=0){
			t_args=t_args+String(L",",1);
		}
		t_args=t_args+(p_TransType(t_arg->m_type)+String(L" ",1)+t_arg->m_munged);
	}
	String t_t=p_TransType(t_decl->m_retType)+String(L" ",1)+t_decl->m_munged+p_Bra(t_args);
	if(((t_decl->p_ClassScope())!=0) && ((t_decl->p_ClassScope()->p_IsInterface())!=0)){
		p_Emit(String(L"public ",7)+t_t+String(L";",1));
	}else{
		if((t_decl->p_IsAbstract())!=0){
			p_Emit(String(L"public abstract ",16)+t_t+String(L";",1));
		}else{
			String t_q=String(L"public ",7);
			if(t_decl->p_IsStatic()){
				t_q=t_q+String(L"static ",7);
			}else{
				if(!t_decl->p_IsVirtual()){
					t_q=t_q+String(L"final ",6);
				}
			}
			p_Emit(t_q+t_t+String(L"{",1));
			p_EmitBlock((t_decl),true);
			p_Emit(String(L"}",1));
		}
	}
	p_EndLocalScope();
	m_unsafe=0;
	return 0;
}
String c_JavaTranslator::p_TransDecl(c_Decl* t_decl){
	String t_id=t_decl->m_munged;
	c_ValDecl* t_vdecl=dynamic_cast<c_ValDecl*>(t_decl);
	if((t_vdecl)!=0){
		return p_TransType(t_vdecl->m_type)+String(L" ",1)+t_id;
	}
	bb_config_InternalErr(String(L"Internal error",14));
	return String();
}
int c_JavaTranslator::p_EmitClassDecl(c_ClassDecl* t_classDecl){
	String t_classid=t_classDecl->m_munged;
	String t_superid=t_classDecl->m_superClass->m_munged;
	if((t_classDecl->p_IsInterface())!=0){
		String t_bases=String();
		Array<c_ClassDecl* > t_=t_classDecl->m_implments;
		int t_2=0;
		while(t_2<t_.Length()){
			c_ClassDecl* t_iface=t_[t_2];
			t_2=t_2+1;
			if((t_bases).Length()!=0){
				t_bases=t_bases+String(L",",1);
			}else{
				t_bases=String(L" extends ",9);
			}
			t_bases=t_bases+t_iface->m_munged;
		}
		p_Emit(String(L"interface ",10)+t_classid+t_bases+String(L"{",1));
		c_Enumerator2* t_3=t_classDecl->p_Semanted()->p_ObjectEnumerator();
		while(t_3->p_HasNext()){
			c_Decl* t_decl=t_3->p_NextObject();
			c_FuncDecl* t_fdecl=dynamic_cast<c_FuncDecl*>(t_decl);
			if(!((t_fdecl)!=0)){
				continue;
			}
			p_EmitFuncDecl(t_fdecl);
		}
		p_Emit(String(L"}",1));
		return 0;
	}
	String t_bases2=String();
	Array<c_ClassDecl* > t_4=t_classDecl->m_implments;
	int t_5=0;
	while(t_5<t_4.Length()){
		c_ClassDecl* t_iface2=t_4[t_5];
		t_5=t_5+1;
		if((t_bases2).Length()!=0){
			t_bases2=t_bases2+String(L",",1);
		}else{
			t_bases2=String(L" implements ",12);
		}
		t_bases2=t_bases2+t_iface2->m_munged;
	}
	String t_q=String();
	if((t_classDecl->p_IsAbstract())!=0){
		t_q=String(L"abstract ",9);
	}else{
		if((t_classDecl->p_IsFinal())!=0){
			t_q=String(L"final ",6);
		}
	}
	p_Emit(t_q+String(L"class ",6)+t_classid+String(L" extends ",9)+t_superid+t_bases2+String(L"{",1));
	c_Enumerator2* t_6=t_classDecl->p_Semanted()->p_ObjectEnumerator();
	while(t_6->p_HasNext()){
		c_Decl* t_decl2=t_6->p_NextObject();
		c_FieldDecl* t_tdecl=dynamic_cast<c_FieldDecl*>(t_decl2);
		if((t_tdecl)!=0){
			p_Emit(p_TransDecl(t_tdecl)+String(L"=",1)+t_tdecl->m_init->p_Trans()+String(L";",1));
			continue;
		}
		c_FuncDecl* t_fdecl2=dynamic_cast<c_FuncDecl*>(t_decl2);
		if((t_fdecl2)!=0){
			p_EmitFuncDecl(t_fdecl2);
			continue;
		}
		c_GlobalDecl* t_gdecl=dynamic_cast<c_GlobalDecl*>(t_decl2);
		if((t_gdecl)!=0){
			p_Emit(String(L"static ",7)+p_TransDecl(t_gdecl)+String(L";",1));
			continue;
		}
	}
	p_Emit(String(L"}",1));
	return 0;
}
String c_JavaTranslator::p_TransStatic(c_Decl* t_decl){
	if(((t_decl->p_IsExtern())!=0) && ((dynamic_cast<c_ModuleDecl*>(t_decl->m_scope))!=0)){
		return t_decl->m_munged;
	}else{
		if(((bb_decl__env)!=0) && ((t_decl->m_scope)!=0) && t_decl->m_scope==(bb_decl__env->p_ClassScope())){
			return t_decl->m_munged;
		}else{
			if((dynamic_cast<c_ClassDecl*>(t_decl->m_scope))!=0){
				return t_decl->m_scope->m_munged+String(L".",1)+t_decl->m_munged;
			}else{
				if((dynamic_cast<c_ModuleDecl*>(t_decl->m_scope))!=0){
					return t_decl->m_scope->m_munged+String(L".",1)+t_decl->m_munged;
				}
			}
		}
	}
	bb_config_InternalErr(String(L"Internal error",14));
	return String();
}
String c_JavaTranslator::p_TransGlobal(c_GlobalDecl* t_decl){
	return p_TransStatic(t_decl);
}
String c_JavaTranslator::p_TransApp(c_AppDecl* t_app){
	m_langutil=bb_config_GetConfigVar(String(L"ANDROID_LANGUTIL_ENABLED",24))==String(L"1",1);
	t_app->m_mainModule->m_munged=String(L"bb_",3);
	t_app->m_mainFunc->m_munged=String(L"bbMain",6);
	c_ValueEnumerator* t_=t_app->m_imported->p_Values()->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_ModuleDecl* t_decl=t_->p_NextObject();
		p_MungDecl(t_decl);
	}
	c_Enumerator2* t_2=t_app->p_Semanted()->p_ObjectEnumerator();
	while(t_2->p_HasNext()){
		c_Decl* t_decl2=t_2->p_NextObject();
		p_MungDecl(t_decl2);
		c_ClassDecl* t_cdecl=dynamic_cast<c_ClassDecl*>(t_decl2);
		if(!((t_cdecl)!=0)){
			continue;
		}
		c_Enumerator2* t_3=t_cdecl->p_Semanted()->p_ObjectEnumerator();
		while(t_3->p_HasNext()){
			c_Decl* t_decl3=t_3->p_NextObject();
			if(((dynamic_cast<c_FuncDecl*>(t_decl3))!=0) && dynamic_cast<c_FuncDecl*>(t_decl3)->p_IsCtor()){
				t_decl3->m_ident=t_cdecl->m_ident+String(L"_",1)+t_decl3->m_ident;
			}
			p_MungDecl(t_decl3);
		}
	}
	c_Enumerator2* t_4=t_app->p_Semanted()->p_ObjectEnumerator();
	while(t_4->p_HasNext()){
		c_Decl* t_decl4=t_4->p_NextObject();
		c_ClassDecl* t_cdecl2=dynamic_cast<c_ClassDecl*>(t_decl4);
		if((t_cdecl2)!=0){
			p_EmitClassDecl(t_cdecl2);
		}
	}
	c_ValueEnumerator* t_5=t_app->m_imported->p_Values()->p_ObjectEnumerator();
	while(t_5->p_HasNext()){
		c_ModuleDecl* t_mdecl=t_5->p_NextObject();
		p_Emit(String(L"class ",6)+t_mdecl->m_munged+String(L"{",1));
		c_Enumerator2* t_6=t_mdecl->p_Semanted()->p_ObjectEnumerator();
		while(t_6->p_HasNext()){
			c_Decl* t_decl5=t_6->p_NextObject();
			if(((t_decl5->p_IsExtern())!=0) || ((t_decl5->m_scope->p_ClassScope())!=0)){
				continue;
			}
			c_GlobalDecl* t_gdecl=dynamic_cast<c_GlobalDecl*>(t_decl5);
			if((t_gdecl)!=0){
				p_Emit(String(L"static ",7)+p_TransDecl(t_gdecl)+String(L";",1));
				continue;
			}
			c_FuncDecl* t_fdecl=dynamic_cast<c_FuncDecl*>(t_decl5);
			if((t_fdecl)!=0){
				p_EmitFuncDecl(t_fdecl);
				continue;
			}
		}
		if(t_mdecl==t_app->m_mainModule){
			p_BeginLocalScope();
			p_Emit(String(L"public static int bbInit(){",27));
			c_Enumerator6* t_7=t_app->m_semantedGlobals->p_ObjectEnumerator();
			while(t_7->p_HasNext()){
				c_GlobalDecl* t_decl6=t_7->p_NextObject();
				p_Emit(p_TransGlobal(t_decl6)+String(L"=",1)+t_decl6->m_init->p_Trans()+String(L";",1));
			}
			p_Emit(String(L"return 0;",9));
			p_Emit(String(L"}",1));
			p_EndLocalScope();
		}
		p_Emit(String(L"}",1));
	}
	return p_JoinLines();
}
String c_JavaTranslator::p_TransValue(c_Type* t_ty,String t_value){
	if((t_value).Length()!=0){
		if(((dynamic_cast<c_IntType*>(t_ty))!=0) && t_value.StartsWith(String(L"$",1))){
			return String(L"0x",2)+t_value.Slice(1);
		}
		if((dynamic_cast<c_BoolType*>(t_ty))!=0){
			return String(L"true",4);
		}
		if((dynamic_cast<c_IntType*>(t_ty))!=0){
			return t_value;
		}
		if((dynamic_cast<c_FloatType*>(t_ty))!=0){
			return t_value+String(L"f",1);
		}
		if((dynamic_cast<c_StringType*>(t_ty))!=0){
			return p_Enquote(t_value);
		}
	}else{
		if((dynamic_cast<c_BoolType*>(t_ty))!=0){
			return String(L"false",5);
		}
		if((dynamic_cast<c_NumericType*>(t_ty))!=0){
			return String(L"0",1);
		}
		if((dynamic_cast<c_StringType*>(t_ty))!=0){
			return String(L"\"\"",2);
		}
		if((dynamic_cast<c_ArrayType*>(t_ty))!=0){
			c_Type* t_elemTy=dynamic_cast<c_ArrayType*>(t_ty)->m_elemType;
			if((dynamic_cast<c_BoolType*>(t_elemTy))!=0){
				return String(L"bb_std_lang.emptyBoolArray",26);
			}
			if((dynamic_cast<c_IntType*>(t_elemTy))!=0){
				return String(L"bb_std_lang.emptyIntArray",25);
			}
			if((dynamic_cast<c_FloatType*>(t_elemTy))!=0){
				return String(L"bb_std_lang.emptyFloatArray",27);
			}
			if((dynamic_cast<c_StringType*>(t_elemTy))!=0){
				return String(L"bb_std_lang.emptyStringArray",28);
			}
			String t_t=String(L"[0]",3);
			while((dynamic_cast<c_ArrayType*>(t_elemTy))!=0){
				t_elemTy=dynamic_cast<c_ArrayType*>(t_elemTy)->m_elemType;
				t_t=t_t+String(L"[]",2);
			}
			return String(L"new ",4)+p_TransType(t_elemTy)+t_t;
		}
		if((dynamic_cast<c_ObjectType*>(t_ty))!=0){
			return String(L"null",4);
		}
	}
	bb_config_InternalErr(String(L"Internal error",14));
	return String();
}
String c_JavaTranslator::p_TransLocalDecl(String t_munged,c_Expr* t_init){
	return p_TransType(t_init->m_exprType)+String(L" ",1)+t_munged+String(L"=",1)+t_init->p_Trans();
}
int c_JavaTranslator::p_EmitEnter(c_FuncDecl* t_func){
	if((m_unsafe)!=0){
		return 0;
	}
	p_Emit(String(L"bb_std_lang.pushErr();",22));
	return 0;
}
int c_JavaTranslator::p_EmitSetErr(String t_info){
	if((m_unsafe)!=0){
		return 0;
	}
	p_Emit(String(L"bb_std_lang.errInfo=\"",21)+t_info.Replace(String(L"\\",1),String(L"/",1))+String(L"\";",2));
	return 0;
}
int c_JavaTranslator::p_EmitLeave(){
	if((m_unsafe)!=0){
		return 0;
	}
	p_Emit(String(L"bb_std_lang.popErr();",21));
	return 0;
}
String c_JavaTranslator::p_TransField(c_FieldDecl* t_decl,c_Expr* t_lhs){
	if((t_lhs)!=0){
		return p_TransSubExpr(t_lhs,2)+String(L".",1)+t_decl->m_munged;
	}
	return t_decl->m_munged;
}
String c_JavaTranslator::p_TransArgs(Array<c_Expr* > t_args){
	String t_t=String();
	Array<c_Expr* > t_=t_args;
	int t_2=0;
	while(t_2<t_.Length()){
		c_Expr* t_arg=t_[t_2];
		t_2=t_2+1;
		if((t_t).Length()!=0){
			t_t=t_t+String(L",",1);
		}
		t_t=t_t+t_arg->p_Trans();
	}
	return p_Bra(t_t);
}
String c_JavaTranslator::p_TransFunc(c_FuncDecl* t_decl,Array<c_Expr* > t_args,c_Expr* t_lhs){
	if(t_decl->p_IsMethod()){
		if((t_lhs)!=0){
			return p_TransSubExpr(t_lhs,2)+String(L".",1)+t_decl->m_munged+p_TransArgs(t_args);
		}
		return t_decl->m_munged+p_TransArgs(t_args);
	}
	return p_TransStatic(t_decl)+p_TransArgs(t_args);
}
String c_JavaTranslator::p_TransSuperFunc(c_FuncDecl* t_decl,Array<c_Expr* > t_args){
	return String(L"super.",6)+t_decl->m_munged+p_TransArgs(t_args);
}
String c_JavaTranslator::p_TransConstExpr(c_ConstExpr* t_expr){
	return p_TransValue(t_expr->m_exprType,t_expr->m_value);
}
String c_JavaTranslator::p_TransNewObjectExpr(c_NewObjectExpr* t_expr){
	String t_t=String(L"(new ",5)+t_expr->m_classDecl->m_munged+String(L"())",3);
	if((t_expr->m_ctor)!=0){
		t_t=t_t+(String(L".",1)+t_expr->m_ctor->m_munged+p_TransArgs(t_expr->m_args));
	}
	return t_t;
}
String c_JavaTranslator::p_TransNewArrayExpr(c_NewArrayExpr* t_expr){
	String t_texpr=t_expr->m_expr->p_Trans();
	c_Type* t_elemTy=dynamic_cast<c_ArrayType*>(t_expr->m_exprType)->m_elemType;
	if((dynamic_cast<c_StringType*>(t_elemTy))!=0){
		return String(L"bb_std_lang.stringArray",23)+p_Bra(t_texpr);
	}
	String t_t=String(L"[",1)+t_texpr+String(L"]",1);
	while((dynamic_cast<c_ArrayType*>(t_elemTy))!=0){
		t_elemTy=dynamic_cast<c_ArrayType*>(t_elemTy)->m_elemType;
		t_t=t_t+String(L"[]",2);
	}
	return String(L"new ",4)+p_TransType(t_elemTy)+t_t;
}
String c_JavaTranslator::p_TransSelfExpr(c_SelfExpr* t_expr){
	return String(L"this",4);
}
String c_JavaTranslator::p_TransCastExpr(c_CastExpr* t_expr){
	String t_texpr=p_Bra(t_expr->m_expr->p_Trans());
	c_Type* t_dst=t_expr->m_exprType;
	c_Type* t_src=t_expr->m_expr->m_exprType;
	if((dynamic_cast<c_BoolType*>(t_dst))!=0){
		if((dynamic_cast<c_BoolType*>(t_src))!=0){
			return t_texpr;
		}
		if((dynamic_cast<c_IntType*>(t_src))!=0){
			return p_Bra(t_texpr+String(L"!=0",3));
		}
		if((dynamic_cast<c_FloatType*>(t_src))!=0){
			return p_Bra(t_texpr+String(L"!=0.0f",6));
		}
		if((dynamic_cast<c_StringType*>(t_src))!=0){
			return p_Bra(t_texpr+String(L".length()!=0",12));
		}
		if((dynamic_cast<c_ArrayType*>(t_src))!=0){
			return p_Bra(t_texpr+String(L".length!=0",10));
		}
		if((dynamic_cast<c_ObjectType*>(t_src))!=0){
			return p_Bra(t_texpr+String(L"!=null",6));
		}
	}else{
		if((dynamic_cast<c_IntType*>(t_dst))!=0){
			if((dynamic_cast<c_BoolType*>(t_src))!=0){
				return p_Bra(t_texpr+String(L"?1:0",4));
			}
			if((dynamic_cast<c_IntType*>(t_src))!=0){
				return t_texpr;
			}
			if((dynamic_cast<c_FloatType*>(t_src))!=0){
				return String(L"(int)",5)+t_texpr;
			}
			if(m_langutil){
				if((dynamic_cast<c_StringType*>(t_src))!=0){
					return String(L"LangUtil.parseInt(",18)+t_texpr+String(L".trim())",8);
				}
			}else{
				if((dynamic_cast<c_StringType*>(t_src))!=0){
					return String(L"Integer.parseInt(",17)+t_texpr+String(L".trim())",8);
				}
			}
		}else{
			if((dynamic_cast<c_FloatType*>(t_dst))!=0){
				if((dynamic_cast<c_IntType*>(t_src))!=0){
					return String(L"(float)",7)+t_texpr;
				}
				if((dynamic_cast<c_FloatType*>(t_src))!=0){
					return t_texpr;
				}
				if(m_langutil){
					if((dynamic_cast<c_StringType*>(t_src))!=0){
						return String(L"LangUtil.parseFloat(",20)+t_texpr+String(L".trim())",8);
					}
				}else{
					if((dynamic_cast<c_StringType*>(t_src))!=0){
						return String(L"Float.parseFloat(",17)+t_texpr+String(L".trim())",8);
					}
				}
			}else{
				if((dynamic_cast<c_StringType*>(t_dst))!=0){
					if((dynamic_cast<c_IntType*>(t_src))!=0){
						return String(L"String.valueOf",14)+t_texpr;
					}
					if((dynamic_cast<c_FloatType*>(t_src))!=0){
						return String(L"String.valueOf",14)+t_texpr;
					}
					if((dynamic_cast<c_StringType*>(t_src))!=0){
						return t_texpr;
					}
				}else{
					if(((dynamic_cast<c_ObjectType*>(t_dst))!=0) && ((dynamic_cast<c_ObjectType*>(t_src))!=0)){
						if((t_src->p_GetClass()->p_ExtendsClass(t_dst->p_GetClass()))!=0){
							return t_texpr;
						}else{
							return String(L"bb_std_lang.as(",15)+p_TransType(t_dst)+String(L".class,",7)+t_texpr+String(L")",1);
						}
					}
				}
			}
		}
	}
	bb_config_Err(String(L"Java translator can't convert ",30)+t_src->p_ToString()+String(L" to ",4)+t_dst->p_ToString());
	return String();
}
String c_JavaTranslator::p_TransUnaryExpr(c_UnaryExpr* t_expr){
	String t_texpr=t_expr->m_expr->p_Trans();
	if(p_ExprPri(t_expr->m_expr)>p_ExprPri(t_expr)){
		t_texpr=p_Bra(t_texpr);
	}
	return p_TransUnaryOp(t_expr->m_op)+t_texpr;
}
String c_JavaTranslator::p_TransBinaryExpr(c_BinaryExpr* t_expr){
	String t_lhs=t_expr->m_lhs->p_Trans();
	String t_rhs=t_expr->m_rhs->p_Trans();
	if(((dynamic_cast<c_BinaryCompareExpr*>(t_expr))!=0) && ((dynamic_cast<c_StringType*>(t_expr->m_lhs->m_exprType))!=0) && ((dynamic_cast<c_StringType*>(t_expr->m_rhs->m_exprType))!=0)){
		if(p_ExprPri(t_expr->m_lhs)>2){
			t_lhs=p_Bra(t_lhs);
		}
		return p_Bra(t_lhs+String(L".compareTo",10)+p_Bra(t_rhs)+p_TransBinaryOp(t_expr->m_op,String())+String(L"0",1));
	}
	int t_pri=p_ExprPri(t_expr);
	if(p_ExprPri(t_expr->m_lhs)>t_pri){
		t_lhs=p_Bra(t_lhs);
	}
	if(p_ExprPri(t_expr->m_rhs)>=t_pri){
		t_rhs=p_Bra(t_rhs);
	}
	return t_lhs+p_TransBinaryOp(t_expr->m_op,t_rhs)+t_rhs;
}
String c_JavaTranslator::p_TransIndexExpr(c_IndexExpr* t_expr){
	String t_texpr=t_expr->m_expr->p_Trans();
	String t_index=t_expr->m_index->p_Trans();
	if((dynamic_cast<c_StringType*>(t_expr->m_expr->m_exprType))!=0){
		return String(L"(int)",5)+t_texpr+String(L".charAt(",8)+t_index+String(L")",1);
	}
	return t_texpr+String(L"[",1)+t_index+String(L"]",1);
}
String c_JavaTranslator::p_TransSliceExpr(c_SliceExpr* t_expr){
	String t_texpr=t_expr->m_expr->p_Trans();
	String t_from=String(L",0",2);
	String t_term=String();
	if((t_expr->m_from)!=0){
		t_from=String(L",",1)+t_expr->m_from->p_Trans();
	}
	if((t_expr->m_term)!=0){
		t_term=String(L",",1)+t_expr->m_term->p_Trans();
	}
	if((dynamic_cast<c_ArrayType*>(t_expr->m_exprType))!=0){
		return String(L"((",2)+p_TransType(t_expr->m_exprType)+String(L")bb_std_lang.sliceArray",23)+p_Bra(t_texpr+t_from+t_term)+String(L")",1);
	}else{
		if((dynamic_cast<c_StringType*>(t_expr->m_exprType))!=0){
			return String(L"bb_std_lang.slice(",18)+t_texpr+t_from+t_term+String(L")",1);
		}
	}
	bb_config_InternalErr(String(L"Internal error",14));
	return String();
}
String c_JavaTranslator::p_TransArrayExpr(c_ArrayExpr* t_expr){
	String t_t=String();
	Array<c_Expr* > t_=t_expr->m_exprs;
	int t_2=0;
	while(t_2<t_.Length()){
		c_Expr* t_elem=t_[t_2];
		t_2=t_2+1;
		if((t_t).Length()!=0){
			t_t=t_t+String(L",",1);
		}
		t_t=t_t+t_elem->p_Trans();
	}
	return String(L"new ",4)+p_TransType(t_expr->m_exprType)+String(L"{",1)+t_t+String(L"}",1);
}
String c_JavaTranslator::p_TransIntrinsicExpr(c_Decl* t_decl,c_Expr* t_expr,Array<c_Expr* > t_args){
	String t_texpr=String();
	String t_arg0=String();
	String t_arg1=String();
	String t_arg2=String();
	if((t_expr)!=0){
		t_texpr=p_TransSubExpr(t_expr,2);
	}
	if(t_args.Length()>0 && ((t_args[0])!=0)){
		t_arg0=t_args[0]->p_Trans();
	}
	if(t_args.Length()>1 && ((t_args[1])!=0)){
		t_arg1=t_args[1]->p_Trans();
	}
	if(t_args.Length()>2 && ((t_args[2])!=0)){
		t_arg2=t_args[2]->p_Trans();
	}
	String t_id=t_decl->m_munged.Slice(1);
	String t_fmath=String(L"(float)Math.",12);
	String t_1=t_id;
	if(t_1==String(L"print",5)){
		return String(L"bb_std_lang.print",17)+p_Bra(t_arg0);
	}else{
		if(t_1==String(L"error",5)){
			return String(L"bb_std_lang.error",17)+p_Bra(t_arg0);
		}else{
			if(t_1==String(L"debuglog",8)){
				return String(L"bb_std_lang.debugLog",20)+p_Bra(t_arg0);
			}else{
				if(t_1==String(L"debugstop",9)){
					return String(L"bb_std_lang.debugStop()",23);
				}else{
					if(t_1==String(L"length",6)){
						if((dynamic_cast<c_StringType*>(t_expr->m_exprType))!=0){
							return t_texpr+String(L".length()",9);
						}
						return String(L"bb_std_lang.length",18)+p_Bra(t_texpr);
					}else{
						if(t_1==String(L"resize",6)){
							c_Type* t_ty=dynamic_cast<c_ArrayType*>(t_expr->m_exprType)->m_elemType;
							if((dynamic_cast<c_StringType*>(t_ty))!=0){
								return String(L"bb_std_lang.resize(",19)+t_texpr+String(L",",1)+t_arg0+String(L")",1);
							}
							String t_ety=p_TransType(t_ty);
							return String(L"(",1)+t_ety+String(L"[])bb_std_lang.resize(",22)+t_texpr+String(L",",1)+t_arg0+String(L",",1)+t_ety+String(L".class)",7);
						}else{
							if(t_1==String(L"compare",7)){
								return t_texpr+String(L".compareTo",10)+p_Bra(t_arg0);
							}else{
								if(t_1==String(L"find",4)){
									return t_texpr+String(L".indexOf",8)+p_Bra(t_arg0+String(L",",1)+t_arg1);
								}else{
									if(t_1==String(L"findlast",8)){
										return t_texpr+String(L".lastIndexOf",12)+p_Bra(t_arg0);
									}else{
										if(t_1==String(L"findlast2",9)){
											return t_texpr+String(L".lastIndexOf",12)+p_Bra(t_arg0+String(L",",1)+t_arg1);
										}else{
											if(t_1==String(L"trim",4)){
												return t_texpr+String(L".trim()",7);
											}else{
												if(t_1==String(L"join",4)){
													return String(L"bb_std_lang.join",16)+p_Bra(t_texpr+String(L",",1)+t_arg0);
												}else{
													if(t_1==String(L"split",5)){
														return String(L"bb_std_lang.split",17)+p_Bra(t_texpr+String(L",",1)+t_arg0);
													}else{
														if(t_1==String(L"replace",7)){
															return String(L"bb_std_lang.replace",19)+p_Bra(t_texpr+String(L",",1)+t_arg0+String(L",",1)+t_arg1);
														}else{
															if(t_1==String(L"tolower",7)){
																return t_texpr+String(L".toLowerCase()",14);
															}else{
																if(t_1==String(L"toupper",7)){
																	return t_texpr+String(L".toUpperCase()",14);
																}else{
																	if(t_1==String(L"contains",8)){
																		return p_Bra(t_texpr+String(L".indexOf",8)+p_Bra(t_arg0)+String(L"!=-1",4));
																	}else{
																		if(t_1==String(L"startswith",10)){
																			return t_texpr+String(L".startsWith",11)+p_Bra(t_arg0);
																		}else{
																			if(t_1==String(L"endswith",8)){
																				return t_texpr+String(L".endsWith",9)+p_Bra(t_arg0);
																			}else{
																				if(t_1==String(L"tochars",7)){
																					return String(L"bb_std_lang.toChars",19)+p_Bra(t_texpr);
																				}else{
																					if(t_1==String(L"fromchar",8)){
																						return String(L"String.valueOf",14)+p_Bra(String(L"(char)",6)+p_Bra(t_arg0));
																					}else{
																						if(t_1==String(L"fromchars",9)){
																							return String(L"bb_std_lang.fromChars",21)+p_Bra(t_arg0);
																						}else{
																							if(t_1==String(L"sin",3) || t_1==String(L"cos",3)){
																								return t_fmath+t_id+p_Bra(p_Bra(t_arg0)+String(L"*bb_std_lang.D2R",16));
																							}else{
																								if(t_1==String(L"tan",3)){
																									return String(L"(float)Math.",12)+t_id+p_Bra(p_Bra(t_arg0)+String(L"*bb_std_lang.D2R",16));
																								}else{
																									if(t_1==String(L"asin",4) || t_1==String(L"acos",4) || t_1==String(L"atan",4)){
																										return String(L"(float)",7)+p_Bra(String(L"Math.",5)+t_id+p_Bra(t_arg0)+String(L"*bb_std_lang.R2D",16));
																									}else{
																										if(t_1==String(L"atan2",5)){
																											return String(L"(float)",7)+p_Bra(String(L"Math.",5)+t_id+p_Bra(t_arg0+String(L",",1)+t_arg1)+String(L"*bb_std_lang.R2D",16));
																										}else{
																											if(t_1==String(L"sinr",4) || t_1==String(L"cosr",4)){
																												return t_fmath+t_id.Slice(0,-1)+p_Bra(t_arg0);
																											}else{
																												if(t_1==String(L"tanr",4)){
																													return String(L"(float)Math.",12)+t_id.Slice(0,-1)+p_Bra(t_arg0);
																												}else{
																													if(t_1==String(L"asinr",5) || t_1==String(L"acosr",5) || t_1==String(L"atanr",5)){
																														return String(L"(float)Math.",12)+t_id.Slice(0,-1)+p_Bra(t_arg0);
																													}else{
																														if(t_1==String(L"atan2r",6)){
																															return String(L"(float)Math.",12)+t_id.Slice(0,-1)+p_Bra(t_arg0+String(L",",1)+t_arg1);
																														}else{
																															if(t_1==String(L"sqrt",4) || t_1==String(L"floor",5) || t_1==String(L"ceil",4)){
																																return t_fmath+t_id+p_Bra(t_arg0);
																															}else{
																																if(t_1==String(L"log",3) || t_1==String(L"exp",3)){
																																	return String(L"(float)Math.",12)+t_id+p_Bra(t_arg0);
																																}else{
																																	if(t_1==String(L"pow",3)){
																																		return String(L"(float)Math.",12)+t_id+p_Bra(t_arg0+String(L",",1)+t_arg1);
																																	}
																																}
																															}
																														}
																													}
																												}
																											}
																										}
																									}
																								}
																							}
																						}
																					}
																				}
																			}
																		}
																	}
																}
															}
														}
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	bb_config_InternalErr(String(L"Internal error",14));
	return String();
}
String c_JavaTranslator::p_TransTryStmt(c_TryStmt* t_stmt){
	p_Emit(String(L"try{",4));
	int t_unr=p_EmitBlock(t_stmt->m_block,true);
	Array<c_CatchStmt* > t_=t_stmt->m_catches;
	int t_2=0;
	while(t_2<t_.Length()){
		c_CatchStmt* t_c=t_[t_2];
		t_2=t_2+1;
		p_MungDecl(t_c->m_init);
		p_Emit(String(L"}catch(",7)+p_TransType(t_c->m_init->m_type)+String(L" ",1)+t_c->m_init->m_munged+String(L"){",2));
		int t_unr2=p_EmitBlock(t_c->m_block,true);
	}
	p_Emit(String(L"}",1));
	return String();
}
void c_JavaTranslator::mark(){
	c_CTranslator::mark();
}
bool bb_transcc_MatchPathAlt(String t_text,String t_alt){
	if(!t_alt.Contains(String(L"*",1))){
		return t_alt==t_text;
	}
	Array<String > t_bits=t_alt.Split(String(L"*",1));
	if(!t_text.StartsWith(t_bits[0])){
		return false;
	}
	int t_n=t_bits.Length()-1;
	int t_i=t_bits[0].Length();
	for(int t_j=1;t_j<t_n;t_j=t_j+1){
		String t_bit=t_bits[t_j];
		t_i=t_text.Find(t_bit,t_i);
		if(t_i==-1){
			return false;
		}
		t_i+=t_bit.Length();
	}
	return t_text.Slice(t_i).EndsWith(t_bits[t_n]);
}
bool bb_transcc_MatchPath(String t_text,String t_pattern){
	t_text=String(L"/",1)+t_text;
	Array<String > t_alts=t_pattern.Split(String(L"|",1));
	bool t_match=false;
	Array<String > t_=t_alts;
	int t_2=0;
	while(t_2<t_.Length()){
		String t_alt=t_[t_2];
		t_2=t_2+1;
		if(!((t_alt).Length()!=0)){
			continue;
		}
		if(t_alt.StartsWith(String(L"!",1))){
			if(bb_transcc_MatchPathAlt(t_text,t_alt.Slice(1))){
				return false;
			}
		}else{
			if(bb_transcc_MatchPathAlt(t_text,t_alt)){
				t_match=true;
			}
		}
	}
	return t_match;
}
String bb_transcc_ReplaceBlock(String t_text,String t_tag,String t_repText,String t_mark){
	String t_beginTag=t_mark+String(L"${",2)+t_tag+String(L"_BEGIN}",7);
	int t_i=t_text.Find(t_beginTag,0);
	if(t_i==-1){
		bb_transcc_Die(String(L"Error updating target project - can't find block begin tag '",60)+t_tag+String(L"'. You may need to delete target .build directory.",50));
	}
	t_i+=t_beginTag.Length();
	while(t_i<t_text.Length() && (int)t_text[t_i-1]!=10){
		t_i+=1;
	}
	String t_endTag=t_mark+String(L"${",2)+t_tag+String(L"_END}",5);
	int t_i2=t_text.Find(t_endTag,t_i-1);
	if(t_i2==-1){
		bb_transcc_Die(String(L"Error updating target project - can't find block end tag '",58)+t_tag+String(L"'.",2));
	}
	if(!((t_repText).Length()!=0) || (int)t_repText[t_repText.Length()-1]==10){
		t_i2+=1;
	}
	return t_text.Slice(0,t_i)+t_repText+t_text.Slice(t_i2);
}
c_NodeEnumerator3::c_NodeEnumerator3(){
	m_node=0;
}
c_NodeEnumerator3* c_NodeEnumerator3::m_new(c_Node2* t_node){
	gc_assign(this->m_node,t_node);
	return this;
}
c_NodeEnumerator3* c_NodeEnumerator3::m_new2(){
	return this;
}
bool c_NodeEnumerator3::p_HasNext(){
	return m_node!=0;
}
c_Node2* c_NodeEnumerator3::p_NextObject(){
	c_Node2* t_t=m_node;
	gc_assign(m_node,m_node->p_NextNode());
	return t_t;
}
void c_NodeEnumerator3::mark(){
	Object::mark();
	gc_mark_q(m_node);
}
String bb_config_Enquote(String t_str,String t_lang){
	String t_1=t_lang;
	if(t_1==String(L"cpp",3) || t_1==String(L"java",4) || t_1==String(L"as",2) || t_1==String(L"js",2) || t_1==String(L"cs",2)){
		t_str=t_str.Replace(String(L"\\",1),String(L"\\\\",2));
		t_str=t_str.Replace(String(L"\"",1),String(L"\\\"",2));
		t_str=t_str.Replace(String(L"\n",1),String(L"\\n",2));
		t_str=t_str.Replace(String(L"\r",1),String(L"\\r",2));
		t_str=t_str.Replace(String(L"\t",1),String(L"\\t",2));
		for(int t_i=0;t_i<t_str.Length();t_i=t_i+1){
			if((int)t_str[t_i]>=32 && (int)t_str[t_i]<128){
				continue;
			}
			String t_t=String();
			int t_n=(int)t_str[t_i];
			while((t_n)!=0){
				int t_c=(t_n&15)+48;
				if(t_c>=58){
					t_c+=39;
				}
				t_t=String((Char)(t_c),1)+t_t;
				t_n=t_n>>4&268435455;
			}
			if(!((t_t).Length()!=0)){
				t_t=String(L"0",1);
			}
			String t_2=t_lang;
			if(t_2==String(L"cpp",3)){
				t_t=String(L"\" L\"\\x",6)+t_t+String(L"\" L\"",4);
			}else{
				t_t=String(L"\\u",2)+(String(L"0000",4)+t_t).Slice(-4);
			}
			t_str=t_str.Slice(0,t_i)+t_t+t_str.Slice(t_i+1);
			t_i+=t_t.Length()-1;
		}
		String t_3=t_lang;
		if(t_3==String(L"cpp",3)){
			t_str=String(L"L\"",2)+t_str+String(L"\"",1);
		}else{
			t_str=String(L"\"",1)+t_str+String(L"\"",1);
		}
		return t_str;
	}
	bb_config_InternalErr(String(L"Internal error",14));
	return String();
}
c_CppTranslator::c_CppTranslator(){
	m_unsafe=false;
	m_gc_mode=0;
	m_dbgLocals=(new c_Stack9)->m_new();
	m_lastDbgInfo=String();
	m_pure=0;
}
c_CppTranslator* c_CppTranslator::m_new(){
	c_CTranslator::m_new();
	return this;
}
String c_CppTranslator::p_TransType(c_Type* t_ty){
	if((dynamic_cast<c_VoidType*>(t_ty))!=0){
		return String(L"void",4);
	}
	if((dynamic_cast<c_BoolType*>(t_ty))!=0){
		return String(L"bool",4);
	}
	if((dynamic_cast<c_IntType*>(t_ty))!=0){
		return String(L"int",3);
	}
	if((dynamic_cast<c_FloatType*>(t_ty))!=0){
		return String(L"Float",5);
	}
	if((dynamic_cast<c_StringType*>(t_ty))!=0){
		return String(L"String",6);
	}
	if((dynamic_cast<c_ArrayType*>(t_ty))!=0){
		return String(L"Array<",6)+p_TransRefType(dynamic_cast<c_ArrayType*>(t_ty)->m_elemType)+String(L" >",2);
	}
	if((dynamic_cast<c_ObjectType*>(t_ty))!=0){
		return t_ty->p_GetClass()->m_munged+String(L"*",1);
	}
	bb_config_InternalErr(String(L"Internal error",14));
	return String();
}
String c_CppTranslator::p_TransRefType(c_Type* t_ty){
	return p_TransType(t_ty);
}
String c_CppTranslator::p_TransValue(c_Type* t_ty,String t_value){
	if((t_value).Length()!=0){
		if((dynamic_cast<c_BoolType*>(t_ty))!=0){
			return String(L"true",4);
		}
		if((dynamic_cast<c_IntType*>(t_ty))!=0){
			return t_value;
		}
		if((dynamic_cast<c_FloatType*>(t_ty))!=0){
			return String(L"FLOAT(",6)+t_value+String(L")",1);
		}
		if((dynamic_cast<c_StringType*>(t_ty))!=0){
			return String(L"String(",7)+p_Enquote(t_value)+String(L",",1)+String(t_value.Length())+String(L")",1);
		}
	}else{
		if((dynamic_cast<c_BoolType*>(t_ty))!=0){
			return String(L"false",5);
		}
		if((dynamic_cast<c_NumericType*>(t_ty))!=0){
			return String(L"0",1);
		}
		if((dynamic_cast<c_StringType*>(t_ty))!=0){
			return String(L"String()",8);
		}
		if((dynamic_cast<c_ArrayType*>(t_ty))!=0){
			return String(L"Array<",6)+p_TransRefType(dynamic_cast<c_ArrayType*>(t_ty)->m_elemType)+String(L" >()",4);
		}
		if((dynamic_cast<c_ObjectType*>(t_ty))!=0){
			return String(L"0",1);
		}
	}
	bb_config_InternalErr(String(L"Internal error",14));
	return String();
}
c_Expr* c_CppTranslator::p_Uncast(c_Expr* t_expr){
	do{
		c_CastExpr* t_cexpr=dynamic_cast<c_CastExpr*>(t_expr);
		if(!((t_cexpr)!=0)){
			return t_expr;
		}
		t_expr=t_cexpr->m_expr;
	}while(!(false));
}
bool c_CppTranslator::p_IsGcObject(c_Expr* t_expr){
	t_expr=p_Uncast(t_expr);
	if((dynamic_cast<c_ConstExpr*>(t_expr))!=0){
		return false;
	}
	if(!((dynamic_cast<c_ObjectType*>(t_expr->m_exprType))!=0) && !((dynamic_cast<c_ArrayType*>(t_expr->m_exprType))!=0)){
		return false;
	}
	if(((dynamic_cast<c_ObjectType*>(t_expr->m_exprType))!=0) && !((t_expr->m_exprType->p_GetClass()->p_ExtendsObject())!=0)){
		return false;
	}
	return true;
}
String c_CppTranslator::p_GcRetain(c_Expr* t_expr,String t_texpr){
	if(!((t_texpr).Length()!=0)){
		t_texpr=t_expr->p_Trans();
	}
	if(m_unsafe || m_gc_mode!=2 || !p_IsGcObject(t_expr)){
		return t_texpr;
	}
	t_expr=p_Uncast(t_expr);
	if(((dynamic_cast<c_NewObjectExpr*>(t_expr))!=0) || ((dynamic_cast<c_NewArrayExpr*>(t_expr))!=0) || ((dynamic_cast<c_ArrayExpr*>(t_expr))!=0)){
		return t_texpr;
	}
	if(((dynamic_cast<c_VarExpr*>(t_expr))!=0) && ((dynamic_cast<c_LocalDecl*>(dynamic_cast<c_VarExpr*>(t_expr)->m_decl))!=0)){
		return t_texpr;
	}
	return String(L"gc_retain(",10)+t_texpr+String(L")",1);
}
String c_CppTranslator::p_TransLocalDecl(String t_munged,c_Expr* t_init){
	String t_tinit=p_GcRetain(t_init,String());
	return p_TransType(t_init->m_exprType)+String(L" ",1)+t_munged+String(L"=",1)+t_tinit;
}
int c_CppTranslator::p_BeginLocalScope(){
	c_CTranslator::p_BeginLocalScope();
	return 0;
}
int c_CppTranslator::p_EndLocalScope(){
	c_CTranslator::p_EndLocalScope();
	m_dbgLocals->p_Clear();
	m_lastDbgInfo=String();
	return 0;
}
int c_CppTranslator::p_EmitEnter(c_FuncDecl* t_func){
	if(m_unsafe){
		return 0;
	}
	String t_id=t_func->m_ident;
	if((dynamic_cast<c_ClassDecl*>(t_func->m_scope))!=0){
		t_id=t_func->m_scope->m_ident+String(L".",1)+t_id;
	}
	p_Emit(String(L"DBG_ENTER(\"",11)+t_id+String(L"\")",2));
	if(t_func->p_IsCtor() || t_func->p_IsMethod()){
		p_Emit(t_func->m_scope->m_munged+String(L" *self=this;",12));
		p_Emit(String(L"DBG_LOCAL(self,\"Self\")",22));
	}
	return 0;
}
int c_CppTranslator::p_EmitEnterBlock(){
	if(m_unsafe){
		return 0;
	}
	p_Emit(String(L"DBG_BLOCK();",12));
	return 0;
}
bool c_CppTranslator::p_IsDebuggable(c_Type* t_type){
	return true;
}
int c_CppTranslator::p_EmitSetErr(String t_info){
	if(m_unsafe){
		return 0;
	}
	if(t_info==m_lastDbgInfo){
		return 0;
	}
	m_lastDbgInfo=t_info;
	c_Enumerator7* t_=m_dbgLocals->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_LocalDecl* t_decl=t_->p_NextObject();
		if(((t_decl->m_ident).Length()!=0) && p_IsDebuggable(t_decl->m_type)){
			p_Emit(String(L"DBG_LOCAL(",10)+t_decl->m_munged+String(L",\"",2)+t_decl->m_ident+String(L"\")",2));
		}
	}
	m_dbgLocals->p_Clear();
	p_Emit(String(L"DBG_INFO(\"",10)+t_info.Replace(String(L"\\",1),String(L"/",1))+String(L"\");",3));
	return 0;
}
int c_CppTranslator::p_EmitLeaveBlock(){
	m_dbgLocals->p_Clear();
	return 0;
}
String c_CppTranslator::p_TransStatic(c_Decl* t_decl){
	if(((t_decl->p_IsExtern())!=0) && ((dynamic_cast<c_ModuleDecl*>(t_decl->m_scope))!=0)){
		return t_decl->m_munged;
	}else{
		if(((bb_decl__env)!=0) && ((t_decl->m_scope)!=0) && t_decl->m_scope==(bb_decl__env->p_ClassScope())){
			return t_decl->m_munged;
		}else{
			if((dynamic_cast<c_ClassDecl*>(t_decl->m_scope))!=0){
				return t_decl->m_scope->m_munged+String(L"::",2)+t_decl->m_munged;
			}else{
				if((dynamic_cast<c_ModuleDecl*>(t_decl->m_scope))!=0){
					return t_decl->m_munged;
				}
			}
		}
	}
	bb_config_InternalErr(String(L"Internal error",14));
	return String();
}
String c_CppTranslator::p_TransGlobal(c_GlobalDecl* t_decl){
	return p_TransStatic(t_decl);
}
int c_CppTranslator::p_EmitFuncProto(c_FuncDecl* t_decl){
	String t_args=String();
	Array<c_ArgDecl* > t_=t_decl->m_argDecls;
	int t_2=0;
	while(t_2<t_.Length()){
		c_ArgDecl* t_arg=t_[t_2];
		t_2=t_2+1;
		if((t_args).Length()!=0){
			t_args=t_args+String(L",",1);
		}
		t_args=t_args+p_TransType(t_arg->m_type);
	}
	String t_t=p_TransType(t_decl->m_retType)+String(L" ",1)+t_decl->m_munged+p_Bra(t_args);
	if((t_decl->p_IsAbstract())!=0){
		t_t=t_t+String(L"=0",2);
	}
	String t_q=String();
	if(t_decl->p_IsMethod() && t_decl->p_IsVirtual() && !((t_decl->m_overrides)!=0)){
		t_q=t_q+String(L"virtual ",8);
	}else{
		if(t_decl->p_IsStatic() && ((t_decl->p_ClassScope())!=0)){
			t_q=t_q+String(L"static ",7);
		}
	}
	p_Emit(t_q+t_t+String(L";",1));
	return 0;
}
int c_CppTranslator::p_EmitClassProto(c_ClassDecl* t_classDecl){
	String t_classid=t_classDecl->m_munged;
	String t_superid=t_classDecl->m_superClass->m_munged;
	if((t_classDecl->p_IsInterface())!=0){
		String t_bases=String();
		Array<c_ClassDecl* > t_=t_classDecl->m_implments;
		int t_2=0;
		while(t_2<t_.Length()){
			c_ClassDecl* t_iface=t_[t_2];
			t_2=t_2+1;
			if((t_bases).Length()!=0){
				t_bases=t_bases+String(L",",1);
			}else{
				t_bases=String(L" : ",3);
			}
			t_bases=t_bases+(String(L"public virtual ",15)+t_iface->m_munged);
		}
		if(!((t_bases).Length()!=0)){
			t_bases=String(L" : public virtual gc_interface",30);
		}
		p_Emit(String(L"class ",6)+t_classid+t_bases+String(L"{",1));
		p_Emit(String(L"public:",7));
		c_Enumerator2* t_3=t_classDecl->p_Semanted()->p_ObjectEnumerator();
		while(t_3->p_HasNext()){
			c_Decl* t_decl=t_3->p_NextObject();
			c_FuncDecl* t_fdecl=dynamic_cast<c_FuncDecl*>(t_decl);
			if(!((t_fdecl)!=0)){
				continue;
			}
			p_EmitFuncProto(t_fdecl);
		}
		p_Emit(String(L"};",2));
		return 0;
	}
	String t_bases2=String(L" : public ",10)+t_superid;
	Array<c_ClassDecl* > t_4=t_classDecl->m_implments;
	int t_5=0;
	while(t_5<t_4.Length()){
		c_ClassDecl* t_iface2=t_4[t_5];
		t_5=t_5+1;
		t_bases2=t_bases2+(String(L",public virtual ",16)+t_iface2->m_munged);
	}
	p_Emit(String(L"class ",6)+t_classid+t_bases2+String(L"{",1));
	p_Emit(String(L"public:",7));
	c_Enumerator2* t_6=t_classDecl->p_Semanted()->p_ObjectEnumerator();
	while(t_6->p_HasNext()){
		c_Decl* t_decl2=t_6->p_NextObject();
		c_FieldDecl* t_fdecl2=dynamic_cast<c_FieldDecl*>(t_decl2);
		if((t_fdecl2)!=0){
			p_Emit(p_TransRefType(t_fdecl2->m_type)+String(L" ",1)+t_fdecl2->m_munged+String(L";",1));
			continue;
		}
	}
	p_Emit(t_classid+String(L"();",3));
	c_Enumerator2* t_7=t_classDecl->p_Semanted()->p_ObjectEnumerator();
	while(t_7->p_HasNext()){
		c_Decl* t_decl3=t_7->p_NextObject();
		c_FuncDecl* t_fdecl3=dynamic_cast<c_FuncDecl*>(t_decl3);
		if((t_fdecl3)!=0){
			p_EmitFuncProto(t_fdecl3);
			continue;
		}
		c_GlobalDecl* t_gdecl=dynamic_cast<c_GlobalDecl*>(t_decl3);
		if((t_gdecl)!=0){
			p_Emit(String(L"static ",7)+p_TransRefType(t_gdecl->m_type)+String(L" ",1)+t_gdecl->m_munged+String(L";",1));
			continue;
		}
	}
	p_Emit(String(L"void mark();",12));
	if(bb_config_ENV_CONFIG==String(L"debug",5)){
		p_Emit(String(L"String debug();",15));
	}
	p_Emit(String(L"};",2));
	if(bb_config_ENV_CONFIG==String(L"debug",5)){
		p_Emit(String(L"String dbg_type(",16)+t_classid+String(L"**p){return \"",13)+t_classDecl->m_ident+String(L"\";}",3));
	}
	return 0;
}
int c_CppTranslator::p_BeginLoop(){
	if(m_gc_mode!=2){
		return 0;
	}
	p_Emit(String(L"GC_ENTER",8));
	return 0;
}
int c_CppTranslator::p_EndLoop(){
	if(m_gc_mode!=2){
		return 0;
	}
	return 0;
}
int c_CppTranslator::p_EmitFuncDecl(c_FuncDecl* t_decl){
	if((t_decl->p_IsAbstract())!=0){
		return 0;
	}
	m_unsafe=t_decl->m_ident.EndsWith(String(L"__UNSAFE__",10));
	p_BeginLocalScope();
	String t_args=String();
	Array<c_ArgDecl* > t_=t_decl->m_argDecls;
	int t_2=0;
	while(t_2<t_.Length()){
		c_ArgDecl* t_arg=t_[t_2];
		t_2=t_2+1;
		p_MungDecl(t_arg);
		if((t_args).Length()!=0){
			t_args=t_args+String(L",",1);
		}
		t_args=t_args+(p_TransType(t_arg->m_type)+String(L" ",1)+t_arg->m_munged);
		m_dbgLocals->p_Push25(t_arg);
	}
	String t_id=t_decl->m_munged;
	if((t_decl->p_ClassScope())!=0){
		t_id=t_decl->p_ClassScope()->m_munged+String(L"::",2)+t_id;
	}
	p_Emit(p_TransType(t_decl->m_retType)+String(L" ",1)+t_id+p_Bra(t_args)+String(L"{",1));
	p_BeginLoop();
	p_EmitBlock((t_decl),true);
	p_EndLoop();
	p_Emit(String(L"}",1));
	p_EndLocalScope();
	m_unsafe=false;
	return 0;
}
String c_CppTranslator::p_TransField(c_FieldDecl* t_decl,c_Expr* t_lhs){
	if((t_lhs)!=0){
		return p_TransSubExpr(t_lhs,2)+String(L"->",2)+t_decl->m_munged;
	}
	return t_decl->m_munged;
}
int c_CppTranslator::p_EmitMark(String t_id,c_Type* t_ty,bool t_queue){
	if(m_gc_mode==0){
		return 0;
	}
	if(!((dynamic_cast<c_ObjectType*>(t_ty))!=0) && !((dynamic_cast<c_ArrayType*>(t_ty))!=0)){
		return 0;
	}
	if(((dynamic_cast<c_ObjectType*>(t_ty))!=0) && !((t_ty->p_GetClass()->p_ExtendsObject())!=0)){
		return 0;
	}
	if(t_queue){
		p_Emit(String(L"gc_mark_q(",10)+t_id+String(L");",2));
	}else{
		p_Emit(String(L"gc_mark(",8)+t_id+String(L");",2));
	}
	return 0;
}
int c_CppTranslator::p_EmitClassDecl(c_ClassDecl* t_classDecl){
	if((t_classDecl->p_IsInterface())!=0){
		return 0;
	}
	String t_classid=t_classDecl->m_munged;
	String t_superid=t_classDecl->m_superClass->m_munged;
	p_BeginLocalScope();
	p_Emit(t_classid+String(L"::",2)+t_classid+String(L"(){",3));
	if(m_gc_mode==2){
		p_Emit(String(L"GC_CTOR",7));
	}
	c_Enumerator2* t_=t_classDecl->p_Semanted()->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_Decl* t_decl=t_->p_NextObject();
		c_FieldDecl* t_fdecl=dynamic_cast<c_FieldDecl*>(t_decl);
		if(!((t_fdecl)!=0)){
			continue;
		}
		p_Emit(p_TransField(t_fdecl,0)+String(L"=",1)+t_fdecl->m_init->p_Trans()+String(L";",1));
	}
	p_Emit(String(L"}",1));
	p_EndLocalScope();
	c_Enumerator2* t_2=t_classDecl->p_Semanted()->p_ObjectEnumerator();
	while(t_2->p_HasNext()){
		c_Decl* t_decl2=t_2->p_NextObject();
		c_FuncDecl* t_fdecl2=dynamic_cast<c_FuncDecl*>(t_decl2);
		if((t_fdecl2)!=0){
			p_EmitFuncDecl(t_fdecl2);
			continue;
		}
		c_GlobalDecl* t_gdecl=dynamic_cast<c_GlobalDecl*>(t_decl2);
		if((t_gdecl)!=0){
			p_Emit(p_TransRefType(t_gdecl->m_type)+String(L" ",1)+t_classid+String(L"::",2)+t_gdecl->m_munged+String(L";",1));
			continue;
		}
	}
	p_Emit(String(L"void ",5)+t_classid+String(L"::mark(){",9));
	if((t_classDecl->m_superClass)!=0){
		p_Emit(t_classDecl->m_superClass->m_munged+String(L"::mark();",9));
	}
	c_Enumerator2* t_3=t_classDecl->p_Semanted()->p_ObjectEnumerator();
	while(t_3->p_HasNext()){
		c_Decl* t_decl3=t_3->p_NextObject();
		c_FieldDecl* t_fdecl3=dynamic_cast<c_FieldDecl*>(t_decl3);
		if((t_fdecl3)!=0){
			p_EmitMark(p_TransField(t_fdecl3,0),t_fdecl3->m_type,true);
		}
	}
	p_Emit(String(L"}",1));
	if(bb_config_ENV_CONFIG==String(L"debug",5)){
		p_Emit(String(L"String ",7)+t_classid+String(L"::debug(){",10));
		p_Emit(String(L"String t=\"(",11)+t_classDecl->m_ident+String(L")\\n\";",5));
		if(((t_classDecl->m_superClass)!=0) && !((t_classDecl->m_superClass->p_IsExtern())!=0)){
			p_Emit(String(L"t=",2)+t_classDecl->m_superClass->m_munged+String(L"::debug()+t;",12));
		}
		c_Enumerator2* t_4=t_classDecl->p_Decls()->p_ObjectEnumerator();
		while(t_4->p_HasNext()){
			c_Decl* t_decl4=t_4->p_NextObject();
			if(!((t_decl4->p_IsSemanted())!=0)){
				continue;
			}
			c_VarDecl* t_vdecl=dynamic_cast<c_VarDecl*>(t_decl4);
			if(!((t_vdecl)!=0)){
				continue;
			}
			if(!p_IsDebuggable(t_vdecl->m_type)){
				continue;
			}
			if((dynamic_cast<c_FieldDecl*>(t_decl4))!=0){
				p_Emit(String(L"t+=dbg_decl(\"",13)+t_decl4->m_ident+String(L"\",&",3)+t_decl4->m_munged+String(L");",2));
			}else{
				if((dynamic_cast<c_GlobalDecl*>(t_decl4))!=0){
					p_Emit(String(L"t+=dbg_decl(\"",13)+t_decl4->m_ident+String(L"\",&",3)+t_classDecl->m_munged+String(L"::",2)+t_decl4->m_munged+String(L");",2));
				}
			}
		}
		p_Emit(String(L"return t;",9));
		p_Emit(String(L"}",1));
	}
	return 0;
}
String c_CppTranslator::p_TransApp(c_AppDecl* t_app){
	if(!((bb_config_GetConfigVar(String(L"CPP_GC_MODE",11))).Length()!=0)){
		bb_config_SetConfigVar(String(L"CPP_GC_MODE",11),String(L"1",1),(c_Type::m_boolType));
	}
	m_gc_mode=(bb_config_GetConfigVar(String(L"CPP_GC_MODE",11))).ToInt();
	t_app->m_mainFunc->m_munged=String(L"bbMain",6);
	c_ValueEnumerator* t_=t_app->m_imported->p_Values()->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_ModuleDecl* t_decl=t_->p_NextObject();
		p_MungDecl(t_decl);
	}
	c_Enumerator2* t_2=t_app->p_Semanted()->p_ObjectEnumerator();
	while(t_2->p_HasNext()){
		c_Decl* t_decl2=t_2->p_NextObject();
		p_MungDecl(t_decl2);
		c_ClassDecl* t_cdecl=dynamic_cast<c_ClassDecl*>(t_decl2);
		if(!((t_cdecl)!=0)){
			continue;
		}
		p_Emit(String(L"class ",6)+t_decl2->m_munged+String(L";",1));
		c_Enumerator2* t_3=t_cdecl->p_Semanted()->p_ObjectEnumerator();
		while(t_3->p_HasNext()){
			c_Decl* t_decl3=t_3->p_NextObject();
			p_MungDecl(t_decl3);
		}
	}
	c_Enumerator2* t_4=t_app->p_Semanted()->p_ObjectEnumerator();
	while(t_4->p_HasNext()){
		c_Decl* t_decl4=t_4->p_NextObject();
		c_GlobalDecl* t_gdecl=dynamic_cast<c_GlobalDecl*>(t_decl4);
		if((t_gdecl)!=0){
			p_Emit(String(L"extern ",7)+p_TransRefType(t_gdecl->m_type)+String(L" ",1)+t_gdecl->m_munged+String(L";",1));
			continue;
		}
		c_FuncDecl* t_fdecl=dynamic_cast<c_FuncDecl*>(t_decl4);
		if((t_fdecl)!=0){
			p_EmitFuncProto(t_fdecl);
			continue;
		}
		c_ClassDecl* t_cdecl2=dynamic_cast<c_ClassDecl*>(t_decl4);
		if((t_cdecl2)!=0){
			p_EmitClassProto(t_cdecl2);
			continue;
		}
	}
	c_Enumerator2* t_5=t_app->m_allSemantedDecls->p_ObjectEnumerator();
	while(t_5->p_HasNext()){
		c_Decl* t_decl5=t_5->p_NextObject();
		c_ClassDecl* t_cdecl3=dynamic_cast<c_ClassDecl*>(t_decl5);
		if(!((t_cdecl3)!=0) || ((t_cdecl3->p_ExtendsObject())!=0) || t_cdecl3->m_munged==String(L"String",6) || t_cdecl3->m_munged==String(L"Array",5)){
			continue;
		}
		p_Emit(String(L"void gc_mark( ",14)+t_cdecl3->m_munged+String(L" *p ){}",7));
		if(bb_config_ENV_CONFIG==String(L"debug",5)){
			p_Emit(String(L"String dbg_type( ",17)+t_cdecl3->m_munged+String(L" **p ){ return \"",16)+t_decl5->m_ident+String(L"\"; }",4));
			p_Emit(String(L"String dbg_value( ",18)+t_cdecl3->m_munged+String(L" **p ){ return dbg_ptr_value( *p ); }",37));
		}
	}
	c_Enumerator2* t_6=t_app->p_Semanted()->p_ObjectEnumerator();
	while(t_6->p_HasNext()){
		c_Decl* t_decl6=t_6->p_NextObject();
		c_GlobalDecl* t_gdecl2=dynamic_cast<c_GlobalDecl*>(t_decl6);
		if((t_gdecl2)!=0){
			p_Emit(p_TransRefType(t_gdecl2->m_type)+String(L" ",1)+t_gdecl2->m_munged+String(L";",1));
			continue;
		}
		c_FuncDecl* t_fdecl2=dynamic_cast<c_FuncDecl*>(t_decl6);
		if((t_fdecl2)!=0){
			p_EmitFuncDecl(t_fdecl2);
			continue;
		}
		c_ClassDecl* t_cdecl4=dynamic_cast<c_ClassDecl*>(t_decl6);
		if((t_cdecl4)!=0){
			p_EmitClassDecl(t_cdecl4);
			continue;
		}
	}
	p_BeginLocalScope();
	p_Emit(String(L"int bbInit(){",13));
	p_Emit(String(L"GC_CTOR",7));
	c_Enumerator6* t_7=t_app->m_semantedGlobals->p_ObjectEnumerator();
	while(t_7->p_HasNext()){
		c_GlobalDecl* t_decl7=t_7->p_NextObject();
		String t_munged=p_TransGlobal(t_decl7);
		p_Emit(t_munged+String(L"=",1)+t_decl7->m_init->p_Trans()+String(L";",1));
		if(bb_config_ENV_CONFIG==String(L"debug",5) && p_IsDebuggable(t_decl7->m_type)){
			p_Emit(String(L"DBG_GLOBAL(\"",12)+t_decl7->m_ident+String(L"\",&",3)+t_munged+String(L");",2));
		}
	}
	p_Emit(String(L"return 0;",9));
	p_Emit(String(L"}",1));
	p_EndLocalScope();
	p_Emit(String(L"void gc_mark(){",15));
	c_Enumerator6* t_8=t_app->m_semantedGlobals->p_ObjectEnumerator();
	while(t_8->p_HasNext()){
		c_GlobalDecl* t_decl8=t_8->p_NextObject();
		p_EmitMark(p_TransGlobal(t_decl8),t_decl8->m_type,true);
	}
	p_Emit(String(L"}",1));
	return p_JoinLines();
}
int c_CppTranslator::p_CheckSafe(c_Decl* t_decl){
	if(!m_unsafe || ((t_decl->p_IsExtern())!=0) || t_decl->m_ident.EndsWith(String(L"__UNSAFE__",10))){
		return 0;
	}
	bb_config_Err(String(L"Unsafe call!!!!!",16));
	return 0;
}
String c_CppTranslator::p_TransArgs2(Array<c_Expr* > t_args,c_FuncDecl* t_decl){
	String t_t=String();
	Array<c_Expr* > t_=t_args;
	int t_2=0;
	while(t_2<t_.Length()){
		c_Expr* t_arg=t_[t_2];
		t_2=t_2+1;
		if((t_t).Length()!=0){
			t_t=t_t+String(L",",1);
		}
		String t_targ=p_GcRetain(t_arg,String());
		t_t=t_t+t_targ;
	}
	return p_Bra(t_t);
}
String c_CppTranslator::p_TransFunc(c_FuncDecl* t_decl,Array<c_Expr* > t_args,c_Expr* t_lhs){
	m_pure=0;
	p_CheckSafe(t_decl);
	if(t_decl->p_IsMethod()){
		if((t_lhs)!=0){
			return p_TransSubExpr(t_lhs,2)+String(L"->",2)+t_decl->m_munged+p_TransArgs2(t_args,t_decl);
		}
		return t_decl->m_munged+p_TransArgs2(t_args,t_decl);
	}
	return p_TransStatic(t_decl)+p_TransArgs2(t_args,t_decl);
}
String c_CppTranslator::p_TransSuperFunc(c_FuncDecl* t_decl,Array<c_Expr* > t_args){
	m_pure=0;
	p_CheckSafe(t_decl);
	return t_decl->p_ClassScope()->m_munged+String(L"::",2)+t_decl->m_munged+p_TransArgs2(t_args,t_decl);
}
String c_CppTranslator::p_TransConstExpr(c_ConstExpr* t_expr){
	return p_TransValue(t_expr->m_exprType,t_expr->m_value);
}
String c_CppTranslator::p_TransNewObjectExpr(c_NewObjectExpr* t_expr){
	m_pure=0;
	String t_t=String(L"(new ",5)+t_expr->m_classDecl->m_munged+String(L")",1);
	if((t_expr->m_ctor)!=0){
		t_t=t_t+(String(L"->",2)+t_expr->m_ctor->m_munged+p_TransArgs2(t_expr->m_args,t_expr->m_ctor));
	}
	return t_t;
}
String c_CppTranslator::p_TransNewArrayExpr(c_NewArrayExpr* t_expr){
	m_pure=0;
	String t_texpr=t_expr->m_expr->p_Trans();
	return String(L"Array<",6)+p_TransRefType(t_expr->m_ty)+String(L" >",2)+p_Bra(t_expr->m_expr->p_Trans());
}
String c_CppTranslator::p_TransSelfExpr(c_SelfExpr* t_expr){
	return String(L"this",4);
}
String c_CppTranslator::p_TransCastExpr(c_CastExpr* t_expr){
	String t_t=p_Bra(t_expr->m_expr->p_Trans());
	c_Type* t_dst=t_expr->m_exprType;
	c_Type* t_src=t_expr->m_expr->m_exprType;
	if((dynamic_cast<c_BoolType*>(t_dst))!=0){
		if((dynamic_cast<c_BoolType*>(t_src))!=0){
			return t_t;
		}
		if((dynamic_cast<c_IntType*>(t_src))!=0){
			return p_Bra(t_t+String(L"!=0",3));
		}
		if((dynamic_cast<c_FloatType*>(t_src))!=0){
			return p_Bra(t_t+String(L"!=0",3));
		}
		if((dynamic_cast<c_ArrayType*>(t_src))!=0){
			return p_Bra(t_t+String(L".Length()!=0",12));
		}
		if((dynamic_cast<c_StringType*>(t_src))!=0){
			return p_Bra(t_t+String(L".Length()!=0",12));
		}
		if((dynamic_cast<c_ObjectType*>(t_src))!=0){
			return p_Bra(t_t+String(L"!=0",3));
		}
	}else{
		if((dynamic_cast<c_IntType*>(t_dst))!=0){
			if((dynamic_cast<c_BoolType*>(t_src))!=0){
				return p_Bra(t_t+String(L"?1:0",4));
			}
			if((dynamic_cast<c_IntType*>(t_src))!=0){
				return t_t;
			}
			if((dynamic_cast<c_FloatType*>(t_src))!=0){
				return String(L"int",3)+p_Bra(t_t);
			}
			if((dynamic_cast<c_StringType*>(t_src))!=0){
				return t_t+String(L".ToInt()",8);
			}
		}else{
			if((dynamic_cast<c_FloatType*>(t_dst))!=0){
				if((dynamic_cast<c_IntType*>(t_src))!=0){
					return String(L"Float",5)+p_Bra(t_t);
				}
				if((dynamic_cast<c_FloatType*>(t_src))!=0){
					return t_t;
				}
				if((dynamic_cast<c_StringType*>(t_src))!=0){
					return t_t+String(L".ToFloat()",10);
				}
			}else{
				if((dynamic_cast<c_StringType*>(t_dst))!=0){
					if((dynamic_cast<c_IntType*>(t_src))!=0){
						return String(L"String",6)+p_Bra(t_t);
					}
					if((dynamic_cast<c_FloatType*>(t_src))!=0){
						return String(L"String",6)+p_Bra(t_t);
					}
					if((dynamic_cast<c_StringType*>(t_src))!=0){
						return t_t;
					}
				}else{
					if(((dynamic_cast<c_ObjectType*>(t_dst))!=0) && ((dynamic_cast<c_ObjectType*>(t_src))!=0)){
						if(((t_src->p_GetClass()->p_IsInterface())!=0) && !((t_dst->p_GetClass()->p_IsInterface())!=0)){
							return String(L"dynamic_cast<",13)+p_TransType(t_dst)+String(L">",1)+p_Bra(t_t);
						}else{
							if((t_src->p_GetClass()->p_ExtendsClass(t_dst->p_GetClass()))!=0){
								return t_t;
							}else{
								return String(L"dynamic_cast<",13)+p_TransType(t_dst)+String(L">",1)+p_Bra(t_t);
							}
						}
					}
				}
			}
		}
	}
	bb_config_Err(String(L"C++ translator can't convert ",29)+t_src->p_ToString()+String(L" to ",4)+t_dst->p_ToString());
	return String();
}
String c_CppTranslator::p_TransUnaryExpr(c_UnaryExpr* t_expr){
	int t_pri=p_ExprPri(t_expr);
	String t_t_expr=p_TransSubExpr(t_expr->m_expr,t_pri);
	return p_TransUnaryOp(t_expr->m_op)+t_t_expr;
}
String c_CppTranslator::p_TransBinaryExpr(c_BinaryExpr* t_expr){
	int t_pri=p_ExprPri(t_expr);
	String t_t_lhs=p_TransSubExpr(t_expr->m_lhs,t_pri);
	String t_t_rhs=p_TransSubExpr(t_expr->m_rhs,t_pri-1);
	if(t_expr->m_op==String(L"mod",3) && ((dynamic_cast<c_FloatType*>(t_expr->m_exprType))!=0)){
		return String(L"(Float)fmod(",12)+t_t_lhs+String(L",",1)+t_t_rhs+String(L")",1);
	}
	return t_t_lhs+p_TransBinaryOp(t_expr->m_op,t_t_rhs)+t_t_rhs;
}
String c_CppTranslator::p_TransIndexExpr(c_IndexExpr* t_expr){
	String t_t_expr=p_TransSubExpr(t_expr->m_expr,2);
	String t_t_index=t_expr->m_index->p_Trans();
	if((dynamic_cast<c_StringType*>(t_expr->m_expr->m_exprType))!=0){
		if(bb_config_ENV_CONFIG==String(L"debug",5)){
			return String(L"(int)",5)+t_t_expr+String(L".At(",4)+t_t_index+String(L")",1);
		}
		return String(L"(int)",5)+t_t_expr+String(L"[",1)+t_t_index+String(L"]",1);
	}
	if(bb_config_ENV_CONFIG==String(L"debug",5)){
		return t_t_expr+String(L".At(",4)+t_t_index+String(L")",1);
	}
	return t_t_expr+String(L"[",1)+t_t_index+String(L"]",1);
}
String c_CppTranslator::p_TransSliceExpr(c_SliceExpr* t_expr){
	String t_t_expr=p_TransSubExpr(t_expr->m_expr,2);
	String t_t_args=String(L"0",1);
	if((t_expr->m_from)!=0){
		t_t_args=t_expr->m_from->p_Trans();
	}
	if((t_expr->m_term)!=0){
		t_t_args=t_t_args+(String(L",",1)+t_expr->m_term->p_Trans());
	}
	return t_t_expr+String(L".Slice(",7)+t_t_args+String(L")",1);
}
String c_CppTranslator::p_TransArrayExpr(c_ArrayExpr* t_expr){
	m_pure=0;
	c_Type* t_elemType=dynamic_cast<c_ArrayType*>(t_expr->m_exprType)->m_elemType;
	String t_t=String();
	Array<c_Expr* > t_=t_expr->m_exprs;
	int t_2=0;
	while(t_2<t_.Length()){
		c_Expr* t_elem=t_[t_2];
		t_2=t_2+1;
		String t_e=t_elem->p_Trans();
		if((t_t).Length()!=0){
			t_t=t_t+String(L",",1);
		}
		t_t=t_t+t_e;
	}
	c_LocalDecl* t_tmp=(new c_LocalDecl)->m_new(String(),0,(c_Type::m_voidType),0);
	p_MungDecl(t_tmp);
	p_Emit(p_TransRefType(t_elemType)+String(L" ",1)+t_tmp->m_munged+String(L"[]={",4)+t_t+String(L"};",2));
	return String(L"Array<",6)+p_TransRefType(t_elemType)+String(L" >(",3)+t_tmp->m_munged+String(L",",1)+String(t_expr->m_exprs.Length())+String(L")",1);
}
String c_CppTranslator::p_TransIntrinsicExpr(c_Decl* t_decl,c_Expr* t_expr,Array<c_Expr* > t_args){
	m_pure=0;
	String t_texpr=String();
	String t_arg0=String();
	String t_arg1=String();
	String t_arg2=String();
	if((t_expr)!=0){
		t_texpr=p_TransSubExpr(t_expr,2);
	}
	if(t_args.Length()>0 && ((t_args[0])!=0)){
		t_arg0=t_args[0]->p_Trans();
	}
	if(t_args.Length()>1 && ((t_args[1])!=0)){
		t_arg1=t_args[1]->p_Trans();
	}
	if(t_args.Length()>2 && ((t_args[2])!=0)){
		t_arg2=t_args[2]->p_Trans();
	}
	String t_id=t_decl->m_munged.Slice(1);
	String t_id2=t_id.Slice(0,1).ToUpper()+t_id.Slice(1);
	String t_1=t_id;
	if(t_1==String(L"print",5)){
		return String(L"bbPrint",7)+p_Bra(t_arg0);
	}else{
		if(t_1==String(L"error",5)){
			return String(L"bbError",7)+p_Bra(t_arg0);
		}else{
			if(t_1==String(L"debuglog",8)){
				return String(L"bbDebugLog",10)+p_Bra(t_arg0);
			}else{
				if(t_1==String(L"debugstop",9)){
					return String(L"bbDebugStop()",13);
				}else{
					if(t_1==String(L"length",6)){
						return t_texpr+String(L".Length()",9);
					}else{
						if(t_1==String(L"resize",6)){
							return t_texpr+String(L".Resize",7)+p_Bra(t_arg0);
						}else{
							if(t_1==String(L"compare",7)){
								return t_texpr+String(L".Compare",8)+p_Bra(t_arg0);
							}else{
								if(t_1==String(L"find",4)){
									return t_texpr+String(L".Find",5)+p_Bra(t_arg0+String(L",",1)+t_arg1);
								}else{
									if(t_1==String(L"findlast",8)){
										return t_texpr+String(L".FindLast",9)+p_Bra(t_arg0);
									}else{
										if(t_1==String(L"findlast2",9)){
											return t_texpr+String(L".FindLast",9)+p_Bra(t_arg0+String(L",",1)+t_arg1);
										}else{
											if(t_1==String(L"trim",4)){
												return t_texpr+String(L".Trim()",7);
											}else{
												if(t_1==String(L"join",4)){
													return t_texpr+String(L".Join",5)+p_Bra(t_arg0);
												}else{
													if(t_1==String(L"split",5)){
														return t_texpr+String(L".Split",6)+p_Bra(t_arg0);
													}else{
														if(t_1==String(L"replace",7)){
															return t_texpr+String(L".Replace",8)+p_Bra(t_arg0+String(L",",1)+t_arg1);
														}else{
															if(t_1==String(L"tolower",7)){
																return t_texpr+String(L".ToLower()",10);
															}else{
																if(t_1==String(L"toupper",7)){
																	return t_texpr+String(L".ToUpper()",10);
																}else{
																	if(t_1==String(L"contains",8)){
																		return t_texpr+String(L".Contains",9)+p_Bra(t_arg0);
																	}else{
																		if(t_1==String(L"startswith",10)){
																			return t_texpr+String(L".StartsWith",11)+p_Bra(t_arg0);
																		}else{
																			if(t_1==String(L"endswith",8)){
																				return t_texpr+String(L".EndsWith",9)+p_Bra(t_arg0);
																			}else{
																				if(t_1==String(L"tochars",7)){
																					return t_texpr+String(L".ToChars()",10);
																				}else{
																					if(t_1==String(L"fromchar",8)){
																						return String(L"String",6)+p_Bra(String(L"(Char)",6)+p_Bra(t_arg0)+String(L",1",2));
																					}else{
																						if(t_1==String(L"fromchars",9)){
																							return String(L"String::FromChars",17)+p_Bra(t_arg0);
																						}else{
																							if(t_1==String(L"sin",3) || t_1==String(L"cos",3) || t_1==String(L"tan",3)){
																								return String(L"(Float)",7)+t_id+p_Bra(p_Bra(t_arg0)+String(L"*D2R",4));
																							}else{
																								if(t_1==String(L"asin",4) || t_1==String(L"acos",4) || t_1==String(L"atan",4)){
																									return String(L"(Float)",7)+p_Bra(t_id+p_Bra(t_arg0)+String(L"*R2D",4));
																								}else{
																									if(t_1==String(L"atan2",5)){
																										return String(L"(Float)",7)+p_Bra(t_id+p_Bra(t_arg0+String(L",",1)+t_arg1)+String(L"*R2D",4));
																									}else{
																										if(t_1==String(L"sinr",4) || t_1==String(L"cosr",4) || t_1==String(L"tanr",4)){
																											return String(L"(Float)",7)+t_id.Slice(0,-1)+p_Bra(t_arg0);
																										}else{
																											if(t_1==String(L"asinr",5) || t_1==String(L"acosr",5) || t_1==String(L"atanr",5)){
																												return String(L"(Float)",7)+t_id.Slice(0,-1)+p_Bra(t_arg0);
																											}else{
																												if(t_1==String(L"atan2r",6)){
																													return String(L"(Float)",7)+t_id.Slice(0,-1)+p_Bra(t_arg0+String(L",",1)+t_arg1);
																												}else{
																													if(t_1==String(L"sqrt",4) || t_1==String(L"floor",5) || t_1==String(L"ceil",4) || t_1==String(L"log",3) || t_1==String(L"exp",3)){
																														return String(L"(Float)",7)+t_id+p_Bra(t_arg0);
																													}else{
																														if(t_1==String(L"pow",3)){
																															return String(L"(Float)",7)+t_id+p_Bra(t_arg0+String(L",",1)+t_arg1);
																														}
																													}
																												}
																											}
																										}
																									}
																								}
																							}
																						}
																					}
																				}
																			}
																		}
																	}
																}
															}
														}
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	bb_config_InternalErr(String(L"Internal error",14));
	return String();
}
String c_CppTranslator::p_TransTryStmt(c_TryStmt* t_stmt){
	p_Emit(String(L"try{",4));
	int t_unr=p_EmitBlock(t_stmt->m_block,true);
	Array<c_CatchStmt* > t_=t_stmt->m_catches;
	int t_2=0;
	while(t_2<t_.Length()){
		c_CatchStmt* t_c=t_[t_2];
		t_2=t_2+1;
		p_MungDecl(t_c->m_init);
		p_Emit(String(L"}catch(",7)+p_TransType(t_c->m_init->m_type)+String(L" ",1)+t_c->m_init->m_munged+String(L"){",2));
		m_dbgLocals->p_Push25(t_c->m_init);
		int t_unr2=p_EmitBlock(t_c->m_block,true);
	}
	p_Emit(String(L"}",1));
	return String();
}
String c_CppTranslator::p_TransDeclStmt(c_DeclStmt* t_stmt){
	c_LocalDecl* t_decl=dynamic_cast<c_LocalDecl*>(t_stmt->m_decl);
	if((t_decl)!=0){
		if((t_decl->m_ident).Length()!=0){
			m_dbgLocals->p_Push25(t_decl);
		}
		p_MungDecl(t_decl);
		return p_TransLocalDecl(t_decl->m_munged,t_decl->m_init);
	}
	c_ConstDecl* t_cdecl=dynamic_cast<c_ConstDecl*>(t_stmt->m_decl);
	if((t_cdecl)!=0){
		return String();
	}
	bb_config_InternalErr(String(L"Internal error",14));
	return String();
}
bool c_CppTranslator::p_IsLocalVar(c_Expr* t_expr){
	t_expr=p_Uncast(t_expr);
	c_VarExpr* t_vexpr=dynamic_cast<c_VarExpr*>(t_expr);
	return ((t_vexpr)!=0) && ((dynamic_cast<c_LocalDecl*>(t_vexpr->m_decl))!=0);
}
String c_CppTranslator::p_TransAssignStmt2(c_AssignStmt* t_stmt){
	if(m_gc_mode==0 || t_stmt->m_op!=String(L"=",1) || !p_IsGcObject(t_stmt->m_rhs)){
		return c_CTranslator::p_TransAssignStmt2(t_stmt);
	}
	String t_tlhs=t_stmt->m_lhs->p_TransVar();
	String t_trhs=t_stmt->m_rhs->p_Trans();
	if(p_IsLocalVar(t_stmt->m_lhs)){
		return t_tlhs+String(L"=",1)+p_GcRetain(t_stmt->m_rhs,t_trhs);
	}
	return String(L"gc_assign(",10)+t_tlhs+String(L",",1)+t_trhs+String(L")",1);
}
void c_CppTranslator::mark(){
	c_CTranslator::mark();
	gc_mark_q(m_dbgLocals);
}
c_JsTranslator::c_JsTranslator(){
}
c_JsTranslator* c_JsTranslator::m_new(){
	c_CTranslator::m_new();
	return this;
}
String c_JsTranslator::p_TransValue(c_Type* t_ty,String t_value){
	if((t_value).Length()!=0){
		if((dynamic_cast<c_BoolType*>(t_ty))!=0){
			return String(L"true",4);
		}
		if((dynamic_cast<c_NumericType*>(t_ty))!=0){
			return t_value;
		}
		if((dynamic_cast<c_StringType*>(t_ty))!=0){
			return p_Enquote(t_value);
		}
	}else{
		if((dynamic_cast<c_BoolType*>(t_ty))!=0){
			return String(L"false",5);
		}
		if((dynamic_cast<c_NumericType*>(t_ty))!=0){
			return String(L"0",1);
		}
		if((dynamic_cast<c_StringType*>(t_ty))!=0){
			return String(L"\"\"",2);
		}
		if((dynamic_cast<c_ArrayType*>(t_ty))!=0){
			return String(L"[]",2);
		}
		if((dynamic_cast<c_ObjectType*>(t_ty))!=0){
			return String(L"null",4);
		}
	}
	bb_config_InternalErr(String(L"Internal error",14));
	return String();
}
String c_JsTranslator::p_TransLocalDecl(String t_munged,c_Expr* t_init){
	return String(L"var ",4)+t_munged+String(L"=",1)+t_init->p_Trans();
}
int c_JsTranslator::p_EmitEnter(c_FuncDecl* t_func){
	p_Emit(String(L"push_err();",11));
	return 0;
}
int c_JsTranslator::p_EmitSetErr(String t_info){
	p_Emit(String(L"err_info=\"",10)+t_info.Replace(String(L"\\",1),String(L"/",1))+String(L"\";",2));
	return 0;
}
int c_JsTranslator::p_EmitLeave(){
	p_Emit(String(L"pop_err();",10));
	return 0;
}
String c_JsTranslator::p_TransStatic(c_Decl* t_decl){
	if(((t_decl->p_IsExtern())!=0) && ((dynamic_cast<c_ModuleDecl*>(t_decl->m_scope))!=0)){
		return t_decl->m_munged;
	}else{
		if(((bb_decl__env)!=0) && ((t_decl->m_scope)!=0) && t_decl->m_scope==(bb_decl__env->p_ClassScope())){
			return t_decl->m_scope->m_munged+String(L".",1)+t_decl->m_munged;
		}else{
			if((dynamic_cast<c_ClassDecl*>(t_decl->m_scope))!=0){
				return t_decl->m_scope->m_munged+String(L".",1)+t_decl->m_munged;
			}else{
				if((dynamic_cast<c_ModuleDecl*>(t_decl->m_scope))!=0){
					return t_decl->m_munged;
				}
			}
		}
	}
	bb_config_InternalErr(String(L"Internal error",14));
	return String();
}
String c_JsTranslator::p_TransGlobal(c_GlobalDecl* t_decl){
	return p_TransStatic(t_decl);
}
String c_JsTranslator::p_TransField(c_FieldDecl* t_decl,c_Expr* t_lhs){
	String t_t_lhs=String(L"this",4);
	if((t_lhs)!=0){
		t_t_lhs=p_TransSubExpr(t_lhs,2);
		if(bb_config_ENV_CONFIG==String(L"debug",5)){
			t_t_lhs=String(L"dbg_object",10)+p_Bra(t_t_lhs);
		}
	}
	return t_t_lhs+String(L".",1)+t_decl->m_munged;
}
int c_JsTranslator::p_EmitFuncDecl(c_FuncDecl* t_decl){
	p_BeginLocalScope();
	String t_args=String();
	Array<c_ArgDecl* > t_=t_decl->m_argDecls;
	int t_2=0;
	while(t_2<t_.Length()){
		c_ArgDecl* t_arg=t_[t_2];
		t_2=t_2+1;
		p_MungDecl(t_arg);
		if((t_args).Length()!=0){
			t_args=t_args+String(L",",1);
		}
		t_args=t_args+t_arg->m_munged;
	}
	t_args=p_Bra(t_args);
	if(t_decl->p_IsMethod()){
		p_Emit(t_decl->m_scope->m_munged+String(L".prototype.",11)+t_decl->m_munged+String(L"=function",9)+t_args+String(L"{",1));
	}else{
		if((t_decl->p_ClassScope())!=0){
			p_Emit(p_TransStatic(t_decl)+String(L"=function",9)+t_args+String(L"{",1));
		}else{
			p_Emit(String(L"function ",9)+t_decl->m_munged+t_args+String(L"{",1));
		}
	}
	if(!((t_decl->p_IsAbstract())!=0)){
		p_EmitBlock((t_decl),true);
	}
	p_Emit(String(L"}",1));
	p_EndLocalScope();
	return 0;
}
int c_JsTranslator::p_EmitClassDecl(c_ClassDecl* t_classDecl){
	if((t_classDecl->p_IsInterface())!=0){
		return 0;
	}
	String t_classid=t_classDecl->m_munged;
	String t_superid=t_classDecl->m_superClass->m_munged;
	p_Emit(String(L"function ",9)+t_classid+String(L"(){",3));
	p_Emit(t_superid+String(L".call(this);",12));
	c_Enumerator2* t_=t_classDecl->p_Semanted()->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_Decl* t_decl=t_->p_NextObject();
		c_FieldDecl* t_fdecl=dynamic_cast<c_FieldDecl*>(t_decl);
		if((t_fdecl)!=0){
			p_Emit(String(L"this.",5)+t_fdecl->m_munged+String(L"=",1)+t_fdecl->m_init->p_Trans()+String(L";",1));
		}
	}
	String t_impls=String();
	c_ClassDecl* t_tdecl=t_classDecl;
	c_StringSet* t_iset=(new c_StringSet)->m_new();
	while((t_tdecl)!=0){
		Array<c_ClassDecl* > t_2=t_tdecl->m_implmentsAll;
		int t_3=0;
		while(t_3<t_2.Length()){
			c_ClassDecl* t_iface=t_2[t_3];
			t_3=t_3+1;
			String t_t=t_iface->m_munged;
			if(t_iset->p_Contains(t_t)){
				continue;
			}
			t_iset->p_Insert(t_t);
			if((t_impls).Length()!=0){
				t_impls=t_impls+String(L",",1);
			}
			t_impls=t_impls+(t_t+String(L":1",2));
		}
		t_tdecl=t_tdecl->m_superClass;
	}
	if((t_impls).Length()!=0){
		p_Emit(String(L"this.implments={",16)+t_impls+String(L"};",2));
	}
	p_Emit(String(L"}",1));
	if(t_superid!=String(L"Object",6)){
		p_Emit(t_classid+String(L".prototype=extend_class(",24)+t_superid+String(L");",2));
	}
	c_Enumerator2* t_4=t_classDecl->p_Semanted()->p_ObjectEnumerator();
	while(t_4->p_HasNext()){
		c_Decl* t_decl2=t_4->p_NextObject();
		if((t_decl2->p_IsExtern())!=0){
			continue;
		}
		c_FuncDecl* t_fdecl2=dynamic_cast<c_FuncDecl*>(t_decl2);
		if((t_fdecl2)!=0){
			p_EmitFuncDecl(t_fdecl2);
			continue;
		}
		c_GlobalDecl* t_gdecl=dynamic_cast<c_GlobalDecl*>(t_decl2);
		if((t_gdecl)!=0){
			p_Emit(p_TransGlobal(t_gdecl)+String(L"=",1)+p_TransValue(t_gdecl->m_type,String())+String(L";",1));
			continue;
		}
	}
	return 0;
}
String c_JsTranslator::p_TransApp(c_AppDecl* t_app){
	t_app->m_mainFunc->m_munged=String(L"bbMain",6);
	c_ValueEnumerator* t_=t_app->m_imported->p_Values()->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_ModuleDecl* t_decl=t_->p_NextObject();
		p_MungDecl(t_decl);
	}
	c_Enumerator2* t_2=t_app->p_Semanted()->p_ObjectEnumerator();
	while(t_2->p_HasNext()){
		c_Decl* t_decl2=t_2->p_NextObject();
		p_MungDecl(t_decl2);
		c_ClassDecl* t_cdecl=dynamic_cast<c_ClassDecl*>(t_decl2);
		if(!((t_cdecl)!=0)){
			continue;
		}
		c_Enumerator2* t_3=t_cdecl->p_Semanted()->p_ObjectEnumerator();
		while(t_3->p_HasNext()){
			c_Decl* t_decl3=t_3->p_NextObject();
			p_MungDecl(t_decl3);
		}
	}
	c_Enumerator2* t_4=t_app->p_Semanted()->p_ObjectEnumerator();
	while(t_4->p_HasNext()){
		c_Decl* t_decl4=t_4->p_NextObject();
		c_GlobalDecl* t_gdecl=dynamic_cast<c_GlobalDecl*>(t_decl4);
		if((t_gdecl)!=0){
			p_Emit(String(L"var ",4)+p_TransGlobal(t_gdecl)+String(L"=",1)+p_TransValue(t_gdecl->m_type,String())+String(L";",1));
			continue;
		}
		c_FuncDecl* t_fdecl=dynamic_cast<c_FuncDecl*>(t_decl4);
		if((t_fdecl)!=0){
			p_EmitFuncDecl(t_fdecl);
			continue;
		}
		c_ClassDecl* t_cdecl2=dynamic_cast<c_ClassDecl*>(t_decl4);
		if((t_cdecl2)!=0){
			p_EmitClassDecl(t_cdecl2);
			continue;
		}
	}
	p_Emit(String(L"function bbInit(){",18));
	c_Enumerator6* t_5=t_app->m_semantedGlobals->p_ObjectEnumerator();
	while(t_5->p_HasNext()){
		c_GlobalDecl* t_decl5=t_5->p_NextObject();
		p_Emit(p_TransGlobal(t_decl5)+String(L"=",1)+t_decl5->m_init->p_Trans()+String(L";",1));
	}
	p_Emit(String(L"}",1));
	return p_JoinLines();
}
String c_JsTranslator::p_TransArgs3(Array<c_Expr* > t_args,String t_first){
	String t_t=t_first;
	Array<c_Expr* > t_=t_args;
	int t_2=0;
	while(t_2<t_.Length()){
		c_Expr* t_arg=t_[t_2];
		t_2=t_2+1;
		if((t_t).Length()!=0){
			t_t=t_t+String(L",",1);
		}
		t_t=t_t+t_arg->p_Trans();
	}
	return p_Bra(t_t);
}
String c_JsTranslator::p_TransFunc(c_FuncDecl* t_decl,Array<c_Expr* > t_args,c_Expr* t_lhs){
	if(t_decl->p_IsMethod()){
		String t_t_lhs=String(L"this",4);
		if((t_lhs)!=0){
			t_t_lhs=p_TransSubExpr(t_lhs,2);
		}
		return t_t_lhs+String(L".",1)+t_decl->m_munged+p_TransArgs3(t_args,String());
	}
	return p_TransStatic(t_decl)+p_TransArgs3(t_args,String());
}
String c_JsTranslator::p_TransSuperFunc(c_FuncDecl* t_decl,Array<c_Expr* > t_args){
	if(t_decl->p_IsCtor()){
		return p_TransStatic(t_decl)+String(L".call",5)+p_TransArgs3(t_args,String(L"this",4));
	}
	return t_decl->m_scope->m_munged+String(L".prototype.",11)+t_decl->m_munged+String(L".call",5)+p_TransArgs3(t_args,String(L"this",4));
}
String c_JsTranslator::p_TransConstExpr(c_ConstExpr* t_expr){
	return p_TransValue(t_expr->m_exprType,t_expr->m_value);
}
String c_JsTranslator::p_TransNewObjectExpr(c_NewObjectExpr* t_expr){
	String t_t=String(L"new ",4)+t_expr->m_classDecl->m_munged;
	if((t_expr->m_ctor)!=0){
		t_t=p_TransStatic(t_expr->m_ctor)+String(L".call",5)+p_TransArgs3(t_expr->m_args,t_t);
	}else{
		t_t=String(L"(",1)+t_t+String(L")",1);
	}
	return t_t;
}
String c_JsTranslator::p_TransNewArrayExpr(c_NewArrayExpr* t_expr){
	String t_texpr=t_expr->m_expr->p_Trans();
	c_Type* t_ty=dynamic_cast<c_ArrayType*>(t_expr->m_exprType)->m_elemType;
	if((dynamic_cast<c_BoolType*>(t_ty))!=0){
		return String(L"new_bool_array(",15)+t_texpr+String(L")",1);
	}
	if((dynamic_cast<c_NumericType*>(t_ty))!=0){
		return String(L"new_number_array(",17)+t_texpr+String(L")",1);
	}
	if((dynamic_cast<c_StringType*>(t_ty))!=0){
		return String(L"new_string_array(",17)+t_texpr+String(L")",1);
	}
	if((dynamic_cast<c_ObjectType*>(t_ty))!=0){
		return String(L"new_object_array(",17)+t_texpr+String(L")",1);
	}
	if((dynamic_cast<c_ArrayType*>(t_ty))!=0){
		return String(L"new_array_array(",16)+t_texpr+String(L")",1);
	}
	bb_config_InternalErr(String(L"Internal error",14));
	return String();
}
String c_JsTranslator::p_TransSelfExpr(c_SelfExpr* t_expr){
	return String(L"this",4);
}
String c_JsTranslator::p_TransCastExpr(c_CastExpr* t_expr){
	c_Type* t_dst=t_expr->m_exprType;
	c_Type* t_src=t_expr->m_expr->m_exprType;
	String t_texpr=p_Bra(t_expr->m_expr->p_Trans());
	if((dynamic_cast<c_BoolType*>(t_dst))!=0){
		if((dynamic_cast<c_BoolType*>(t_src))!=0){
			return t_texpr;
		}
		if((dynamic_cast<c_IntType*>(t_src))!=0){
			return p_Bra(t_texpr+String(L"!=0",3));
		}
		if((dynamic_cast<c_FloatType*>(t_src))!=0){
			return p_Bra(t_texpr+String(L"!=0.0",5));
		}
		if((dynamic_cast<c_StringType*>(t_src))!=0){
			return p_Bra(t_texpr+String(L".length!=0",10));
		}
		if((dynamic_cast<c_ArrayType*>(t_src))!=0){
			return p_Bra(t_texpr+String(L".length!=0",10));
		}
		if((dynamic_cast<c_ObjectType*>(t_src))!=0){
			return p_Bra(t_texpr+String(L"!=null",6));
		}
	}else{
		if((dynamic_cast<c_IntType*>(t_dst))!=0){
			if((dynamic_cast<c_BoolType*>(t_src))!=0){
				return p_Bra(t_texpr+String(L"?1:0",4));
			}
			if((dynamic_cast<c_IntType*>(t_src))!=0){
				return t_texpr;
			}
			if((dynamic_cast<c_FloatType*>(t_src))!=0){
				return p_Bra(t_texpr+String(L"|0",2));
			}
			if((dynamic_cast<c_StringType*>(t_src))!=0){
				return String(L"parseInt",8)+p_Bra(t_texpr+String(L",10",3));
			}
		}else{
			if((dynamic_cast<c_FloatType*>(t_dst))!=0){
				if((dynamic_cast<c_NumericType*>(t_src))!=0){
					return t_texpr;
				}
				if((dynamic_cast<c_StringType*>(t_src))!=0){
					return String(L"parseFloat",10)+t_texpr;
				}
			}else{
				if((dynamic_cast<c_StringType*>(t_dst))!=0){
					if((dynamic_cast<c_NumericType*>(t_src))!=0){
						return String(L"String",6)+t_texpr;
					}
					if((dynamic_cast<c_StringType*>(t_src))!=0){
						return t_texpr;
					}
				}else{
					if(((dynamic_cast<c_ObjectType*>(t_dst))!=0) && ((dynamic_cast<c_ObjectType*>(t_src))!=0)){
						if((t_src->p_GetClass()->p_ExtendsClass(t_dst->p_GetClass()))!=0){
							return t_texpr;
						}else{
							if((t_dst->p_GetClass()->p_IsInterface())!=0){
								return String(L"object_implements",17)+p_Bra(t_texpr+String(L",\"",2)+t_dst->p_GetClass()->m_munged+String(L"\"",1));
							}else{
								return String(L"object_downcast",15)+p_Bra(t_texpr+String(L",",1)+t_dst->p_GetClass()->m_munged);
							}
						}
					}
				}
			}
		}
	}
	bb_config_Err(String(L"JS translator can't convert ",28)+t_src->p_ToString()+String(L" to ",4)+t_dst->p_ToString());
	return String();
}
String c_JsTranslator::p_TransUnaryExpr(c_UnaryExpr* t_expr){
	int t_pri=p_ExprPri(t_expr);
	String t_t_expr=p_TransSubExpr(t_expr->m_expr,t_pri);
	return p_TransUnaryOp(t_expr->m_op)+t_t_expr;
}
String c_JsTranslator::p_TransBinaryExpr(c_BinaryExpr* t_expr){
	int t_pri=p_ExprPri(t_expr);
	String t_t_lhs=p_TransSubExpr(t_expr->m_lhs,t_pri);
	String t_t_rhs=p_TransSubExpr(t_expr->m_rhs,t_pri-1);
	String t_t_expr=t_t_lhs+p_TransBinaryOp(t_expr->m_op,t_t_rhs)+t_t_rhs;
	if(t_expr->m_op==String(L"/",1) && ((dynamic_cast<c_IntType*>(t_expr->m_exprType))!=0)){
		t_t_expr=p_Bra(p_Bra(t_t_expr)+String(L"|0",2));
	}
	return t_t_expr;
}
String c_JsTranslator::p_TransIndexExpr(c_IndexExpr* t_expr){
	String t_t_expr=p_TransSubExpr(t_expr->m_expr,2);
	if((dynamic_cast<c_StringType*>(t_expr->m_expr->m_exprType))!=0){
		String t_t_index=t_expr->m_index->p_Trans();
		if(bb_config_ENV_CONFIG==String(L"debug",5)){
			return String(L"dbg_charCodeAt(",15)+t_t_expr+String(L",",1)+t_t_index+String(L")",1);
		}
		return t_t_expr+String(L".charCodeAt(",12)+t_t_index+String(L")",1);
	}else{
		if(bb_config_ENV_CONFIG==String(L"debug",5)){
			String t_t_index2=t_expr->m_index->p_Trans();
			return String(L"dbg_array(",10)+t_t_expr+String(L",",1)+t_t_index2+String(L")[dbg_index]",12);
		}else{
			String t_t_index3=t_expr->m_index->p_Trans();
			return t_t_expr+String(L"[",1)+t_t_index3+String(L"]",1);
		}
	}
}
String c_JsTranslator::p_TransSliceExpr(c_SliceExpr* t_expr){
	String t_t_expr=p_TransSubExpr(t_expr->m_expr,2);
	String t_t_args=String(L"0",1);
	if((t_expr->m_from)!=0){
		t_t_args=t_expr->m_from->p_Trans();
	}
	if((t_expr->m_term)!=0){
		t_t_args=t_t_args+(String(L",",1)+t_expr->m_term->p_Trans());
	}
	return t_t_expr+String(L".slice(",7)+t_t_args+String(L")",1);
}
String c_JsTranslator::p_TransArrayExpr(c_ArrayExpr* t_expr){
	String t_t=String();
	Array<c_Expr* > t_=t_expr->m_exprs;
	int t_2=0;
	while(t_2<t_.Length()){
		c_Expr* t_elem=t_[t_2];
		t_2=t_2+1;
		if((t_t).Length()!=0){
			t_t=t_t+String(L",",1);
		}
		t_t=t_t+t_elem->p_Trans();
	}
	return String(L"[",1)+t_t+String(L"]",1);
}
String c_JsTranslator::p_TransTryStmt(c_TryStmt* t_stmt){
	p_Emit(String(L"try{",4));
	int t_unr=p_EmitBlock(t_stmt->m_block,true);
	p_Emit(String(L"}catch(_eek_){",14));
	for(int t_i=0;t_i<t_stmt->m_catches.Length();t_i=t_i+1){
		c_CatchStmt* t_c=t_stmt->m_catches[t_i];
		p_MungDecl(t_c->m_init);
		if((t_i)!=0){
			p_Emit(String(L"}else if(",9)+t_c->m_init->m_munged+String(L"=object_downcast(_eek_,",23)+t_c->m_init->m_type->p_GetClass()->m_munged+String(L")){",3));
		}else{
			p_Emit(String(L"if(",3)+t_c->m_init->m_munged+String(L"=object_downcast(_eek_,",23)+t_c->m_init->m_type->p_GetClass()->m_munged+String(L")){",3));
		}
		int t_unr2=p_EmitBlock(t_c->m_block,true);
	}
	p_Emit(String(L"}else{",6));
	p_Emit(String(L"throw _eek_;",12));
	p_Emit(String(L"}",1));
	p_Emit(String(L"}",1));
	return String();
}
String c_JsTranslator::p_TransIntrinsicExpr(c_Decl* t_decl,c_Expr* t_expr,Array<c_Expr* > t_args){
	String t_texpr=String();
	String t_arg0=String();
	String t_arg1=String();
	String t_arg2=String();
	if((t_expr)!=0){
		t_texpr=p_TransSubExpr(t_expr,2);
	}
	if(t_args.Length()>0 && ((t_args[0])!=0)){
		t_arg0=t_args[0]->p_Trans();
	}
	if(t_args.Length()>1 && ((t_args[1])!=0)){
		t_arg1=t_args[1]->p_Trans();
	}
	if(t_args.Length()>2 && ((t_args[2])!=0)){
		t_arg2=t_args[2]->p_Trans();
	}
	String t_id=t_decl->m_munged.Slice(1);
	String t_1=t_id;
	if(t_1==String(L"print",5)){
		return String(L"print",5)+p_Bra(t_arg0);
	}else{
		if(t_1==String(L"error",5)){
			return String(L"error",5)+p_Bra(t_arg0);
		}else{
			if(t_1==String(L"debuglog",8)){
				return String(L"debugLog",8)+p_Bra(t_arg0);
			}else{
				if(t_1==String(L"debugstop",9)){
					return String(L"debugStop()",11);
				}else{
					if(t_1==String(L"length",6)){
						return t_texpr+String(L".length",7);
					}else{
						if(t_1==String(L"resize",6)){
							c_Type* t_ty=dynamic_cast<c_ArrayType*>(t_expr->m_exprType)->m_elemType;
							if((dynamic_cast<c_BoolType*>(t_ty))!=0){
								return String(L"resize_bool_array",17)+p_Bra(t_texpr+String(L",",1)+t_arg0);
							}
							if((dynamic_cast<c_NumericType*>(t_ty))!=0){
								return String(L"resize_number_array",19)+p_Bra(t_texpr+String(L",",1)+t_arg0);
							}
							if((dynamic_cast<c_StringType*>(t_ty))!=0){
								return String(L"resize_string_array",19)+p_Bra(t_texpr+String(L",",1)+t_arg0);
							}
							if((dynamic_cast<c_ArrayType*>(t_ty))!=0){
								return String(L"resize_array_array",18)+p_Bra(t_texpr+String(L",",1)+t_arg0);
							}
							if((dynamic_cast<c_ObjectType*>(t_ty))!=0){
								return String(L"resize_object_array",19)+p_Bra(t_texpr+String(L",",1)+t_arg0);
							}
							bb_config_InternalErr(String(L"Internal error",14));
						}else{
							if(t_1==String(L"compare",7)){
								return String(L"string_compare",14)+p_Bra(t_texpr+String(L",",1)+t_arg0);
							}else{
								if(t_1==String(L"find",4)){
									return t_texpr+String(L".indexOf",8)+p_Bra(t_arg0+String(L",",1)+t_arg1);
								}else{
									if(t_1==String(L"findlast",8)){
										return t_texpr+String(L".lastIndexOf",12)+p_Bra(t_arg0);
									}else{
										if(t_1==String(L"findlast2",9)){
											return t_texpr+String(L".lastIndexOf",12)+p_Bra(t_arg0+String(L",",1)+t_arg1);
										}else{
											if(t_1==String(L"trim",4)){
												return String(L"string_trim",11)+p_Bra(t_texpr);
											}else{
												if(t_1==String(L"join",4)){
													return t_arg0+String(L".join",5)+p_Bra(t_texpr);
												}else{
													if(t_1==String(L"split",5)){
														return t_texpr+String(L".split",6)+p_Bra(t_arg0);
													}else{
														if(t_1==String(L"replace",7)){
															return String(L"string_replace",14)+p_Bra(t_texpr+String(L",",1)+t_arg0+String(L",",1)+t_arg1);
														}else{
															if(t_1==String(L"tolower",7)){
																return t_texpr+String(L".toLowerCase()",14);
															}else{
																if(t_1==String(L"toupper",7)){
																	return t_texpr+String(L".toUpperCase()",14);
																}else{
																	if(t_1==String(L"contains",8)){
																		return p_Bra(t_texpr+String(L".indexOf",8)+p_Bra(t_arg0)+String(L"!=-1",4));
																	}else{
																		if(t_1==String(L"startswith",10)){
																			return String(L"string_startswith",17)+p_Bra(t_texpr+String(L",",1)+t_arg0);
																		}else{
																			if(t_1==String(L"endswith",8)){
																				return String(L"string_endswith",15)+p_Bra(t_texpr+String(L",",1)+t_arg0);
																			}else{
																				if(t_1==String(L"tochars",7)){
																					return String(L"string_tochars",14)+p_Bra(t_texpr);
																				}else{
																					if(t_1==String(L"fromchar",8)){
																						return String(L"String.fromCharCode",19)+p_Bra(t_arg0);
																					}else{
																						if(t_1==String(L"fromchars",9)){
																							return String(L"string_fromchars",16)+p_Bra(t_arg0);
																						}else{
																							if(t_1==String(L"sin",3) || t_1==String(L"cos",3) || t_1==String(L"tan",3)){
																								return String(L"Math.",5)+t_id+p_Bra(p_Bra(t_arg0)+String(L"*D2R",4));
																							}else{
																								if(t_1==String(L"asin",4) || t_1==String(L"acos",4) || t_1==String(L"atan",4)){
																									return p_Bra(String(L"Math.",5)+t_id+p_Bra(t_arg0)+String(L"*R2D",4));
																								}else{
																									if(t_1==String(L"atan2",5)){
																										return p_Bra(String(L"Math.",5)+t_id+p_Bra(t_arg0+String(L",",1)+t_arg1)+String(L"*R2D",4));
																									}else{
																										if(t_1==String(L"sinr",4) || t_1==String(L"cosr",4) || t_1==String(L"tanr",4)){
																											return String(L"Math.",5)+t_id.Slice(0,-1)+p_Bra(t_arg0);
																										}else{
																											if(t_1==String(L"asinr",5) || t_1==String(L"acosr",5) || t_1==String(L"atanr",5)){
																												return String(L"Math.",5)+t_id.Slice(0,-1)+p_Bra(t_arg0);
																											}else{
																												if(t_1==String(L"atan2r",6)){
																													return String(L"Math.",5)+t_id.Slice(0,-1)+p_Bra(t_arg0+String(L",",1)+t_arg1);
																												}else{
																													if(t_1==String(L"sqrt",4) || t_1==String(L"floor",5) || t_1==String(L"ceil",4) || t_1==String(L"log",3) || t_1==String(L"exp",3)){
																														return String(L"Math.",5)+t_id+p_Bra(t_arg0);
																													}else{
																														if(t_1==String(L"pow",3)){
																															return String(L"Math.",5)+t_id+p_Bra(t_arg0+String(L",",1)+t_arg1);
																														}
																													}
																												}
																											}
																										}
																									}
																								}
																							}
																						}
																					}
																				}
																			}
																		}
																	}
																}
															}
														}
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	bb_config_InternalErr(String(L"Internal error",14));
	return String();
}
void c_JsTranslator::mark(){
	c_CTranslator::mark();
}
int bb_html5_Info_Width;
int bb_html5_Info_Height;
c_Stream::c_Stream(){
}
c_Stream* c_Stream::m_new(){
	return this;
}
void c_Stream::mark(){
	Object::mark();
}
c_FileStream::c_FileStream(){
	m__stream=0;
}
BBFileStream* c_FileStream::m_OpenStream(String t_path,String t_mode){
	BBFileStream* t_stream=(new BBFileStream);
	String t_fmode=t_mode;
	if(t_fmode==String(L"a",1)){
		t_fmode=String(L"u",1);
	}
	if(!t_stream->Open(t_path,t_fmode)){
		return 0;
	}
	if(t_mode==String(L"a",1)){
		t_stream->Seek(t_stream->Length());
	}
	return t_stream;
}
c_FileStream* c_FileStream::m_new(String t_path,String t_mode){
	c_Stream::m_new();
	gc_assign(m__stream,m_OpenStream(t_path,t_mode));
	if(!((m__stream)!=0)){
		bbError(String(L"Failed to open stream",21));
	}
	return this;
}
c_FileStream* c_FileStream::m_new2(BBFileStream* t_stream){
	c_Stream::m_new();
	gc_assign(m__stream,t_stream);
	return this;
}
c_FileStream* c_FileStream::m_new3(){
	c_Stream::m_new();
	return this;
}
c_FileStream* c_FileStream::m_Open(String t_path,String t_mode){
	BBFileStream* t_stream=m_OpenStream(t_path,t_mode);
	if((t_stream)!=0){
		return (new c_FileStream)->m_new2(t_stream);
	}
	return 0;
}
int c_FileStream::p_Read(c_DataBuffer* t_buffer,int t_offset,int t_count){
	return m__stream->Read(t_buffer,t_offset,t_count);
}
void c_FileStream::p_Close(){
	if(!((m__stream)!=0)){
		return;
	}
	m__stream->Close();
	m__stream=0;
}
int c_FileStream::p_Eof(){
	return m__stream->Eof();
}
int c_FileStream::p_Position(){
	return m__stream->Position();
}
int c_FileStream::p_Seek(int t_position){
	return m__stream->Seek(t_position);
}
void c_FileStream::mark(){
	c_Stream::mark();
	gc_mark_q(m__stream);
}
c_DataBuffer::c_DataBuffer(){
}
c_DataBuffer* c_DataBuffer::m_new(int t_length,bool t_direct){
	if(!_New(t_length)){
		bbError(String(L"Allocate DataBuffer failed",26));
	}
	return this;
}
c_DataBuffer* c_DataBuffer::m_new2(){
	return this;
}
void c_DataBuffer::mark(){
	BBDataBuffer::mark();
}
int bb_html5_GetInfo_PNG(String t_path){
	c_FileStream* t_f=c_FileStream::m_Open(t_path,String(L"r",1));
	if((t_f)!=0){
		c_DataBuffer* t_data=(new c_DataBuffer)->m_new(32,false);
		int t_n=t_f->p_Read(t_data,0,24);
		t_f->p_Close();
		if(t_n==24 && t_data->PeekByte(1)==80 && t_data->PeekByte(2)==78 && t_data->PeekByte(3)==71){
			bb_html5_Info_Width=(t_data->PeekByte(16)&255)<<24|(t_data->PeekByte(17)&255)<<16|(t_data->PeekByte(18)&255)<<8|t_data->PeekByte(19)&255;
			bb_html5_Info_Height=(t_data->PeekByte(20)&255)<<24|(t_data->PeekByte(21)&255)<<16|(t_data->PeekByte(22)&255)<<8|t_data->PeekByte(23)&255;
			return 0;
		}
	}
	return -1;
}
int bb_html5_GetInfo_JPG(String t_path){
	c_FileStream* t_f=c_FileStream::m_Open(t_path,String(L"r",1));
	if((t_f)!=0){
		c_DataBuffer* t_buf=(new c_DataBuffer)->m_new(32,false);
		if(t_f->p_Read(t_buf,0,2)==2 && (t_buf->PeekByte(0)&255)==255 && (t_buf->PeekByte(1)&255)==216){
			do{
				while(t_f->p_Read(t_buf,0,1)==1 && (t_buf->PeekByte(0)&255)!=255){
				}
				if((t_f->p_Eof())!=0){
					break;
				}
				while(t_f->p_Read(t_buf,0,1)==1 && (t_buf->PeekByte(0)&255)==255){
				}
				if((t_f->p_Eof())!=0){
					break;
				}
				int t_marker=t_buf->PeekByte(0)&255;
				int t_1=t_marker;
				if(t_1==208 || t_1==209 || t_1==210 || t_1==211 || t_1==212 || t_1==213 || t_1==214 || t_1==215 || t_1==216 || t_1==217 || t_1==0 || t_1==255){
					continue;
				}
				if(t_f->p_Read(t_buf,0,2)!=2){
					break;
				}
				int t_datalen=((t_buf->PeekByte(0)&255)<<8|t_buf->PeekByte(1)&255)-2;
				int t_2=t_marker;
				if(t_2==192 || t_2==193 || t_2==194 || t_2==195){
					if(((t_datalen)!=0) && t_f->p_Read(t_buf,0,5)==5){
						int t_bpp=t_buf->PeekByte(0)&255;
						bb_html5_Info_Width=(t_buf->PeekByte(3)&255)<<8|t_buf->PeekByte(4)&255;
						bb_html5_Info_Height=(t_buf->PeekByte(1)&255)<<8|t_buf->PeekByte(2)&255;
						t_f->p_Close();
						return 0;
					}
				}
				int t_pos=t_f->p_Position()+t_datalen;
				if(t_f->p_Seek(t_pos)!=t_pos){
					break;
				}
			}while(!(false));
		}
		t_f->p_Close();
	}
	return -1;
}
int bb_html5_GetInfo_GIF(String t_path){
	c_FileStream* t_f=c_FileStream::m_Open(t_path,String(L"r",1));
	if((t_f)!=0){
		c_DataBuffer* t_data=(new c_DataBuffer)->m_new(32,false);
		int t_n=t_f->p_Read(t_data,0,10);
		t_f->p_Close();
		if(t_n==10 && t_data->PeekByte(0)==71 && t_data->PeekByte(1)==73 && t_data->PeekByte(2)==70){
			bb_html5_Info_Width=(t_data->PeekByte(7)&255)<<8|t_data->PeekByte(6)&255;
			bb_html5_Info_Height=(t_data->PeekByte(9)&255)<<8|t_data->PeekByte(8)&255;
			return 0;
		}
	}
	return -1;
}
c_AsTranslator::c_AsTranslator(){
}
c_AsTranslator* c_AsTranslator::m_new(){
	c_CTranslator::m_new();
	return this;
}
String c_AsTranslator::p_TransValue(c_Type* t_ty,String t_value){
	if((t_value).Length()!=0){
		if(((dynamic_cast<c_IntType*>(t_ty))!=0) && t_value.StartsWith(String(L"$",1))){
			return String(L"0x",2)+t_value.Slice(1);
		}
		if((dynamic_cast<c_BoolType*>(t_ty))!=0){
			return String(L"true",4);
		}
		if((dynamic_cast<c_NumericType*>(t_ty))!=0){
			return t_value;
		}
		if((dynamic_cast<c_StringType*>(t_ty))!=0){
			return p_Enquote(t_value);
		}
	}else{
		if((dynamic_cast<c_BoolType*>(t_ty))!=0){
			return String(L"false",5);
		}
		if((dynamic_cast<c_NumericType*>(t_ty))!=0){
			return String(L"0",1);
		}
		if((dynamic_cast<c_StringType*>(t_ty))!=0){
			return String(L"\"\"",2);
		}
		if((dynamic_cast<c_ArrayType*>(t_ty))!=0){
			return String(L"[]",2);
		}
		if((dynamic_cast<c_ObjectType*>(t_ty))!=0){
			return String(L"null",4);
		}
	}
	bb_config_InternalErr(String(L"Internal error",14));
	return String();
}
String c_AsTranslator::p_TransType(c_Type* t_ty){
	if((dynamic_cast<c_VoidType*>(t_ty))!=0){
		return String(L"void",4);
	}
	if((dynamic_cast<c_BoolType*>(t_ty))!=0){
		return String(L"Boolean",7);
	}
	if((dynamic_cast<c_IntType*>(t_ty))!=0){
		return String(L"int",3);
	}
	if((dynamic_cast<c_FloatType*>(t_ty))!=0){
		return String(L"Number",6);
	}
	if((dynamic_cast<c_StringType*>(t_ty))!=0){
		return String(L"String",6);
	}
	if((dynamic_cast<c_ArrayType*>(t_ty))!=0){
		return String(L"Array",5);
	}
	if((dynamic_cast<c_ObjectType*>(t_ty))!=0){
		return dynamic_cast<c_ObjectType*>(t_ty)->m_classDecl->m_munged;
	}
	bb_config_InternalErr(String(L"Internal error",14));
	return String();
}
String c_AsTranslator::p_TransLocalDecl(String t_munged,c_Expr* t_init){
	return String(L"var ",4)+t_munged+String(L":",1)+p_TransType(t_init->m_exprType)+String(L"=",1)+t_init->p_Trans();
}
int c_AsTranslator::p_EmitEnter(c_FuncDecl* t_func){
	p_Emit(String(L"pushErr();",10));
	return 0;
}
int c_AsTranslator::p_EmitSetErr(String t_info){
	p_Emit(String(L"_errInfo=\"",10)+t_info.Replace(String(L"\\",1),String(L"/",1))+String(L"\";",2));
	return 0;
}
int c_AsTranslator::p_EmitLeave(){
	p_Emit(String(L"popErr();",9));
	return 0;
}
String c_AsTranslator::p_TransStatic(c_Decl* t_decl){
	if(((t_decl->p_IsExtern())!=0) && ((dynamic_cast<c_ModuleDecl*>(t_decl->m_scope))!=0)){
		return t_decl->m_munged;
	}else{
		if(((bb_decl__env)!=0) && ((t_decl->m_scope)!=0) && t_decl->m_scope==(bb_decl__env->p_ClassScope())){
			return t_decl->m_munged;
		}else{
			if((dynamic_cast<c_ClassDecl*>(t_decl->m_scope))!=0){
				return t_decl->m_scope->m_munged+String(L".",1)+t_decl->m_munged;
			}else{
				if((dynamic_cast<c_ModuleDecl*>(t_decl->m_scope))!=0){
					return t_decl->m_munged;
				}
			}
		}
	}
	bb_config_InternalErr(String(L"Internal error",14));
	return String();
}
String c_AsTranslator::p_TransGlobal(c_GlobalDecl* t_decl){
	return p_TransStatic(t_decl);
}
String c_AsTranslator::p_TransField(c_FieldDecl* t_decl,c_Expr* t_lhs){
	if((t_lhs)!=0){
		String t_t_lhs=p_TransSubExpr(t_lhs,2);
		if(bb_config_ENV_CONFIG==String(L"debug",5)){
			t_t_lhs=String(L"dbg_object",10)+p_Bra(t_t_lhs);
		}
		return t_t_lhs+String(L".",1)+t_decl->m_munged;
	}
	return t_decl->m_munged;
}
String c_AsTranslator::p_TransValDecl(c_ValDecl* t_decl){
	return t_decl->m_munged+String(L":",1)+p_TransType(t_decl->m_type);
}
int c_AsTranslator::p_EmitFuncDecl(c_FuncDecl* t_decl){
	p_BeginLocalScope();
	String t_args=String();
	Array<c_ArgDecl* > t_=t_decl->m_argDecls;
	int t_2=0;
	while(t_2<t_.Length()){
		c_ArgDecl* t_arg=t_[t_2];
		t_2=t_2+1;
		p_MungDecl(t_arg);
		if((t_args).Length()!=0){
			t_args=t_args+String(L",",1);
		}
		t_args=t_args+p_TransValDecl(t_arg);
	}
	String t_t=String(L"function ",9)+t_decl->m_munged+p_Bra(t_args)+String(L":",1)+p_TransType(t_decl->m_retType);
	c_ClassDecl* t_cdecl=t_decl->p_ClassScope();
	if(((t_cdecl)!=0) && ((t_cdecl->p_IsInterface())!=0)){
		p_Emit(t_t+String(L";",1));
	}else{
		String t_q=String(L"internal ",9);
		if((t_cdecl)!=0){
			t_q=String(L"public ",7);
			if(t_decl->p_IsStatic()){
				t_q=t_q+String(L"static ",7);
			}
			if((t_decl->m_overrides)!=0){
				t_q=t_q+String(L"override ",9);
			}
		}
		p_Emit(t_q+t_t+String(L"{",1));
		if((t_decl->p_IsAbstract())!=0){
			if((dynamic_cast<c_VoidType*>(t_decl->m_retType))!=0){
				p_Emit(String(L"return;",7));
			}else{
				p_Emit(String(L"return ",7)+p_TransValue(t_decl->m_retType,String())+String(L";",1));
			}
		}else{
			p_EmitBlock((t_decl),true);
		}
		p_Emit(String(L"}",1));
	}
	p_EndLocalScope();
	return 0;
}
int c_AsTranslator::p_EmitClassDecl(c_ClassDecl* t_classDecl){
	String t_classid=t_classDecl->m_munged;
	String t_superid=t_classDecl->m_superClass->m_munged;
	if((t_classDecl->p_IsInterface())!=0){
		String t_bases=String();
		Array<c_ClassDecl* > t_=t_classDecl->m_implments;
		int t_2=0;
		while(t_2<t_.Length()){
			c_ClassDecl* t_iface=t_[t_2];
			t_2=t_2+1;
			if((t_bases).Length()!=0){
				t_bases=t_bases+String(L",",1);
			}else{
				t_bases=String(L" extends ",9);
			}
			t_bases=t_bases+t_iface->m_munged;
		}
		p_Emit(String(L"interface ",10)+t_classid+t_bases+String(L"{",1));
		c_Enumerator2* t_3=t_classDecl->p_Semanted()->p_ObjectEnumerator();
		while(t_3->p_HasNext()){
			c_Decl* t_decl=t_3->p_NextObject();
			c_FuncDecl* t_fdecl=dynamic_cast<c_FuncDecl*>(t_decl);
			if(!((t_fdecl)!=0)){
				continue;
			}
			p_EmitFuncDecl(t_fdecl);
		}
		p_Emit(String(L"}",1));
		return 0;
	}
	String t_bases2=String();
	Array<c_ClassDecl* > t_4=t_classDecl->m_implments;
	int t_5=0;
	while(t_5<t_4.Length()){
		c_ClassDecl* t_iface2=t_4[t_5];
		t_5=t_5+1;
		if((t_bases2).Length()!=0){
			t_bases2=t_bases2+String(L",",1);
		}else{
			t_bases2=String(L" implements ",12);
		}
		t_bases2=t_bases2+t_iface2->m_munged;
	}
	p_Emit(String(L"class ",6)+t_classid+String(L" extends ",9)+t_superid+t_bases2+String(L"{",1));
	c_Enumerator2* t_6=t_classDecl->p_Semanted()->p_ObjectEnumerator();
	while(t_6->p_HasNext()){
		c_Decl* t_decl2=t_6->p_NextObject();
		c_FieldDecl* t_tdecl=dynamic_cast<c_FieldDecl*>(t_decl2);
		if((t_tdecl)!=0){
			p_Emit(String(L"internal var ",13)+p_TransValDecl(t_tdecl)+String(L"=",1)+t_tdecl->m_init->p_Trans()+String(L";",1));
			continue;
		}
		c_GlobalDecl* t_gdecl=dynamic_cast<c_GlobalDecl*>(t_decl2);
		if((t_gdecl)!=0){
			p_Emit(String(L"internal static var ",20)+p_TransValDecl(t_gdecl)+String(L";",1));
			continue;
		}
		c_FuncDecl* t_fdecl2=dynamic_cast<c_FuncDecl*>(t_decl2);
		if((t_fdecl2)!=0){
			p_EmitFuncDecl(t_fdecl2);
			continue;
		}
	}
	p_Emit(String(L"}",1));
	return 0;
}
String c_AsTranslator::p_TransApp(c_AppDecl* t_app){
	t_app->m_mainFunc->m_munged=String(L"bbMain",6);
	c_ValueEnumerator* t_=t_app->m_imported->p_Values()->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_ModuleDecl* t_decl=t_->p_NextObject();
		p_MungDecl(t_decl);
	}
	c_Enumerator2* t_2=t_app->p_Semanted()->p_ObjectEnumerator();
	while(t_2->p_HasNext()){
		c_Decl* t_decl2=t_2->p_NextObject();
		p_MungDecl(t_decl2);
		c_ClassDecl* t_cdecl=dynamic_cast<c_ClassDecl*>(t_decl2);
		if(!((t_cdecl)!=0)){
			continue;
		}
		c_Enumerator2* t_3=t_cdecl->p_Semanted()->p_ObjectEnumerator();
		while(t_3->p_HasNext()){
			c_Decl* t_decl3=t_3->p_NextObject();
			if(((dynamic_cast<c_FuncDecl*>(t_decl3))!=0) && dynamic_cast<c_FuncDecl*>(t_decl3)->p_IsCtor()){
				t_decl3->m_ident=t_cdecl->m_ident+String(L"_",1)+t_decl3->m_ident;
			}
			p_MungDecl(t_decl3);
		}
	}
	c_Enumerator2* t_4=t_app->p_Semanted()->p_ObjectEnumerator();
	while(t_4->p_HasNext()){
		c_Decl* t_decl4=t_4->p_NextObject();
		c_GlobalDecl* t_gdecl=dynamic_cast<c_GlobalDecl*>(t_decl4);
		if((t_gdecl)!=0){
			p_Emit(String(L"var ",4)+p_TransValDecl(t_gdecl)+String(L";",1));
			continue;
		}
		c_FuncDecl* t_fdecl=dynamic_cast<c_FuncDecl*>(t_decl4);
		if((t_fdecl)!=0){
			p_EmitFuncDecl(t_fdecl);
			continue;
		}
		c_ClassDecl* t_cdecl2=dynamic_cast<c_ClassDecl*>(t_decl4);
		if((t_cdecl2)!=0){
			p_EmitClassDecl(t_cdecl2);
			continue;
		}
	}
	p_BeginLocalScope();
	p_Emit(String(L"function bbInit():void{",23));
	c_Enumerator6* t_5=t_app->m_semantedGlobals->p_ObjectEnumerator();
	while(t_5->p_HasNext()){
		c_GlobalDecl* t_decl5=t_5->p_NextObject();
		p_Emit(p_TransGlobal(t_decl5)+String(L"=",1)+t_decl5->m_init->p_Trans()+String(L";",1));
	}
	p_Emit(String(L"}",1));
	p_EndLocalScope();
	return p_JoinLines();
}
String c_AsTranslator::p_TransArgs(Array<c_Expr* > t_args){
	String t_t=String();
	Array<c_Expr* > t_=t_args;
	int t_2=0;
	while(t_2<t_.Length()){
		c_Expr* t_arg=t_[t_2];
		t_2=t_2+1;
		if((t_t).Length()!=0){
			t_t=t_t+String(L",",1);
		}
		t_t=t_t+t_arg->p_Trans();
	}
	return p_Bra(t_t);
}
String c_AsTranslator::p_TransFunc(c_FuncDecl* t_decl,Array<c_Expr* > t_args,c_Expr* t_lhs){
	if(t_decl->p_IsMethod()){
		String t_t_lhs=String(L"this",4);
		if((t_lhs)!=0){
			t_t_lhs=p_TransSubExpr(t_lhs,2);
		}
		return t_t_lhs+String(L".",1)+t_decl->m_munged+p_TransArgs(t_args);
	}
	return p_TransStatic(t_decl)+p_TransArgs(t_args);
}
String c_AsTranslator::p_TransSuperFunc(c_FuncDecl* t_decl,Array<c_Expr* > t_args){
	return String(L"super.",6)+t_decl->m_munged+p_TransArgs(t_args);
}
String c_AsTranslator::p_TransConstExpr(c_ConstExpr* t_expr){
	return p_TransValue(t_expr->m_exprType,t_expr->m_value);
}
String c_AsTranslator::p_TransNewObjectExpr(c_NewObjectExpr* t_expr){
	String t_t=String(L"(new ",5)+t_expr->m_classDecl->m_munged+String(L")",1);
	if((t_expr->m_ctor)!=0){
		t_t=t_t+(String(L".",1)+t_expr->m_ctor->m_munged+p_TransArgs(t_expr->m_args));
	}
	return t_t;
}
String c_AsTranslator::p_TransNewArrayExpr(c_NewArrayExpr* t_expr){
	String t_texpr=t_expr->m_expr->p_Trans();
	c_Type* t_ty=t_expr->m_ty;
	if((dynamic_cast<c_BoolType*>(t_ty))!=0){
		return String(L"new_bool_array(",15)+t_texpr+String(L")",1);
	}
	if((dynamic_cast<c_NumericType*>(t_ty))!=0){
		return String(L"new_number_array(",17)+t_texpr+String(L")",1);
	}
	if((dynamic_cast<c_StringType*>(t_ty))!=0){
		return String(L"new_string_array(",17)+t_texpr+String(L")",1);
	}
	if((dynamic_cast<c_ObjectType*>(t_ty))!=0){
		return String(L"new_object_array(",17)+t_texpr+String(L")",1);
	}
	if((dynamic_cast<c_ArrayType*>(t_ty))!=0){
		return String(L"new_array_array(",16)+t_texpr+String(L")",1);
	}
	bb_config_InternalErr(String(L"Internal error",14));
	return String();
}
String c_AsTranslator::p_TransSelfExpr(c_SelfExpr* t_expr){
	return String(L"this",4);
}
String c_AsTranslator::p_TransCastExpr(c_CastExpr* t_expr){
	c_Type* t_dst=t_expr->m_exprType;
	c_Type* t_src=t_expr->m_expr->m_exprType;
	String t_texpr=p_Bra(t_expr->m_expr->p_Trans());
	if((dynamic_cast<c_BoolType*>(t_dst))!=0){
		if((dynamic_cast<c_BoolType*>(t_src))!=0){
			return t_texpr;
		}
		if((dynamic_cast<c_IntType*>(t_src))!=0){
			return p_Bra(t_texpr+String(L"!=0",3));
		}
		if((dynamic_cast<c_FloatType*>(t_src))!=0){
			return p_Bra(t_texpr+String(L"!=0.0",5));
		}
		if((dynamic_cast<c_StringType*>(t_src))!=0){
			return p_Bra(t_texpr+String(L".length!=0",10));
		}
		if((dynamic_cast<c_ArrayType*>(t_src))!=0){
			return p_Bra(t_texpr+String(L".length!=0",10));
		}
		if((dynamic_cast<c_ObjectType*>(t_src))!=0){
			return p_Bra(t_texpr+String(L"!=null",6));
		}
	}else{
		if((dynamic_cast<c_IntType*>(t_dst))!=0){
			if((dynamic_cast<c_BoolType*>(t_src))!=0){
				return p_Bra(t_texpr+String(L"?1:0",4));
			}
			if((dynamic_cast<c_IntType*>(t_src))!=0){
				return t_texpr;
			}
			if((dynamic_cast<c_FloatType*>(t_src))!=0){
				return p_Bra(t_texpr+String(L"|0",2));
			}
			if((dynamic_cast<c_StringType*>(t_src))!=0){
				return String(L"parseInt",8)+p_Bra(t_texpr+String(L",10",3));
			}
		}else{
			if((dynamic_cast<c_FloatType*>(t_dst))!=0){
				if((dynamic_cast<c_NumericType*>(t_src))!=0){
					return t_texpr;
				}
				if((dynamic_cast<c_StringType*>(t_src))!=0){
					return String(L"parseFloat",10)+t_texpr;
				}
			}else{
				if((dynamic_cast<c_StringType*>(t_dst))!=0){
					if((dynamic_cast<c_NumericType*>(t_src))!=0){
						return String(L"String",6)+t_texpr;
					}
					if((dynamic_cast<c_StringType*>(t_src))!=0){
						return t_texpr;
					}
				}else{
					if(((dynamic_cast<c_ObjectType*>(t_dst))!=0) && ((dynamic_cast<c_ObjectType*>(t_src))!=0)){
						if((t_src->p_GetClass()->p_ExtendsClass(t_dst->p_GetClass()))!=0){
							return t_texpr;
						}else{
							return p_Bra(t_texpr+String(L" as ",4)+p_TransType(t_dst));
						}
					}
				}
			}
		}
	}
	bb_config_Err(String(L"AS translator can't convert ",28)+t_src->p_ToString()+String(L" to ",4)+t_dst->p_ToString());
	return String();
}
String c_AsTranslator::p_TransUnaryExpr(c_UnaryExpr* t_expr){
	int t_pri=p_ExprPri(t_expr);
	String t_t_expr=p_TransSubExpr(t_expr->m_expr,t_pri);
	return p_TransUnaryOp(t_expr->m_op)+t_t_expr;
}
String c_AsTranslator::p_TransBinaryExpr(c_BinaryExpr* t_expr){
	int t_pri=p_ExprPri(t_expr);
	String t_t_lhs=p_TransSubExpr(t_expr->m_lhs,t_pri);
	String t_t_rhs=p_TransSubExpr(t_expr->m_rhs,t_pri-1);
	String t_t_expr=t_t_lhs+p_TransBinaryOp(t_expr->m_op,t_t_rhs)+t_t_rhs;
	if(t_expr->m_op==String(L"/",1) && ((dynamic_cast<c_IntType*>(t_expr->m_exprType))!=0)){
		t_t_expr=p_Bra(p_Bra(t_t_expr)+String(L"|0",2));
	}
	return t_t_expr;
}
String c_AsTranslator::p_TransIndexExpr(c_IndexExpr* t_expr){
	String t_t_expr=p_TransSubExpr(t_expr->m_expr,2);
	if((dynamic_cast<c_StringType*>(t_expr->m_expr->m_exprType))!=0){
		String t_t_index=t_expr->m_index->p_Trans();
		if(bb_config_ENV_CONFIG==String(L"debug",5)){
			return String(L"dbg_charCodeAt(",15)+t_t_expr+String(L",",1)+t_t_index+String(L")",1);
		}
		return t_t_expr+String(L".charCodeAt(",12)+t_t_index+String(L")",1);
	}else{
		if(bb_config_ENV_CONFIG==String(L"debug",5)){
			String t_t_index2=t_expr->m_index->p_Trans();
			return String(L"dbg_array(",10)+t_t_expr+String(L",",1)+t_t_index2+String(L")[dbg_index]",12);
		}else{
			String t_t_index3=t_expr->m_index->p_Trans();
			return t_t_expr+String(L"[",1)+t_t_index3+String(L"]",1);
		}
	}
}
String c_AsTranslator::p_TransSliceExpr(c_SliceExpr* t_expr){
	String t_t_expr=p_TransSubExpr(t_expr->m_expr,2);
	String t_t_args=String(L"0",1);
	if((t_expr->m_from)!=0){
		t_t_args=t_expr->m_from->p_Trans();
	}
	if((t_expr->m_term)!=0){
		t_t_args=t_t_args+(String(L",",1)+t_expr->m_term->p_Trans());
	}
	return t_t_expr+String(L".slice(",7)+t_t_args+String(L")",1);
}
String c_AsTranslator::p_TransArrayExpr(c_ArrayExpr* t_expr){
	String t_t=String();
	Array<c_Expr* > t_=t_expr->m_exprs;
	int t_2=0;
	while(t_2<t_.Length()){
		c_Expr* t_elem=t_[t_2];
		t_2=t_2+1;
		if((t_t).Length()!=0){
			t_t=t_t+String(L",",1);
		}
		t_t=t_t+t_elem->p_Trans();
	}
	return String(L"[",1)+t_t+String(L"]",1);
}
String c_AsTranslator::p_TransIntrinsicExpr(c_Decl* t_decl,c_Expr* t_expr,Array<c_Expr* > t_args){
	String t_texpr=String();
	String t_arg0=String();
	String t_arg1=String();
	String t_arg2=String();
	if((t_expr)!=0){
		t_texpr=p_TransSubExpr(t_expr,2);
	}
	if(t_args.Length()>0 && ((t_args[0])!=0)){
		t_arg0=t_args[0]->p_Trans();
	}
	if(t_args.Length()>1 && ((t_args[1])!=0)){
		t_arg1=t_args[1]->p_Trans();
	}
	if(t_args.Length()>2 && ((t_args[2])!=0)){
		t_arg2=t_args[2]->p_Trans();
	}
	String t_id=t_decl->m_munged.Slice(1);
	String t_1=t_id;
	if(t_1==String(L"print",5)){
		return String(L"print",5)+p_Bra(t_arg0);
	}else{
		if(t_1==String(L"error",5)){
			return String(L"error",5)+p_Bra(t_arg0);
		}else{
			if(t_1==String(L"debuglog",8)){
				return String(L"debugLog",8)+p_Bra(t_arg0);
			}else{
				if(t_1==String(L"debugstop",9)){
					return String(L"debugStop()",11);
				}else{
					if(t_1==String(L"length",6)){
						return t_texpr+String(L".length",7);
					}else{
						if(t_1==String(L"resize",6)){
							c_Type* t_ty=dynamic_cast<c_ArrayType*>(t_expr->m_exprType)->m_elemType;
							if((dynamic_cast<c_BoolType*>(t_ty))!=0){
								return String(L"resize_bool_array",17)+p_Bra(t_texpr+String(L",",1)+t_arg0);
							}
							if((dynamic_cast<c_NumericType*>(t_ty))!=0){
								return String(L"resize_number_array",19)+p_Bra(t_texpr+String(L",",1)+t_arg0);
							}
							if((dynamic_cast<c_StringType*>(t_ty))!=0){
								return String(L"resize_string_array",19)+p_Bra(t_texpr+String(L",",1)+t_arg0);
							}
							if((dynamic_cast<c_ArrayType*>(t_ty))!=0){
								return String(L"resize_array_array",18)+p_Bra(t_texpr+String(L",",1)+t_arg0);
							}
							if((dynamic_cast<c_ObjectType*>(t_ty))!=0){
								return String(L"resize_object_array",19)+p_Bra(t_texpr+String(L",",1)+t_arg0);
							}
							bb_config_InternalErr(String(L"Internal error",14));
						}else{
							if(t_1==String(L"compare",7)){
								return String(L"string_compare",14)+p_Bra(t_texpr+String(L",",1)+t_arg0);
							}else{
								if(t_1==String(L"find",4)){
									return t_texpr+String(L".indexOf",8)+p_Bra(t_arg0+String(L",",1)+t_arg1);
								}else{
									if(t_1==String(L"findlast",8)){
										return t_texpr+String(L".lastIndexOf",12)+p_Bra(t_arg0);
									}else{
										if(t_1==String(L"findlast2",9)){
											return t_texpr+String(L".lastIndexOf",12)+p_Bra(t_arg0+String(L",",1)+t_arg1);
										}else{
											if(t_1==String(L"trim",4)){
												return String(L"string_trim",11)+p_Bra(t_texpr);
											}else{
												if(t_1==String(L"join",4)){
													return t_arg0+String(L".join",5)+p_Bra(t_texpr);
												}else{
													if(t_1==String(L"split",5)){
														return t_texpr+String(L".split",6)+p_Bra(t_arg0);
													}else{
														if(t_1==String(L"replace",7)){
															return String(L"string_replace",14)+p_Bra(t_texpr+String(L",",1)+t_arg0+String(L",",1)+t_arg1);
														}else{
															if(t_1==String(L"tolower",7)){
																return t_texpr+String(L".toLowerCase()",14);
															}else{
																if(t_1==String(L"toupper",7)){
																	return t_texpr+String(L".toUpperCase()",14);
																}else{
																	if(t_1==String(L"contains",8)){
																		return p_Bra(t_texpr+String(L".indexOf",8)+p_Bra(t_arg0)+String(L"!=-1",4));
																	}else{
																		if(t_1==String(L"startswith",10)){
																			return String(L"string_startswith",17)+p_Bra(t_texpr+String(L",",1)+t_arg0);
																		}else{
																			if(t_1==String(L"endswith",8)){
																				return String(L"string_endswith",15)+p_Bra(t_texpr+String(L",",1)+t_arg0);
																			}else{
																				if(t_1==String(L"tochars",7)){
																					return String(L"string_tochars",14)+p_Bra(t_texpr);
																				}else{
																					if(t_1==String(L"fromchar",8)){
																						return String(L"String.fromCharCode",19)+p_Bra(t_arg0);
																					}else{
																						if(t_1==String(L"fromchars",9)){
																							return String(L"string_fromchars",16)+p_Bra(t_arg0);
																						}else{
																							if(t_1==String(L"sin",3) || t_1==String(L"cos",3) || t_1==String(L"tan",3)){
																								return String(L"Math.",5)+t_id+p_Bra(p_Bra(t_arg0)+String(L"*D2R",4));
																							}else{
																								if(t_1==String(L"asin",4) || t_1==String(L"acos",4) || t_1==String(L"atan",4)){
																									return p_Bra(String(L"Math.",5)+t_id+p_Bra(t_arg0)+String(L"*R2D",4));
																								}else{
																									if(t_1==String(L"atan2",5)){
																										return p_Bra(String(L"Math.",5)+t_id+p_Bra(t_arg0+String(L",",1)+t_arg1)+String(L"*R2D",4));
																									}else{
																										if(t_1==String(L"sinr",4) || t_1==String(L"cosr",4) || t_1==String(L"tanr",4)){
																											return String(L"Math.",5)+t_id.Slice(0,-1)+p_Bra(t_arg0);
																										}else{
																											if(t_1==String(L"asinr",5) || t_1==String(L"acosr",5) || t_1==String(L"atanr",5)){
																												return String(L"Math.",5)+t_id.Slice(0,-1)+p_Bra(t_arg0);
																											}else{
																												if(t_1==String(L"atan2r",6)){
																													return String(L"Math.",5)+t_id.Slice(0,-1)+p_Bra(t_arg0+String(L",",1)+t_arg1);
																												}else{
																													if(t_1==String(L"sqrt",4) || t_1==String(L"floor",5) || t_1==String(L"ceil",4) || t_1==String(L"log",3) || t_1==String(L"exp",3)){
																														return String(L"Math.",5)+t_id+p_Bra(t_arg0);
																													}else{
																														if(t_1==String(L"pow",3)){
																															return String(L"Math.",5)+t_id+p_Bra(t_arg0+String(L",",1)+t_arg1);
																														}
																													}
																												}
																											}
																										}
																									}
																								}
																							}
																						}
																					}
																				}
																			}
																		}
																	}
																}
															}
														}
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	bb_config_InternalErr(String(L"Internal error",14));
	return String();
}
String c_AsTranslator::p_TransTryStmt(c_TryStmt* t_stmt){
	p_Emit(String(L"try{",4));
	int t_unr=p_EmitBlock(t_stmt->m_block,true);
	Array<c_CatchStmt* > t_=t_stmt->m_catches;
	int t_2=0;
	while(t_2<t_.Length()){
		c_CatchStmt* t_c=t_[t_2];
		t_2=t_2+1;
		p_MungDecl(t_c->m_init);
		p_Emit(String(L"}catch(",7)+t_c->m_init->m_munged+String(L":",1)+p_TransType(t_c->m_init->m_type)+String(L"){",2));
		int t_unr2=p_EmitBlock(t_c->m_block,true);
	}
	p_Emit(String(L"}",1));
	return String();
}
void c_AsTranslator::mark(){
	c_CTranslator::mark();
}
c_CsTranslator::c_CsTranslator(){
}
c_CsTranslator* c_CsTranslator::m_new(){
	c_CTranslator::m_new();
	return this;
}
String c_CsTranslator::p_TransType(c_Type* t_ty){
	if((dynamic_cast<c_VoidType*>(t_ty))!=0){
		return String(L"void",4);
	}
	if((dynamic_cast<c_BoolType*>(t_ty))!=0){
		return String(L"bool",4);
	}
	if((dynamic_cast<c_IntType*>(t_ty))!=0){
		return String(L"int",3);
	}
	if((dynamic_cast<c_FloatType*>(t_ty))!=0){
		return String(L"float",5);
	}
	if((dynamic_cast<c_StringType*>(t_ty))!=0){
		return String(L"String",6);
	}
	if((dynamic_cast<c_ArrayType*>(t_ty))!=0){
		return p_TransType(dynamic_cast<c_ArrayType*>(t_ty)->m_elemType)+String(L"[]",2);
	}
	if((dynamic_cast<c_ObjectType*>(t_ty))!=0){
		return t_ty->p_GetClass()->m_munged;
	}
	bb_config_InternalErr(String(L"Internal error",14));
	return String();
}
String c_CsTranslator::p_TransValue(c_Type* t_ty,String t_value){
	if((t_value).Length()!=0){
		if(((dynamic_cast<c_IntType*>(t_ty))!=0) && t_value.StartsWith(String(L"$",1))){
			return String(L"0x",2)+t_value.Slice(1);
		}
		if((dynamic_cast<c_BoolType*>(t_ty))!=0){
			return String(L"true",4);
		}
		if((dynamic_cast<c_IntType*>(t_ty))!=0){
			return t_value;
		}
		if((dynamic_cast<c_FloatType*>(t_ty))!=0){
			return t_value+String(L"f",1);
		}
		if((dynamic_cast<c_StringType*>(t_ty))!=0){
			return p_Enquote(t_value);
		}
	}else{
		if((dynamic_cast<c_BoolType*>(t_ty))!=0){
			return String(L"false",5);
		}
		if((dynamic_cast<c_NumericType*>(t_ty))!=0){
			return String(L"0",1);
		}
		if((dynamic_cast<c_StringType*>(t_ty))!=0){
			return String(L"\"\"",2);
		}
		if((dynamic_cast<c_ArrayType*>(t_ty))!=0){
			c_Type* t_elemTy=dynamic_cast<c_ArrayType*>(t_ty)->m_elemType;
			String t_t=String(L"[0]",3);
			while((dynamic_cast<c_ArrayType*>(t_elemTy))!=0){
				t_elemTy=dynamic_cast<c_ArrayType*>(t_elemTy)->m_elemType;
				t_t=t_t+String(L"[]",2);
			}
			return String(L"new ",4)+p_TransType(t_elemTy)+t_t;
		}
		if((dynamic_cast<c_ObjectType*>(t_ty))!=0){
			return String(L"null",4);
		}
	}
	bb_config_InternalErr(String(L"Internal error",14));
	return String();
}
String c_CsTranslator::p_TransLocalDecl(String t_munged,c_Expr* t_init){
	return p_TransType(t_init->m_exprType)+String(L" ",1)+t_munged+String(L"=",1)+t_init->p_Trans();
}
int c_CsTranslator::p_EmitEnter(c_FuncDecl* t_func){
	p_Emit(String(L"bb_std_lang.pushErr();",22));
	return 0;
}
int c_CsTranslator::p_EmitSetErr(String t_info){
	p_Emit(String(L"bb_std_lang.errInfo=\"",21)+t_info.Replace(String(L"\\",1),String(L"/",1))+String(L"\";",2));
	return 0;
}
int c_CsTranslator::p_EmitLeave(){
	p_Emit(String(L"bb_std_lang.popErr();",21));
	return 0;
}
String c_CsTranslator::p_TransStatic(c_Decl* t_decl){
	if(((t_decl->p_IsExtern())!=0) && ((dynamic_cast<c_ModuleDecl*>(t_decl->m_scope))!=0)){
		return t_decl->m_munged;
	}else{
		if(((bb_decl__env)!=0) && ((t_decl->m_scope)!=0) && t_decl->m_scope==(bb_decl__env->p_ClassScope())){
			return t_decl->m_munged;
		}else{
			if((dynamic_cast<c_ClassDecl*>(t_decl->m_scope))!=0){
				return t_decl->m_scope->m_munged+String(L".",1)+t_decl->m_munged;
			}else{
				if((dynamic_cast<c_ModuleDecl*>(t_decl->m_scope))!=0){
					return t_decl->m_scope->m_munged+String(L".",1)+t_decl->m_munged;
				}
			}
		}
	}
	bb_config_InternalErr(String(L"Internal error",14));
	return String();
}
String c_CsTranslator::p_TransGlobal(c_GlobalDecl* t_decl){
	return p_TransStatic(t_decl);
}
String c_CsTranslator::p_TransField(c_FieldDecl* t_decl,c_Expr* t_lhs){
	if((t_lhs)!=0){
		return p_TransSubExpr(t_lhs,2)+String(L".",1)+t_decl->m_munged;
	}
	return t_decl->m_munged;
}
int c_CsTranslator::p_EmitFuncDecl(c_FuncDecl* t_decl){
	p_BeginLocalScope();
	String t_args=String();
	Array<c_ArgDecl* > t_=t_decl->m_argDecls;
	int t_2=0;
	while(t_2<t_.Length()){
		c_ArgDecl* t_arg=t_[t_2];
		t_2=t_2+1;
		p_MungDecl(t_arg);
		if((t_args).Length()!=0){
			t_args=t_args+String(L",",1);
		}
		t_args=t_args+(p_TransType(t_arg->m_type)+String(L" ",1)+t_arg->m_munged);
	}
	String t_t=p_TransType(t_decl->m_retType)+String(L" ",1)+t_decl->m_munged+p_Bra(t_args);
	if(((t_decl->p_ClassScope())!=0) && ((t_decl->p_ClassScope()->p_IsInterface())!=0)){
		p_Emit(t_t+String(L";",1));
	}else{
		if((t_decl->p_IsAbstract())!=0){
			if((t_decl->m_overrides)!=0){
				p_Emit(String(L"public abstract override ",25)+t_t+String(L";",1));
			}else{
				p_Emit(String(L"public abstract ",16)+t_t+String(L";",1));
			}
		}else{
			String t_q=String(L"public ",7);
			if(t_decl->p_IsStatic()){
				t_q=t_q+String(L"static ",7);
			}else{
				if((t_decl->m_overrides)!=0){
					t_q=t_q+String(L"override ",9);
					if(!t_decl->p_IsVirtual()){
						t_q=t_q+String(L"sealed ",7);
					}
				}else{
					if(t_decl->p_IsVirtual()){
						t_q=t_q+String(L"virtual ",8);
					}
				}
			}
			p_Emit(t_q+t_t+String(L"{",1));
			p_EmitBlock((t_decl),true);
			p_Emit(String(L"}",1));
		}
	}
	p_EndLocalScope();
	return 0;
}
String c_CsTranslator::p_TransDecl(c_Decl* t_decl){
	c_ValDecl* t_vdecl=dynamic_cast<c_ValDecl*>(t_decl);
	if((t_vdecl)!=0){
		return p_TransType(t_vdecl->m_type)+String(L" ",1)+t_decl->m_munged;
	}
	bb_config_InternalErr(String(L"Internal error",14));
	return String();
}
int c_CsTranslator::p_EmitClassDecl(c_ClassDecl* t_classDecl){
	String t_classid=t_classDecl->m_munged;
	if((t_classDecl->p_IsInterface())!=0){
		String t_bases=String();
		Array<c_ClassDecl* > t_=t_classDecl->m_implments;
		int t_2=0;
		while(t_2<t_.Length()){
			c_ClassDecl* t_iface=t_[t_2];
			t_2=t_2+1;
			if((t_bases).Length()!=0){
				t_bases=t_bases+String(L",",1);
			}else{
				t_bases=String(L" : ",3);
			}
			t_bases=t_bases+t_iface->m_munged;
		}
		p_Emit(String(L"interface ",10)+t_classid+t_bases+String(L"{",1));
		c_Enumerator2* t_3=t_classDecl->p_Semanted()->p_ObjectEnumerator();
		while(t_3->p_HasNext()){
			c_Decl* t_decl=t_3->p_NextObject();
			c_FuncDecl* t_fdecl=dynamic_cast<c_FuncDecl*>(t_decl);
			if(!((t_fdecl)!=0)){
				continue;
			}
			p_EmitFuncDecl(t_fdecl);
		}
		p_Emit(String(L"}",1));
		return 0;
	}
	String t_superid=t_classDecl->m_superClass->m_munged;
	String t_bases2=String(L" : ",3)+t_superid;
	Array<c_ClassDecl* > t_4=t_classDecl->m_implments;
	int t_5=0;
	while(t_5<t_4.Length()){
		c_ClassDecl* t_iface2=t_4[t_5];
		t_5=t_5+1;
		t_bases2=t_bases2+(String(L",",1)+t_iface2->m_munged);
	}
	String t_q=String();
	if((t_classDecl->p_IsAbstract())!=0){
		t_q=t_q+String(L"abstract ",9);
	}else{
		if((t_classDecl->p_IsFinal())!=0){
			t_q=t_q+String(L"sealed ",7);
		}
	}
	p_Emit(t_q+String(L"class ",6)+t_classid+t_bases2+String(L"{",1));
	c_Enumerator2* t_6=t_classDecl->p_Semanted()->p_ObjectEnumerator();
	while(t_6->p_HasNext()){
		c_Decl* t_decl2=t_6->p_NextObject();
		c_FieldDecl* t_tdecl=dynamic_cast<c_FieldDecl*>(t_decl2);
		if((t_tdecl)!=0){
			p_Emit(String(L"public ",7)+p_TransDecl(t_tdecl)+String(L"=",1)+t_tdecl->m_init->p_Trans()+String(L";",1));
			continue;
		}
		c_FuncDecl* t_fdecl2=dynamic_cast<c_FuncDecl*>(t_decl2);
		if((t_fdecl2)!=0){
			p_EmitFuncDecl(t_fdecl2);
			continue;
		}
		c_GlobalDecl* t_gdecl=dynamic_cast<c_GlobalDecl*>(t_decl2);
		if((t_gdecl)!=0){
			p_Emit(String(L"public static ",14)+p_TransDecl(t_gdecl)+String(L";",1));
			continue;
		}
	}
	p_Emit(String(L"}",1));
	return 0;
}
String c_CsTranslator::p_TransApp(c_AppDecl* t_app){
	t_app->m_mainModule->m_munged=String(L"bb_",3);
	t_app->m_mainFunc->m_munged=String(L"bbMain",6);
	c_ValueEnumerator* t_=t_app->m_imported->p_Values()->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_ModuleDecl* t_decl=t_->p_NextObject();
		p_MungDecl(t_decl);
	}
	c_Enumerator2* t_2=t_app->p_Semanted()->p_ObjectEnumerator();
	while(t_2->p_HasNext()){
		c_Decl* t_decl2=t_2->p_NextObject();
		p_MungDecl(t_decl2);
		c_ClassDecl* t_cdecl=dynamic_cast<c_ClassDecl*>(t_decl2);
		if(!((t_cdecl)!=0)){
			continue;
		}
		c_Enumerator2* t_3=t_cdecl->p_Semanted()->p_ObjectEnumerator();
		while(t_3->p_HasNext()){
			c_Decl* t_decl3=t_3->p_NextObject();
			if(((dynamic_cast<c_FuncDecl*>(t_decl3))!=0) && dynamic_cast<c_FuncDecl*>(t_decl3)->p_IsCtor()){
				t_decl3->m_ident=t_cdecl->m_ident+String(L"_",1)+t_decl3->m_ident;
			}
			p_MungDecl(t_decl3);
		}
	}
	c_Enumerator2* t_4=t_app->p_Semanted()->p_ObjectEnumerator();
	while(t_4->p_HasNext()){
		c_Decl* t_decl4=t_4->p_NextObject();
		c_ClassDecl* t_cdecl2=dynamic_cast<c_ClassDecl*>(t_decl4);
		if((t_cdecl2)!=0){
			p_EmitClassDecl(t_cdecl2);
			continue;
		}
	}
	c_ValueEnumerator* t_5=t_app->m_imported->p_Values()->p_ObjectEnumerator();
	while(t_5->p_HasNext()){
		c_ModuleDecl* t_mdecl=t_5->p_NextObject();
		p_Emit(String(L"class ",6)+t_mdecl->m_munged+String(L"{",1));
		c_Enumerator2* t_6=t_mdecl->p_Semanted()->p_ObjectEnumerator();
		while(t_6->p_HasNext()){
			c_Decl* t_decl5=t_6->p_NextObject();
			if(((t_decl5->p_IsExtern())!=0) || ((t_decl5->m_scope->p_ClassScope())!=0)){
				continue;
			}
			c_GlobalDecl* t_gdecl=dynamic_cast<c_GlobalDecl*>(t_decl5);
			if((t_gdecl)!=0){
				p_Emit(String(L"public static ",14)+p_TransDecl(t_gdecl)+String(L";",1));
				continue;
			}
			c_FuncDecl* t_fdecl=dynamic_cast<c_FuncDecl*>(t_decl5);
			if((t_fdecl)!=0){
				p_EmitFuncDecl(t_fdecl);
				continue;
			}
		}
		if(t_mdecl==t_app->m_mainModule){
			p_BeginLocalScope();
			p_Emit(String(L"public static int bbInit(){",27));
			c_Enumerator6* t_7=t_app->m_semantedGlobals->p_ObjectEnumerator();
			while(t_7->p_HasNext()){
				c_GlobalDecl* t_decl6=t_7->p_NextObject();
				p_Emit(p_TransGlobal(t_decl6)+String(L"=",1)+t_decl6->m_init->p_Trans()+String(L";",1));
			}
			p_Emit(String(L"return 0;",9));
			p_Emit(String(L"}",1));
			p_EndLocalScope();
		}
		p_Emit(String(L"}",1));
	}
	return p_JoinLines();
}
String c_CsTranslator::p_TransArgs(Array<c_Expr* > t_args){
	String t_t=String();
	Array<c_Expr* > t_=t_args;
	int t_2=0;
	while(t_2<t_.Length()){
		c_Expr* t_arg=t_[t_2];
		t_2=t_2+1;
		if((t_t).Length()!=0){
			t_t=t_t+String(L",",1);
		}
		t_t=t_t+t_arg->p_Trans();
	}
	return p_Bra(t_t);
}
String c_CsTranslator::p_TransFunc(c_FuncDecl* t_decl,Array<c_Expr* > t_args,c_Expr* t_lhs){
	if(t_decl->p_IsMethod()){
		if((t_lhs)!=0){
			return p_TransSubExpr(t_lhs,2)+String(L".",1)+t_decl->m_munged+p_TransArgs(t_args);
		}
		return t_decl->m_munged+p_TransArgs(t_args);
	}
	return p_TransStatic(t_decl)+p_TransArgs(t_args);
}
String c_CsTranslator::p_TransSuperFunc(c_FuncDecl* t_decl,Array<c_Expr* > t_args){
	return String(L"base.",5)+t_decl->m_munged+p_TransArgs(t_args);
}
String c_CsTranslator::p_TransConstExpr(c_ConstExpr* t_expr){
	return p_TransValue(t_expr->m_exprType,t_expr->m_value);
}
String c_CsTranslator::p_TransNewObjectExpr(c_NewObjectExpr* t_expr){
	String t_t=String(L"(new ",5)+t_expr->m_classDecl->m_munged+String(L"())",3);
	if((t_expr->m_ctor)!=0){
		t_t=t_t+(String(L".",1)+t_expr->m_ctor->m_munged+p_TransArgs(t_expr->m_args));
	}
	return t_t;
}
String c_CsTranslator::p_TransNewArrayExpr(c_NewArrayExpr* t_expr){
	String t_texpr=t_expr->m_expr->p_Trans();
	c_Type* t_elemTy=dynamic_cast<c_ArrayType*>(t_expr->m_exprType)->m_elemType;
	if((dynamic_cast<c_StringType*>(t_elemTy))!=0){
		return String(L"bb_std_lang.stringArray",23)+p_Bra(t_texpr);
	}
	String t_t=String(L"[",1)+t_texpr+String(L"]",1);
	while((dynamic_cast<c_ArrayType*>(t_elemTy))!=0){
		t_elemTy=dynamic_cast<c_ArrayType*>(t_elemTy)->m_elemType;
		t_t=t_t+String(L"[]",2);
	}
	return String(L"new ",4)+p_TransType(t_elemTy)+t_t;
}
String c_CsTranslator::p_TransSelfExpr(c_SelfExpr* t_expr){
	return String(L"this",4);
}
String c_CsTranslator::p_TransCastExpr(c_CastExpr* t_expr){
	c_Type* t_dst=t_expr->m_exprType;
	c_Type* t_src=t_expr->m_expr->m_exprType;
	String t_uexpr=t_expr->m_expr->p_Trans();
	String t_texpr=p_Bra(t_uexpr);
	if((dynamic_cast<c_BoolType*>(t_dst))!=0){
		if((dynamic_cast<c_BoolType*>(t_src))!=0){
			return t_texpr;
		}
		if((dynamic_cast<c_IntType*>(t_src))!=0){
			return p_Bra(t_texpr+String(L"!=0",3));
		}
		if((dynamic_cast<c_FloatType*>(t_src))!=0){
			return p_Bra(t_texpr+String(L"!=0.0f",6));
		}
		if((dynamic_cast<c_StringType*>(t_src))!=0){
			return p_Bra(t_texpr+String(L".Length!=0",10));
		}
		if((dynamic_cast<c_ArrayType*>(t_src))!=0){
			return p_Bra(t_texpr+String(L".Length!=0",10));
		}
		if((dynamic_cast<c_ObjectType*>(t_src))!=0){
			return p_Bra(t_texpr+String(L"!=null",6));
		}
	}else{
		if((dynamic_cast<c_IntType*>(t_dst))!=0){
			if((dynamic_cast<c_BoolType*>(t_src))!=0){
				return p_Bra(t_texpr+String(L"?1:0",4));
			}
			if((dynamic_cast<c_IntType*>(t_src))!=0){
				return t_texpr;
			}
			if((dynamic_cast<c_FloatType*>(t_src))!=0){
				return String(L"(int)",5)+t_texpr;
			}
			if((dynamic_cast<c_StringType*>(t_src))!=0){
				return String(L"int.Parse",9)+t_texpr;
			}
		}else{
			if((dynamic_cast<c_FloatType*>(t_dst))!=0){
				if((dynamic_cast<c_IntType*>(t_src))!=0){
					return String(L"(float)",7)+t_texpr;
				}
				if((dynamic_cast<c_FloatType*>(t_src))!=0){
					return t_texpr;
				}
				if((dynamic_cast<c_StringType*>(t_src))!=0){
					if(bb_config_ENV_TARGET==String(L"xna",3)){
						return String(L"float.Parse",11)+p_Bra(t_uexpr+String(L",CultureInfo.InvariantCulture",29));
					}
					return String(L"float.Parse",11)+p_Bra(t_uexpr);
				}
			}else{
				if((dynamic_cast<c_StringType*>(t_dst))!=0){
					if((dynamic_cast<c_IntType*>(t_src))!=0){
						return t_texpr+String(L".ToString()",11);
					}
					if((dynamic_cast<c_FloatType*>(t_src))!=0){
						if(bb_config_ENV_TARGET==String(L"xna",3)){
							return t_texpr+String(L".ToString(CultureInfo.InvariantCulture)",39);
						}
						return t_texpr+String(L".ToString()",11);
					}
					if((dynamic_cast<c_StringType*>(t_src))!=0){
						return t_texpr;
					}
				}else{
					if(((dynamic_cast<c_ObjectType*>(t_dst))!=0) && ((dynamic_cast<c_ObjectType*>(t_src))!=0)){
						if((t_src->p_GetClass()->p_ExtendsClass(t_dst->p_GetClass()))!=0){
							return t_texpr;
						}else{
							return String(L"(",1)+t_texpr+String(L" as ",4)+p_TransType(t_dst)+String(L")",1);
						}
					}
				}
			}
		}
	}
	bb_config_Err(String(L"CS translator can't convert ",28)+t_src->p_ToString()+String(L" to ",4)+t_dst->p_ToString());
	return String();
}
String c_CsTranslator::p_TransUnaryExpr(c_UnaryExpr* t_expr){
	int t_pri=p_ExprPri(t_expr);
	String t_t_expr=p_TransSubExpr(t_expr->m_expr,t_pri);
	return p_TransUnaryOp(t_expr->m_op)+t_t_expr;
}
String c_CsTranslator::p_TransBinaryExpr(c_BinaryExpr* t_expr){
	if(((dynamic_cast<c_BinaryCompareExpr*>(t_expr))!=0) && ((dynamic_cast<c_StringType*>(t_expr->m_lhs->m_exprType))!=0) && ((dynamic_cast<c_StringType*>(t_expr->m_rhs->m_exprType))!=0)){
		return p_Bra(p_TransSubExpr(t_expr->m_lhs,2)+String(L".CompareTo(",11)+t_expr->m_rhs->p_Trans()+String(L")",1)+p_TransBinaryOp(t_expr->m_op,String())+String(L"0",1));
	}
	int t_pri=p_ExprPri(t_expr);
	String t_t_lhs=p_TransSubExpr(t_expr->m_lhs,t_pri);
	String t_t_rhs=p_TransSubExpr(t_expr->m_rhs,t_pri-1);
	return t_t_lhs+p_TransBinaryOp(t_expr->m_op,t_t_rhs)+t_t_rhs;
}
String c_CsTranslator::p_TransIndexExpr(c_IndexExpr* t_expr){
	String t_t_expr=p_TransSubExpr(t_expr->m_expr,2);
	String t_t_index=t_expr->m_index->p_Trans();
	if((dynamic_cast<c_StringType*>(t_expr->m_expr->m_exprType))!=0){
		return String(L"(int)",5)+t_t_expr+String(L"[",1)+t_t_index+String(L"]",1);
	}
	return t_t_expr+String(L"[",1)+t_t_index+String(L"]",1);
}
String c_CsTranslator::p_TransSliceExpr(c_SliceExpr* t_expr){
	String t_t_expr=t_expr->m_expr->p_Trans();
	String t_t_args=String(L"0",1);
	if((t_expr->m_from)!=0){
		t_t_args=t_expr->m_from->p_Trans();
	}
	if((t_expr->m_term)!=0){
		t_t_args=t_t_args+(String(L",",1)+t_expr->m_term->p_Trans());
	}
	return String(L"((",2)+p_TransType(t_expr->m_exprType)+String(L")bb_std_lang.slice(",19)+t_t_expr+String(L",",1)+t_t_args+String(L"))",2);
}
String c_CsTranslator::p_TransArrayExpr(c_ArrayExpr* t_expr){
	String t_t=String();
	Array<c_Expr* > t_=t_expr->m_exprs;
	int t_2=0;
	while(t_2<t_.Length()){
		c_Expr* t_elem=t_[t_2];
		t_2=t_2+1;
		if((t_t).Length()!=0){
			t_t=t_t+String(L",",1);
		}
		t_t=t_t+t_elem->p_Trans();
	}
	return String(L"new ",4)+p_TransType(t_expr->m_exprType)+String(L"{",1)+t_t+String(L"}",1);
}
String c_CsTranslator::p_TransIntrinsicExpr(c_Decl* t_decl,c_Expr* t_expr,Array<c_Expr* > t_args){
	String t_texpr=String();
	String t_arg0=String();
	String t_arg1=String();
	String t_arg2=String();
	if((t_expr)!=0){
		t_texpr=p_TransSubExpr(t_expr,2);
	}
	if(t_args.Length()>0 && ((t_args[0])!=0)){
		t_arg0=t_args[0]->p_Trans();
	}
	if(t_args.Length()>1 && ((t_args[1])!=0)){
		t_arg1=t_args[1]->p_Trans();
	}
	if(t_args.Length()>2 && ((t_args[2])!=0)){
		t_arg2=t_args[2]->p_Trans();
	}
	String t_id=t_decl->m_munged.Slice(1);
	String t_id2=t_id.Slice(0,1).ToUpper()+t_id.Slice(1);
	String t_1=t_id;
	if(t_1==String(L"print",5)){
		return String(L"bb_std_lang.Print",17)+p_Bra(t_arg0);
	}else{
		if(t_1==String(L"error",5)){
			return String(L"bb_std_lang.Error",17)+p_Bra(t_arg0);
		}else{
			if(t_1==String(L"debuglog",8)){
				return String(L"bb_std_lang.DebugLog",20)+p_Bra(t_arg0);
			}else{
				if(t_1==String(L"debugstop",9)){
					return String(L"bb_std_lang.DebugStop()",23);
				}else{
					if(t_1==String(L"length",6)){
						if((dynamic_cast<c_StringType*>(t_expr->m_exprType))!=0){
							return t_texpr+String(L".Length",7);
						}
						return String(L"bb_std_lang.length",18)+p_Bra(t_texpr);
					}else{
						if(t_1==String(L"resize",6)){
							c_Type* t_ty=dynamic_cast<c_ArrayType*>(t_expr->m_exprType)->m_elemType;
							if((dynamic_cast<c_StringType*>(t_ty))!=0){
								return String(L"bb_std_lang.resize(",19)+t_texpr+String(L",",1)+t_arg0+String(L")",1);
							}
							String t_ety=p_TransType(t_ty);
							return String(L"(",1)+t_ety+String(L"[])bb_std_lang.resize(",22)+t_texpr+String(L",",1)+t_arg0+String(L",typeof(",8)+t_ety+String(L"))",2);
						}else{
							if(t_1==String(L"compare",7)){
								return t_texpr+String(L".CompareTo",10)+p_Bra(t_arg0);
							}else{
								if(t_1==String(L"find",4)){
									return t_texpr+String(L".IndexOf",8)+p_Bra(t_arg0+String(L",",1)+t_arg1);
								}else{
									if(t_1==String(L"findlast",8)){
										return t_texpr+String(L".LastIndexOf",12)+p_Bra(t_arg0);
									}else{
										if(t_1==String(L"findlast2",9)){
											return t_texpr+String(L".LastIndexOf",12)+p_Bra(t_arg0+String(L",",1)+t_arg1);
										}else{
											if(t_1==String(L"trim",4)){
												return t_texpr+String(L".Trim()",7);
											}else{
												if(t_1==String(L"join",4)){
													return String(L"String.Join",11)+p_Bra(t_texpr+String(L",",1)+t_arg0);
												}else{
													if(t_1==String(L"split",5)){
														return String(L"bb_std_lang.split",17)+p_Bra(t_texpr+String(L",",1)+t_arg0);
													}else{
														if(t_1==String(L"replace",7)){
															return t_texpr+String(L".Replace",8)+p_Bra(t_arg0+String(L",",1)+t_arg1);
														}else{
															if(t_1==String(L"tolower",7)){
																return t_texpr+String(L".ToLower()",10);
															}else{
																if(t_1==String(L"toupper",7)){
																	return t_texpr+String(L".ToUpper()",10);
																}else{
																	if(t_1==String(L"contains",8)){
																		return p_Bra(t_texpr+String(L".IndexOf",8)+p_Bra(t_arg0)+String(L"!=-1",4));
																	}else{
																		if(t_1==String(L"startswith",10)){
																			return t_texpr+String(L".StartsWith",11)+p_Bra(t_arg0);
																		}else{
																			if(t_1==String(L"endswith",8)){
																				return t_texpr+String(L".EndsWith",9)+p_Bra(t_arg0);
																			}else{
																				if(t_1==String(L"tochars",7)){
																					return String(L"bb_std_lang.toChars",19)+p_Bra(t_texpr);
																				}else{
																					if(t_1==String(L"fromchar",8)){
																						return String(L"new String",10)+p_Bra(String(L"(char)",6)+p_Bra(t_arg0)+String(L",1",2));
																					}else{
																						if(t_1==String(L"fromchars",9)){
																							return String(L"bb_std_lang.fromChars",21)+p_Bra(t_arg0);
																						}else{
																							if(t_1==String(L"sin",3) || t_1==String(L"cos",3) || t_1==String(L"tan",3)){
																								return String(L"(float)Math.",12)+t_id2+p_Bra(p_Bra(t_arg0)+String(L"*bb_std_lang.D2R",16));
																							}else{
																								if(t_1==String(L"asin",4) || t_1==String(L"acos",4) || t_1==String(L"atan",4)){
																									return String(L"(float)",7)+p_Bra(String(L"Math.",5)+t_id2+p_Bra(t_arg0)+String(L"*bb_std_lang.R2D",16));
																								}else{
																									if(t_1==String(L"atan2",5)){
																										return String(L"(float)",7)+p_Bra(String(L"Math.",5)+t_id2+p_Bra(t_arg0+String(L",",1)+t_arg1)+String(L"*bb_std_lang.R2D",16));
																									}else{
																										if(t_1==String(L"sinr",4) || t_1==String(L"cosr",4) || t_1==String(L"tanr",4)){
																											return String(L"(float)Math.",12)+t_id2.Slice(0,-1)+p_Bra(t_arg0);
																										}else{
																											if(t_1==String(L"asinr",5) || t_1==String(L"acosr",5) || t_1==String(L"atanr",5)){
																												return String(L"(float)Math.",12)+t_id2.Slice(0,-1)+p_Bra(t_arg0);
																											}else{
																												if(t_1==String(L"atan2r",6)){
																													return String(L"(float)Math.",12)+t_id2.Slice(0,-1)+p_Bra(t_arg0+String(L",",1)+t_arg1);
																												}else{
																													if(t_1==String(L"sqrt",4) || t_1==String(L"floor",5) || t_1==String(L"log",3) || t_1==String(L"exp",3)){
																														return String(L"(float)Math.",12)+t_id2+p_Bra(t_arg0);
																													}else{
																														if(t_1==String(L"ceil",4)){
																															return String(L"(float)Math.Ceiling",19)+p_Bra(t_arg0);
																														}else{
																															if(t_1==String(L"pow",3)){
																																return String(L"(float)Math.",12)+t_id2+p_Bra(t_arg0+String(L",",1)+t_arg1);
																															}
																														}
																													}
																												}
																											}
																										}
																									}
																								}
																							}
																						}
																					}
																				}
																			}
																		}
																	}
																}
															}
														}
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	bb_config_InternalErr(String(L"Internal error",14));
	return String();
}
String c_CsTranslator::p_TransTryStmt(c_TryStmt* t_stmt){
	p_Emit(String(L"try{",4));
	int t_unr=p_EmitBlock(t_stmt->m_block,true);
	Array<c_CatchStmt* > t_=t_stmt->m_catches;
	int t_2=0;
	while(t_2<t_.Length()){
		c_CatchStmt* t_c=t_[t_2];
		t_2=t_2+1;
		p_MungDecl(t_c->m_init);
		p_Emit(String(L"}catch(",7)+p_TransType(t_c->m_init->m_type)+String(L" ",1)+t_c->m_init->m_munged+String(L"){",2));
		int t_unr2=p_EmitBlock(t_c->m_block,true);
	}
	p_Emit(String(L"}",1));
	return String();
}
void c_CsTranslator::mark(){
	c_CTranslator::mark();
}
c_List9::c_List9(){
	m__head=((new c_HeadNode9)->m_new());
}
c_List9* c_List9::m_new(){
	return this;
}
c_Node16* c_List9::p_AddLast9(c_ModuleDecl* t_data){
	return (new c_Node16)->m_new(m__head,m__head->m__pred,t_data);
}
c_List9* c_List9::m_new2(Array<c_ModuleDecl* > t_data){
	Array<c_ModuleDecl* > t_=t_data;
	int t_2=0;
	while(t_2<t_.Length()){
		c_ModuleDecl* t_t=t_[t_2];
		t_2=t_2+1;
		p_AddLast9(t_t);
	}
	return this;
}
bool c_List9::p_IsEmpty(){
	return m__head->m__succ==m__head;
}
c_ModuleDecl* c_List9::p_RemoveLast(){
	c_ModuleDecl* t_data=m__head->m__pred->m__data;
	m__head->m__pred->p_Remove();
	return t_data;
}
bool c_List9::p_Equals4(c_ModuleDecl* t_lhs,c_ModuleDecl* t_rhs){
	return t_lhs==t_rhs;
}
c_Node16* c_List9::p_FindLast7(c_ModuleDecl* t_value,c_Node16* t_start){
	while(t_start!=m__head){
		if(p_Equals4(t_value,t_start->m__data)){
			return t_start;
		}
		t_start=t_start->m__pred;
	}
	return 0;
}
c_Node16* c_List9::p_FindLast8(c_ModuleDecl* t_value){
	return p_FindLast7(t_value,m__head->m__pred);
}
void c_List9::p_RemoveLast5(c_ModuleDecl* t_value){
	c_Node16* t_node=p_FindLast8(t_value);
	if((t_node)!=0){
		t_node->p_Remove();
	}
}
void c_List9::mark(){
	Object::mark();
	gc_mark_q(m__head);
}
c_Node16::c_Node16(){
	m__succ=0;
	m__pred=0;
	m__data=0;
}
c_Node16* c_Node16::m_new(c_Node16* t_succ,c_Node16* t_pred,c_ModuleDecl* t_data){
	gc_assign(m__succ,t_succ);
	gc_assign(m__pred,t_pred);
	gc_assign(m__succ->m__pred,this);
	gc_assign(m__pred->m__succ,this);
	gc_assign(m__data,t_data);
	return this;
}
c_Node16* c_Node16::m_new2(){
	return this;
}
int c_Node16::p_Remove(){
	gc_assign(m__succ->m__pred,m__pred);
	gc_assign(m__pred->m__succ,m__succ);
	return 0;
}
void c_Node16::mark(){
	Object::mark();
	gc_mark_q(m__succ);
	gc_mark_q(m__pred);
	gc_mark_q(m__data);
}
c_HeadNode9::c_HeadNode9(){
}
c_HeadNode9* c_HeadNode9::m_new(){
	c_Node16::m_new2();
	gc_assign(m__succ,(this));
	gc_assign(m__pred,(this));
	return this;
}
void c_HeadNode9::mark(){
	c_Node16::mark();
}
c_Enumerator5::c_Enumerator5(){
	m__list=0;
	m__curr=0;
}
c_Enumerator5* c_Enumerator5::m_new(c_List5* t_list){
	gc_assign(m__list,t_list);
	gc_assign(m__curr,t_list->m__head->m__succ);
	return this;
}
c_Enumerator5* c_Enumerator5::m_new2(){
	return this;
}
bool c_Enumerator5::p_HasNext(){
	while(m__curr->m__succ->m__pred!=m__curr){
		gc_assign(m__curr,m__curr->m__succ);
	}
	return m__curr!=m__list->m__head;
}
c_Stmt* c_Enumerator5::p_NextObject(){
	c_Stmt* t_data=m__curr->m__data;
	gc_assign(m__curr,m__curr->m__succ);
	return t_data;
}
void c_Enumerator5::mark(){
	Object::mark();
	gc_mark_q(m__list);
	gc_mark_q(m__curr);
}
c_InvokeExpr::c_InvokeExpr(){
	m_decl=0;
	m_args=Array<c_Expr* >();
}
c_InvokeExpr* c_InvokeExpr::m_new(c_FuncDecl* t_decl,Array<c_Expr* > t_args){
	c_Expr::m_new();
	gc_assign(this->m_decl,t_decl);
	gc_assign(this->m_args,t_args);
	return this;
}
c_InvokeExpr* c_InvokeExpr::m_new2(){
	c_Expr::m_new();
	return this;
}
c_Expr* c_InvokeExpr::p_Semant(){
	if((m_exprType)!=0){
		return (this);
	}
	gc_assign(m_exprType,m_decl->m_retType);
	gc_assign(m_args,p_CastArgs(m_args,m_decl));
	return (this);
}
String c_InvokeExpr::p_ToString(){
	String t_t=String(L"InvokeExpr(",11)+m_decl->p_ToString();
	Array<c_Expr* > t_=m_args;
	int t_2=0;
	while(t_2<t_.Length()){
		c_Expr* t_arg=t_[t_2];
		t_2=t_2+1;
		t_t=t_t+(String(L",",1)+t_arg->p_ToString());
	}
	return t_t+String(L")",1);
}
String c_InvokeExpr::p_Trans(){
	return bb_translator__trans->p_TransInvokeExpr(this);
}
String c_InvokeExpr::p_TransStmt(){
	return bb_translator__trans->p_TransInvokeExpr(this);
}
void c_InvokeExpr::mark(){
	c_Expr::mark();
	gc_mark_q(m_decl);
	gc_mark_q(m_args);
}
c_StmtExpr::c_StmtExpr(){
	m_stmt=0;
	m_expr=0;
}
c_StmtExpr* c_StmtExpr::m_new(c_Stmt* t_stmt,c_Expr* t_expr){
	c_Expr::m_new();
	gc_assign(this->m_stmt,t_stmt);
	gc_assign(this->m_expr,t_expr);
	return this;
}
c_StmtExpr* c_StmtExpr::m_new2(){
	c_Expr::m_new();
	return this;
}
c_Expr* c_StmtExpr::p_Semant(){
	if((m_exprType)!=0){
		return (this);
	}
	m_stmt->p_Semant();
	gc_assign(m_expr,m_expr->p_Semant());
	gc_assign(m_exprType,m_expr->m_exprType);
	return (this);
}
c_Expr* c_StmtExpr::p_Copy(){
	return ((new c_StmtExpr)->m_new(m_stmt,p_CopyExpr(m_expr)));
}
String c_StmtExpr::p_ToString(){
	return String(L"StmtExpr(,",10)+m_expr->p_ToString()+String(L")",1);
}
String c_StmtExpr::p_Trans(){
	return bb_translator__trans->p_TransStmtExpr(this);
}
void c_StmtExpr::mark(){
	c_Expr::mark();
	gc_mark_q(m_stmt);
	gc_mark_q(m_expr);
}
c_MemberVarExpr::c_MemberVarExpr(){
	m_expr=0;
	m_decl=0;
}
c_MemberVarExpr* c_MemberVarExpr::m_new(c_Expr* t_expr,c_VarDecl* t_decl){
	c_Expr::m_new();
	gc_assign(this->m_expr,t_expr);
	gc_assign(this->m_decl,t_decl);
	return this;
}
c_MemberVarExpr* c_MemberVarExpr::m_new2(){
	c_Expr::m_new();
	return this;
}
c_Expr* c_MemberVarExpr::p_Semant(){
	if((m_exprType)!=0){
		return (this);
	}
	if(!((m_decl->p_IsSemanted())!=0)){
		bb_config_InternalErr(String(L"Internal error",14));
	}
	gc_assign(m_exprType,m_decl->m_type);
	return (this);
}
String c_MemberVarExpr::p_ToString(){
	return String(L"MemberVarExpr(",14)+m_expr->p_ToString()+String(L",",1)+m_decl->p_ToString()+String(L")",1);
}
bool c_MemberVarExpr::p_SideEffects(){
	return m_expr->p_SideEffects();
}
c_Expr* c_MemberVarExpr::p_SemantSet(String t_op,c_Expr* t_rhs){
	return p_Semant();
}
String c_MemberVarExpr::p_Trans(){
	return bb_translator__trans->p_TransMemberVarExpr(this);
}
String c_MemberVarExpr::p_TransVar(){
	return bb_translator__trans->p_TransMemberVarExpr(this);
}
void c_MemberVarExpr::mark(){
	c_Expr::mark();
	gc_mark_q(m_expr);
	gc_mark_q(m_decl);
}
c_VarExpr::c_VarExpr(){
	m_decl=0;
}
c_VarExpr* c_VarExpr::m_new(c_VarDecl* t_decl){
	c_Expr::m_new();
	gc_assign(this->m_decl,t_decl);
	return this;
}
c_VarExpr* c_VarExpr::m_new2(){
	c_Expr::m_new();
	return this;
}
c_Expr* c_VarExpr::p_Semant(){
	if((m_exprType)!=0){
		return (this);
	}
	if(!((m_decl->p_IsSemanted())!=0)){
		bb_config_InternalErr(String(L"Internal error",14));
	}
	gc_assign(m_exprType,m_decl->m_type);
	return (this);
}
String c_VarExpr::p_ToString(){
	return String(L"VarExpr(",8)+m_decl->p_ToString()+String(L")",1);
}
bool c_VarExpr::p_SideEffects(){
	return false;
}
c_Expr* c_VarExpr::p_SemantSet(String t_op,c_Expr* t_rhs){
	return p_Semant();
}
String c_VarExpr::p_Trans(){
	p_Semant();
	return bb_translator__trans->p_TransVarExpr(this);
}
String c_VarExpr::p_TransVar(){
	p_Semant();
	return bb_translator__trans->p_TransVarExpr(this);
}
void c_VarExpr::mark(){
	c_Expr::mark();
	gc_mark_q(m_decl);
}
int bb_decl__loopnest;
c_Map8::c_Map8(){
	m_root=0;
}
c_Map8* c_Map8::m_new(){
	return this;
}
c_Node17* c_Map8::p_FindNode(String t_key){
	c_Node17* t_node=m_root;
	while((t_node)!=0){
		int t_cmp=p_Compare(t_key,t_node->m_key);
		if(t_cmp>0){
			t_node=t_node->m_right;
		}else{
			if(t_cmp<0){
				t_node=t_node->m_left;
			}else{
				return t_node;
			}
		}
	}
	return t_node;
}
c_FuncDeclList* c_Map8::p_Get(String t_key){
	c_Node17* t_node=p_FindNode(t_key);
	if((t_node)!=0){
		return t_node->m_value;
	}
	return 0;
}
int c_Map8::p_RotateLeft8(c_Node17* t_node){
	c_Node17* t_child=t_node->m_right;
	gc_assign(t_node->m_right,t_child->m_left);
	if((t_child->m_left)!=0){
		gc_assign(t_child->m_left->m_parent,t_node);
	}
	gc_assign(t_child->m_parent,t_node->m_parent);
	if((t_node->m_parent)!=0){
		if(t_node==t_node->m_parent->m_left){
			gc_assign(t_node->m_parent->m_left,t_child);
		}else{
			gc_assign(t_node->m_parent->m_right,t_child);
		}
	}else{
		gc_assign(m_root,t_child);
	}
	gc_assign(t_child->m_left,t_node);
	gc_assign(t_node->m_parent,t_child);
	return 0;
}
int c_Map8::p_RotateRight8(c_Node17* t_node){
	c_Node17* t_child=t_node->m_left;
	gc_assign(t_node->m_left,t_child->m_right);
	if((t_child->m_right)!=0){
		gc_assign(t_child->m_right->m_parent,t_node);
	}
	gc_assign(t_child->m_parent,t_node->m_parent);
	if((t_node->m_parent)!=0){
		if(t_node==t_node->m_parent->m_right){
			gc_assign(t_node->m_parent->m_right,t_child);
		}else{
			gc_assign(t_node->m_parent->m_left,t_child);
		}
	}else{
		gc_assign(m_root,t_child);
	}
	gc_assign(t_child->m_right,t_node);
	gc_assign(t_node->m_parent,t_child);
	return 0;
}
int c_Map8::p_InsertFixup8(c_Node17* t_node){
	while(((t_node->m_parent)!=0) && t_node->m_parent->m_color==-1 && ((t_node->m_parent->m_parent)!=0)){
		if(t_node->m_parent==t_node->m_parent->m_parent->m_left){
			c_Node17* t_uncle=t_node->m_parent->m_parent->m_right;
			if(((t_uncle)!=0) && t_uncle->m_color==-1){
				t_node->m_parent->m_color=1;
				t_uncle->m_color=1;
				t_uncle->m_parent->m_color=-1;
				t_node=t_uncle->m_parent;
			}else{
				if(t_node==t_node->m_parent->m_right){
					t_node=t_node->m_parent;
					p_RotateLeft8(t_node);
				}
				t_node->m_parent->m_color=1;
				t_node->m_parent->m_parent->m_color=-1;
				p_RotateRight8(t_node->m_parent->m_parent);
			}
		}else{
			c_Node17* t_uncle2=t_node->m_parent->m_parent->m_left;
			if(((t_uncle2)!=0) && t_uncle2->m_color==-1){
				t_node->m_parent->m_color=1;
				t_uncle2->m_color=1;
				t_uncle2->m_parent->m_color=-1;
				t_node=t_uncle2->m_parent;
			}else{
				if(t_node==t_node->m_parent->m_left){
					t_node=t_node->m_parent;
					p_RotateRight8(t_node);
				}
				t_node->m_parent->m_color=1;
				t_node->m_parent->m_parent->m_color=-1;
				p_RotateLeft8(t_node->m_parent->m_parent);
			}
		}
	}
	m_root->m_color=1;
	return 0;
}
bool c_Map8::p_Set8(String t_key,c_FuncDeclList* t_value){
	c_Node17* t_node=m_root;
	c_Node17* t_parent=0;
	int t_cmp=0;
	while((t_node)!=0){
		t_parent=t_node;
		t_cmp=p_Compare(t_key,t_node->m_key);
		if(t_cmp>0){
			t_node=t_node->m_right;
		}else{
			if(t_cmp<0){
				t_node=t_node->m_left;
			}else{
				gc_assign(t_node->m_value,t_value);
				return false;
			}
		}
	}
	t_node=(new c_Node17)->m_new(t_key,t_value,-1,t_parent);
	if((t_parent)!=0){
		if(t_cmp>0){
			gc_assign(t_parent->m_right,t_node);
		}else{
			gc_assign(t_parent->m_left,t_node);
		}
		p_InsertFixup8(t_node);
	}else{
		gc_assign(m_root,t_node);
	}
	return true;
}
void c_Map8::mark(){
	Object::mark();
	gc_mark_q(m_root);
}
c_StringMap8::c_StringMap8(){
}
c_StringMap8* c_StringMap8::m_new(){
	c_Map8::m_new();
	return this;
}
int c_StringMap8::p_Compare(String t_lhs,String t_rhs){
	return t_lhs.Compare(t_rhs);
}
void c_StringMap8::mark(){
	c_Map8::mark();
}
c_Node17::c_Node17(){
	m_key=String();
	m_right=0;
	m_left=0;
	m_value=0;
	m_color=0;
	m_parent=0;
}
c_Node17* c_Node17::m_new(String t_key,c_FuncDeclList* t_value,int t_color,c_Node17* t_parent){
	this->m_key=t_key;
	gc_assign(this->m_value,t_value);
	this->m_color=t_color;
	gc_assign(this->m_parent,t_parent);
	return this;
}
c_Node17* c_Node17::m_new2(){
	return this;
}
void c_Node17::mark(){
	Object::mark();
	gc_mark_q(m_right);
	gc_mark_q(m_left);
	gc_mark_q(m_value);
	gc_mark_q(m_parent);
}
c_Map9::c_Map9(){
	m_root=0;
}
c_Map9* c_Map9::m_new(){
	return this;
}
c_Node18* c_Map9::p_FindNode(String t_key){
	c_Node18* t_node=m_root;
	while((t_node)!=0){
		int t_cmp=p_Compare(t_key,t_node->m_key);
		if(t_cmp>0){
			t_node=t_node->m_right;
		}else{
			if(t_cmp<0){
				t_node=t_node->m_left;
			}else{
				return t_node;
			}
		}
	}
	return t_node;
}
bool c_Map9::p_Contains(String t_key){
	return p_FindNode(t_key)!=0;
}
int c_Map9::p_RotateLeft9(c_Node18* t_node){
	c_Node18* t_child=t_node->m_right;
	gc_assign(t_node->m_right,t_child->m_left);
	if((t_child->m_left)!=0){
		gc_assign(t_child->m_left->m_parent,t_node);
	}
	gc_assign(t_child->m_parent,t_node->m_parent);
	if((t_node->m_parent)!=0){
		if(t_node==t_node->m_parent->m_left){
			gc_assign(t_node->m_parent->m_left,t_child);
		}else{
			gc_assign(t_node->m_parent->m_right,t_child);
		}
	}else{
		gc_assign(m_root,t_child);
	}
	gc_assign(t_child->m_left,t_node);
	gc_assign(t_node->m_parent,t_child);
	return 0;
}
int c_Map9::p_RotateRight9(c_Node18* t_node){
	c_Node18* t_child=t_node->m_left;
	gc_assign(t_node->m_left,t_child->m_right);
	if((t_child->m_right)!=0){
		gc_assign(t_child->m_right->m_parent,t_node);
	}
	gc_assign(t_child->m_parent,t_node->m_parent);
	if((t_node->m_parent)!=0){
		if(t_node==t_node->m_parent->m_right){
			gc_assign(t_node->m_parent->m_right,t_child);
		}else{
			gc_assign(t_node->m_parent->m_left,t_child);
		}
	}else{
		gc_assign(m_root,t_child);
	}
	gc_assign(t_child->m_right,t_node);
	gc_assign(t_node->m_parent,t_child);
	return 0;
}
int c_Map9::p_InsertFixup9(c_Node18* t_node){
	while(((t_node->m_parent)!=0) && t_node->m_parent->m_color==-1 && ((t_node->m_parent->m_parent)!=0)){
		if(t_node->m_parent==t_node->m_parent->m_parent->m_left){
			c_Node18* t_uncle=t_node->m_parent->m_parent->m_right;
			if(((t_uncle)!=0) && t_uncle->m_color==-1){
				t_node->m_parent->m_color=1;
				t_uncle->m_color=1;
				t_uncle->m_parent->m_color=-1;
				t_node=t_uncle->m_parent;
			}else{
				if(t_node==t_node->m_parent->m_right){
					t_node=t_node->m_parent;
					p_RotateLeft9(t_node);
				}
				t_node->m_parent->m_color=1;
				t_node->m_parent->m_parent->m_color=-1;
				p_RotateRight9(t_node->m_parent->m_parent);
			}
		}else{
			c_Node18* t_uncle2=t_node->m_parent->m_parent->m_left;
			if(((t_uncle2)!=0) && t_uncle2->m_color==-1){
				t_node->m_parent->m_color=1;
				t_uncle2->m_color=1;
				t_uncle2->m_parent->m_color=-1;
				t_node=t_uncle2->m_parent;
			}else{
				if(t_node==t_node->m_parent->m_left){
					t_node=t_node->m_parent;
					p_RotateRight9(t_node);
				}
				t_node->m_parent->m_color=1;
				t_node->m_parent->m_parent->m_color=-1;
				p_RotateLeft9(t_node->m_parent->m_parent);
			}
		}
	}
	m_root->m_color=1;
	return 0;
}
bool c_Map9::p_Set9(String t_key,c_FuncDecl* t_value){
	c_Node18* t_node=m_root;
	c_Node18* t_parent=0;
	int t_cmp=0;
	while((t_node)!=0){
		t_parent=t_node;
		t_cmp=p_Compare(t_key,t_node->m_key);
		if(t_cmp>0){
			t_node=t_node->m_right;
		}else{
			if(t_cmp<0){
				t_node=t_node->m_left;
			}else{
				gc_assign(t_node->m_value,t_value);
				return false;
			}
		}
	}
	t_node=(new c_Node18)->m_new(t_key,t_value,-1,t_parent);
	if((t_parent)!=0){
		if(t_cmp>0){
			gc_assign(t_parent->m_right,t_node);
		}else{
			gc_assign(t_parent->m_left,t_node);
		}
		p_InsertFixup9(t_node);
	}else{
		gc_assign(m_root,t_node);
	}
	return true;
}
void c_Map9::mark(){
	Object::mark();
	gc_mark_q(m_root);
}
c_StringMap9::c_StringMap9(){
}
c_StringMap9* c_StringMap9::m_new(){
	c_Map9::m_new();
	return this;
}
int c_StringMap9::p_Compare(String t_lhs,String t_rhs){
	return t_lhs.Compare(t_rhs);
}
void c_StringMap9::mark(){
	c_Map9::mark();
}
c_Node18::c_Node18(){
	m_key=String();
	m_right=0;
	m_left=0;
	m_value=0;
	m_color=0;
	m_parent=0;
}
c_Node18* c_Node18::m_new(String t_key,c_FuncDecl* t_value,int t_color,c_Node18* t_parent){
	this->m_key=t_key;
	gc_assign(this->m_value,t_value);
	this->m_color=t_color;
	gc_assign(this->m_parent,t_parent);
	return this;
}
c_Node18* c_Node18::m_new2(){
	return this;
}
void c_Node18::mark(){
	Object::mark();
	gc_mark_q(m_right);
	gc_mark_q(m_left);
	gc_mark_q(m_value);
	gc_mark_q(m_parent);
}
c_Map10::c_Map10(){
	m_root=0;
}
c_Map10* c_Map10::m_new(){
	return this;
}
c_Node19* c_Map10::p_FindNode(String t_key){
	c_Node19* t_node=m_root;
	while((t_node)!=0){
		int t_cmp=p_Compare(t_key,t_node->m_key);
		if(t_cmp>0){
			t_node=t_node->m_right;
		}else{
			if(t_cmp<0){
				t_node=t_node->m_left;
			}else{
				return t_node;
			}
		}
	}
	return t_node;
}
c_StringSet* c_Map10::p_Get(String t_key){
	c_Node19* t_node=p_FindNode(t_key);
	if((t_node)!=0){
		return t_node->m_value;
	}
	return 0;
}
int c_Map10::p_RotateLeft10(c_Node19* t_node){
	c_Node19* t_child=t_node->m_right;
	gc_assign(t_node->m_right,t_child->m_left);
	if((t_child->m_left)!=0){
		gc_assign(t_child->m_left->m_parent,t_node);
	}
	gc_assign(t_child->m_parent,t_node->m_parent);
	if((t_node->m_parent)!=0){
		if(t_node==t_node->m_parent->m_left){
			gc_assign(t_node->m_parent->m_left,t_child);
		}else{
			gc_assign(t_node->m_parent->m_right,t_child);
		}
	}else{
		gc_assign(m_root,t_child);
	}
	gc_assign(t_child->m_left,t_node);
	gc_assign(t_node->m_parent,t_child);
	return 0;
}
int c_Map10::p_RotateRight10(c_Node19* t_node){
	c_Node19* t_child=t_node->m_left;
	gc_assign(t_node->m_left,t_child->m_right);
	if((t_child->m_right)!=0){
		gc_assign(t_child->m_right->m_parent,t_node);
	}
	gc_assign(t_child->m_parent,t_node->m_parent);
	if((t_node->m_parent)!=0){
		if(t_node==t_node->m_parent->m_right){
			gc_assign(t_node->m_parent->m_right,t_child);
		}else{
			gc_assign(t_node->m_parent->m_left,t_child);
		}
	}else{
		gc_assign(m_root,t_child);
	}
	gc_assign(t_child->m_right,t_node);
	gc_assign(t_node->m_parent,t_child);
	return 0;
}
int c_Map10::p_InsertFixup10(c_Node19* t_node){
	while(((t_node->m_parent)!=0) && t_node->m_parent->m_color==-1 && ((t_node->m_parent->m_parent)!=0)){
		if(t_node->m_parent==t_node->m_parent->m_parent->m_left){
			c_Node19* t_uncle=t_node->m_parent->m_parent->m_right;
			if(((t_uncle)!=0) && t_uncle->m_color==-1){
				t_node->m_parent->m_color=1;
				t_uncle->m_color=1;
				t_uncle->m_parent->m_color=-1;
				t_node=t_uncle->m_parent;
			}else{
				if(t_node==t_node->m_parent->m_right){
					t_node=t_node->m_parent;
					p_RotateLeft10(t_node);
				}
				t_node->m_parent->m_color=1;
				t_node->m_parent->m_parent->m_color=-1;
				p_RotateRight10(t_node->m_parent->m_parent);
			}
		}else{
			c_Node19* t_uncle2=t_node->m_parent->m_parent->m_left;
			if(((t_uncle2)!=0) && t_uncle2->m_color==-1){
				t_node->m_parent->m_color=1;
				t_uncle2->m_color=1;
				t_uncle2->m_parent->m_color=-1;
				t_node=t_uncle2->m_parent;
			}else{
				if(t_node==t_node->m_parent->m_left){
					t_node=t_node->m_parent;
					p_RotateRight10(t_node);
				}
				t_node->m_parent->m_color=1;
				t_node->m_parent->m_parent->m_color=-1;
				p_RotateLeft10(t_node->m_parent->m_parent);
			}
		}
	}
	m_root->m_color=1;
	return 0;
}
bool c_Map10::p_Set10(String t_key,c_StringSet* t_value){
	c_Node19* t_node=m_root;
	c_Node19* t_parent=0;
	int t_cmp=0;
	while((t_node)!=0){
		t_parent=t_node;
		t_cmp=p_Compare(t_key,t_node->m_key);
		if(t_cmp>0){
			t_node=t_node->m_right;
		}else{
			if(t_cmp<0){
				t_node=t_node->m_left;
			}else{
				gc_assign(t_node->m_value,t_value);
				return false;
			}
		}
	}
	t_node=(new c_Node19)->m_new(t_key,t_value,-1,t_parent);
	if((t_parent)!=0){
		if(t_cmp>0){
			gc_assign(t_parent->m_right,t_node);
		}else{
			gc_assign(t_parent->m_left,t_node);
		}
		p_InsertFixup10(t_node);
	}else{
		gc_assign(m_root,t_node);
	}
	return true;
}
void c_Map10::mark(){
	Object::mark();
	gc_mark_q(m_root);
}
c_StringMap10::c_StringMap10(){
}
c_StringMap10* c_StringMap10::m_new(){
	c_Map10::m_new();
	return this;
}
int c_StringMap10::p_Compare(String t_lhs,String t_rhs){
	return t_lhs.Compare(t_rhs);
}
void c_StringMap10::mark(){
	c_Map10::mark();
}
c_Node19::c_Node19(){
	m_key=String();
	m_right=0;
	m_left=0;
	m_value=0;
	m_color=0;
	m_parent=0;
}
c_Node19* c_Node19::m_new(String t_key,c_StringSet* t_value,int t_color,c_Node19* t_parent){
	this->m_key=t_key;
	gc_assign(this->m_value,t_value);
	this->m_color=t_color;
	gc_assign(this->m_parent,t_parent);
	return this;
}
c_Node19* c_Node19::m_new2(){
	return this;
}
void c_Node19::mark(){
	Object::mark();
	gc_mark_q(m_right);
	gc_mark_q(m_left);
	gc_mark_q(m_value);
	gc_mark_q(m_parent);
}
c_Enumerator6::c_Enumerator6(){
	m__list=0;
	m__curr=0;
}
c_Enumerator6* c_Enumerator6::m_new(c_List8* t_list){
	gc_assign(m__list,t_list);
	gc_assign(m__curr,t_list->m__head->m__succ);
	return this;
}
c_Enumerator6* c_Enumerator6::m_new2(){
	return this;
}
bool c_Enumerator6::p_HasNext(){
	while(m__curr->m__succ->m__pred!=m__curr){
		gc_assign(m__curr,m__curr->m__succ);
	}
	return m__curr!=m__list->m__head;
}
c_GlobalDecl* c_Enumerator6::p_NextObject(){
	c_GlobalDecl* t_data=m__curr->m__data;
	gc_assign(m__curr,m__curr->m__succ);
	return t_data;
}
void c_Enumerator6::mark(){
	Object::mark();
	gc_mark_q(m__list);
	gc_mark_q(m__curr);
}
c_Stack9::c_Stack9(){
	m_data=Array<c_LocalDecl* >();
	m_length=0;
}
c_Stack9* c_Stack9::m_new(){
	return this;
}
c_Stack9* c_Stack9::m_new2(Array<c_LocalDecl* > t_data){
	gc_assign(this->m_data,t_data.Slice(0));
	this->m_length=t_data.Length();
	return this;
}
c_LocalDecl* c_Stack9::m_NIL;
void c_Stack9::p_Clear(){
	for(int t_i=0;t_i<m_length;t_i=t_i+1){
		gc_assign(m_data[t_i],m_NIL);
	}
	m_length=0;
}
c_Enumerator7* c_Stack9::p_ObjectEnumerator(){
	return (new c_Enumerator7)->m_new(this);
}
void c_Stack9::p_Length(int t_newlength){
	if(t_newlength<m_length){
		for(int t_i=t_newlength;t_i<m_length;t_i=t_i+1){
			gc_assign(m_data[t_i],m_NIL);
		}
	}else{
		if(t_newlength>m_data.Length()){
			gc_assign(m_data,m_data.Resize(bb_math_Max(m_length*2+10,t_newlength)));
		}
	}
	m_length=t_newlength;
}
int c_Stack9::p_Length2(){
	return m_length;
}
void c_Stack9::p_Push25(c_LocalDecl* t_value){
	if(m_length==m_data.Length()){
		gc_assign(m_data,m_data.Resize(m_length*2+10));
	}
	gc_assign(m_data[m_length],t_value);
	m_length+=1;
}
void c_Stack9::p_Push26(Array<c_LocalDecl* > t_values,int t_offset,int t_count){
	for(int t_i=0;t_i<t_count;t_i=t_i+1){
		p_Push25(t_values[t_offset+t_i]);
	}
}
void c_Stack9::p_Push27(Array<c_LocalDecl* > t_values,int t_offset){
	p_Push26(t_values,t_offset,t_values.Length()-t_offset);
}
void c_Stack9::mark(){
	Object::mark();
	gc_mark_q(m_data);
}
c_Enumerator7::c_Enumerator7(){
	m_stack=0;
	m_index=0;
}
c_Enumerator7* c_Enumerator7::m_new(c_Stack9* t_stack){
	gc_assign(this->m_stack,t_stack);
	return this;
}
c_Enumerator7* c_Enumerator7::m_new2(){
	return this;
}
bool c_Enumerator7::p_HasNext(){
	return m_index<m_stack->p_Length2();
}
c_LocalDecl* c_Enumerator7::p_NextObject(){
	m_index+=1;
	return m_stack->m_data[m_index-1];
}
void c_Enumerator7::mark(){
	Object::mark();
	gc_mark_q(m_stack);
}
int bbInit(){
	GC_CTOR
	c_Type::m_stringType=(new c_StringType)->m_new();
	bb_config__errInfo=String();
	bb_config__cfgScope=(new c_ConfigScope)->m_new();
	bb_config__cfgScopeStack=(new c_Stack2)->m_new();
	bb_decl__env=0;
	bb_decl__envStack=(new c_List2)->m_new();
	c_Toker::m__keywords=0;
	c_Toker::m__symbols=0;
	bb_config_ENV_MODPATH=String();
	bb_parser_FILE_EXT=String(L".cxs",4);
	bb_parser_FILE_EXT_OLD=String(L".monkey",7);
	bb_config_ENV_SAFEMODE=0;
	c_Type::m_intType=(new c_IntType)->m_new();
	c_Type::m_floatType=(new c_FloatType)->m_new();
	c_Type::m_boolType=(new c_BoolType)->m_new();
	c_Type::m_voidType=(new c_VoidType)->m_new();
	c_Type::m_objectType=(new c_IdentType)->m_new(String(L"cerberus.object",15),Array<c_Type* >());
	c_Type::m_throwableType=(new c_IdentType)->m_new(String(L"cerberus.throwable",18),Array<c_Type* >());
	c_Type::m_emptyArrayType=(new c_ArrayType)->m_new(c_Type::m_voidType);
	c_Type::m_nullObjectType=(new c_IdentType)->m_new(String(),Array<c_Type* >());
	c_Stack7::m_NIL=0;
	bb_config__errStack=(new c_StringList)->m_new2();
	c_Stack2::m_NIL=0;
	bb_config_ENV_HOST=String();
	bb_config_ENV_CONFIG=String();
	bb_config_ENV_TARGET=String();
	bb_config_ENV_LANG=String();
	c_Stack8::m_NIL=0;
	c_Stack::m_NIL=String();
	bb_translator__trans=0;
	bb_html5_Info_Width=0;
	bb_html5_Info_Height=0;
	c_ClassDecl::m_nullObjectClass=(new c_ClassDecl)->m_new(String(L"{NULL}",6),1280,Array<String >(),0,Array<c_IdentType* >());
	bb_decl__loopnest=0;
	c_Stack9::m_NIL=0;
	return 0;
}
void gc_mark(){
	gc_mark_q(c_Type::m_stringType);
	gc_mark_q(bb_config__cfgScope);
	gc_mark_q(bb_config__cfgScopeStack);
	gc_mark_q(bb_decl__env);
	gc_mark_q(bb_decl__envStack);
	gc_mark_q(c_Toker::m__keywords);
	gc_mark_q(c_Toker::m__symbols);
	gc_mark_q(c_Type::m_intType);
	gc_mark_q(c_Type::m_floatType);
	gc_mark_q(c_Type::m_boolType);
	gc_mark_q(c_Type::m_voidType);
	gc_mark_q(c_Type::m_objectType);
	gc_mark_q(c_Type::m_throwableType);
	gc_mark_q(c_Type::m_emptyArrayType);
	gc_mark_q(c_Type::m_nullObjectType);
	gc_mark_q(c_Stack7::m_NIL);
	gc_mark_q(bb_config__errStack);
	gc_mark_q(c_Stack2::m_NIL);
	gc_mark_q(c_Stack8::m_NIL);
	gc_mark_q(bb_translator__trans);
	gc_mark_q(c_ClassDecl::m_nullObjectClass);
	gc_mark_q(c_Stack9::m_NIL);
}
//${TRANSCODE_END}

String BBPathToFilePath( String path ){
	return path;
}

int main( int argc,const char **argv ){

	new BBGame();

	try{
	
		bb_std_main( argc,argv );
		
	}catch( ThrowableObject *ex ){
	
		bbPrint( "Cerberus Runtime Error : Uncaught Cerberus Exception" );
	
	}catch( const char *err ){
	
	}
}
