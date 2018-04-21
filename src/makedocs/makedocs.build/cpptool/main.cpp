
#include "main.h"

//${CONFIG_BEGIN}
#define CFG_BRL_OS_IMPLEMENTED 1
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
	
	virtual int GetDeviceWidth(){ return 0; }
	virtual int GetDeviceHeight(){ return 0; }
	virtual void SetDeviceWindow( int width,int height,int flags ){}
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

class c_Set;
class c_StringSet;
class c_Map;
class c_StringMap;
class c_Node;
class c_List;
class c_StringList;
class c_Node2;
class c_HeadNode;
class c_Enumerator;
class c_ILinkResolver;
class c_IPrettifier;
class c_George;
class c_ApiDoccer;
class c_DocsDoccer;
class c_PageMaker;
class c_Stack;
class c_Stack2;
class c_StringStack;
class c_Map2;
class c_StringMap2;
class c_Node3;
class c_Parser;
class c_Toker;
class c_Decl;
class c_Decl2;
class c_ScopeDecl;
class c_ModuleDecl;
class c_Map3;
class c_StringMap3;
class c_Node4;
class c_Stack3;
class c_AliasDecl;
class c_Map4;
class c_StringMap4;
class c_Node5;
class c_Map5;
class c_StringMap5;
class c_Node6;
class c_Map6;
class c_StringMap6;
class c_Node7;
class c_ImportDecl;
class c_ClassDecl;
class c_Map7;
class c_StringMap7;
class c_Node8;
class c_NodeEnumerator;
class c_NodeEnumerator2;
class c_Stack4;
class c_StringObject;
class c_NodeEnumerator3;
class c_Stack5;
class c_IntStack;
class c_NodeEnumerator4;
class c_Stack6;
class c_Enumerator2;
class c_MapValues;
class c_ValueEnumerator;
class c_Markdown;
class c_Enumerator3;
class c_NodeEnumerator5;
class c_NodeEnumerator6;
String bb_os_ExtractDir(String);
class c_Set : public Object{
	public:
	c_Map* m_map;
	c_Set();
	c_Set* m_new(c_Map*);
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
class c_Map : public Object{
	public:
	c_Node* m_root;
	c_Map();
	c_Map* m_new();
	virtual int p_Compare(String,String)=0;
	int p_RotateLeft(c_Node*);
	int p_RotateRight(c_Node*);
	int p_InsertFixup(c_Node*);
	bool p_Set(String,Object*);
	bool p_Insert2(String,Object*);
	c_Node* p_FindNode(String);
	bool p_Contains(String);
	int p_Clear();
	Object* p_Get(String);
	void mark();
};
class c_StringMap : public c_Map{
	public:
	c_StringMap();
	c_StringMap* m_new();
	int p_Compare(String,String);
	void mark();
};
extern c_StringSet* bb_apidoccer_ignore_mods;
class c_Node : public Object{
	public:
	String m_key;
	c_Node* m_right;
	c_Node* m_left;
	Object* m_value;
	int m_color;
	c_Node* m_parent;
	c_Node();
	c_Node* m_new(String,Object*,int,c_Node*);
	c_Node* m_new2();
	void mark();
};
class c_List : public Object{
	public:
	c_Node2* m__head;
	c_List();
	c_List* m_new();
	c_Node2* p_AddLast(String);
	c_List* m_new2(Array<String >);
	bool p_IsEmpty();
	String p_RemoveFirst();
	virtual bool p_Equals(String,String);
	c_Node2* p_Find(String,c_Node2*);
	c_Node2* p_Find2(String);
	void p_RemoveFirst2(String);
	int p_Count();
	c_Enumerator* p_ObjectEnumerator();
	Array<String > p_ToArray();
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
class c_Node2 : public Object{
	public:
	c_Node2* m__succ;
	c_Node2* m__pred;
	String m__data;
	c_Node2();
	c_Node2* m_new(c_Node2*,c_Node2*,String);
	c_Node2* m_new2();
	int p_Remove();
	void mark();
};
class c_HeadNode : public c_Node2{
	public:
	c_HeadNode();
	c_HeadNode* m_new();
	void mark();
};
class c_Enumerator : public Object{
	public:
	c_List* m__list;
	c_Node2* m__curr;
	c_Enumerator();
	c_Enumerator* m_new(c_List*);
	c_Enumerator* m_new2();
	bool p_HasNext();
	String p_NextObject();
	void mark();
};
Array<String > bb_os_LoadDir(String,bool,bool);
int bb_os_DeleteDir(String,bool);
int bb_os_CopyDir(String,String,bool,bool);
class c_ILinkResolver : public virtual gc_interface{
	public:
	virtual String p_ResolveLink(String,String)=0;
};
class c_IPrettifier : public virtual gc_interface{
	public:
	virtual String p_PrettifyLine(String)=0;
	virtual String p_EndPrettyBlock()=0;
	virtual String p_BeginPrettyBlock()=0;
};
class c_George : public Object,public virtual c_ILinkResolver,public virtual c_IPrettifier{
	public:
	String m_styledir;
	c_StringStack* m_iconImgs;
	c_StringStack* m_iconUrls;
	c_StringMap2* m_dataDirs;
	String m_errinfo;
	c_StringMap2* m_pages;
	c_StringMap6* m_indexcats;
	c_StringMap2* m_content;
	String m_docbase;
	int m_inrem;
	c_Toker* m_ptoker;
	c_George();
	c_George* m_new(String);
	c_George* m_new2();
	void p_AddIconLink(String,String);
	void p_SetErrInfo(String);
	String p_MakeUrl(String);
	c_StringMap2* p_GetIndex(String);
	void p_AddToIndex(String,String,String);
	void p_AddPage(String,String);
	void p_Err(String);
	void p_SetPageContent(String,String);
	void p_MakeIndices();
	String p_GetPageUrl(String);
	String p_MakeLink(String,String);
	String p_ResolveLink(String,String);
	void p_SetDocBase(String);
	void p_MakeDocs();
	int p_CopyDataDirs();
	String p_BeginPrettyBlock();
	String p_EndPrettyBlock();
	String p_HtmlEsc(String);
	static String m_CerberusKeywords;
	String p_PrettifyLine(String);
	void mark();
};
class c_ApiDoccer : public Object,public virtual c_ILinkResolver{
	public:
	c_George* m_george;
	c_PageMaker* m_scopeTemplate;
	c_StringMap5* m_scopes;
	c_ScopeDecl* m_linkScope;
	c_ApiDoccer();
	c_ApiDoccer* m_new(c_George*);
	c_ApiDoccer* m_new2();
	bool p_LoadExample(c_Decl2*,String);
	void p_EndSect(String,c_StringStack*,c_Decl2*);
	void p_ParseCerberusdocFile(String,String);
	void p_AddDocsToDecl(c_StringMap7*,c_Decl2*);
	void p_ParseCerberusFile(String,String);
	void p_ParseModules(String,String);
	void p_ParseDocs();
	void p_ResolveScopes();
	String p_Capitalize(String);
	String p_HtmlEsc(String);
	String p_StripLinks(String);
	void p_AddDecl(c_Decl2*,c_PageMaker*,c_Markdown*);
	String p_Pluralize(String);
	void p_MakeScopeDocs(c_ScopeDecl*,c_PageMaker*);
	void p_MakeDocs();
	String p_ResolveLink(String,String);
	void mark();
};
class c_DocsDoccer : public Object,public virtual c_ILinkResolver{
	public:
	c_George* m_george;
	c_StringMap2* m_docs;
	c_DocsDoccer();
	c_DocsDoccer* m_new(c_George*);
	c_DocsDoccer* m_new2();
	void p_ParseDocs2(String,String);
	void p_ParseDocs();
	void p_MakeDocs();
	String p_ResolveLink(String,String);
	void mark();
};
String bb_modpath_LoadModpath();
class c_PageMaker : public Object{
	public:
	String m__template;
	c_StringMap* m__decls;
	c_Stack* m__scopes;
	c_Stack4* m__lists;
	c_IntStack* m__iters;
	c_PageMaker();
	c_PageMaker* m_new(String);
	c_PageMaker* m_new2();
	void p_Clear();
	void p_SetString(String,String);
	void p_BeginList(String);
	void p_AddItem();
	void p_EndList();
	Object* p_GetValue(String);
	String p_GetString(String);
	c_Stack* p_GetList(String);
	String p_MakePage();
	void mark();
};
class c_Stack : public Object{
	public:
	Array<c_StringMap* > m_data;
	int m_length;
	c_Stack();
	c_Stack* m_new();
	c_Stack* m_new2(Array<c_StringMap* >);
	void p_Push(c_StringMap*);
	void p_Push2(Array<c_StringMap* >,int,int);
	void p_Push3(Array<c_StringMap* >,int);
	static c_StringMap* m_NIL;
	void p_Clear();
	c_StringMap* p_Top();
	c_StringMap* p_Pop();
	void p_Length(int);
	int p_Length2();
	c_StringMap* p_Get2(int);
	void mark();
};
String bb_os_ExtractExt(String);
String bb_os_StripExt(String);
class c_Stack2 : public Object{
	public:
	Array<String > m_data;
	int m_length;
	c_Stack2();
	c_Stack2* m_new();
	c_Stack2* m_new2(Array<String >);
	void p_Push4(String);
	void p_Push5(Array<String >,int,int);
	void p_Push6(Array<String >,int);
	Array<String > p_ToArray();
	bool p_IsEmpty();
	static String m_NIL;
	void p_Clear();
	void p_Length(int);
	int p_Length2();
	String p_Get2(int);
	void mark();
};
class c_StringStack : public c_Stack2{
	public:
	c_StringStack();
	c_StringStack* m_new(Array<String >);
	c_StringStack* m_new2();
	String p_Join(String);
	void mark();
};
class c_Map2 : public Object{
	public:
	c_Node3* m_root;
	c_Map2();
	c_Map2* m_new();
	virtual int p_Compare(String,String)=0;
	int p_RotateLeft2(c_Node3*);
	int p_RotateRight2(c_Node3*);
	int p_InsertFixup2(c_Node3*);
	bool p_Add(String,String);
	bool p_Set2(String,String);
	c_Node3* p_FindNode(String);
	bool p_Contains(String);
	c_Node3* p_FirstNode();
	c_NodeEnumerator3* p_ObjectEnumerator();
	String p_Get(String);
	void mark();
};
class c_StringMap2 : public c_Map2{
	public:
	c_StringMap2();
	c_StringMap2* m_new();
	int p_Compare(String,String);
	void mark();
};
class c_Node3 : public Object{
	public:
	String m_key;
	c_Node3* m_right;
	c_Node3* m_left;
	String m_value;
	int m_color;
	c_Node3* m_parent;
	c_Node3();
	c_Node3* m_new(String,String,int,c_Node3*);
	c_Node3* m_new2();
	c_Node3* p_NextNode();
	String p_Key();
	String p_Value();
	void mark();
};
String bb_os_StripDir(String);
class c_Parser : public Object{
	public:
	c_Toker* m__toker;
	String m__toke;
	int m__tokeType;
	c_Parser();
	String p_Bump();
	c_Parser* m_new(String);
	c_Parser* m_new2();
	void p_SetText(String);
	String p_Toke();
	String p_Parse();
	void p_Err(String);
	void p_Parse2(String);
	int p_TokeType();
	String p_Parse3(int);
	bool p_CParse(String);
	String p_CParse2(int);
	String p_ParseIdent();
	String p_ParseIdentSeq();
	String p_ParseType();
	String p_ParseTypeSeq();
	String p_ParseArgs();
	c_Decl* p_ParseDecl();
	c_Toker* p_GetToker();
	String p_GetText();
	void mark();
};
class c_Toker : public Object{
	public:
	String m__text;
	int m__len;
	String m__toke;
	int m__type;
	int m__pos;
	c_Toker();
	c_Toker* m_new(String);
	c_Toker* m_new2(c_Toker*);
	c_Toker* m_new3();
	int p_Chr(int);
	String p_Str(int);
	int p_IsAlpha(int);
	int p_IsDigit(int);
	int p_IsBinDigit(int);
	int p_IsHexDigit(int);
	String p_Bump();
	int p_TokeType();
	void p_SetText(String);
	String p_Text();
	int p_Cursor();
	String p_Toke();
	void mark();
};
class c_Decl : public Object{
	public:
	String m_kind;
	String m_ident;
	String m_type;
	String m_exts;
	String m_impls;
	String m_init;
	c_Decl();
	c_Decl* m_new(String,String);
	c_Decl* m_new2();
	void mark();
};
class c_Decl2 : public Object{
	public:
	String m_kind;
	String m_ident;
	String m_type;
	String m_texts;
	String m_timpls;
	c_ScopeDecl* m_scope;
	String m_path;
	String m_uident;
	c_StringMap2* m_docs;
	String m_egdir;
	String m_srcinfo;
	c_Decl2();
	void p_Init(String,String,String,String,String,c_ScopeDecl*);
	c_Decl2* m_new(c_Decl2*,c_ScopeDecl*);
	c_Decl2* m_new2(c_Decl*,c_ScopeDecl*);
	c_Decl2* m_new3();
	virtual String p_PagePath();
	virtual c_Decl2* p_FindDeclHere(String);
	c_Decl2* p_FindDecl(String);
	void mark();
};
class c_ScopeDecl : public c_Decl2{
	public:
	c_StringMap3* m_declsByUident;
	c_Stack3* m_decls;
	c_StringMap4* m_declsByKind;
	c_PageMaker* m_template;
	c_ScopeDecl();
	c_Stack3* p_GetDecls(String);
	c_ScopeDecl* m_new(c_Decl*,c_ScopeDecl*);
	c_ScopeDecl* m_new2();
	void p_SortDecls();
	c_Decl2* p_FindDeclHere(String);
	void mark();
};
class c_ModuleDecl : public c_ScopeDecl{
	public:
	c_ApiDoccer* m_doccer;
	bool m_busy;
	c_ModuleDecl();
	c_ModuleDecl* m_new(c_Decl*,c_ApiDoccer*);
	c_ModuleDecl* m_new2();
	c_Decl2* p_FindDeclHere(String);
	void mark();
};
class c_Map3 : public Object{
	public:
	c_Node4* m_root;
	c_Map3();
	c_Map3* m_new();
	virtual int p_Compare(String,String)=0;
	c_Node4* p_FindNode(String);
	bool p_Contains(String);
	int p_RotateLeft3(c_Node4*);
	int p_RotateRight3(c_Node4*);
	int p_InsertFixup3(c_Node4*);
	bool p_Set3(String,c_Decl2*);
	int p_Clear();
	c_Node4* p_FirstNode();
	c_NodeEnumerator6* p_ObjectEnumerator();
	c_Decl2* p_Get(String);
	void mark();
};
class c_StringMap3 : public c_Map3{
	public:
	c_StringMap3();
	c_StringMap3* m_new();
	int p_Compare(String,String);
	void mark();
};
class c_Node4 : public Object{
	public:
	String m_key;
	c_Node4* m_right;
	c_Node4* m_left;
	c_Decl2* m_value;
	int m_color;
	c_Node4* m_parent;
	c_Node4();
	c_Node4* m_new(String,c_Decl2*,int,c_Node4*);
	c_Node4* m_new2();
	c_Node4* p_NextNode();
	c_Decl2* p_Value();
	void mark();
};
class c_Stack3 : public Object{
	public:
	Array<c_Decl2* > m_data;
	int m_length;
	c_Stack3();
	c_Stack3* m_new();
	c_Stack3* m_new2(Array<c_Decl2* >);
	void p_Push7(c_Decl2*);
	void p_Push8(Array<c_Decl2* >,int,int);
	void p_Push9(Array<c_Decl2* >,int);
	c_Enumerator2* p_ObjectEnumerator();
	static c_Decl2* m_NIL;
	void p_Length(int);
	int p_Length2();
	void p_Clear();
	void mark();
};
class c_AliasDecl : public c_Decl2{
	public:
	c_Decl2* m_decl;
	c_AliasDecl();
	c_AliasDecl* m_new(c_Decl2*,c_ScopeDecl*);
	c_AliasDecl* m_new2();
	String p_PagePath();
	void mark();
};
class c_Map4 : public Object{
	public:
	c_Node5* m_root;
	c_Map4();
	c_Map4* m_new();
	virtual int p_Compare(String,String)=0;
	c_Node5* p_FindNode(String);
	c_Stack3* p_Get(String);
	int p_RotateLeft4(c_Node5*);
	int p_RotateRight4(c_Node5*);
	int p_InsertFixup4(c_Node5*);
	bool p_Set4(String,c_Stack3*);
	c_Node5* p_FirstNode();
	c_NodeEnumerator5* p_ObjectEnumerator();
	void mark();
};
class c_StringMap4 : public c_Map4{
	public:
	c_StringMap4();
	c_StringMap4* m_new();
	int p_Compare(String,String);
	void mark();
};
class c_Node5 : public Object{
	public:
	String m_key;
	c_Node5* m_right;
	c_Node5* m_left;
	c_Stack3* m_value;
	int m_color;
	c_Node5* m_parent;
	c_Node5();
	c_Node5* m_new(String,c_Stack3*,int,c_Node5*);
	c_Node5* m_new2();
	c_Node5* p_NextNode();
	c_Stack3* p_Value();
	String p_Key();
	void mark();
};
class c_Map5 : public Object{
	public:
	c_Node6* m_root;
	c_Map5();
	c_Map5* m_new();
	virtual int p_Compare(String,String)=0;
	int p_RotateLeft5(c_Node6*);
	int p_RotateRight5(c_Node6*);
	int p_InsertFixup5(c_Node6*);
	bool p_Set5(String,c_ScopeDecl*);
	c_Node6* p_FirstNode();
	c_NodeEnumerator4* p_ObjectEnumerator();
	c_MapValues* p_Values();
	c_Node6* p_FindNode(String);
	c_ScopeDecl* p_Get(String);
	void mark();
};
class c_StringMap5 : public c_Map5{
	public:
	c_StringMap5();
	c_StringMap5* m_new();
	int p_Compare(String,String);
	void mark();
};
class c_Node6 : public Object{
	public:
	String m_key;
	c_Node6* m_right;
	c_Node6* m_left;
	c_ScopeDecl* m_value;
	int m_color;
	c_Node6* m_parent;
	c_Node6();
	c_Node6* m_new(String,c_ScopeDecl*,int,c_Node6*);
	c_Node6* m_new2();
	c_Node6* p_NextNode();
	c_ScopeDecl* p_Value();
	void mark();
};
class c_Map6 : public Object{
	public:
	c_Node7* m_root;
	c_Map6();
	c_Map6* m_new();
	virtual int p_Compare(String,String)=0;
	c_Node7* p_FindNode(String);
	c_StringMap2* p_Get(String);
	int p_RotateLeft6(c_Node7*);
	int p_RotateRight6(c_Node7*);
	int p_InsertFixup6(c_Node7*);
	bool p_Set6(String,c_StringMap2*);
	c_Node7* p_FirstNode();
	c_NodeEnumerator2* p_ObjectEnumerator();
	void mark();
};
class c_StringMap6 : public c_Map6{
	public:
	c_StringMap6();
	c_StringMap6* m_new();
	int p_Compare(String,String);
	void mark();
};
class c_Node7 : public Object{
	public:
	String m_key;
	c_Node7* m_right;
	c_Node7* m_left;
	c_StringMap2* m_value;
	int m_color;
	c_Node7* m_parent;
	c_Node7();
	c_Node7* m_new(String,c_StringMap2*,int,c_Node7*);
	c_Node7* m_new2();
	c_Node7* p_NextNode();
	String p_Key();
	c_StringMap2* p_Value();
	void mark();
};
class c_ImportDecl : public c_Decl2{
	public:
	c_ImportDecl();
	c_ImportDecl* m_new(c_Decl*,c_ScopeDecl*);
	c_ImportDecl* m_new2();
	c_ModuleDecl* p_Resolve();
	String p_PagePath();
	c_Decl2* p_FindDeclHere(String);
	void mark();
};
class c_ClassDecl : public c_ScopeDecl{
	public:
	c_ClassDecl* m_exts;
	c_Stack6* m_extby;
	c_ClassDecl();
	c_ClassDecl* m_new(c_Decl*,c_ModuleDecl*);
	c_ClassDecl* m_new2();
	void p_SetSuper(c_ClassDecl*);
	c_Decl2* p_FindDeclHere(String);
	void mark();
};
class c_Map7 : public Object{
	public:
	c_Node8* m_root;
	c_Map7();
	c_Map7* m_new();
	virtual int p_Compare(String,String)=0;
	int p_RotateLeft7(c_Node8*);
	int p_RotateRight7(c_Node8*);
	int p_InsertFixup7(c_Node8*);
	bool p_Set7(String,c_StringStack*);
	c_Node8* p_FindNode(String);
	c_StringStack* p_Get(String);
	c_Node8* p_FirstNode();
	c_NodeEnumerator* p_ObjectEnumerator();
	void mark();
};
class c_StringMap7 : public c_Map7{
	public:
	c_StringMap7();
	c_StringMap7* m_new();
	int p_Compare(String,String);
	void mark();
};
class c_Node8 : public Object{
	public:
	String m_key;
	c_Node8* m_right;
	c_Node8* m_left;
	c_StringStack* m_value;
	int m_color;
	c_Node8* m_parent;
	c_Node8();
	c_Node8* m_new(String,c_StringStack*,int,c_Node8*);
	c_Node8* m_new2();
	c_Node8* p_NextNode();
	String p_Key();
	c_StringStack* p_Value();
	void mark();
};
class c_NodeEnumerator : public Object{
	public:
	c_Node8* m_node;
	c_NodeEnumerator();
	c_NodeEnumerator* m_new(c_Node8*);
	c_NodeEnumerator* m_new2();
	bool p_HasNext();
	c_Node8* p_NextObject();
	void mark();
};
String bb_os_StripAll(String);
class c_NodeEnumerator2 : public Object{
	public:
	c_Node7* m_node;
	c_NodeEnumerator2();
	c_NodeEnumerator2* m_new(c_Node7*);
	c_NodeEnumerator2* m_new2();
	bool p_HasNext();
	c_Node7* p_NextObject();
	void mark();
};
class c_Stack4 : public Object{
	public:
	Array<c_Stack* > m_data;
	int m_length;
	c_Stack4();
	c_Stack4* m_new();
	c_Stack4* m_new2(Array<c_Stack* >);
	static c_Stack* m_NIL;
	void p_Clear();
	void p_Push10(c_Stack*);
	void p_Push11(Array<c_Stack* >,int,int);
	void p_Push12(Array<c_Stack* >,int);
	c_Stack* p_Top();
	c_Stack* p_Pop();
	void mark();
};
class c_StringObject : public Object{
	public:
	String m_value;
	c_StringObject();
	c_StringObject* m_new(int);
	c_StringObject* m_new2(Float);
	c_StringObject* m_new3(String);
	c_StringObject* m_new4();
	void mark();
};
class c_NodeEnumerator3 : public Object{
	public:
	c_Node3* m_node;
	c_NodeEnumerator3();
	c_NodeEnumerator3* m_new(c_Node3*);
	c_NodeEnumerator3* m_new2();
	bool p_HasNext();
	c_Node3* p_NextObject();
	void mark();
};
class c_Stack5 : public Object{
	public:
	Array<int > m_data;
	int m_length;
	c_Stack5();
	c_Stack5* m_new();
	c_Stack5* m_new2(Array<int >);
	static int m_NIL;
	void p_Clear();
	void p_Length(int);
	int p_Length2();
	int p_Get2(int);
	void p_Push13(int);
	void p_Push14(Array<int >,int,int);
	void p_Push15(Array<int >,int);
	int p_Pop();
	void mark();
};
class c_IntStack : public c_Stack5{
	public:
	c_IntStack();
	c_IntStack* m_new(Array<int >);
	c_IntStack* m_new2();
	void mark();
};
int bb_math_Max(int,int);
Float bb_math_Max2(Float,Float);
int bb_math_Min(int,int);
Float bb_math_Min2(Float,Float);
class c_NodeEnumerator4 : public Object{
	public:
	c_Node6* m_node;
	c_NodeEnumerator4();
	c_NodeEnumerator4* m_new(c_Node6*);
	c_NodeEnumerator4* m_new2();
	bool p_HasNext();
	c_Node6* p_NextObject();
	void mark();
};
class c_Stack6 : public Object{
	public:
	Array<c_ClassDecl* > m_data;
	int m_length;
	c_Stack6();
	c_Stack6* m_new();
	c_Stack6* m_new2(Array<c_ClassDecl* >);
	void p_Push16(c_ClassDecl*);
	void p_Push17(Array<c_ClassDecl* >,int,int);
	void p_Push18(Array<c_ClassDecl* >,int);
	bool p_IsEmpty();
	c_Enumerator3* p_ObjectEnumerator();
	static c_ClassDecl* m_NIL;
	void p_Length(int);
	int p_Length2();
	void mark();
};
class c_Enumerator2 : public Object{
	public:
	c_Stack3* m_stack;
	int m_index;
	c_Enumerator2();
	c_Enumerator2* m_new(c_Stack3*);
	c_Enumerator2* m_new2();
	bool p_HasNext();
	c_Decl2* p_NextObject();
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
	c_Node6* m_node;
	c_ValueEnumerator();
	c_ValueEnumerator* m_new(c_Node6*);
	c_ValueEnumerator* m_new2();
	bool p_HasNext();
	c_ScopeDecl* p_NextObject();
	void mark();
};
class c_Markdown : public Object{
	public:
	c_ILinkResolver* m__resolver;
	c_IPrettifier* m__prettifier;
	String m__blk;
	c_Markdown();
	c_Markdown* m_new(c_ILinkResolver*,c_IPrettifier*);
	c_Markdown* m_new2();
	String p_Prettify(String);
	String p_SetBlock(String);
	int p_Find3(String,String,int);
	String p_ReplaceSpanTags(String,String,String);
	String p_ReplacePrefixTags(String,String,String);
	String p_ReplaceLinks(String);
	String p_ReplaceEscs(String);
	String p_SpanToHtml(String);
	String p_LineToHtml(String);
	String p_ToHtml(String);
	void mark();
};
class c_Enumerator3 : public Object{
	public:
	c_Stack6* m_stack;
	int m_index;
	c_Enumerator3();
	c_Enumerator3* m_new(c_Stack6*);
	c_Enumerator3* m_new2();
	bool p_HasNext();
	c_ClassDecl* p_NextObject();
	void mark();
};
class c_NodeEnumerator5 : public Object{
	public:
	c_Node5* m_node;
	c_NodeEnumerator5();
	c_NodeEnumerator5* m_new(c_Node5*);
	c_NodeEnumerator5* m_new2();
	bool p_HasNext();
	c_Node5* p_NextObject();
	void mark();
};
class c_NodeEnumerator6 : public Object{
	public:
	c_Node4* m_node;
	c_NodeEnumerator6();
	c_NodeEnumerator6* m_new(c_Node4*);
	c_NodeEnumerator6* m_new2();
	bool p_HasNext();
	c_Node4* p_NextObject();
	void mark();
};
int bbMain();
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
c_Set::c_Set(){
	m_map=0;
}
c_Set* c_Set::m_new(c_Map* t_map){
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
	c_Set::m_new((new c_StringMap)->m_new());
	return this;
}
void c_StringSet::mark(){
	c_Set::mark();
}
c_Map::c_Map(){
	m_root=0;
}
c_Map* c_Map::m_new(){
	return this;
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
bool c_Map::p_Set(String t_key,Object* t_value){
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
bool c_Map::p_Insert2(String t_key,Object* t_value){
	return p_Set(t_key,t_value);
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
bool c_Map::p_Contains(String t_key){
	return p_FindNode(t_key)!=0;
}
int c_Map::p_Clear(){
	m_root=0;
	return 0;
}
Object* c_Map::p_Get(String t_key){
	c_Node* t_node=p_FindNode(t_key);
	if((t_node)!=0){
		return t_node->m_value;
	}
	return 0;
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
c_StringSet* bb_apidoccer_ignore_mods;
c_Node::c_Node(){
	m_key=String();
	m_right=0;
	m_left=0;
	m_value=0;
	m_color=0;
	m_parent=0;
}
c_Node* c_Node::m_new(String t_key,Object* t_value,int t_color,c_Node* t_parent){
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
c_List::c_List(){
	m__head=((new c_HeadNode)->m_new());
}
c_List* c_List::m_new(){
	return this;
}
c_Node2* c_List::p_AddLast(String t_data){
	return (new c_Node2)->m_new(m__head,m__head->m__pred,t_data);
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
c_Node2* c_List::p_Find(String t_value,c_Node2* t_start){
	while(t_start!=m__head){
		if(p_Equals(t_value,t_start->m__data)){
			return t_start;
		}
		t_start=t_start->m__succ;
	}
	return 0;
}
c_Node2* c_List::p_Find2(String t_value){
	return p_Find(t_value,m__head->m__succ);
}
void c_List::p_RemoveFirst2(String t_value){
	c_Node2* t_node=p_Find2(t_value);
	if((t_node)!=0){
		t_node->p_Remove();
	}
}
int c_List::p_Count(){
	int t_n=0;
	c_Node2* t_node=m__head->m__succ;
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
c_Node2::c_Node2(){
	m__succ=0;
	m__pred=0;
	m__data=String();
}
c_Node2* c_Node2::m_new(c_Node2* t_succ,c_Node2* t_pred,String t_data){
	gc_assign(m__succ,t_succ);
	gc_assign(m__pred,t_pred);
	gc_assign(m__succ->m__pred,this);
	gc_assign(m__pred->m__succ,this);
	m__data=t_data;
	return this;
}
c_Node2* c_Node2::m_new2(){
	return this;
}
int c_Node2::p_Remove(){
	gc_assign(m__succ->m__pred,m__pred);
	gc_assign(m__pred->m__succ,m__succ);
	return 0;
}
void c_Node2::mark(){
	Object::mark();
	gc_mark_q(m__succ);
	gc_mark_q(m__pred);
}
c_HeadNode::c_HeadNode(){
}
c_HeadNode* c_HeadNode::m_new(){
	c_Node2::m_new2();
	gc_assign(m__succ,(this));
	gc_assign(m__pred,(this));
	return this;
}
void c_HeadNode::mark(){
	c_Node2::mark();
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
c_George::c_George(){
	m_styledir=String();
	m_iconImgs=(new c_StringStack)->m_new2();
	m_iconUrls=(new c_StringStack)->m_new2();
	m_dataDirs=(new c_StringMap2)->m_new();
	m_errinfo=String();
	m_pages=(new c_StringMap2)->m_new();
	m_indexcats=(new c_StringMap6)->m_new();
	m_content=(new c_StringMap2)->m_new();
	m_docbase=String();
	m_inrem=0;
	m_ptoker=(new c_Toker)->m_new3();
}
c_George* c_George::m_new(String t_styledir){
	this->m_styledir=t_styledir;
	return this;
}
c_George* c_George::m_new2(){
	return this;
}
void c_George::p_AddIconLink(String t_iconImg,String t_iconUrl){
	m_iconImgs->p_Push4(t_iconImg);
	m_iconUrls->p_Push4(t_iconUrl);
}
void c_George::p_SetErrInfo(String t_errinfo){
	this->m_errinfo=t_errinfo;
}
String c_George::p_MakeUrl(String t_path){
	String t_url=t_path.Replace(String(L"/",1),String(L"_",1));
	int t_i=t_url.Find(String(L"#",1),0);
	if(t_i==-1){
		return t_url+String(L".html",5);
	}
	return t_url.Slice(0,t_i)+String(L".html",5)+t_url.Slice(t_i);
}
c_StringMap2* c_George::p_GetIndex(String t_cat){
	c_StringMap2* t_index=m_indexcats->p_Get(t_cat);
	if(!((t_index)!=0)){
		t_index=(new c_StringMap2)->m_new();
		m_indexcats->p_Set6(t_cat,t_index);
	}
	return t_index;
}
void c_George::p_AddToIndex(String t_cat,String t_ident,String t_path){
	int t_i=t_ident.Find(String(L"(",1),0);
	if(t_i!=-1){
		t_ident=t_ident.Slice(0,t_i);
	}
	c_StringMap2* t_index=p_GetIndex(t_cat);
	String t_uident=t_ident;
	int t_n=1;
	while(t_index->p_Contains(t_uident)){
		t_n+=1;
		t_uident=t_ident+String(L"(",1)+String(t_n)+String(L")",1);
	}
	t_index->p_Set2(t_uident,t_path);
}
void c_George::p_AddPage(String t_path,String t_icon){
	if(m_pages->p_Contains(t_path)){
		bbPrint(String(L"Overwriting page:",17)+t_path);
	}
	String t_url=p_MakeUrl(t_path);
	m_pages->p_Set2(t_path,t_url);
	String t_id=bb_os_StripDir(t_path);
	int t_i=t_id.Find(String(L"#",1),0);
	if(t_i==-1){
		int t_i2=t_path.Find(String(L"/",1),0);
		if(t_i2!=-1 && t_path.Find(String(L"/",1),t_i2+1)==-1){
			p_AddToIndex(t_path.Slice(0,t_i2),t_id,t_path);
		}
	}else{
		t_id=t_id.Slice(t_i+1);
	}
	p_AddToIndex(String(L"Index",5),t_id,t_path);
}
void c_George::p_Err(String t_msg){
	bbPrint(m_errinfo+String(L"  :  ",5)+t_msg);
}
void c_George::p_SetPageContent(String t_page,String t_html){
	m_content->p_Set2(t_page,t_html);
}
void c_George::p_MakeIndices(){
	c_PageMaker* t_maker=(new c_PageMaker)->m_new(LoadString(m_styledir+String(L"/index_template.html",20)));
	c_NodeEnumerator2* t_=m_indexcats->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_Node7* t_it=t_->p_NextObject();
		String t_cat=t_it->p_Key();
		if(m_pages->p_Contains(t_cat)){
			continue;
		}
		c_StringMap2* t_index=t_it->p_Value();
		String t_url=t_cat+String(L".html",5);
		t_maker->p_Clear();
		t_maker->p_SetString(String(L"INDEX",5),t_cat);
		t_maker->p_BeginList(String(L"ITEMS",5));
		c_NodeEnumerator3* t_2=t_index->p_ObjectEnumerator();
		while(t_2->p_HasNext()){
			c_Node3* t_it2=t_2->p_NextObject();
			t_maker->p_AddItem();
			t_maker->p_SetString(String(L"IDENT",5),t_it2->p_Key());
			t_maker->p_SetString(String(L"URL",3),m_pages->p_Get(t_it2->p_Value()));
		}
		t_maker->p_EndList();
		String t_page=t_maker->p_MakePage();
		m_pages->p_Set2(t_cat,t_url);
		p_SetPageContent(t_cat,t_page);
	}
}
String c_George::p_GetPageUrl(String t_path){
	return m_pages->p_Get(t_path);
}
String c_George::p_MakeLink(String t_url,String t_text){
	bool t_isImg=false;
	if(t_url.Find(String(L"<img>",5),0)>-1){
		t_isImg=true;
		t_url=t_url.Slice(5);
	}
	if(!((t_text).Length()!=0)){
		t_text=t_url;
	}
	if((t_url).Length()!=0){
		if(t_isImg==true){
			return String(L"<img src=\"",10)+t_url+String(L"\" alt=\"",7)+t_text+String(L"\" />",4);
		}else{
			return String(L"<a href=\"",9)+t_url+String(L"\">",2)+t_text+String(L"</a>",4);
		}
	}
	return t_text;
}
String c_George::p_ResolveLink(String t_link,String t_text){
	if(t_link.StartsWith(String(L"#",1)) || t_link.StartsWith(String(L"<img>",5)) || t_link.StartsWith(String(L"http:",5)) || t_link.StartsWith(String(L"https:",6))){
		return p_MakeLink(t_link,t_text);
	}
	String t_url=String();
	String t_path=t_link;
	String t_hash=String();
	int t_i=t_path.Find(String(L"#",1),0);
	if(t_i!=-1){
		t_hash=t_path.Slice(t_i);
		t_path=t_path.Slice(0,t_i);
	}
	t_url=m_pages->p_Get(t_path);
	if(!((t_url).Length()!=0)){
		t_path=p_GetIndex(String(L"Index",5))->p_Get(t_path);
		if((t_path).Length()!=0){
			t_url=m_pages->p_Get(t_path);
		}
	}
	if((t_url).Length()!=0){
		t_url=t_url+t_hash;
	}
	if(!((t_text).Length()!=0)){
		t_text=bb_os_StripDir(t_link);
		int t_i2=t_text.Find(String(L"#",1),0);
		if(t_i2!=-1){
			t_text=t_text.Slice(t_i2+1);
		}
	}
	if(!((t_url).Length()!=0)){
		p_Err(String(L"Can't find link:",16)+t_link);
		c_NodeEnumerator3* t_=m_pages->p_ObjectEnumerator();
		while(t_->p_HasNext()){
			c_Node3* t_pg=t_->p_NextObject();
		}
	}
	return p_MakeLink(t_url,t_text);
}
void c_George::p_SetDocBase(String t_docbase){
	this->m_docbase=t_docbase;
}
void c_George::p_MakeDocs(){
	Array<String > t_=bb_os_LoadDir(m_styledir,true,false);
	int t_2=0;
	while(t_2<t_.Length()){
		String t_f=t_[t_2];
		t_2=t_2+1;
		if(t_f.EndsWith(String(L"_template.html",14))){
			continue;
		}
		String t_dir=bb_os_ExtractDir(t_f);
		if((t_dir).Length()!=0){
			CreateDir(String(L"docs/html/",10)+t_dir);
		}
		CopyFile(m_styledir+String(L"/",1)+t_f,String(L"docs/html/",10)+t_f);
	}
	c_PageMaker* t_maker=(new c_PageMaker)->m_new(LoadString(m_styledir+String(L"/page_template.html",19)));
	c_NodeEnumerator3* t_3=m_pages->p_ObjectEnumerator();
	while(t_3->p_HasNext()){
		c_Node3* t_it=t_3->p_NextObject();
		String t_path=t_it->p_Key();
		String t_url=t_it->p_Value();
		String t_page=m_content->p_Get(t_path);
		if(!((t_page).Length()!=0)){
			continue;
		}
		t_maker->p_Clear();
		t_maker->p_SetString(String(L"CONTENT",7),t_page);
		if((m_iconImgs->p_Length2())!=0){
			t_maker->p_BeginList(String(L"ICONLINKS",9));
			for(int t_i=0;t_i<m_iconImgs->p_Length2();t_i=t_i+1){
				t_maker->p_AddItem();
				t_maker->p_SetString(String(L"ICON",4),m_iconImgs->p_Get2(t_i));
				t_maker->p_SetString(String(L"URL",3),m_iconUrls->p_Get2(t_i));
			}
			t_maker->p_EndList();
		}
		if(t_path!=String(L"Home",4) && t_path!=String(L"Home2",5)){
			t_maker->p_BeginList(String(L"NAVLINKS",8));
			String t_tpath=String();
			Array<String > t_4=t_path.Split(String(L"/",1));
			int t_5=0;
			while(t_5<t_4.Length()){
				String t_bit=t_4[t_5];
				t_5=t_5+1;
				t_maker->p_AddItem();
				t_maker->p_SetString(String(L"IDENT",5),t_bit);
				if((t_tpath).Length()!=0){
					t_tpath=t_tpath+String(L"/",1);
				}
				t_tpath=t_tpath+t_bit;
				t_maker->p_SetString(String(L"URL",3),m_pages->p_Get(t_tpath));
			}
			t_maker->p_EndList();
		}
		t_page=t_maker->p_MakePage();
		SaveString(t_page,String(L"docs/html/",10)+t_url);
	}
	c_StringStack* t_out=(new c_StringStack)->m_new2();
	c_NodeEnumerator3* t_6=p_GetIndex(String(L"Index",5))->p_ObjectEnumerator();
	while(t_6->p_HasNext()){
		c_Node3* t_it2=t_6->p_NextObject();
		String t_url2=m_pages->p_Get(t_it2->p_Value());
		if((t_url2).Length()!=0){
			t_out->p_Push4(t_it2->p_Key()+String(L":",1)+t_url2);
		}
	}
	SaveString(t_out->p_Join(String(L"\n",1)),String(L"docs/html/index.txt",19));
	bb_os_CopyDir(m_styledir+String(L"/data",5),String(L"docs/html/data",14),true,false);
}
int c_George::p_CopyDataDirs(){
	c_NodeEnumerator3* t_=m_dataDirs->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_Node3* t_dir=t_->p_NextObject();
		if(CreateDir(String(L"docs/html/data/",15)+t_dir->p_Value())==1){
			String t_subDir=String();
			t_subDir=bb_os_StripDir(bb_os_StripExt(t_dir->p_Key()));
			if(bb_os_CopyDir(t_dir->p_Key(),String(L"docs/html/data/",15)+t_dir->p_Value()+String(L"/",1)+t_subDir,true,false)==0){
				bbPrint(String(L"... error in copying data directory: ",37)+t_dir->p_Key()+String(L"  to  docs/html/data/",21)+t_dir->p_Value()+String(L"/",1)+t_subDir);
			}
		}else{
			bbPrint(String(L"... error in creating data directory -> docs/html/data/",55)+t_dir->p_Value());
		}
	}
	return 0;
}
String c_George::p_BeginPrettyBlock(){
	return String(L"<div class=pretty>",18);
}
String c_George::p_EndPrettyBlock(){
	return String(L"</div>",6);
}
String c_George::p_HtmlEsc(String t_str){
	return t_str.Replace(String(L"&",1),String(L"&amp;",5)).Replace(String(L"<",1),String(L"&lt;",4)).Replace(String(L">",1),String(L"&gt;",4));
}
String c_George::m_CerberusKeywords;
String c_George::p_PrettifyLine(String t_text){
	if(t_text==String(L"</pre >",7)){
		t_text=String(L"</pre>",6);
	}
	if((m_inrem)!=0){
		if(t_text.StartsWith(String(L"#End",4))){
			m_inrem-=1;
		}
		return String(L"<code class=r>",14)+p_HtmlEsc(t_text)+String(L"</code><br>",11);
	}else{
		if(t_text.StartsWith(String(L"#Rem",4))){
			m_inrem+=1;
			return String(L"<code class=r>",14)+p_HtmlEsc(t_text)+String(L"</code><br>",11);
		}
	}
	m_ptoker->p_SetText(t_text);
	String t_str=String();
	String t_out=String();
	String t_ccls=String();
	do{
		if(!((m_ptoker->p_Bump()).Length()!=0)){
			break;
		}
		String t_cls=String(L"d",1);
		String t_toke=m_ptoker->p_Toke();
		String t_esc=t_toke;
		int t_1=m_ptoker->p_TokeType();
		if(t_1==1){
			if(!((t_toke).Length()!=0)){
				break;
			}
			t_cls=String(L"r",1);
			t_esc=p_HtmlEsc(t_esc);
		}else{
			if(t_1==2){
				t_toke=String();
				String t_=t_esc;
				int t_2=0;
				while(t_2<t_.Length()){
					int t_c=(int)t_[t_2];
					t_2=t_2+1;
					if(t_c==9){
						t_toke=t_toke+String(L"    ",4).Slice(0,4-(t_str+t_toke).Length() % 4);
					}else{
						t_toke=t_toke+String(L" ",1);
					}
				}
				t_esc=t_toke.Replace(String(L" ",1),String(L"&nbsp;",6));
			}else{
				if(t_1==3){
					t_cls=String(L"i",1);
					if(m_CerberusKeywords.Contains(String(L";",1)+t_toke.ToLower()+String(L";",1))){
						t_cls=String(L"k",1);
					}
				}else{
					if(t_1==4 || t_1==5 || t_1==6){
						t_cls=String(L"l",1);
						t_esc=p_HtmlEsc(t_esc).Replace(String(L" ",1),String(L"&nbsp;",6));
					}else{
						if(t_1==7){
							t_esc=p_HtmlEsc(t_esc);
						}
					}
				}
			}
		}
		if(t_cls!=t_ccls){
			if((t_out).Length()!=0){
				t_out=t_out+String(L"</code>",7);
			}
			if((t_cls).Length()!=0){
				t_out=t_out+(String(L"<code class=",12)+t_cls+String(L">",1));
			}else{
				t_out=t_out+String(L"<code>",6);
			}
			t_ccls=t_cls;
		}
		t_str=t_str+t_toke;
		t_out=t_out+t_esc;
	}while(!(false));
	if((t_out).Length()!=0){
		t_out=t_out+String(L"</code>",7);
	}
	return t_out+String(L"<br>",4);
}
void c_George::mark(){
	Object::mark();
	gc_mark_q(m_iconImgs);
	gc_mark_q(m_iconUrls);
	gc_mark_q(m_dataDirs);
	gc_mark_q(m_pages);
	gc_mark_q(m_indexcats);
	gc_mark_q(m_content);
	gc_mark_q(m_ptoker);
}
c_ApiDoccer::c_ApiDoccer(){
	m_george=0;
	m_scopeTemplate=0;
	m_scopes=(new c_StringMap5)->m_new();
	m_linkScope=0;
}
c_ApiDoccer* c_ApiDoccer::m_new(c_George* t_george){
	gc_assign(this->m_george,t_george);
	return this;
}
c_ApiDoccer* c_ApiDoccer::m_new2(){
	return this;
}
bool c_ApiDoccer::p_LoadExample(c_Decl2* t_decl,String t_dir){
	if(!((t_dir).Length()!=0) || t_decl->m_ident!=t_decl->m_uident){
		return false;
	}
	String t_src=LoadString(t_dir+String(L"/",1)+t_decl->m_ident+String(L"_example.cxs",12));
	if(!((t_src).Length()!=0)){
		LoadString(t_dir+String(L"/",1)+t_decl->m_ident+String(L"_example.monkey",15));
	}
	if(!((t_src).Length()!=0)){
		return false;
	}
	t_decl->m_docs->p_Set2(String(L"example",7),t_src);
	t_decl->m_egdir=t_dir+String(L"/",1)+t_decl->m_ident+String(L"_example.data",13);
	return true;
}
void c_ApiDoccer::p_EndSect(String t_sect,c_StringStack* t_docs,c_Decl2* t_doccing){
	if(!t_docs->p_IsEmpty()){
		String t_t=t_docs->p_Join(String(L"\n",1)).Trim();
		if((t_t).Length()!=0){
			t_doccing->m_docs->p_Set2(t_sect,t_t);
		}
		t_docs->p_Clear();
	}
}
void c_ApiDoccer::p_ParseCerberusdocFile(String t_srcpath,String t_modpath){
	m_george->p_SetErrInfo(t_srcpath);
	c_Parser* t_parser=(new c_Parser)->m_new(String());
	c_Decl* t_pdecl=(new c_Decl)->m_new(String(L"module",6),String());
	t_pdecl->m_ident=t_modpath;
	c_ModuleDecl* t_mdecl=(new c_ModuleDecl)->m_new(t_pdecl,this);
	m_scopes->p_Set5(t_mdecl->m_path,(t_mdecl));
	m_george->p_AddPage(t_mdecl->p_PagePath(),String());
	c_ScopeDecl* t_scope=(t_mdecl);
	String t_sect=String(L"description",11);
	c_StringStack* t_docs=(new c_StringStack)->m_new2();
	c_Decl2* t_doccing=(t_mdecl);
	String t_egdir=bb_os_ExtractDir(t_srcpath)+String(L"/examples",9);
	if(FileType(t_egdir)!=2 && bb_os_StripDir(bb_os_ExtractDir(t_srcpath))==String(L"cerberusdoc",11)){
		t_egdir=bb_os_ExtractDir(bb_os_ExtractDir(t_srcpath))+String(L"/examples",9);
		if(FileType(t_egdir)!=2){
			t_egdir=String();
		}
	}
	if(FileType(t_egdir)!=2 && bb_os_StripDir(bb_os_ExtractDir(t_srcpath))==String(L"monkeydoc",9)){
		t_egdir=bb_os_ExtractDir(bb_os_ExtractDir(t_srcpath))+String(L"/examples",9);
		if(FileType(t_egdir)!=2){
			t_egdir=String();
		}
	}
	p_LoadExample(t_doccing,t_egdir);
	String t_src=LoadString(t_srcpath);
	Array<String > t_=t_src.Split(String(L"\n",1));
	int t_2=0;
	while(t_2<t_.Length()){
		String t_line=t_[t_2];
		t_2=t_2+1;
		if(t_line.StartsWith(String(L"# ",2))){
			t_parser->p_SetText(t_line.Slice(2));
			c_Decl* t_pdecl2=t_parser->p_ParseDecl();
			if(!((t_pdecl2)!=0)){
				m_george->p_Err(String(L"Error parsing line: ",20)+t_line);
				continue;
			}
			String t_5=t_pdecl2->m_kind;
			if(t_5==String(L"module",6)){
				p_EndSect(t_sect,t_docs,t_doccing);
				t_sect=String(L"description",11);
				t_scope=(t_mdecl);
				t_doccing=(t_mdecl);
			}else{
				if(t_5==String(L"import",6)){
					if(t_scope!=(t_mdecl)){
						m_george->p_Err(String(L"Import not at Module scope",26));
					}
					(new c_ImportDecl)->m_new(t_pdecl2,(t_mdecl));
				}else{
					if(t_5==String(L"class",5) || t_5==String(L"interface",9)){
						if(t_pdecl2->m_ident.Contains(String(L".",1))){
							t_pdecl2->m_ident=bb_os_ExtractExt(t_pdecl2->m_ident);
						}
						p_EndSect(t_sect,t_docs,t_doccing);
						t_sect=String(L"description",11);
						c_ClassDecl* t_cdecl=(new c_ClassDecl)->m_new(t_pdecl2,t_mdecl);
						m_scopes->p_Set5(t_cdecl->m_path,(t_cdecl));
						t_scope=(t_cdecl);
						t_doccing=(t_scope);
						m_george->p_AddPage(t_cdecl->p_PagePath(),String());
						if(t_cdecl->m_kind==String(L"class",5)){
							m_george->p_AddToIndex(String(L"Classes",7),t_cdecl->m_ident,t_cdecl->p_PagePath());
						}
						if(t_cdecl->m_kind==String(L"interface",9)){
							m_george->p_AddToIndex(String(L"Interfaces",10),t_cdecl->m_ident,t_cdecl->p_PagePath());
						}
						p_LoadExample(t_doccing,t_egdir);
					}else{
						if(t_5==String(L"function",8) || t_5==String(L"method",6) || t_5==String(L"global",6) || t_5==String(L"field",5) || t_5==String(L"const",5) || t_5==String(L"property",8) || t_5==String(L"ctor",4)){
							p_EndSect(t_sect,t_docs,t_doccing);
							t_sect=String(L"description",11);
							t_doccing=(new c_Decl2)->m_new2(t_pdecl2,t_scope);
							m_george->p_AddPage(t_doccing->p_PagePath(),String());
							if(t_doccing->m_kind==String(L"function",8) && t_scope==(t_mdecl)){
								m_george->p_AddToIndex(String(L"Functions",9),t_doccing->m_ident,t_doccing->p_PagePath());
							}
							p_LoadExample(t_doccing,t_egdir);
						}else{
							m_george->p_Err(String(L"Unrecognized decl kind: ",24)+t_pdecl2->m_kind);
						}
					}
				}
			}
		}else{
			if(t_line.StartsWith(String(L"'# ",3))){
			}else{
				t_parser->p_SetText(t_line);
				if(t_parser->p_TokeType()==3){
					String t_id=t_parser->p_ParseIdent();
					if(t_parser->p_Toke()==String(L":",1)){
						String t_6=t_id.ToLower();
						if(t_6==String(L"params",6) || t_6==String(L"returns",7) || t_6==String(L"in",2) || t_6==String(L"out",3) || t_6==String(L"example",7) || t_6==String(L"links",5)){
							p_EndSect(t_sect,t_docs,t_doccing);
							t_sect=t_id.ToLower();
							c_Toker* t_toker=t_parser->p_GetToker();
							String t_t=t_toker->p_Text().Slice(t_toker->p_Cursor()).Trim();
							if((t_t).Length()!=0){
								t_docs->p_Push4(t_t);
							}
							continue;
						}
					}
				}
				t_docs->p_Push4(t_line);
			}
		}
	}
	p_EndSect(t_sect,t_docs,t_doccing);
	m_george->p_SetErrInfo(String());
}
void c_ApiDoccer::p_AddDocsToDecl(c_StringMap7* t_docs,c_Decl2* t_decl){
	if(!((t_docs)!=0)){
		return;
	}
	c_NodeEnumerator* t_=t_docs->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_Node8* t_it=t_->p_NextObject();
		t_decl->m_docs->p_Set2(t_it->p_Key(),t_it->p_Value()->p_Join(String(L"\n",1)));
	}
}
void c_ApiDoccer::p_ParseCerberusFile(String t_srcpath,String t_modpath){
	m_george->p_SetErrInfo(t_srcpath);
	c_Parser* t_parser=(new c_Parser)->m_new(String());
	c_ModuleDecl* t_mdecl=0;
	c_ScopeDecl* t_docscope=0;
	c_StringMap7* t_docs=0;
	String t_sect=String();
	bool t_pub=true;
	bool t_mdoc=false;
	bool t_comment=false;
	String t_egdir=bb_os_ExtractDir(t_srcpath)+String(L"/examples",9);
	if(FileType(t_egdir)!=2){
		t_egdir=String();
	}
	String t_src=LoadString(t_srcpath).Replace(String(L"\r",1),String());
	int t_srcline=0;
	String t_parserbump=String();
	Array<String > t_=t_src.Split(String(L"\n",1));
	int t_2=0;
	while(t_2<t_.Length()){
		String t_line=t_[t_2];
		t_2=t_2+1;
		t_srcline+=1;
		t_parser->p_SetText(t_line);
		if(t_parser->p_Toke()==String(L"#",1)){
			String t_22=t_parser->p_Bump();
			if(t_22==String(L"rem",3)){
				t_comment=true;
				t_parserbump=t_parser->p_Bump();
				if(t_parserbump==String(L"cerberusdoc",11)){
					String t_opt=t_parser->p_Bump();
					if(t_opt==String(L"on",2)){
						t_mdoc=true;
					}else{
						if(t_opt==String(L"off",3)){
							t_mdoc=false;
						}else{
							t_mdoc=true;
							if(!((t_mdecl)!=0)){
								if(t_opt!=String(L"module",6)){
									return;
								}
								t_parser->p_Bump();
								String t_id=t_parser->p_ParseIdent();
								if(t_id!=t_modpath){
									m_george->p_Err(String(L"Modpath (",9)+t_modpath+String(L") does not match module ident (",31)+t_id+String(L")",1));
									return;
								}
							}
							t_docs=(new c_StringMap7)->m_new();
							t_sect=String(L"description",11);
							t_docs->p_Set7(t_sect,(new c_StringStack)->m_new2());
							String t_text=t_parser->p_GetText().Trim();
							if((t_text).Length()!=0){
								t_docs->p_Get(t_sect)->p_Push4(t_text);
							}
						}
					}
				}
				if(t_parserbump==String(L"monkeydoc",9)){
					String t_opt2=t_parser->p_Bump();
					if(t_opt2==String(L"on",2)){
						t_mdoc=true;
					}else{
						if(t_opt2==String(L"off",3)){
							t_mdoc=false;
						}else{
							t_mdoc=true;
							if(!((t_mdecl)!=0)){
								if(t_opt2!=String(L"module",6)){
									return;
								}
								t_parser->p_Bump();
								String t_id2=t_parser->p_ParseIdent();
								if(t_id2!=t_modpath){
									m_george->p_Err(String(L"Modpath (",9)+t_modpath+String(L") does not match module ident (",31)+t_id2+String(L")",1));
									return;
								}
							}
							t_docs=(new c_StringMap7)->m_new();
							t_sect=String(L"description",11);
							t_docs->p_Set7(t_sect,(new c_StringStack)->m_new2());
							String t_text2=t_parser->p_GetText().Trim();
							if((t_text2).Length()!=0){
								t_docs->p_Get(t_sect)->p_Push4(t_text2);
							}
						}
					}
				}
			}else{
				if(t_22==String(L"end",3)){
					t_comment=false;
					if((t_sect).Length()!=0){
						if(!((t_mdecl)!=0)){
							t_mdecl=(new c_ModuleDecl)->m_new((new c_Decl)->m_new(String(L"module",6),t_modpath),this);
							t_mdecl->m_srcinfo=t_srcpath+String(L":1",2);
							m_scopes->p_Set5(t_mdecl->m_path,(t_mdecl));
							m_george->p_AddPage(t_mdecl->p_PagePath(),String());
							t_docscope=(t_mdecl);
							p_AddDocsToDecl(t_docs,(t_mdecl));
							t_docs=0;
							p_LoadExample((t_mdecl),t_egdir);
						}
						t_sect=String();
					}
				}
			}
			continue;
		}
		if((t_sect).Length()!=0){
			if(t_parser->p_TokeType()==3){
				String t_id3=t_parser->p_Toke();
				if(t_parser->p_Bump()==String(L":",1)){
					String t_3=t_id3.ToLower();
					if(t_3==String(L"params",6) || t_3==String(L"returns",7) || t_3==String(L"in",2) || t_3==String(L"out",3) || t_3==String(L"example",7) || t_3==String(L"links",5)){
						t_sect=t_id3.ToLower();
						t_docs->p_Set7(t_sect,(new c_StringStack)->m_new2());
						String t_text3=t_parser->p_GetText().Trim();
						if((t_text3).Length()!=0){
							t_docs->p_Get(t_sect)->p_Push4(t_text3);
						}
						continue;
					}
				}
			}
			t_docs->p_Get(t_sect)->p_Push4(t_line);
			continue;
		}
		if(!t_comment){
			String t_4=t_parser->p_Toke();
			if(t_4==String(L"public",6)){
				t_pub=true;
			}else{
				if(t_4==String(L"private",7)){
					t_pub=false;
				}else{
					if(t_4==String(L"extern",6)){
						t_pub=t_parser->p_Bump()!=String(L"private",7);
					}else{
						if(t_4==String(L"import",6)){
							if(t_pub && t_mdoc){
								c_Decl* t_pdecl=t_parser->p_ParseDecl();
								if((t_pdecl)!=0){
									String t_ident=t_pdecl->m_ident;
									String t_p=bb_os_ExtractDir(t_srcpath)+String(L"/",1)+t_ident.Replace(String(L".",1),String(L"/",1));
									if(FileType(t_p+String(L".cxs",4))==1 || FileType(t_p+String(L"/",1)+t_p+String(L".cxs",4))==1){
										if(bb_os_StripDir(bb_os_ExtractDir(t_srcpath))==bb_os_StripAll(t_srcpath)){
											t_pdecl->m_ident=t_modpath+String(L".",1)+t_ident;
										}else{
											if(t_modpath.Contains(String(L".",1))){
												String t_id4=bb_os_StripExt(t_modpath);
												if(t_id4.Contains(String(L".",1))){
													t_id4=bb_os_ExtractExt(t_id4);
												}
												if(t_id4!=t_ident){
													t_pdecl->m_ident=t_id4+String(L".",1)+t_ident;
												}
											}
										}
									}
									(new c_ImportDecl)->m_new(t_pdecl,(t_mdecl));
								}
							}
						}else{
							if(t_4==String(L"class",5) || t_4==String(L"interface",9)){
								if((t_pub || ((t_docs)!=0)) && t_mdoc){
									c_ClassDecl* t_cdecl=(new c_ClassDecl)->m_new(t_parser->p_ParseDecl(),t_mdecl);
									t_cdecl->m_srcinfo=t_srcpath+String(L":",1)+String(t_srcline);
									m_scopes->p_Set5(t_cdecl->m_path,(t_cdecl));
									p_AddDocsToDecl(t_docs,(t_cdecl));
									t_docscope=(t_cdecl);
									m_george->p_AddPage(t_cdecl->p_PagePath(),String());
									if(t_cdecl->m_kind==String(L"class",5)){
										m_george->p_AddToIndex(String(L"Classes",7),t_cdecl->m_ident,t_cdecl->p_PagePath());
									}
									if(t_cdecl->m_kind==String(L"interface",9)){
										m_george->p_AddToIndex(String(L"Interfaces",10),t_cdecl->m_ident,t_cdecl->p_PagePath());
									}
									p_LoadExample((t_cdecl),t_egdir);
								}else{
									if(!t_mdoc){
										return;
									}
								}
								t_docs=0;
							}else{
								if(t_4==String(L"function",8) || t_4==String(L"method",6) || t_4==String(L"global",6) || t_4==String(L"field",5) || t_4==String(L"const",5) || t_4==String(L"property",8) || t_4==String(L"ctor",4)){
									if((t_pub || ((t_docs)!=0)) && t_mdoc){
										c_Decl2* t_decl=(new c_Decl2)->m_new2(t_parser->p_ParseDecl(),t_docscope);
										t_decl->m_srcinfo=t_srcpath+String(L":",1)+String(t_srcline);
										p_AddDocsToDecl(t_docs,t_decl);
										m_george->p_AddPage(t_decl->p_PagePath(),String());
										if(t_decl->m_kind==String(L"function",8) && t_docscope==(t_mdecl)){
											m_george->p_AddToIndex(String(L"Functions",9),t_decl->m_ident,t_decl->p_PagePath());
										}
										p_LoadExample(t_decl,t_egdir);
									}else{
										if(!t_mdoc){
											return;
										}
									}
									t_docs=0;
								}
							}
						}
					}
				}
			}
		}
	}
	m_george->p_SetErrInfo(String());
}
void c_ApiDoccer::p_ParseModules(String t_dir,String t_modpath){
	c_PageMaker* t_tmp=m_scopeTemplate;
	String t_p=t_dir+String(L"/scope_template.html",20);
	if(FileType(t_p)==1){
		gc_assign(m_scopeTemplate,(new c_PageMaker)->m_new(LoadString(t_p)));
	}
	Array<String > t_=LoadDir(t_dir);
	int t_2=0;
	while(t_2<t_.Length()){
		String t_f=t_[t_2];
		t_2=t_2+1;
		String t_p2=t_dir+String(L"/",1)+t_f;
		int t_7=FileType(t_p2);
		if(t_7==2){
			if(t_f==String(L"3rdparty.cerberusdoc",20)){
				Array<String > t_3=LoadDir(t_p2);
				int t_4=0;
				while(t_4<t_3.Length()){
					String t_t=t_3[t_4];
					t_4=t_4+1;
					String t_q=t_p2+String(L"/",1)+t_t;
					int t_8=FileType(t_q);
					if(t_8==2){
						bb_os_CopyDir(t_q,String(L"docs/cerberusdoc/3rd party modules/",35)+t_t,true,false);
					}else{
						if(t_8==1){
							String t_9=bb_os_ExtractExt(t_t);
							if(t_9==String(L"png",3)){
								CopyFile(t_q,String(L"docs/html/3rd party modules_",28)+t_t);
								m_george->p_AddIconLink(String(L"3rd party modules_",18)+t_t,String(L"3rd party modules_",18)+bb_os_StripExt(t_t)+String(L".html",5));
							}else{
								if(t_9==String(L"cerberusdoc",11)){
									CopyFile(t_q,String(L"docs/cerberusdoc/3rd party modules/",35)+t_t);
								}
							}
						}
					}
				}
				continue;
			}
			if(t_p2.Find(String(L".data",5),0)>-1){
				m_george->m_dataDirs->p_Add(t_p2,bb_os_StripExt(t_modpath));
			}
			if(bb_apidoccer_ignore_mods->p_Contains(t_f)){
				continue;
			}
			if((t_modpath).Length()!=0){
				p_ParseModules(t_p2,t_modpath+String(L".",1)+t_f);
			}else{
				p_ParseModules(t_p2,t_f);
			}
		}else{
			if(t_7==1){
				String t_name=bb_os_StripExt(t_f);
				String t_ext=bb_os_ExtractExt(t_f);
				if(t_ext==String(L"cxs",3)){
					String t_q2=t_modpath;
					if(t_name!=bb_os_StripDir(t_dir)){
						t_q2=t_q2+(String(L".",1)+t_name);
					}
					String t_t2=t_dir+String(L"/",1)+t_name+String(L".cerberusdoc",12);
					if(FileType(t_t2)==1){
						p_ParseCerberusdocFile(t_t2,t_q2);
						continue;
					}
					t_t2=t_dir+String(L"/cerberusdoc/",13)+t_name+String(L".cerberusdoc",12);
					if(FileType(t_t2)==1){
						p_ParseCerberusdocFile(t_t2,t_q2);
						continue;
					}
					p_ParseCerberusFile(t_p2,t_q2);
				}
				if(t_ext==String(L"monkey",6)){
					String t_q3=t_modpath;
					if(t_name!=bb_os_StripDir(t_dir)){
						t_q3=t_q3+(String(L".",1)+t_name);
					}
					String t_t3=t_dir+String(L"/",1)+t_name+String(L".monkeydoc",10);
					if(FileType(t_t3)==1){
						p_ParseCerberusdocFile(t_t3,t_q3);
						continue;
					}
					t_t3=t_dir+String(L"/monkeydoc/",11)+t_name+String(L".monkeydoc",10);
					if(FileType(t_t3)==1){
						p_ParseCerberusdocFile(t_t3,t_q3);
						continue;
					}
					p_ParseCerberusFile(t_p2,t_q3);
				}
			}
		}
	}
	gc_assign(m_scopeTemplate,t_tmp);
}
void c_ApiDoccer::p_ParseDocs(){
	String t_modpath=bb_modpath_LoadModpath();
	t_modpath=t_modpath.Replace(String(L"\\",1),String(L"/",1));
	t_modpath=t_modpath.Replace(String(L"|",1),String(L";",1));
	Array<String > t_=t_modpath.Split(String(L";",1));
	int t_2=0;
	while(t_2<t_.Length()){
		String t_p=t_[t_2];
		t_2=t_2+1;
		if(FileType(t_p)==2){
			p_ParseModules(t_p,String());
		}
	}
}
void c_ApiDoccer::p_ResolveScopes(){
	c_NodeEnumerator4* t_=m_scopes->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_Node6* t_it=t_->p_NextObject();
		c_ScopeDecl* t_scope=t_it->p_Value();
		m_george->p_SetErrInfo(t_scope->m_path);
		gc_assign(m_linkScope,t_scope);
		c_ClassDecl* t_cdecl=dynamic_cast<c_ClassDecl*>(t_scope);
		if(((t_cdecl)!=0) && ((t_cdecl->m_texts).Length()!=0)){
			int t_i=t_cdecl->m_texts.Find(String(L"]]",2),0);
			if(t_i!=-1){
				c_ClassDecl* t_exts=dynamic_cast<c_ClassDecl*>(t_cdecl->m_scope->p_FindDecl(t_cdecl->m_texts.Slice(2,t_i)));
				if((t_exts)!=0){
					t_cdecl->p_SetSuper(t_exts);
				}
			}
			if(!((t_cdecl->m_exts)!=0)){
				m_george->p_Err(String(L"Can't find super class: ",24)+t_cdecl->m_texts);
			}
		}
	}
	m_linkScope=0;
	m_george->p_SetErrInfo(String());
}
String c_ApiDoccer::p_Capitalize(String t_str){
	return t_str.Slice(0,1).ToUpper()+t_str.Slice(1);
}
String c_ApiDoccer::p_HtmlEsc(String t_str){
	return t_str.Replace(String(L"&",1),String(L"&amp;",5)).Replace(String(L"<",1),String(L"&lt;",4)).Replace(String(L">",1),String(L"&gt;",4));
}
String c_ApiDoccer::p_StripLinks(String t_str){
	return t_str.Replace(String(L"[[",2),String()).Replace(String(L"]]",2),String());
}
void c_ApiDoccer::p_AddDecl(c_Decl2* t_decl,c_PageMaker* t_maker,c_Markdown* t_markdown){
	t_maker->p_SetString(String(L"KIND",4),p_Capitalize(t_decl->m_kind));
	t_maker->p_SetString(String(L"IDENT",5),t_decl->m_ident);
	t_maker->p_SetString(String(L"UIDENT",6),t_decl->m_uident);
	t_maker->p_SetString(String(L"URL",3),m_george->p_GetPageUrl(t_decl->p_PagePath()));
	t_maker->p_SetString(String(L"TYPE",4),p_StripLinks(p_HtmlEsc(t_decl->m_type)));
	t_maker->p_SetString(String(L"XTYPE",5),t_markdown->p_ToHtml(p_HtmlEsc(t_decl->m_type)));
	t_maker->p_SetString(String(L"EXTENDS",7),p_StripLinks(p_HtmlEsc(t_decl->m_texts)));
	t_maker->p_SetString(String(L"XEXTENDS",8),t_markdown->p_ToHtml(p_HtmlEsc(t_decl->m_texts)));
	t_maker->p_SetString(String(L"IMPLEMENTS",10),p_StripLinks(p_HtmlEsc(t_decl->m_timpls)));
	t_maker->p_SetString(String(L"XIMPLEMENTS",11),t_markdown->p_ToHtml(p_HtmlEsc(t_decl->m_timpls)));
	if((dynamic_cast<c_AliasDecl*>(t_decl))!=0){
		String t_t=String(L"[[",2)+dynamic_cast<c_AliasDecl*>(t_decl)->m_decl->m_scope->p_PagePath()+String(L"]]",2);
		t_maker->p_SetString(String(L"INHERITED_FROM",14),p_StripLinks(t_t));
		t_maker->p_SetString(String(L"XINHERITED_FROM",15),t_markdown->p_ToHtml(t_t));
	}else{
		t_maker->p_SetString(String(L"INHERITED_FROM",14),String());
		t_maker->p_SetString(String(L"XINHERITED_FROM",15),String());
	}
	c_ClassDecl* t_cdecl=dynamic_cast<c_ClassDecl*>(t_decl);
	if(((t_cdecl)!=0) && !t_cdecl->m_extby->p_IsEmpty()){
		String t_extby=String();
		String t_xextby=String();
		c_Enumerator3* t_=t_cdecl->m_extby->p_ObjectEnumerator();
		while(t_->p_HasNext()){
			c_ClassDecl* t_decl2=t_->p_NextObject();
			if((t_extby).Length()!=0){
				t_extby=t_extby+String(L", ",2);
			}
			if((t_xextby).Length()!=0){
				t_xextby=t_xextby+String(L", ",2);
			}
			t_extby=t_extby+t_decl2->m_ident;
			t_xextby=t_xextby+m_george->p_ResolveLink(t_decl2->p_PagePath(),t_decl2->m_ident);
		}
		t_maker->p_SetString(String(L"EXTENDED_BY",11),t_extby);
		t_maker->p_SetString(String(L"XEXTENDED_BY",12),t_xextby);
	}
	String t_eg=t_decl->m_docs->p_Get(String(L"example",7));
	if((t_eg).Length()!=0){
		t_eg=t_eg.Trim();
		if(t_eg.StartsWith(String(L"<pre>",5))){
			t_eg=t_eg.Slice(5);
		}
		if(t_eg.EndsWith(String(L"</pre>",6))){
			t_eg=t_eg.Slice(0,-6);
		}
		t_decl->m_docs->p_Set2(String(L"example",7),String(L"<pre>",5)+t_eg+String(L"</pre>",6));
		String t_file=t_decl->m_path.Replace(String(L".",1),String(L"_",1))+String(L".cxs",4);
		SaveString(t_eg,String(L"docs/html/examples/",19)+t_file);
		t_decl->m_docs->p_Set2(String(L"EXAMPLE_URL",11),String(L"examples/",9)+t_file);
		if(FileType(t_decl->m_egdir)==2){
			bb_os_CopyDir(t_decl->m_egdir,String(L"docs/html/examples/",19)+bb_os_StripExt(t_file)+String(L".data",5),true,false);
		}
	}
	c_NodeEnumerator3* t_2=t_decl->m_docs->p_ObjectEnumerator();
	while(t_2->p_HasNext()){
		c_Node3* t_it=t_2->p_NextObject();
		String t_html=t_it->p_Value();
		if((t_html).Length()!=0){
			t_html=t_markdown->p_ToHtml(t_it->p_Value());
		}
		t_maker->p_SetString(t_it->p_Key().ToUpper(),t_html);
	}
}
String c_ApiDoccer::p_Pluralize(String t_str){
	if(t_str.EndsWith(String(L"s",1))){
		return t_str+String(L"es",2);
	}
	if(t_str.EndsWith(String(L"y",1))){
		return t_str.Slice(0,-1)+String(L"ies",3);
	}
	return t_str+String(L"s",1);
}
void c_ApiDoccer::p_MakeScopeDocs(c_ScopeDecl* t_scope,c_PageMaker* t_maker){
	m_george->p_SetErrInfo(t_scope->m_path);
	gc_assign(m_linkScope,t_scope);
	c_Markdown* t_markdown=(new c_Markdown)->m_new((this),(m_george));
	String t_desc=t_scope->m_docs->p_Get(String(L"description",11));
	if((t_desc).Length()!=0){
		String t_summary=t_desc;
		int t_i=t_desc.Find(String(L".",1),0);
		if(t_i!=-1){
			t_i+=1;
			while(t_i<t_desc.Length() && (int)t_desc[t_i]>32){
				t_i+=1;
			}
			t_summary=t_desc.Slice(0,t_i);
		}
		t_scope->m_docs->p_Set2(String(L"summary",7),t_summary);
		if(t_summary==t_desc){
			if(!((t_scope->m_docs->p_Get(String(L"example",7))).Length()!=0) && !((t_scope->m_docs->p_Get(String(L"links",5))).Length()!=0)){
				t_scope->m_docs->p_Set2(String(L"description",11),String());
			}
		}
	}
	t_maker->p_Clear();
	p_AddDecl((t_scope),t_maker,t_markdown);
	String t_[]={String(L"class",5),String(L"function",8),String(L"method",6),String(L"property",8),String(L"global",6),String(L"field",5),String(L"const",5),String(L"import",6),String(L"interface",9),String(L"ctor",4),String(L"inherited_method",16),String(L"inherited_function",18),String(L"inherited_property",18),String(L"inherited_field",15)};
	Array<String > t_kinds=Array<String >(t_,14);
	t_scope->p_SortDecls();
	Array<String > t_2=t_kinds;
	int t_3=0;
	while(t_3<t_2.Length()){
		String t_kind=t_2[t_3];
		t_3=t_3+1;
		c_Stack3* t_decls=t_scope->p_GetDecls(t_kind);
		if(!((t_decls->p_Length2())!=0)){
			continue;
		}
		t_maker->p_BeginList(p_Pluralize(t_kind).ToUpper());
		c_Enumerator2* t_4=t_decls->p_ObjectEnumerator();
		while(t_4->p_HasNext()){
			c_Decl2* t_decl=t_4->p_NextObject();
			t_maker->p_AddItem();
			p_AddDecl(t_decl,t_maker,t_markdown);
		}
		t_maker->p_EndList();
	}
	m_linkScope=0;
	m_george->p_SetErrInfo(String());
}
void c_ApiDoccer::p_MakeDocs(){
	p_ResolveScopes();
	String t_template=LoadString(m_george->m_styledir+String(L"/scope_template.html",20));
	c_PageMaker* t_maker=(new c_PageMaker)->m_new(t_template);
	c_StringStack* t_decls_txt=(new c_StringStack)->m_new2();
	c_ValueEnumerator* t_=m_scopes->p_Values()->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_ScopeDecl* t_decl=t_->p_NextObject();
		p_MakeScopeDocs(t_decl,t_maker);
		String t_page=t_maker->p_MakePage();
		if((t_decl->m_template)!=0){
			t_decl->m_template->p_SetString(String(L"CONTENT",7),t_page);
			t_page=t_decl->m_template->p_MakePage();
		}
		m_george->p_SetPageContent(t_decl->p_PagePath(),t_page);
		t_decls_txt->p_Push4(p_Capitalize(t_decl->m_kind)+String(L" ",1)+t_decl->m_path+p_StripLinks(t_decl->m_type.Replace(String(L" ",1),String()))+String(L";",1)+m_george->p_GetPageUrl(t_decl->p_PagePath())+String(L";",1)+t_decl->m_srcinfo);
		c_NodeEnumerator5* t_2=t_decl->m_declsByKind->p_ObjectEnumerator();
		while(t_2->p_HasNext()){
			c_Node5* t_it=t_2->p_NextObject();
			String t_kind=t_it->p_Key();
			String t_tysep=String(L":",1);
			String t_1=t_kind;
			if(t_1==String(L"ctor",4)){
				t_kind=String(L"method",6);
				t_tysep=String();
			}else{
				if(t_1==String(L"import",6) || t_1==String(L"class",5) || t_1==String(L"interface",9)){
					continue;
				}
			}
			c_Enumerator2* t_3=t_it->p_Value()->p_ObjectEnumerator();
			while(t_3->p_HasNext()){
				c_Decl2* t_decl2=t_3->p_NextObject();
				t_decls_txt->p_Push4(p_Capitalize(t_kind)+String(L" ",1)+t_decl2->m_path+t_tysep+p_StripLinks(t_decl2->m_type.Replace(String(L" ",1),String()))+String(L";",1)+m_george->p_GetPageUrl(t_decl2->p_PagePath())+String(L";",1)+t_decl2->m_srcinfo);
			}
		}
	}
	SaveString(t_decls_txt->p_Join(String(L"\n",1)),String(L"docs/html/decls.txt",19));
}
String c_ApiDoccer::p_ResolveLink(String t_link,String t_text){
	if(t_link.Length()==1 && t_link==t_link.ToUpper()){
		return t_link;
	}
	c_Decl2* t_decl=m_linkScope->p_FindDecl(t_link);
	if((t_decl)!=0){
		return m_george->p_ResolveLink(t_decl->p_PagePath(),t_text);
	}
	return m_george->p_ResolveLink(t_link,t_text);
}
void c_ApiDoccer::mark(){
	Object::mark();
	gc_mark_q(m_george);
	gc_mark_q(m_scopeTemplate);
	gc_mark_q(m_scopes);
	gc_mark_q(m_linkScope);
}
c_DocsDoccer::c_DocsDoccer(){
	m_george=0;
	m_docs=(new c_StringMap2)->m_new();
}
c_DocsDoccer* c_DocsDoccer::m_new(c_George* t_george){
	gc_assign(this->m_george,t_george);
	return this;
}
c_DocsDoccer* c_DocsDoccer::m_new2(){
	return this;
}
void c_DocsDoccer::p_ParseDocs2(String t_dir,String t_indexcat){
	Array<String > t_=bb_os_LoadDir(t_dir,true,false);
	int t_2=0;
	while(t_2<t_.Length()){
		String t_f=t_[t_2];
		t_2=t_2+1;
		if(bb_os_ExtractExt(t_f)!=String(L"cerberusdoc",11) && bb_os_ExtractExt(t_f)!=String(L"monkeydoc",9)){
			continue;
		}
		String t_docpath=bb_os_StripExt(t_f);
		m_george->p_AddPage(t_docpath,String());
		m_docs->p_Set2(t_docpath,t_dir+String(L"/",1)+t_f);
	}
}
void c_DocsDoccer::p_ParseDocs(){
	p_ParseDocs2(String(L"docs/cerberusdoc",16),String());
	p_ParseDocs2(String(L"docs/monkeydoc",14),String());
}
void c_DocsDoccer::p_MakeDocs(){
	c_Markdown* t_markdown=(new c_Markdown)->m_new((this),(m_george));
	c_NodeEnumerator3* t_=m_docs->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_Node3* t_it=t_->p_NextObject();
		m_george->p_SetDocBase(t_it->p_Key());
		m_george->p_SetErrInfo(t_it->p_Value());
		String t_src=LoadString(t_it->p_Value());
		m_george->p_SetPageContent(t_it->p_Key(),t_markdown->p_ToHtml(t_src));
		String t_data=bb_os_StripExt(t_it->p_Value())+String(L".data",5);
		if(FileType(t_data)==2){
			bb_os_CopyDir(t_data,String(L"docs/html/data/",15)+bb_os_StripExt(bb_os_StripDir(t_it->p_Key())),true,false);
		}
	}
	m_george->p_SetDocBase(String());
}
String c_DocsDoccer::p_ResolveLink(String t_link,String t_text){
	if(t_link.StartsWith(String(L"../",3)) && (t_link.EndsWith(String(L".cxs",4)) || t_link.EndsWith(String(L".monkey",7)))){
		if(!((t_text).Length()!=0)){
			t_text=bb_os_StripDir(t_link);
		}
		return m_george->p_MakeLink(t_link,t_text);
	}
	return m_george->p_ResolveLink(t_link,t_text);
}
void c_DocsDoccer::mark(){
	Object::mark();
	gc_mark_q(m_george);
	gc_mark_q(m_docs);
}
String bb_modpath_LoadModpath(){
	String t_modpath=GetEnv(String(L"MODPATH",7));
	if((t_modpath).Length()!=0){
		return t_modpath;
	}
	String t_cfg=LoadString(String(L"bin/config.",11)+HostOS()+String(L".txt",4));
	Array<String > t_=t_cfg.Split(String(L"\n",1));
	int t_2=0;
	while(t_2<t_.Length()){
		String t_line=t_[t_2];
		t_2=t_2+1;
		t_line=t_line.Trim();
		if(t_line.StartsWith(String(L"'",1))){
			continue;
		}
		Array<String > t_bits=t_line.Split(String(L"=",1));
		if(t_bits.Length()!=2){
			continue;
		}
		String t_key=t_bits[0].Trim();
		String t_val=t_bits[1].Trim();
		if(t_key!=String(L"MODPATH",7)){
			continue;
		}
		int t_i=0;
		do{
			t_i=t_val.Find(String(L"${",2),t_i);
			if(t_i==-1){
				break;
			}
			int t_e=t_val.Find(String(L"}",1),t_i+2);
			if(t_e==-1){
				break;
			}
			String t_t=t_val.Slice(t_i+2,t_e);
			String t_1=t_t;
			if(t_1==String(L"MONKEYDIR",9)){
				t_t=CurrentDir();
			}else{
				if(t_1==String(L"CERBERUSDIR",11)){
					t_t=CurrentDir();
				}else{
					t_t=GetEnv(t_t);
				}
			}
			t_val=t_val.Slice(0,t_i)+t_t+t_val.Slice(t_e+1);
			t_i+=t_t.Length();
		}while(!(false));
		if(t_val.StartsWith(String(L"\"",1)) && t_val.EndsWith(String(L"\"",1))){
			t_val=t_val.Slice(1,-1);
		}
		return t_val;
	}
	return String();
}
c_PageMaker::c_PageMaker(){
	m__template=String();
	m__decls=(new c_StringMap)->m_new();
	m__scopes=(new c_Stack)->m_new();
	m__lists=(new c_Stack4)->m_new();
	m__iters=(new c_IntStack)->m_new2();
}
c_PageMaker* c_PageMaker::m_new(String t_template){
	m__template=t_template;
	m__scopes->p_Push(m__decls);
	return this;
}
c_PageMaker* c_PageMaker::m_new2(){
	return this;
}
void c_PageMaker::p_Clear(){
	m__decls->p_Clear();
	m__scopes->p_Clear();
	m__scopes->p_Push(m__decls);
	m__lists->p_Clear();
}
void c_PageMaker::p_SetString(String t_name,String t_value){
	m__scopes->p_Top()->p_Set(t_name,((new c_StringObject)->m_new3(t_value)));
}
void c_PageMaker::p_BeginList(String t_name){
	c_Stack* t_list=(new c_Stack)->m_new();
	m__scopes->p_Top()->p_Set(t_name,(t_list));
	m__scopes->p_Push(0);
	m__lists->p_Push10(t_list);
}
void c_PageMaker::p_AddItem(){
	c_StringMap* t_scope=(new c_StringMap)->m_new();
	m__scopes->p_Pop();
	m__scopes->p_Push(t_scope);
	m__lists->p_Top()->p_Push(t_scope);
}
void c_PageMaker::p_EndList(){
	m__scopes->p_Pop();
	m__lists->p_Pop();
}
Object* c_PageMaker::p_GetValue(String t_name){
	for(int t_i=m__scopes->p_Length2()-1;t_i>=0;t_i=t_i+-1){
		c_StringMap* t_sc=m__scopes->p_Get2(t_i);
		if(t_sc->p_Contains(t_name)){
			return t_sc->p_Get(t_name);
		}
	}
	return 0;
}
String c_PageMaker::p_GetString(String t_name){
	Object* t_val=p_GetValue(t_name);
	c_StringObject* t_str=dynamic_cast<c_StringObject*>(t_val);
	if((t_str)!=0){
		return t_str->m_value;
	}
	c_Stack* t_list=dynamic_cast<c_Stack*>(t_val);
	if(((t_list)!=0) && ((t_list->p_Length2())!=0)){
		return String(t_list->p_Length2());
	}
	return String();
}
c_Stack* c_PageMaker::p_GetList(String t_name){
	Object* t_val=p_GetValue(t_name);
	c_Stack* t_list=dynamic_cast<c_Stack*>(t_val);
	if(((t_list)!=0) && ((t_list->p_Length2())!=0)){
		return t_list;
	}
	return 0;
}
String c_PageMaker::p_MakePage(){
	m__iters->p_Clear();
	m__lists->p_Clear();
	m__scopes->p_Clear();
	m__scopes->p_Push(m__decls);
	c_StringStack* t_output=(new c_StringStack)->m_new2();
	int t_i=0;
	int t_ifnest=0;
	int t_iftrue=0;
	do{
		int t_i0=m__template.Find(String(L"${",2),t_i);
		if(t_i0==-1){
			break;
		}
		int t_i1=m__template.Find(String(L"}",1),t_i0+2);
		if(t_i1==-1){
			break;
		}
		bool t_cc=t_ifnest==t_iftrue;
		if(t_cc && t_i<t_i0){
			t_output->p_Push4(m__template.Slice(t_i,t_i0));
		}
		t_i=t_i1+1;
		Array<String > t_bits=m__template.Slice(t_i0+2,t_i1).Split(String(L" ",1)).Resize(5);
		String t_1=t_bits[0];
		if(t_1==String(L"IF",2)){
			t_ifnest+=1;
			if(t_cc){
				int t_i2=1;
				bool t_inv=false;
				if(t_bits[t_i2]==String(L"NOT",3)){
					t_inv=true;
					t_i2+=1;
				}
				if(t_bits[t_i2]==String(L"FIRST",5)){
					t_cc=m__iters->p_Get2(m__iters->p_Length2()-2)==0;
				}else{
					if(t_bits[t_i2]==String(L"LAST",4)){
						t_cc=m__iters->p_Get2(m__iters->p_Length2()-2)==m__lists->p_Top()->p_Length2()-1;
					}else{
						if(t_bits[t_i2+1]==String(L"EQ",2)){
							t_cc=p_GetString(t_bits[t_i2])==t_bits[t_i2+2];
						}else{
							if(t_bits[t_i2+1]==String(L"NE",2)){
								t_cc=p_GetString(t_bits[t_i2])!=t_bits[t_i2+2];
							}else{
								t_cc=p_GetString(t_bits[t_i2])!=String();
							}
						}
					}
				}
				if(t_inv){
					t_cc=!t_cc;
				}
				if(t_cc){
					t_iftrue=t_ifnest;
				}
			}
		}else{
			if(t_1==String(L"ENDIF",5)){
				t_ifnest-=1;
				t_iftrue=bb_math_Min(t_iftrue,t_ifnest);
			}else{
				if(t_1==String(L"FOR",3)){
					t_ifnest+=1;
					if(t_cc){
						c_Stack* t_list=p_GetList(t_bits[1]);
						if((t_list)!=0){
							t_iftrue=t_ifnest;
							m__iters->p_Push13(0);
							m__iters->p_Push13(t_i);
							m__lists->p_Push10(t_list);
							m__scopes->p_Push(t_list->p_Get2(0));
						}
					}
				}else{
					if(t_1==String(L"NEXT",4)){
						if(t_cc){
							m__scopes->p_Pop();
							c_Stack* t_list2=m__lists->p_Top();
							int t_p=m__iters->p_Pop();
							int t_j=m__iters->p_Pop()+1;
							if(t_j<t_list2->p_Length2()){
								m__iters->p_Push13(t_j);
								m__iters->p_Push13(t_p);
								m__scopes->p_Push(t_list2->p_Get2(t_j));
								t_i=t_p;
							}else{
								m__lists->p_Pop();
							}
						}
						t_ifnest-=1;
						t_iftrue=bb_math_Min(t_iftrue,t_ifnest);
					}else{
						if(t_cc){
							String t_str=p_GetString(t_bits[0]);
							if((t_str).Length()!=0){
								t_output->p_Push4(t_str);
							}
						}
					}
				}
			}
		}
	}while(!(false));
	if(t_i<m__template.Length()){
		t_output->p_Push4(m__template.Slice(t_i));
	}
	return t_output->p_Join(String());
}
void c_PageMaker::mark(){
	Object::mark();
	gc_mark_q(m__decls);
	gc_mark_q(m__scopes);
	gc_mark_q(m__lists);
	gc_mark_q(m__iters);
}
c_Stack::c_Stack(){
	m_data=Array<c_StringMap* >();
	m_length=0;
}
c_Stack* c_Stack::m_new(){
	return this;
}
c_Stack* c_Stack::m_new2(Array<c_StringMap* > t_data){
	gc_assign(this->m_data,t_data.Slice(0));
	this->m_length=t_data.Length();
	return this;
}
void c_Stack::p_Push(c_StringMap* t_value){
	if(m_length==m_data.Length()){
		gc_assign(m_data,m_data.Resize(m_length*2+10));
	}
	gc_assign(m_data[m_length],t_value);
	m_length+=1;
}
void c_Stack::p_Push2(Array<c_StringMap* > t_values,int t_offset,int t_count){
	for(int t_i=0;t_i<t_count;t_i=t_i+1){
		p_Push(t_values[t_offset+t_i]);
	}
}
void c_Stack::p_Push3(Array<c_StringMap* > t_values,int t_offset){
	p_Push2(t_values,t_offset,t_values.Length()-t_offset);
}
c_StringMap* c_Stack::m_NIL;
void c_Stack::p_Clear(){
	for(int t_i=0;t_i<m_length;t_i=t_i+1){
		gc_assign(m_data[t_i],m_NIL);
	}
	m_length=0;
}
c_StringMap* c_Stack::p_Top(){
	return m_data[m_length-1];
}
c_StringMap* c_Stack::p_Pop(){
	m_length-=1;
	c_StringMap* t_v=m_data[m_length];
	gc_assign(m_data[m_length],m_NIL);
	return t_v;
}
void c_Stack::p_Length(int t_newlength){
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
int c_Stack::p_Length2(){
	return m_length;
}
c_StringMap* c_Stack::p_Get2(int t_index){
	return m_data[t_index];
}
void c_Stack::mark(){
	Object::mark();
	gc_mark_q(m_data);
}
String bb_os_ExtractExt(String t_path){
	int t_i=t_path.FindLast(String(L".",1));
	if(t_i!=-1 && t_path.Find(String(L"/",1),t_i+1)==-1 && t_path.Find(String(L"\\",1),t_i+1)==-1){
		return t_path.Slice(t_i+1);
	}
	return String();
}
String bb_os_StripExt(String t_path){
	int t_i=t_path.FindLast(String(L".",1));
	if(t_i!=-1 && t_path.Find(String(L"/",1),t_i+1)==-1 && t_path.Find(String(L"\\",1),t_i+1)==-1){
		return t_path.Slice(0,t_i);
	}
	return t_path;
}
c_Stack2::c_Stack2(){
	m_data=Array<String >();
	m_length=0;
}
c_Stack2* c_Stack2::m_new(){
	return this;
}
c_Stack2* c_Stack2::m_new2(Array<String > t_data){
	gc_assign(this->m_data,t_data.Slice(0));
	this->m_length=t_data.Length();
	return this;
}
void c_Stack2::p_Push4(String t_value){
	if(m_length==m_data.Length()){
		gc_assign(m_data,m_data.Resize(m_length*2+10));
	}
	m_data[m_length]=t_value;
	m_length+=1;
}
void c_Stack2::p_Push5(Array<String > t_values,int t_offset,int t_count){
	for(int t_i=0;t_i<t_count;t_i=t_i+1){
		p_Push4(t_values[t_offset+t_i]);
	}
}
void c_Stack2::p_Push6(Array<String > t_values,int t_offset){
	p_Push5(t_values,t_offset,t_values.Length()-t_offset);
}
Array<String > c_Stack2::p_ToArray(){
	Array<String > t_t=Array<String >(m_length);
	for(int t_i=0;t_i<m_length;t_i=t_i+1){
		t_t[t_i]=m_data[t_i];
	}
	return t_t;
}
bool c_Stack2::p_IsEmpty(){
	return m_length==0;
}
String c_Stack2::m_NIL;
void c_Stack2::p_Clear(){
	for(int t_i=0;t_i<m_length;t_i=t_i+1){
		m_data[t_i]=m_NIL;
	}
	m_length=0;
}
void c_Stack2::p_Length(int t_newlength){
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
int c_Stack2::p_Length2(){
	return m_length;
}
String c_Stack2::p_Get2(int t_index){
	return m_data[t_index];
}
void c_Stack2::mark(){
	Object::mark();
	gc_mark_q(m_data);
}
c_StringStack::c_StringStack(){
}
c_StringStack* c_StringStack::m_new(Array<String > t_data){
	c_Stack2::m_new2(t_data);
	return this;
}
c_StringStack* c_StringStack::m_new2(){
	c_Stack2::m_new();
	return this;
}
String c_StringStack::p_Join(String t_separator){
	return t_separator.Join(p_ToArray());
}
void c_StringStack::mark(){
	c_Stack2::mark();
}
c_Map2::c_Map2(){
	m_root=0;
}
c_Map2* c_Map2::m_new(){
	return this;
}
int c_Map2::p_RotateLeft2(c_Node3* t_node){
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
int c_Map2::p_RotateRight2(c_Node3* t_node){
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
int c_Map2::p_InsertFixup2(c_Node3* t_node){
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
					p_RotateLeft2(t_node);
				}
				t_node->m_parent->m_color=1;
				t_node->m_parent->m_parent->m_color=-1;
				p_RotateRight2(t_node->m_parent->m_parent);
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
bool c_Map2::p_Add(String t_key,String t_value){
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
		p_InsertFixup2(t_node);
	}else{
		gc_assign(m_root,t_node);
	}
	return true;
}
bool c_Map2::p_Set2(String t_key,String t_value){
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
				t_node->m_value=t_value;
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
		p_InsertFixup2(t_node);
	}else{
		gc_assign(m_root,t_node);
	}
	return true;
}
c_Node3* c_Map2::p_FindNode(String t_key){
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
bool c_Map2::p_Contains(String t_key){
	return p_FindNode(t_key)!=0;
}
c_Node3* c_Map2::p_FirstNode(){
	if(!((m_root)!=0)){
		return 0;
	}
	c_Node3* t_node=m_root;
	while((t_node->m_left)!=0){
		t_node=t_node->m_left;
	}
	return t_node;
}
c_NodeEnumerator3* c_Map2::p_ObjectEnumerator(){
	return (new c_NodeEnumerator3)->m_new(p_FirstNode());
}
String c_Map2::p_Get(String t_key){
	c_Node3* t_node=p_FindNode(t_key);
	if((t_node)!=0){
		return t_node->m_value;
	}
	return String();
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
c_Node3::c_Node3(){
	m_key=String();
	m_right=0;
	m_left=0;
	m_value=String();
	m_color=0;
	m_parent=0;
}
c_Node3* c_Node3::m_new(String t_key,String t_value,int t_color,c_Node3* t_parent){
	this->m_key=t_key;
	this->m_value=t_value;
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
String c_Node3::p_Key(){
	return m_key;
}
String c_Node3::p_Value(){
	return m_value;
}
void c_Node3::mark(){
	Object::mark();
	gc_mark_q(m_right);
	gc_mark_q(m_left);
	gc_mark_q(m_parent);
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
c_Parser::c_Parser(){
	m__toker=0;
	m__toke=String();
	m__tokeType=0;
}
String c_Parser::p_Bump(){
	do{
		String t_toke=m__toker->p_Bump();
		int t_type=m__toker->p_TokeType();
		if(t_type==1){
			continue;
		}
		if(t_type==2){
			continue;
		}
		m__toke=t_toke.ToLower();
		m__tokeType=t_type;
		return m__toke;
	}while(!(false));
}
c_Parser* c_Parser::m_new(String t_src){
	gc_assign(m__toker,(new c_Toker)->m_new(t_src));
	p_Bump();
	return this;
}
c_Parser* c_Parser::m_new2(){
	return this;
}
void c_Parser::p_SetText(String t_src){
	m__toker->p_SetText(t_src);
	m__toke=String();
	m__tokeType=0;
	p_Bump();
}
String c_Parser::p_Toke(){
	return m__toke;
}
String c_Parser::p_Parse(){
	String t_tmp=p_Toke();
	p_Bump();
	return t_tmp;
}
void c_Parser::p_Err(String t_msg){
	bbPrint(String(L"Toke=",5)+p_Toke());
	String t_text=m__toker->p_Text();
	int t_cursor=m__toker->p_Cursor();
	bbPrint(t_text.Slice(0,t_cursor)+String(L"<<<<<HERE?",10)+t_text.Slice(t_cursor));
	bbError(t_msg);
}
void c_Parser::p_Parse2(String t_toke){
	if(p_Toke()!=t_toke){
		p_Err(String(L"Parse error",11));
	}
	p_Bump();
}
int c_Parser::p_TokeType(){
	return m__tokeType;
}
String c_Parser::p_Parse3(int t_type){
	if(p_TokeType()!=t_type){
		p_Err(String(L"Parse error",11));
	}
	return p_Parse();
}
bool c_Parser::p_CParse(String t_toke){
	if(p_Toke()!=t_toke){
		return false;
	}
	p_Bump();
	return true;
}
String c_Parser::p_CParse2(int t_type){
	if(p_TokeType()!=t_type){
		return String();
	}
	return p_Parse();
}
String c_Parser::p_ParseIdent(){
	p_CParse(String(L"@",1));
	if(p_TokeType()!=3){
		p_Err(String(L"Parse error",11));
	}
	String t_id=m__toker->p_Toke();
	p_Bump();
	return t_id;
}
String c_Parser::p_ParseIdentSeq(){
	c_StringStack* t_args=(new c_StringStack)->m_new2();
	do{
		t_args->p_Push4(p_ParseIdent());
	}while(!(!p_CParse(String(L",",1))));
	return t_args->p_Join(String(L",",1));
}
String c_Parser::p_ParseType(){
	String t_ty=String();
	if(p_CParse(String(L":",1)) || p_TokeType()==3){
		if(p_CParse(String(L"=",1))){
			if(p_CParse(String(L"new",3))){
				if(p_TokeType()==3){
					t_ty=p_ParseType();
				}
			}else{
				int t_2=p_TokeType();
				if(t_2==4){
					p_Parse();
					t_ty=String(L"int",3);
				}else{
					if(t_2==5){
						p_Parse();
						t_ty=String(L"float",5);
					}else{
						if(t_2==6){
							p_Parse();
							t_ty=String(L"string",6);
						}
					}
				}
			}
			if(!((t_ty).Length()!=0)){
				bbPrint(String(L"Inferred types not allowed: ",28)+m__toker->p_Text());
				return String();
			}
		}else{
			if(p_TokeType()==3){
				String t_id=p_ParseIdent();
				String t_3=t_id.ToLower();
				if(t_3==String(L"void",4) || t_3==String(L"bool",4) || t_3==String(L"int",3) || t_3==String(L"float",5) || t_3==String(L"string",6) || t_3==String(L"object",6)){
					t_ty=t_ty+t_id;
				}else{
					t_ty=t_ty+(String(L"[[",2)+t_id+String(L"]]",2));
				}
			}else{
				p_Err(String(L"Parse error",11));
			}
		}
	}else{
		if(p_CParse(String(L"$",1))){
			t_ty=String(L"string",6);
		}else{
			if(p_CParse(String(L"#",1))){
				t_ty=String(L"float",5);
			}else{
				if(p_CParse(String(L"%",1))){
					t_ty=String(L"int",3);
				}else{
					if(p_CParse(String(L"?",1))){
						t_ty=String(L"bool",4);
					}else{
						t_ty=String(L"int",3);
					}
				}
			}
		}
	}
	if(p_CParse(String(L"<",1))){
		c_StringStack* t_args=(new c_StringStack)->m_new2();
		do{
			t_args->p_Push4(p_ParseType());
		}while(!(!p_CParse(String(L",",1))));
		p_Parse2(String(L">",1));
		t_ty=t_ty+(String(L"<",1)+t_args->p_Join(String(L", ",2))+String(L">",1));
	}
	while(p_CParse(String(L"[",1))){
		while(((p_TokeType())!=0) && p_Toke()!=String(L"]",1)){
			p_Parse();
		}
		p_CParse(String(L"]",1));
		t_ty=t_ty+String(L"[]",2);
	}
	return t_ty;
}
String c_Parser::p_ParseTypeSeq(){
	c_StringStack* t_args=(new c_StringStack)->m_new2();
	do{
		t_args->p_Push4(p_ParseType());
	}while(!(!p_CParse(String(L",",1))));
	return t_args->p_Join(String(L",",1));
}
String c_Parser::p_ParseArgs(){
	if(!p_CParse(String(L"(",1))){
		p_Err(String(L"Parse error",11));
	}
	if(p_CParse(String(L")",1))){
		return String(L" ()",3);
	}
	c_StringStack* t_args=(new c_StringStack)->m_new2();
	do{
		String t_id=p_ParseIdent();
		String t_ty=p_ParseType();
		if(p_CParse(String(L"=",1))){
			t_ty=t_ty+String(L"=",1);
			while(p_Toke()!=String(L",",1) && p_Toke()!=String(L")",1)){
				t_ty=t_ty+p_Toke();
				p_Bump();
			}
		}
		t_args->p_Push4(t_id+String(L":",1)+t_ty);
	}while(!(!p_CParse(String(L",",1))));
	p_Parse2(String(L")",1));
	return String(L" ( ",3)+t_args->p_Join(String(L", ",2))+String(L" )",2);
}
c_Decl* c_Parser::p_ParseDecl(){
	c_Decl* t_decl=0;
	String t_1=p_Toke();
	if(t_1==String(L"module",6)){
		t_decl=(new c_Decl)->m_new(p_Parse(),String());
		t_decl->m_ident=p_ParseIdent();
	}else{
		if(t_1==String(L"class",5) || t_1==String(L"interface",9)){
			t_decl=(new c_Decl)->m_new(p_Parse(),String());
			t_decl->m_ident=p_ParseIdent();
			if(p_CParse(String(L"<",1))){
				t_decl->m_type=String(L"<",1)+p_ParseIdentSeq()+String(L">",1);
				p_Parse2(String(L">",1));
			}
			if(p_CParse(String(L"extends",7))){
				t_decl->m_exts=p_ParseType();
			}
			if(p_CParse(String(L"implements",10))){
				t_decl->m_impls=p_ParseTypeSeq();
			}
		}else{
			if(t_1==String(L"import",6)){
				String t_kind=p_Parse();
				if(p_TokeType()!=3){
					return 0;
				}
				t_decl=(new c_Decl)->m_new(t_kind,String());
				t_decl->m_ident=p_ParseIdent();
			}else{
				if(t_1==String(L"const",5)){
					t_decl=(new c_Decl)->m_new(p_Parse(),String());
					t_decl->m_ident=p_ParseIdent();
					t_decl->m_type=p_ParseType();
					if(p_CParse(String(L"=",1))){
						t_decl->m_init=p_Parse();
					}
				}else{
					if(t_1==String(L"global",6) || t_1==String(L"field",5)){
						t_decl=(new c_Decl)->m_new(p_Parse(),String());
						t_decl->m_ident=p_ParseIdent();
						t_decl->m_type=p_ParseType();
					}else{
						if(t_1==String(L"method",6) || t_1==String(L"function",8)){
							t_decl=(new c_Decl)->m_new(p_Parse(),String());
							t_decl->m_ident=p_ParseIdent();
							t_decl->m_type=p_ParseType();
							t_decl->m_type=t_decl->m_type+p_ParseArgs();
							if(t_decl->m_ident==String(L"New",3)){
								int t_i=t_decl->m_type.Find(String(L"(",1),0);
								if(t_i!=-1){
									t_decl->m_type=t_decl->m_type.Slice(t_i);
								}
								t_decl->m_kind=String(L"ctor",4);
							}else{
								if(p_CParse(String(L"property",8))){
									t_decl->m_kind=String(L"property",8);
								}
							}
						}else{
							p_Err(String(L"Parse error",11));
						}
					}
				}
			}
		}
	}
	return t_decl;
}
c_Toker* c_Parser::p_GetToker(){
	return m__toker;
}
String c_Parser::p_GetText(){
	return m__toker->p_Text().Slice(m__toker->p_Cursor());
}
void c_Parser::mark(){
	Object::mark();
	gc_mark_q(m__toker);
}
c_Toker::c_Toker(){
	m__text=String();
	m__len=0;
	m__toke=String();
	m__type=0;
	m__pos=0;
}
c_Toker* c_Toker::m_new(String t_text){
	m__text=t_text;
	m__len=m__text.Length();
	return this;
}
c_Toker* c_Toker::m_new2(c_Toker* t_toker){
	m__text=t_toker->m__text;
	m__len=t_toker->m__len;
	m__toke=t_toker->m__toke;
	m__type=t_toker->m__type;
	m__pos=t_toker->m__pos;
	return this;
}
c_Toker* c_Toker::m_new3(){
	return this;
}
int c_Toker::p_Chr(int t_offset){
	if(m__pos+t_offset<m__len){
		return (int)m__text[m__pos+t_offset];
	}
	return 0;
}
String c_Toker::p_Str(int t_offset){
	if(m__pos+t_offset<m__len){
		return m__text.Slice(m__pos+t_offset,m__pos+t_offset+1);
	}
	return String();
}
int c_Toker::p_IsAlpha(int t_ch){
	return ((t_ch>=65 && t_ch<=90 || t_ch>=97 && t_ch<=122)?1:0);
}
int c_Toker::p_IsDigit(int t_ch){
	return ((t_ch>=48 && t_ch<=57)?1:0);
}
int c_Toker::p_IsBinDigit(int t_ch){
	return ((t_ch==48 || t_ch==49)?1:0);
}
int c_Toker::p_IsHexDigit(int t_ch){
	return ((t_ch>=48 && t_ch<=57 || t_ch>=65 && t_ch<=70 || t_ch>=97 && t_ch<=102)?1:0);
}
String c_Toker::p_Bump(){
	if(m__pos==m__len){
		m__toke=String();
		m__type=0;
		return m__toke;
	}
	int t_stp=m__pos;
	int t_chr=p_Chr(0);
	String t_str=p_Str(0);
	m__pos+=1;
	if(t_chr==10){
		m__type=1;
	}else{
		if(t_chr==13){
			m__pos+=1;
		}else{
			if(t_str==String(L"'",1)){
				while(((p_Chr(0))!=0) && p_Chr(0)!=10){
					m__pos+=1;
				}
				if(p_Chr(0)==10){
					m__pos+=1;
				}
				m__type=1;
			}else{
				if(t_chr<=32){
					while(((p_Chr(0))!=0) && p_Chr(0)<=32){
						m__pos+=1;
					}
					m__type=2;
				}else{
					if(((p_IsAlpha(t_chr))!=0) || t_str==String(L"_",1)){
						while(((p_IsAlpha(p_Chr(0)))!=0) || ((p_IsDigit(p_Chr(0)))!=0) || p_Str(0)==String(L"_",1)){
							m__pos+=1;
						}
						while(p_Chr(0)==46){
							m__pos+=1;
							while(((p_IsAlpha(p_Chr(0)))!=0) || ((p_IsDigit(p_Chr(0)))!=0) || p_Str(0)==String(L"_",1)){
								m__pos+=1;
							}
						}
						m__type=3;
					}else{
						if(t_chr==34){
							while(p_Chr(0)!=34 && p_Chr(0)!=10){
								m__pos+=1;
							}
							if(p_Chr(0)==34){
								m__pos+=1;
							}
							m__type=6;
						}else{
							if(((p_IsDigit(t_chr))!=0) || t_str==String(L".",1) && ((p_IsDigit(p_Chr(0)))!=0)){
								m__type=4;
								if(t_str==String(L".",1)){
									m__type=5;
								}
								while((p_IsDigit(p_Chr(0)))!=0){
									m__pos+=1;
								}
								if(m__type==4 && p_Str(0)==String(L".",1) && ((p_IsDigit(p_Chr(1)))!=0)){
									m__type=5;
									m__pos+=2;
									while((p_IsDigit(p_Chr(0)))!=0){
										m__pos+=1;
									}
								}
								if(p_Str(0).ToLower()==String(L"e",1)){
									m__type=5;
									m__pos+=1;
									if(p_Str(0)==String(L"+",1) || p_Str(0)==String(L"-",1)){
										m__pos+=1;
									}
									while((p_IsDigit(p_Chr(0)))!=0){
										m__pos+=1;
									}
								}
							}else{
								if(t_str==String(L"%",1) && ((p_IsBinDigit(p_Chr(0)))!=0)){
									m__type=4;
									m__pos+=1;
									while((p_IsBinDigit(p_Chr(0)))!=0){
										m__pos+=1;
									}
								}else{
									if(t_str==String(L"$",1) && ((p_IsHexDigit(p_Chr(0)))!=0)){
										m__type=4;
										m__pos+=1;
										while((p_IsHexDigit(p_Chr(0)))!=0){
											m__pos+=1;
										}
									}else{
										while(p_Chr(0)==35){
											m__pos+=1;
										}
										m__type=7;
									}
								}
							}
						}
					}
				}
			}
		}
	}
	m__toke=m__text.Slice(t_stp,m__pos);
	return m__toke;
}
int c_Toker::p_TokeType(){
	return m__type;
}
void c_Toker::p_SetText(String t_text){
	m__text=t_text;
	m__len=m__text.Length();
	m__toke=String();
	m__type=0;
	m__pos=0;
}
String c_Toker::p_Text(){
	return m__text;
}
int c_Toker::p_Cursor(){
	return m__pos;
}
String c_Toker::p_Toke(){
	return m__toke;
}
void c_Toker::mark(){
	Object::mark();
}
c_Decl::c_Decl(){
	m_kind=String();
	m_ident=String();
	m_type=String();
	m_exts=String();
	m_impls=String();
	m_init=String();
}
c_Decl* c_Decl::m_new(String t_kind,String t_ident){
	this->m_kind=t_kind;
	this->m_ident=t_ident;
	return this;
}
c_Decl* c_Decl::m_new2(){
	return this;
}
void c_Decl::mark(){
	Object::mark();
}
c_Decl2::c_Decl2(){
	m_kind=String();
	m_ident=String();
	m_type=String();
	m_texts=String();
	m_timpls=String();
	m_scope=0;
	m_path=String();
	m_uident=String();
	m_docs=(new c_StringMap2)->m_new();
	m_egdir=String();
	m_srcinfo=String();
}
void c_Decl2::p_Init(String t_kind,String t_ident,String t_type,String t_texts,String t_timpls,c_ScopeDecl* t_scope){
	this->m_kind=t_kind;
	this->m_ident=t_ident;
	this->m_type=t_type;
	this->m_texts=t_texts;
	this->m_timpls=t_timpls;
	gc_assign(this->m_scope,t_scope);
	m_path=t_ident;
	m_uident=t_ident;
	if((t_scope)!=0){
		int t_i=1;
		while(t_scope->m_declsByUident->p_Contains(m_uident)){
			t_i+=1;
			m_uident=t_ident+String(L"(",1)+String(t_i)+String(L")",1);
		}
		m_path=t_scope->m_path+String(L".",1)+t_ident;
		t_scope->m_decls->p_Push7(this);
		t_scope->m_declsByUident->p_Set3(m_uident,this);
		if((dynamic_cast<c_AliasDecl*>(this))!=0){
			t_scope->p_GetDecls(String(L"inherited_",10)+t_kind)->p_Push7(this);
		}else{
			t_scope->p_GetDecls(t_kind)->p_Push7(this);
		}
	}
	m_docs->p_Set2(String(L"description",11),String());
	m_docs->p_Set2(String(L"example",7),String());
	m_docs->p_Set2(String(L"links",5),String());
	m_docs->p_Set2(String(L"params",6),String());
	m_docs->p_Set2(String(L"returns",7),String());
}
c_Decl2* c_Decl2::m_new(c_Decl2* t_decl,c_ScopeDecl* t_scope){
	p_Init(t_decl->m_kind,t_decl->m_ident,t_decl->m_type,t_decl->m_texts,t_decl->m_timpls,t_scope);
	return this;
}
c_Decl2* c_Decl2::m_new2(c_Decl* t_pdecl,c_ScopeDecl* t_scope){
	p_Init(t_pdecl->m_kind,t_pdecl->m_ident,t_pdecl->m_type,t_pdecl->m_exts,t_pdecl->m_impls,t_scope);
	return this;
}
c_Decl2* c_Decl2::m_new3(){
	return this;
}
String c_Decl2::p_PagePath(){
	if((dynamic_cast<c_ModuleDecl*>(this))!=0){
		return String(L"Modules/",8)+m_uident;
	}else{
		if((dynamic_cast<c_ScopeDecl*>(this))!=0){
			return m_scope->p_PagePath()+String(L"/",1)+m_uident;
		}else{
			return m_scope->p_PagePath()+String(L"#",1)+m_uident;
		}
	}
}
c_Decl2* c_Decl2::p_FindDeclHere(String t_path){
	if(t_path==m_uident){
		return this;
	}
	return 0;
}
c_Decl2* c_Decl2::p_FindDecl(String t_path){
	c_Decl2* t_decl=p_FindDeclHere(t_path);
	if(!((t_decl)!=0) && ((m_scope)!=0)){
		t_decl=m_scope->p_FindDecl(t_path);
	}
	return t_decl;
}
void c_Decl2::mark(){
	Object::mark();
	gc_mark_q(m_scope);
	gc_mark_q(m_docs);
}
c_ScopeDecl::c_ScopeDecl(){
	m_declsByUident=(new c_StringMap3)->m_new();
	m_decls=(new c_Stack3)->m_new();
	m_declsByKind=(new c_StringMap4)->m_new();
	m_template=0;
}
c_Stack3* c_ScopeDecl::p_GetDecls(String t_kind){
	c_Stack3* t_decls=m_declsByKind->p_Get(t_kind);
	if(!((t_decls)!=0)){
		t_decls=(new c_Stack3)->m_new();
		m_declsByKind->p_Set4(t_kind,t_decls);
	}
	return t_decls;
}
c_ScopeDecl* c_ScopeDecl::m_new(c_Decl* t_pdecl,c_ScopeDecl* t_scope){
	c_Decl2::m_new2(t_pdecl,t_scope);
	return this;
}
c_ScopeDecl* c_ScopeDecl::m_new2(){
	c_Decl2::m_new3();
	return this;
}
void c_ScopeDecl::p_SortDecls(){
	c_StringMap3* t_map=(new c_StringMap3)->m_new();
	c_NodeEnumerator5* t_=m_declsByKind->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_Node5* t_it=t_->p_NextObject();
		c_Stack3* t_stack=t_it->p_Value();
		t_map->p_Clear();
		c_Enumerator2* t_2=t_stack->p_ObjectEnumerator();
		while(t_2->p_HasNext()){
			c_Decl2* t_decl=t_2->p_NextObject();
			t_map->p_Set3(t_decl->m_uident,t_decl);
		}
		t_stack->p_Clear();
		c_NodeEnumerator6* t_3=t_map->p_ObjectEnumerator();
		while(t_3->p_HasNext()){
			c_Node4* t_it2=t_3->p_NextObject();
			t_stack->p_Push7(t_it2->p_Value());
		}
	}
}
c_Decl2* c_ScopeDecl::p_FindDeclHere(String t_path){
	int t_i=t_path.Find(String(L".",1),0);
	if(t_i==-1){
		return m_declsByUident->p_Get(t_path);
	}
	c_Decl2* t_t=m_declsByUident->p_Get(t_path.Slice(0,t_i));
	if((t_t)!=0){
		return t_t->p_FindDeclHere(t_path.Slice(t_i+1));
	}
	return 0;
}
void c_ScopeDecl::mark(){
	c_Decl2::mark();
	gc_mark_q(m_declsByUident);
	gc_mark_q(m_decls);
	gc_mark_q(m_declsByKind);
	gc_mark_q(m_template);
}
c_ModuleDecl::c_ModuleDecl(){
	m_doccer=0;
	m_busy=false;
}
c_ModuleDecl* c_ModuleDecl::m_new(c_Decl* t_pdecl,c_ApiDoccer* t_doccer){
	c_ScopeDecl::m_new(t_pdecl,0);
	gc_assign(this->m_doccer,t_doccer);
	gc_assign(this->m_template,t_doccer->m_scopeTemplate);
	return this;
}
c_ModuleDecl* c_ModuleDecl::m_new2(){
	c_ScopeDecl::m_new2();
	return this;
}
c_Decl2* c_ModuleDecl::p_FindDeclHere(String t_path){
	c_Decl2* t_decl=c_ScopeDecl::p_FindDeclHere(t_path);
	if(((t_decl)!=0) || m_busy){
		return t_decl;
	}
	m_busy=true;
	c_Enumerator2* t_=p_GetDecls(String(L"import",6))->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_Decl2* t_imp=t_->p_NextObject();
		t_decl=t_imp->p_FindDeclHere(t_path);
		if((t_decl)!=0){
			break;
		}
	}
	if(!((t_decl)!=0)){
		t_decl=m_doccer->m_scopes->p_Get(String(L"cerberus.lang",13))->p_FindDeclHere(t_path);
	}
	m_busy=false;
	return t_decl;
}
void c_ModuleDecl::mark(){
	c_ScopeDecl::mark();
	gc_mark_q(m_doccer);
}
c_Map3::c_Map3(){
	m_root=0;
}
c_Map3* c_Map3::m_new(){
	return this;
}
c_Node4* c_Map3::p_FindNode(String t_key){
	c_Node4* t_node=m_root;
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
bool c_Map3::p_Contains(String t_key){
	return p_FindNode(t_key)!=0;
}
int c_Map3::p_RotateLeft3(c_Node4* t_node){
	c_Node4* t_child=t_node->m_right;
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
int c_Map3::p_RotateRight3(c_Node4* t_node){
	c_Node4* t_child=t_node->m_left;
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
int c_Map3::p_InsertFixup3(c_Node4* t_node){
	while(((t_node->m_parent)!=0) && t_node->m_parent->m_color==-1 && ((t_node->m_parent->m_parent)!=0)){
		if(t_node->m_parent==t_node->m_parent->m_parent->m_left){
			c_Node4* t_uncle=t_node->m_parent->m_parent->m_right;
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
			c_Node4* t_uncle2=t_node->m_parent->m_parent->m_left;
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
bool c_Map3::p_Set3(String t_key,c_Decl2* t_value){
	c_Node4* t_node=m_root;
	c_Node4* t_parent=0;
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
	t_node=(new c_Node4)->m_new(t_key,t_value,-1,t_parent);
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
int c_Map3::p_Clear(){
	m_root=0;
	return 0;
}
c_Node4* c_Map3::p_FirstNode(){
	if(!((m_root)!=0)){
		return 0;
	}
	c_Node4* t_node=m_root;
	while((t_node->m_left)!=0){
		t_node=t_node->m_left;
	}
	return t_node;
}
c_NodeEnumerator6* c_Map3::p_ObjectEnumerator(){
	return (new c_NodeEnumerator6)->m_new(p_FirstNode());
}
c_Decl2* c_Map3::p_Get(String t_key){
	c_Node4* t_node=p_FindNode(t_key);
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
c_Node4::c_Node4(){
	m_key=String();
	m_right=0;
	m_left=0;
	m_value=0;
	m_color=0;
	m_parent=0;
}
c_Node4* c_Node4::m_new(String t_key,c_Decl2* t_value,int t_color,c_Node4* t_parent){
	this->m_key=t_key;
	gc_assign(this->m_value,t_value);
	this->m_color=t_color;
	gc_assign(this->m_parent,t_parent);
	return this;
}
c_Node4* c_Node4::m_new2(){
	return this;
}
c_Node4* c_Node4::p_NextNode(){
	c_Node4* t_node=0;
	if((m_right)!=0){
		t_node=m_right;
		while((t_node->m_left)!=0){
			t_node=t_node->m_left;
		}
		return t_node;
	}
	t_node=this;
	c_Node4* t_parent=this->m_parent;
	while(((t_parent)!=0) && t_node==t_parent->m_right){
		t_node=t_parent;
		t_parent=t_parent->m_parent;
	}
	return t_parent;
}
c_Decl2* c_Node4::p_Value(){
	return m_value;
}
void c_Node4::mark(){
	Object::mark();
	gc_mark_q(m_right);
	gc_mark_q(m_left);
	gc_mark_q(m_value);
	gc_mark_q(m_parent);
}
c_Stack3::c_Stack3(){
	m_data=Array<c_Decl2* >();
	m_length=0;
}
c_Stack3* c_Stack3::m_new(){
	return this;
}
c_Stack3* c_Stack3::m_new2(Array<c_Decl2* > t_data){
	gc_assign(this->m_data,t_data.Slice(0));
	this->m_length=t_data.Length();
	return this;
}
void c_Stack3::p_Push7(c_Decl2* t_value){
	if(m_length==m_data.Length()){
		gc_assign(m_data,m_data.Resize(m_length*2+10));
	}
	gc_assign(m_data[m_length],t_value);
	m_length+=1;
}
void c_Stack3::p_Push8(Array<c_Decl2* > t_values,int t_offset,int t_count){
	for(int t_i=0;t_i<t_count;t_i=t_i+1){
		p_Push7(t_values[t_offset+t_i]);
	}
}
void c_Stack3::p_Push9(Array<c_Decl2* > t_values,int t_offset){
	p_Push8(t_values,t_offset,t_values.Length()-t_offset);
}
c_Enumerator2* c_Stack3::p_ObjectEnumerator(){
	return (new c_Enumerator2)->m_new(this);
}
c_Decl2* c_Stack3::m_NIL;
void c_Stack3::p_Length(int t_newlength){
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
int c_Stack3::p_Length2(){
	return m_length;
}
void c_Stack3::p_Clear(){
	for(int t_i=0;t_i<m_length;t_i=t_i+1){
		gc_assign(m_data[t_i],m_NIL);
	}
	m_length=0;
}
void c_Stack3::mark(){
	Object::mark();
	gc_mark_q(m_data);
}
c_AliasDecl::c_AliasDecl(){
	m_decl=0;
}
c_AliasDecl* c_AliasDecl::m_new(c_Decl2* t_decl,c_ScopeDecl* t_scope){
	c_Decl2::m_new(t_decl,t_scope);
	gc_assign(this->m_decl,t_decl);
	return this;
}
c_AliasDecl* c_AliasDecl::m_new2(){
	c_Decl2::m_new3();
	return this;
}
String c_AliasDecl::p_PagePath(){
	return m_decl->p_PagePath();
}
void c_AliasDecl::mark(){
	c_Decl2::mark();
	gc_mark_q(m_decl);
}
c_Map4::c_Map4(){
	m_root=0;
}
c_Map4* c_Map4::m_new(){
	return this;
}
c_Node5* c_Map4::p_FindNode(String t_key){
	c_Node5* t_node=m_root;
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
c_Stack3* c_Map4::p_Get(String t_key){
	c_Node5* t_node=p_FindNode(t_key);
	if((t_node)!=0){
		return t_node->m_value;
	}
	return 0;
}
int c_Map4::p_RotateLeft4(c_Node5* t_node){
	c_Node5* t_child=t_node->m_right;
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
int c_Map4::p_RotateRight4(c_Node5* t_node){
	c_Node5* t_child=t_node->m_left;
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
int c_Map4::p_InsertFixup4(c_Node5* t_node){
	while(((t_node->m_parent)!=0) && t_node->m_parent->m_color==-1 && ((t_node->m_parent->m_parent)!=0)){
		if(t_node->m_parent==t_node->m_parent->m_parent->m_left){
			c_Node5* t_uncle=t_node->m_parent->m_parent->m_right;
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
			c_Node5* t_uncle2=t_node->m_parent->m_parent->m_left;
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
bool c_Map4::p_Set4(String t_key,c_Stack3* t_value){
	c_Node5* t_node=m_root;
	c_Node5* t_parent=0;
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
	t_node=(new c_Node5)->m_new(t_key,t_value,-1,t_parent);
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
c_Node5* c_Map4::p_FirstNode(){
	if(!((m_root)!=0)){
		return 0;
	}
	c_Node5* t_node=m_root;
	while((t_node->m_left)!=0){
		t_node=t_node->m_left;
	}
	return t_node;
}
c_NodeEnumerator5* c_Map4::p_ObjectEnumerator(){
	return (new c_NodeEnumerator5)->m_new(p_FirstNode());
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
c_Node5::c_Node5(){
	m_key=String();
	m_right=0;
	m_left=0;
	m_value=0;
	m_color=0;
	m_parent=0;
}
c_Node5* c_Node5::m_new(String t_key,c_Stack3* t_value,int t_color,c_Node5* t_parent){
	this->m_key=t_key;
	gc_assign(this->m_value,t_value);
	this->m_color=t_color;
	gc_assign(this->m_parent,t_parent);
	return this;
}
c_Node5* c_Node5::m_new2(){
	return this;
}
c_Node5* c_Node5::p_NextNode(){
	c_Node5* t_node=0;
	if((m_right)!=0){
		t_node=m_right;
		while((t_node->m_left)!=0){
			t_node=t_node->m_left;
		}
		return t_node;
	}
	t_node=this;
	c_Node5* t_parent=this->m_parent;
	while(((t_parent)!=0) && t_node==t_parent->m_right){
		t_node=t_parent;
		t_parent=t_parent->m_parent;
	}
	return t_parent;
}
c_Stack3* c_Node5::p_Value(){
	return m_value;
}
String c_Node5::p_Key(){
	return m_key;
}
void c_Node5::mark(){
	Object::mark();
	gc_mark_q(m_right);
	gc_mark_q(m_left);
	gc_mark_q(m_value);
	gc_mark_q(m_parent);
}
c_Map5::c_Map5(){
	m_root=0;
}
c_Map5* c_Map5::m_new(){
	return this;
}
int c_Map5::p_RotateLeft5(c_Node6* t_node){
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
int c_Map5::p_RotateRight5(c_Node6* t_node){
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
int c_Map5::p_InsertFixup5(c_Node6* t_node){
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
					p_RotateLeft5(t_node);
				}
				t_node->m_parent->m_color=1;
				t_node->m_parent->m_parent->m_color=-1;
				p_RotateRight5(t_node->m_parent->m_parent);
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
bool c_Map5::p_Set5(String t_key,c_ScopeDecl* t_value){
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
		p_InsertFixup5(t_node);
	}else{
		gc_assign(m_root,t_node);
	}
	return true;
}
c_Node6* c_Map5::p_FirstNode(){
	if(!((m_root)!=0)){
		return 0;
	}
	c_Node6* t_node=m_root;
	while((t_node->m_left)!=0){
		t_node=t_node->m_left;
	}
	return t_node;
}
c_NodeEnumerator4* c_Map5::p_ObjectEnumerator(){
	return (new c_NodeEnumerator4)->m_new(p_FirstNode());
}
c_MapValues* c_Map5::p_Values(){
	return (new c_MapValues)->m_new(this);
}
c_Node6* c_Map5::p_FindNode(String t_key){
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
c_ScopeDecl* c_Map5::p_Get(String t_key){
	c_Node6* t_node=p_FindNode(t_key);
	if((t_node)!=0){
		return t_node->m_value;
	}
	return 0;
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
c_Node6::c_Node6(){
	m_key=String();
	m_right=0;
	m_left=0;
	m_value=0;
	m_color=0;
	m_parent=0;
}
c_Node6* c_Node6::m_new(String t_key,c_ScopeDecl* t_value,int t_color,c_Node6* t_parent){
	this->m_key=t_key;
	gc_assign(this->m_value,t_value);
	this->m_color=t_color;
	gc_assign(this->m_parent,t_parent);
	return this;
}
c_Node6* c_Node6::m_new2(){
	return this;
}
c_Node6* c_Node6::p_NextNode(){
	c_Node6* t_node=0;
	if((m_right)!=0){
		t_node=m_right;
		while((t_node->m_left)!=0){
			t_node=t_node->m_left;
		}
		return t_node;
	}
	t_node=this;
	c_Node6* t_parent=this->m_parent;
	while(((t_parent)!=0) && t_node==t_parent->m_right){
		t_node=t_parent;
		t_parent=t_parent->m_parent;
	}
	return t_parent;
}
c_ScopeDecl* c_Node6::p_Value(){
	return m_value;
}
void c_Node6::mark(){
	Object::mark();
	gc_mark_q(m_right);
	gc_mark_q(m_left);
	gc_mark_q(m_value);
	gc_mark_q(m_parent);
}
c_Map6::c_Map6(){
	m_root=0;
}
c_Map6* c_Map6::m_new(){
	return this;
}
c_Node7* c_Map6::p_FindNode(String t_key){
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
c_StringMap2* c_Map6::p_Get(String t_key){
	c_Node7* t_node=p_FindNode(t_key);
	if((t_node)!=0){
		return t_node->m_value;
	}
	return 0;
}
int c_Map6::p_RotateLeft6(c_Node7* t_node){
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
int c_Map6::p_RotateRight6(c_Node7* t_node){
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
int c_Map6::p_InsertFixup6(c_Node7* t_node){
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
					p_RotateLeft6(t_node);
				}
				t_node->m_parent->m_color=1;
				t_node->m_parent->m_parent->m_color=-1;
				p_RotateRight6(t_node->m_parent->m_parent);
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
bool c_Map6::p_Set6(String t_key,c_StringMap2* t_value){
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
		p_InsertFixup6(t_node);
	}else{
		gc_assign(m_root,t_node);
	}
	return true;
}
c_Node7* c_Map6::p_FirstNode(){
	if(!((m_root)!=0)){
		return 0;
	}
	c_Node7* t_node=m_root;
	while((t_node->m_left)!=0){
		t_node=t_node->m_left;
	}
	return t_node;
}
c_NodeEnumerator2* c_Map6::p_ObjectEnumerator(){
	return (new c_NodeEnumerator2)->m_new(p_FirstNode());
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
c_Node7::c_Node7(){
	m_key=String();
	m_right=0;
	m_left=0;
	m_value=0;
	m_color=0;
	m_parent=0;
}
c_Node7* c_Node7::m_new(String t_key,c_StringMap2* t_value,int t_color,c_Node7* t_parent){
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
String c_Node7::p_Key(){
	return m_key;
}
c_StringMap2* c_Node7::p_Value(){
	return m_value;
}
void c_Node7::mark(){
	Object::mark();
	gc_mark_q(m_right);
	gc_mark_q(m_left);
	gc_mark_q(m_value);
	gc_mark_q(m_parent);
}
c_ImportDecl::c_ImportDecl(){
}
c_ImportDecl* c_ImportDecl::m_new(c_Decl* t_pdecl,c_ScopeDecl* t_scope){
	c_Decl2::m_new2(t_pdecl,t_scope);
	return this;
}
c_ImportDecl* c_ImportDecl::m_new2(){
	c_Decl2::m_new3();
	return this;
}
c_ModuleDecl* c_ImportDecl::p_Resolve(){
	c_ApiDoccer* t_doccer=dynamic_cast<c_ModuleDecl*>(m_scope)->m_doccer;
	c_ModuleDecl* t_mdecl=dynamic_cast<c_ModuleDecl*>(t_doccer->m_scopes->p_Get(m_ident));
	if(!((t_mdecl)!=0)){
		t_doccer->m_george->p_Err(String(L"Can't find import module: ",26)+m_ident);
	}
	return t_mdecl;
}
String c_ImportDecl::p_PagePath(){
	c_ModuleDecl* t_mdecl=p_Resolve();
	if((t_mdecl)!=0){
		return t_mdecl->p_PagePath();
	}
	return String();
}
c_Decl2* c_ImportDecl::p_FindDeclHere(String t_path){
	c_ModuleDecl* t_mdecl=p_Resolve();
	if((t_mdecl)!=0){
		return t_mdecl->p_FindDeclHere(t_path);
	}
	return 0;
}
void c_ImportDecl::mark(){
	c_Decl2::mark();
}
c_ClassDecl::c_ClassDecl(){
	m_exts=0;
	m_extby=(new c_Stack6)->m_new();
}
c_ClassDecl* c_ClassDecl::m_new(c_Decl* t_pdecl,c_ModuleDecl* t_scope){
	c_ScopeDecl::m_new(t_pdecl,(t_scope));
	gc_assign(this->m_template,t_scope->m_template);
	return this;
}
c_ClassDecl* c_ClassDecl::m_new2(){
	c_ScopeDecl::m_new2();
	return this;
}
void c_ClassDecl::p_SetSuper(c_ClassDecl* t_supr){
	gc_assign(m_exts,t_supr);
	t_supr->m_extby->p_Push16(this);
	while((t_supr)!=0){
		String t_2[]={String(L"method",6),String(L"function",8),String(L"property",8)};
		Array<String > t_=Array<String >(t_2,3);
		int t_3=0;
		while(t_3<t_.Length()){
			String t_kind=t_[t_3];
			t_3=t_3+1;
			c_Enumerator2* t_4=t_supr->p_GetDecls(t_kind)->p_ObjectEnumerator();
			while(t_4->p_HasNext()){
				c_Decl2* t_decl=t_4->p_NextObject();
				bool t_found=false;
				c_Enumerator2* t_5=p_GetDecls(t_kind)->p_ObjectEnumerator();
				while(t_5->p_HasNext()){
					c_Decl2* t_tdecl=t_5->p_NextObject();
					if(t_decl->m_ident==t_tdecl->m_ident && t_decl->m_type==t_tdecl->m_type){
						t_found=true;
						break;
					}
				}
				if(t_found){
					continue;
				}
				c_Enumerator2* t_6=p_GetDecls(String(L"inherited_",10)+t_kind)->p_ObjectEnumerator();
				while(t_6->p_HasNext()){
					c_Decl2* t_tdecl2=t_6->p_NextObject();
					if(t_decl->m_ident==t_tdecl2->m_ident && t_decl->m_type==t_tdecl2->m_type){
						t_found=true;
						break;
					}
				}
				if(t_found){
					continue;
				}
				(new c_AliasDecl)->m_new(t_decl,(this));
			}
		}
		t_supr=t_supr->m_exts;
	}
}
c_Decl2* c_ClassDecl::p_FindDeclHere(String t_path){
	c_Decl2* t_decl=c_ScopeDecl::p_FindDeclHere(t_path);
	if(!((t_decl)!=0) && ((m_exts)!=0)){
		t_decl=m_exts->p_FindDeclHere(t_path);
	}
	return t_decl;
}
void c_ClassDecl::mark(){
	c_ScopeDecl::mark();
	gc_mark_q(m_exts);
	gc_mark_q(m_extby);
}
c_Map7::c_Map7(){
	m_root=0;
}
c_Map7* c_Map7::m_new(){
	return this;
}
int c_Map7::p_RotateLeft7(c_Node8* t_node){
	c_Node8* t_child=t_node->m_right;
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
int c_Map7::p_RotateRight7(c_Node8* t_node){
	c_Node8* t_child=t_node->m_left;
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
int c_Map7::p_InsertFixup7(c_Node8* t_node){
	while(((t_node->m_parent)!=0) && t_node->m_parent->m_color==-1 && ((t_node->m_parent->m_parent)!=0)){
		if(t_node->m_parent==t_node->m_parent->m_parent->m_left){
			c_Node8* t_uncle=t_node->m_parent->m_parent->m_right;
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
			c_Node8* t_uncle2=t_node->m_parent->m_parent->m_left;
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
bool c_Map7::p_Set7(String t_key,c_StringStack* t_value){
	c_Node8* t_node=m_root;
	c_Node8* t_parent=0;
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
	t_node=(new c_Node8)->m_new(t_key,t_value,-1,t_parent);
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
c_Node8* c_Map7::p_FindNode(String t_key){
	c_Node8* t_node=m_root;
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
c_StringStack* c_Map7::p_Get(String t_key){
	c_Node8* t_node=p_FindNode(t_key);
	if((t_node)!=0){
		return t_node->m_value;
	}
	return 0;
}
c_Node8* c_Map7::p_FirstNode(){
	if(!((m_root)!=0)){
		return 0;
	}
	c_Node8* t_node=m_root;
	while((t_node->m_left)!=0){
		t_node=t_node->m_left;
	}
	return t_node;
}
c_NodeEnumerator* c_Map7::p_ObjectEnumerator(){
	return (new c_NodeEnumerator)->m_new(p_FirstNode());
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
c_Node8::c_Node8(){
	m_key=String();
	m_right=0;
	m_left=0;
	m_value=0;
	m_color=0;
	m_parent=0;
}
c_Node8* c_Node8::m_new(String t_key,c_StringStack* t_value,int t_color,c_Node8* t_parent){
	this->m_key=t_key;
	gc_assign(this->m_value,t_value);
	this->m_color=t_color;
	gc_assign(this->m_parent,t_parent);
	return this;
}
c_Node8* c_Node8::m_new2(){
	return this;
}
c_Node8* c_Node8::p_NextNode(){
	c_Node8* t_node=0;
	if((m_right)!=0){
		t_node=m_right;
		while((t_node->m_left)!=0){
			t_node=t_node->m_left;
		}
		return t_node;
	}
	t_node=this;
	c_Node8* t_parent=this->m_parent;
	while(((t_parent)!=0) && t_node==t_parent->m_right){
		t_node=t_parent;
		t_parent=t_parent->m_parent;
	}
	return t_parent;
}
String c_Node8::p_Key(){
	return m_key;
}
c_StringStack* c_Node8::p_Value(){
	return m_value;
}
void c_Node8::mark(){
	Object::mark();
	gc_mark_q(m_right);
	gc_mark_q(m_left);
	gc_mark_q(m_value);
	gc_mark_q(m_parent);
}
c_NodeEnumerator::c_NodeEnumerator(){
	m_node=0;
}
c_NodeEnumerator* c_NodeEnumerator::m_new(c_Node8* t_node){
	gc_assign(this->m_node,t_node);
	return this;
}
c_NodeEnumerator* c_NodeEnumerator::m_new2(){
	return this;
}
bool c_NodeEnumerator::p_HasNext(){
	return m_node!=0;
}
c_Node8* c_NodeEnumerator::p_NextObject(){
	c_Node8* t_t=m_node;
	gc_assign(m_node,m_node->p_NextNode());
	return t_t;
}
void c_NodeEnumerator::mark(){
	Object::mark();
	gc_mark_q(m_node);
}
String bb_os_StripAll(String t_path){
	return bb_os_StripDir(bb_os_StripExt(t_path));
}
c_NodeEnumerator2::c_NodeEnumerator2(){
	m_node=0;
}
c_NodeEnumerator2* c_NodeEnumerator2::m_new(c_Node7* t_node){
	gc_assign(this->m_node,t_node);
	return this;
}
c_NodeEnumerator2* c_NodeEnumerator2::m_new2(){
	return this;
}
bool c_NodeEnumerator2::p_HasNext(){
	return m_node!=0;
}
c_Node7* c_NodeEnumerator2::p_NextObject(){
	c_Node7* t_t=m_node;
	gc_assign(m_node,m_node->p_NextNode());
	return t_t;
}
void c_NodeEnumerator2::mark(){
	Object::mark();
	gc_mark_q(m_node);
}
c_Stack4::c_Stack4(){
	m_data=Array<c_Stack* >();
	m_length=0;
}
c_Stack4* c_Stack4::m_new(){
	return this;
}
c_Stack4* c_Stack4::m_new2(Array<c_Stack* > t_data){
	gc_assign(this->m_data,t_data.Slice(0));
	this->m_length=t_data.Length();
	return this;
}
c_Stack* c_Stack4::m_NIL;
void c_Stack4::p_Clear(){
	for(int t_i=0;t_i<m_length;t_i=t_i+1){
		gc_assign(m_data[t_i],m_NIL);
	}
	m_length=0;
}
void c_Stack4::p_Push10(c_Stack* t_value){
	if(m_length==m_data.Length()){
		gc_assign(m_data,m_data.Resize(m_length*2+10));
	}
	gc_assign(m_data[m_length],t_value);
	m_length+=1;
}
void c_Stack4::p_Push11(Array<c_Stack* > t_values,int t_offset,int t_count){
	for(int t_i=0;t_i<t_count;t_i=t_i+1){
		p_Push10(t_values[t_offset+t_i]);
	}
}
void c_Stack4::p_Push12(Array<c_Stack* > t_values,int t_offset){
	p_Push11(t_values,t_offset,t_values.Length()-t_offset);
}
c_Stack* c_Stack4::p_Top(){
	return m_data[m_length-1];
}
c_Stack* c_Stack4::p_Pop(){
	m_length-=1;
	c_Stack* t_v=m_data[m_length];
	gc_assign(m_data[m_length],m_NIL);
	return t_v;
}
void c_Stack4::mark(){
	Object::mark();
	gc_mark_q(m_data);
}
c_StringObject::c_StringObject(){
	m_value=String();
}
c_StringObject* c_StringObject::m_new(int t_value){
	this->m_value=String(t_value);
	return this;
}
c_StringObject* c_StringObject::m_new2(Float t_value){
	this->m_value=String(t_value);
	return this;
}
c_StringObject* c_StringObject::m_new3(String t_value){
	this->m_value=t_value;
	return this;
}
c_StringObject* c_StringObject::m_new4(){
	return this;
}
void c_StringObject::mark(){
	Object::mark();
}
c_NodeEnumerator3::c_NodeEnumerator3(){
	m_node=0;
}
c_NodeEnumerator3* c_NodeEnumerator3::m_new(c_Node3* t_node){
	gc_assign(this->m_node,t_node);
	return this;
}
c_NodeEnumerator3* c_NodeEnumerator3::m_new2(){
	return this;
}
bool c_NodeEnumerator3::p_HasNext(){
	return m_node!=0;
}
c_Node3* c_NodeEnumerator3::p_NextObject(){
	c_Node3* t_t=m_node;
	gc_assign(m_node,m_node->p_NextNode());
	return t_t;
}
void c_NodeEnumerator3::mark(){
	Object::mark();
	gc_mark_q(m_node);
}
c_Stack5::c_Stack5(){
	m_data=Array<int >();
	m_length=0;
}
c_Stack5* c_Stack5::m_new(){
	return this;
}
c_Stack5* c_Stack5::m_new2(Array<int > t_data){
	gc_assign(this->m_data,t_data.Slice(0));
	this->m_length=t_data.Length();
	return this;
}
int c_Stack5::m_NIL;
void c_Stack5::p_Clear(){
	for(int t_i=0;t_i<m_length;t_i=t_i+1){
		m_data[t_i]=m_NIL;
	}
	m_length=0;
}
void c_Stack5::p_Length(int t_newlength){
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
int c_Stack5::p_Length2(){
	return m_length;
}
int c_Stack5::p_Get2(int t_index){
	return m_data[t_index];
}
void c_Stack5::p_Push13(int t_value){
	if(m_length==m_data.Length()){
		gc_assign(m_data,m_data.Resize(m_length*2+10));
	}
	m_data[m_length]=t_value;
	m_length+=1;
}
void c_Stack5::p_Push14(Array<int > t_values,int t_offset,int t_count){
	for(int t_i=0;t_i<t_count;t_i=t_i+1){
		p_Push13(t_values[t_offset+t_i]);
	}
}
void c_Stack5::p_Push15(Array<int > t_values,int t_offset){
	p_Push14(t_values,t_offset,t_values.Length()-t_offset);
}
int c_Stack5::p_Pop(){
	m_length-=1;
	int t_v=m_data[m_length];
	m_data[m_length]=m_NIL;
	return t_v;
}
void c_Stack5::mark(){
	Object::mark();
	gc_mark_q(m_data);
}
c_IntStack::c_IntStack(){
}
c_IntStack* c_IntStack::m_new(Array<int > t_data){
	c_Stack5::m_new2(t_data);
	return this;
}
c_IntStack* c_IntStack::m_new2(){
	c_Stack5::m_new();
	return this;
}
void c_IntStack::mark(){
	c_Stack5::mark();
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
int bb_math_Min(int t_x,int t_y){
	if(t_x<t_y){
		return t_x;
	}
	return t_y;
}
Float bb_math_Min2(Float t_x,Float t_y){
	if(t_x<t_y){
		return t_x;
	}
	return t_y;
}
c_NodeEnumerator4::c_NodeEnumerator4(){
	m_node=0;
}
c_NodeEnumerator4* c_NodeEnumerator4::m_new(c_Node6* t_node){
	gc_assign(this->m_node,t_node);
	return this;
}
c_NodeEnumerator4* c_NodeEnumerator4::m_new2(){
	return this;
}
bool c_NodeEnumerator4::p_HasNext(){
	return m_node!=0;
}
c_Node6* c_NodeEnumerator4::p_NextObject(){
	c_Node6* t_t=m_node;
	gc_assign(m_node,m_node->p_NextNode());
	return t_t;
}
void c_NodeEnumerator4::mark(){
	Object::mark();
	gc_mark_q(m_node);
}
c_Stack6::c_Stack6(){
	m_data=Array<c_ClassDecl* >();
	m_length=0;
}
c_Stack6* c_Stack6::m_new(){
	return this;
}
c_Stack6* c_Stack6::m_new2(Array<c_ClassDecl* > t_data){
	gc_assign(this->m_data,t_data.Slice(0));
	this->m_length=t_data.Length();
	return this;
}
void c_Stack6::p_Push16(c_ClassDecl* t_value){
	if(m_length==m_data.Length()){
		gc_assign(m_data,m_data.Resize(m_length*2+10));
	}
	gc_assign(m_data[m_length],t_value);
	m_length+=1;
}
void c_Stack6::p_Push17(Array<c_ClassDecl* > t_values,int t_offset,int t_count){
	for(int t_i=0;t_i<t_count;t_i=t_i+1){
		p_Push16(t_values[t_offset+t_i]);
	}
}
void c_Stack6::p_Push18(Array<c_ClassDecl* > t_values,int t_offset){
	p_Push17(t_values,t_offset,t_values.Length()-t_offset);
}
bool c_Stack6::p_IsEmpty(){
	return m_length==0;
}
c_Enumerator3* c_Stack6::p_ObjectEnumerator(){
	return (new c_Enumerator3)->m_new(this);
}
c_ClassDecl* c_Stack6::m_NIL;
void c_Stack6::p_Length(int t_newlength){
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
int c_Stack6::p_Length2(){
	return m_length;
}
void c_Stack6::mark(){
	Object::mark();
	gc_mark_q(m_data);
}
c_Enumerator2::c_Enumerator2(){
	m_stack=0;
	m_index=0;
}
c_Enumerator2* c_Enumerator2::m_new(c_Stack3* t_stack){
	gc_assign(this->m_stack,t_stack);
	return this;
}
c_Enumerator2* c_Enumerator2::m_new2(){
	return this;
}
bool c_Enumerator2::p_HasNext(){
	return m_index<m_stack->p_Length2();
}
c_Decl2* c_Enumerator2::p_NextObject(){
	m_index+=1;
	return m_stack->m_data[m_index-1];
}
void c_Enumerator2::mark(){
	Object::mark();
	gc_mark_q(m_stack);
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
c_ValueEnumerator* c_ValueEnumerator::m_new(c_Node6* t_node){
	gc_assign(this->m_node,t_node);
	return this;
}
c_ValueEnumerator* c_ValueEnumerator::m_new2(){
	return this;
}
bool c_ValueEnumerator::p_HasNext(){
	return m_node!=0;
}
c_ScopeDecl* c_ValueEnumerator::p_NextObject(){
	c_Node6* t_t=m_node;
	gc_assign(m_node,m_node->p_NextNode());
	return t_t->m_value;
}
void c_ValueEnumerator::mark(){
	Object::mark();
	gc_mark_q(m_node);
}
c_Markdown::c_Markdown(){
	m__resolver=0;
	m__prettifier=0;
	m__blk=String();
}
c_Markdown* c_Markdown::m_new(c_ILinkResolver* t_resolver,c_IPrettifier* t_prettifier){
	gc_assign(m__resolver,t_resolver);
	gc_assign(m__prettifier,t_prettifier);
	return this;
}
c_Markdown* c_Markdown::m_new2(){
	return this;
}
String c_Markdown::p_Prettify(String t_text){
	if((m__prettifier)!=0){
		return m__prettifier->p_PrettifyLine(t_text);
	}
	return t_text;
}
String c_Markdown::p_SetBlock(String t_blk){
	String t_t=String();
	if(m__blk!=t_blk){
		if(m__blk==String(L"pre",3) && ((m__prettifier)!=0)){
			t_t=m__prettifier->p_EndPrettyBlock();
		}else{
			if((m__blk).Length()!=0){
				t_t=String(L"</",2)+m__blk+String(L">",1);
			}
		}
		m__blk=t_blk;
		if(m__blk==String(L"pre",3) && ((m__prettifier)!=0)){
			t_t=t_t+m__prettifier->p_BeginPrettyBlock();
		}else{
			if((m__blk).Length()!=0){
				t_t=t_t+(String(L"<",1)+m__blk+String(L">",1));
			}
		}
	}
	return t_t;
}
int c_Markdown::p_Find3(String t_src,String t_text,int t_start){
	int t_i=t_src.Find(t_text,t_start);
	if(t_i==-1){
		return -1;
	}
	int t_j=t_i;
	while(t_j>0 && (int)t_src[t_j-1]==92){
		t_j-=1;
	}
	if(t_j!=t_i && (t_i-t_j&1)==1){
		return -1;
	}
	return t_i;
}
String c_Markdown::p_ReplaceSpanTags(String t_src,String t_tag,String t_html){
	int t_i=0;
	int t_l=t_tag.Length();
	do{
		int t_i0=p_Find3(t_src,t_tag,t_i);
		if(t_i0==-1){
			break;
		}
		int t_i1=p_Find3(t_src,t_tag,t_i0+t_l);
		if(t_i1==-1 || t_i1==t_i0+t_l){
			break;
		}
		String t_r=String(L"<",1)+t_html+String(L">",1)+t_src.Slice(t_i0+t_l,t_i1)+String(L"</",2)+t_html+String(L">",1);
		t_src=t_src.Slice(0,t_i0)+t_r+t_src.Slice(t_i1+t_l);
		t_i=t_i0+t_r.Length();
	}while(!(false));
	return t_src;
}
String c_Markdown::p_ReplacePrefixTags(String t_src,String t_tag,String t_html){
	int t_i=0;
	int t_l=t_tag.Length();
	do{
		int t_i0=p_Find3(t_src,t_tag,t_i);
		if(t_i0==-1){
			break;
		}
		int t_i1=t_i0+t_l;
		while(t_i1<t_src.Length()){
			int t_c=(int)t_src[t_i1];
			if(t_c==95 || t_c>=65 && t_c<=90 || t_c>=97 && t_c<=122 || t_i1>t_i0+t_l && t_c>=48 && t_c<=57){
				t_i1+=1;
				continue;
			}
			break;
		}
		if(t_i1==t_i0+t_l){
			t_i+=t_l;
			continue;
		}
		String t_r=String(L"<",1)+t_html+String(L">",1)+t_src.Slice(t_i0+t_l,t_i1)+String(L"</",2)+t_html+String(L">",1);
		t_src=t_src.Slice(0,t_i0)+t_r+t_src.Slice(t_i1);
		t_i=t_i0+t_r.Length();
	}while(!(false));
	return t_src;
}
String c_Markdown::p_ReplaceLinks(String t_src){
	int t_i=0;
	do{
		int t_i0=p_Find3(t_src,String(L"[[",2),t_i);
		if(t_i0==-1){
			break;
		}
		int t_i1=p_Find3(t_src,String(L"]]",2),t_i0+2);
		if(t_i1==-1){
			break;
		}
		String t_p=t_src.Slice(t_i0+2,t_i1);
		String t_t=String();
		int t_j=t_p.Find(String(L"|",1),0);
		if(t_j!=-1){
			t_t=t_p.Slice(t_j+1);
			t_p=t_p.Slice(0,t_j);
		}
		String t_r=m__resolver->p_ResolveLink(t_p,t_t);
		t_src=t_src.Slice(0,t_i0)+t_r+t_src.Slice(t_i1+2);
		t_i=t_i0+t_r.Length();
	}while(!(false));
	t_i=0;
	do{
		int t_i02=p_Find3(t_src,String(L"![",2),t_i);
		if(t_i02==-1){
			break;
		}
		int t_i12=p_Find3(t_src,String(L"](",2),t_i02+1);
		if(t_i12==-1 || t_i12==t_i02+1){
			break;
		}
		int t_i2=p_Find3(t_src,String(L")",1),t_i12+2);
		if(t_i2==-1 || t_i2==t_i12+2){
			break;
		}
		String t_t2=t_src.Slice(t_i02+2,t_i12);
		String t_p2=t_src.Slice(t_i12+2,t_i2);
		String t_r2=m__resolver->p_ResolveLink(String(L"<img>",5)+t_p2,t_t2);
		t_src=t_src.Slice(0,t_i02)+t_r2+t_src.Slice(t_i2+1);
		t_i=t_i02+t_r2.Length();
	}while(!(false));
	t_i=0;
	do{
		int t_i03=p_Find3(t_src,String(L"[",1),t_i);
		if(t_i03==-1){
			break;
		}
		int t_i13=p_Find3(t_src,String(L"](",2),t_i03+1);
		if(t_i13==-1 || t_i13==t_i03+1){
			break;
		}
		int t_i22=p_Find3(t_src,String(L")",1),t_i13+2);
		if(t_i22==-1 || t_i22==t_i13+2){
			break;
		}
		String t_t3=t_src.Slice(t_i03+1,t_i13);
		String t_p3=t_src.Slice(t_i13+2,t_i22);
		String t_r3=m__resolver->p_ResolveLink(t_p3,t_t3);
		t_src=t_src.Slice(0,t_i03)+t_r3+t_src.Slice(t_i22+1);
		t_i=t_i03+t_r3.Length();
	}while(!(false));
	return t_src;
}
String c_Markdown::p_ReplaceEscs(String t_src){
	int t_i=0;
	do{
		int t_i0=t_src.Find(String(L"\\",1),t_i);
		if(t_i0==-1){
			break;
		}
		String t_r=t_src.Slice(t_i0+1,t_i0+2);
		String t_1=t_r;
		if(t_1==String(L"<",1)){
			t_r=String(L"&lt;",4);
		}else{
			if(t_1==String(L">",1)){
				t_r=String(L"&gt;",4);
			}else{
				if(t_1==String(L"&",1)){
					t_r=String(L"&amp;",5);
				}
			}
		}
		t_src=t_src.Slice(0,t_i0)+t_r+t_src.Slice(t_i0+2);
		t_i=t_i0+t_r.Length();
	}while(!(false));
	return t_src;
}
String c_Markdown::p_SpanToHtml(String t_src){
	t_src=p_ReplaceSpanTags(t_src,String(L"`",1),String(L"code",4));
	t_src=p_ReplaceSpanTags(t_src,String(L"*",1),String(L"b",1));
	t_src=p_ReplaceSpanTags(t_src,String(L"%",1),String(L"i",1));
	t_src=p_ReplacePrefixTags(t_src,String(L"@",1),String(L"b",1));
	t_src=p_ReplaceLinks(t_src);
	t_src=p_ReplaceEscs(t_src);
	return t_src;
}
String c_Markdown::p_LineToHtml(String t_src){
	if(m__blk==String(L"pre",3)){
		int t_i=t_src.Find(String(L"</pre>",6),0);
		if(t_i==-1){
			return p_Prettify(t_src);
		}
		if((t_src.Slice(0,t_i).Trim()).Length()!=0){
			return p_Prettify(t_src.Slice(0,t_i))+p_SetBlock(String());
		}
		return p_SetBlock(String());
	}
	if(!((t_src).Length()!=0)){
		if(m__blk==String(L"table",5)){
			return p_SetBlock(String())+String(L"<p>",3);
		}
		return String(L"<p>",3);
	}
	if(t_src==String(L"-",1) || t_src==String(L"--",2) || t_src==String(L"---",3)){
		return p_SetBlock(String())+String(L"<hr>",4);
	}
	if(t_src.StartsWith(String(L"<pre>",5))){
		String t_t=p_SetBlock(String(L"pre",3));
		if((t_src.Slice(5).Trim()).Length()!=0){
			return t_t+p_Prettify(t_src.Slice(5));
		}
		return t_t;
	}
	if(t_src.StartsWith(String(L"| ",2))){
		t_src=p_SpanToHtml(t_src);
		c_StringStack* t_bits=(new c_StringStack)->m_new2();
		int t_i2=1;
		do{
			int t_i0=p_Find3(t_src,String(L"|",1),t_i2);
			if(t_i0==-1){
				break;
			}
			t_bits->p_Push4(t_src.Slice(t_i2,t_i0).Trim());
			t_i2=t_i0+1;
		}while(!(false));
		t_bits->p_Push4(t_src.Slice(t_i2).Trim());
		String t_tag=String(L"td",2);
		if(m__blk!=String(L"table",5)){
			t_tag=String(L"th",2);
		}
		return p_SetBlock(String(L"table",5))+String(L"<tr><",5)+t_tag+String(L">",1)+t_bits->p_Join(String(L"</",2)+t_tag+String(L"><",2)+t_tag+String(L">",1))+String(L"</",2)+t_tag+String(L"></tr>",6);
	}
	if(t_src.StartsWith(String(L">",1))){
		int t_i3=1;
		while(t_i3<t_src.Length() && (int)t_src[t_i3]==62){
			t_i3+=1;
		}
		if(t_i3<t_src.Length() && (int)t_src[t_i3]<=32){
			String t_t2=p_SetBlock(String());
			t_src=p_SpanToHtml(t_src.Slice(t_i3+1));
			return t_t2+String(L"<h",2)+String(t_i3)+String(L">",1)+t_src+String(L"</h",3)+String(t_i3)+String(L">",1);
		}
	}
	if(t_src.StartsWith(String(L"* ",2))){
		String t_t3=p_SetBlock(String(L"ul",2));
		return t_t3+String(L"<li>",4)+p_SpanToHtml(t_src.Slice(2))+String(L"</li>",5);
	}
	if(t_src.StartsWith(String(L"+ ",2))){
		String t_t4=p_SetBlock(String(L"ol",2));
		return t_t4+String(L"<li>",4)+p_SpanToHtml(t_src.Slice(2))+String(L"</li>",5);
	}
	String t_t5=p_SetBlock(String());
	int t_i4=p_Find3(t_src,String(L"~n",2),t_src.Length()-2);
	if(t_i4!=-1){
		t_src=t_src.Slice(0,-2)+String(L"<br>",4);
	}
	t_src=p_SpanToHtml(t_src);
	return t_t5+t_src;
}
String c_Markdown::p_ToHtml(String t_src){
	String t_html=String();
	if(t_src.Contains(String(L"\n",1))){
		c_StringStack* t_buf=(new c_StringStack)->m_new2();
		Array<String > t_=t_src.Split(String(L"\n",1));
		int t_2=0;
		while(t_2<t_.Length()){
			String t_line=t_[t_2];
			t_2=t_2+1;
			t_buf->p_Push4(p_LineToHtml(t_line));
		}
		t_html=t_buf->p_Join(String(L"\n",1));
	}else{
		t_html=p_LineToHtml(t_src);
	}
	if((m__blk).Length()!=0){
		return t_html+p_SetBlock(String());
	}
	return t_html;
}
void c_Markdown::mark(){
	Object::mark();
	gc_mark_q(m__resolver);
	gc_mark_q(m__prettifier);
}
c_Enumerator3::c_Enumerator3(){
	m_stack=0;
	m_index=0;
}
c_Enumerator3* c_Enumerator3::m_new(c_Stack6* t_stack){
	gc_assign(this->m_stack,t_stack);
	return this;
}
c_Enumerator3* c_Enumerator3::m_new2(){
	return this;
}
bool c_Enumerator3::p_HasNext(){
	return m_index<m_stack->p_Length2();
}
c_ClassDecl* c_Enumerator3::p_NextObject(){
	m_index+=1;
	return m_stack->m_data[m_index-1];
}
void c_Enumerator3::mark(){
	Object::mark();
	gc_mark_q(m_stack);
}
c_NodeEnumerator5::c_NodeEnumerator5(){
	m_node=0;
}
c_NodeEnumerator5* c_NodeEnumerator5::m_new(c_Node5* t_node){
	gc_assign(this->m_node,t_node);
	return this;
}
c_NodeEnumerator5* c_NodeEnumerator5::m_new2(){
	return this;
}
bool c_NodeEnumerator5::p_HasNext(){
	return m_node!=0;
}
c_Node5* c_NodeEnumerator5::p_NextObject(){
	c_Node5* t_t=m_node;
	gc_assign(m_node,m_node->p_NextNode());
	return t_t;
}
void c_NodeEnumerator5::mark(){
	Object::mark();
	gc_mark_q(m_node);
}
c_NodeEnumerator6::c_NodeEnumerator6(){
	m_node=0;
}
c_NodeEnumerator6* c_NodeEnumerator6::m_new(c_Node4* t_node){
	gc_assign(this->m_node,t_node);
	return this;
}
c_NodeEnumerator6* c_NodeEnumerator6::m_new2(){
	return this;
}
bool c_NodeEnumerator6::p_HasNext(){
	return m_node!=0;
}
c_Node4* c_NodeEnumerator6::p_NextObject(){
	c_Node4* t_t=m_node;
	gc_assign(m_node,m_node->p_NextNode());
	return t_t;
}
void c_NodeEnumerator6::mark(){
	Object::mark();
	gc_mark_q(m_node);
}
int bbMain(){
	ChangeDir(bb_os_ExtractDir(AppPath()));
	while(FileType(String(L"docs",4))!=2 || FileType(String(L"modules",7))!=2){
		ChangeDir(String(L"..",2));
	}
	int t_i=1;
	while(t_i<AppArgs().Length()){
		String t_arg=AppArgs()[t_i];
		t_i+=1;
		String t_2=t_arg;
		if(t_2==String(L"-ignore",7)){
			if(t_i<AppArgs().Length()){
				bb_apidoccer_ignore_mods->p_Insert(AppArgs()[t_i]);
				t_i+=1;
			}
		}
	}
	bb_os_DeleteDir(String(L"docs/html",9),true);
	CreateDir(String(L"docs/html",9));
	CreateDir(String(L"docs/html/data",14));
	CreateDir(String(L"docs/html/examples",18));
	CreateDir(String(L"docs/html/3rd party modules",27));
	bb_os_CopyDir(String(L"docs/htmldoc",12),String(L"docs/html",9),true,false);
	bb_os_DeleteDir(String(L"docs/cerberusdoc/3rd party modules",34),true);
	CreateDir(String(L"docs/cerberusdoc/3rd party modules",34));
	String t_style=LoadString(String(L"bin/docstyle.txt",16)).Trim();
	if(!((t_style).Length()!=0) || FileType(String(L"docs/templates/",15)+t_style)!=2){
		t_style=String(L"devolonter",10);
	}
	c_George* t_george=(new c_George)->m_new(String(L"docs/templates/",15)+t_style);
	c_ApiDoccer* t_apidoccer=(new c_ApiDoccer)->m_new(t_george);
	c_DocsDoccer* t_docsdoccer=(new c_DocsDoccer)->m_new(t_george);
	bbPrint(String(L"Parsing apis...",15));
	t_apidoccer->p_ParseDocs();
	bbPrint(String(L"Parsing docs...",15));
	t_docsdoccer->p_ParseDocs();
	bbPrint(String(L"Making indices...",17));
	t_george->p_MakeIndices();
	bbPrint(String(L"Making apis...",14));
	t_apidoccer->p_MakeDocs();
	bbPrint(String(L"Making docs...",14));
	t_docsdoccer->p_MakeDocs();
	t_george->p_MakeDocs();
	bb_os_DeleteDir(String(L"docs/cerberusdoc/3rd party modules",34),true);
	bbPrint(String(L"Copy module data dirs...",24));
	t_george->p_CopyDataDirs();
	bbPrint(String(L"Makedocs finished!",18));
	return 0;
}
int bbInit(){
	GC_CTOR
	bb_apidoccer_ignore_mods=(new c_StringSet)->m_new();
	c_Stack2::m_NIL=String();
	c_Stack::m_NIL=0;
	c_Stack4::m_NIL=0;
	c_Stack5::m_NIL=0;
	c_Stack3::m_NIL=0;
	c_Stack6::m_NIL=0;
	c_George::m_CerberusKeywords=String(L";Void;Strict;Public;Private;Property;Bool;Int;Float;String;Array;Object;Mod;Continue;Exit;Include;Import;Module;Extern;New;Self;Super;Eachin;True;False;Null;Not;Extends;Abstract;Final;Native;Select;Case;Default;Const;Local;Global;Field;Method;Function;Class;Interface;Implements;Enumerate;And;Or;Shl;Shr;End;If;Then;Else;Elseif;Endif;While;Wend;Repeat;Until;Forever;For;To;Step;Next;Return;Inline;Try;Catch;Throw;Throwable;Print;Error;Alias;",441).ToLower();
	return 0;
}
void gc_mark(){
	gc_mark_q(bb_apidoccer_ignore_mods);
	gc_mark_q(c_Stack::m_NIL);
	gc_mark_q(c_Stack4::m_NIL);
	gc_mark_q(c_Stack3::m_NIL);
	gc_mark_q(c_Stack6::m_NIL);
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
